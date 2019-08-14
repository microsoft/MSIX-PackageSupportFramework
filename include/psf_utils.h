//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <cwctype>
#include <filesystem>
#include <string>

#include <windows.h>
#include <appmodel.h>

#include "win32_error.h"

namespace psf
{
	using GetCurrentPackagePath2 = LONG(__stdcall *)(unsigned int, UINT32*, PWSTR);
	inline std::filesystem::path get_module_path(HMODULE module)
	{
		// The GetModuleFileName API wasn't entirely well thought out and doesn't return the expected size on
		// insufficient buffer failures, so we must retry in a loop
		std::wstring result(MAX_PATH, L'\0');
		while (true)
		{
			auto size = ::GetModuleFileNameW(module, result.data(), static_cast<DWORD>(result.size() + 1));
			if (size == 0)
			{
				throw_last_error();
			}
			else if (size <= result.size())
			{
				// NOTE: Return value does _not_ include the null character
				assert(::GetLastError() == ERROR_SUCCESS);
				result.resize(size);
				return result;
			}
			else
			{
				assert(::GetLastError() == ERROR_INSUFFICIENT_BUFFER);
				result.resize(result.size() * 2);
			}
		}
	}

	inline std::filesystem::path current_module_path()
	{
		// NOTE: Since we're getting a handle to the current module, UNCHANGED_REFCOUNT flag is safe
		HMODULE moduleHandle;
		if (!::GetModuleHandleExW(
			GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<const wchar_t*>(&get_module_path),
			&moduleHandle))
		{
			throw_last_error();
		}

		return get_module_path(moduleHandle);
	}

	inline std::filesystem::path current_executable_path()
	{
		return get_module_path(nullptr);
	}

	namespace details
	{
		static const unsigned int PACKAGE_PATH_TYPE_EFFECTIVE = 2;

		inline std::wstring appmodel_string(LONG(__stdcall *AppModelFunc)(UINT32, UINT32*, wchar_t*))
		{
			UINT32 length = MAX_PATH + 1;
			std::wstring result(length - 1, '\0');

			const auto err = AppModelFunc(PACKAGE_PATH_TYPE_EFFECTIVE, &length, result.data());
			if ((err != ERROR_SUCCESS) && (err != ERROR_INSUFFICIENT_BUFFER))
			{
				throw_win32(err, "could not retrieve AppModel string");
			}

			assert(length > 0);
			result.resize(length - 1);
			if (err == ERROR_INSUFFICIENT_BUFFER)
			{
				check_win32(AppModelFunc(1, &length, result.data()));
				result.resize(length - 1);
			}

			return result;
		}

		inline std::wstring appmodel_string(LONG(__stdcall *AppModelFunc)(UINT32*, wchar_t*))
		{
			// NOTE: `length` includes the null character both as input and output, hence the +1/-1 everywhere
			UINT32 length = MAX_PATH + 1;
			std::wstring result(length - 1, '\0');

			const auto err = AppModelFunc(&length, result.data());
			if ((err != ERROR_SUCCESS) && (err != ERROR_INSUFFICIENT_BUFFER))
			{
				throw_win32(err, "could not retrieve AppModel string");
			}

			assert(length > 0);
			result.resize(length - 1);
			if (err == ERROR_INSUFFICIENT_BUFFER)
			{
				check_win32(AppModelFunc(&length, result.data()));
				result.resize(length - 1);
			}

			return result;
		}
	}

	inline std::wstring current_package_full_name()
	{
		return details::appmodel_string(&::GetCurrentPackageFullName);
	}

	inline std::wstring current_package_family_name()
	{
		return details::appmodel_string(&::GetCurrentPackageFamilyName);
	}

	inline std::filesystem::path current_package_path()
	{
		// Use GetCurrentPackagePath2 if avalible
		std::wstring kernelDll = L"kernel.appcore.dll";
		HMODULE appModelDll = LoadLibraryEx(kernelDll.c_str(), nullptr, 0);

		if (appModelDll == nullptr)
		{
			auto message = narrow(kernelDll.c_str());
			throw_last_error(message.c_str());
		}

		auto getCurrentPackagePath2 = reinterpret_cast<GetCurrentPackagePath2>(GetProcAddress(appModelDll, "GetCurrentPackagePath2"));

		std::wstring result;
		if (getCurrentPackagePath2)
		{
			// If GetCurrentPackagePath 2 does exists
			result = details::appmodel_string(getCurrentPackagePath2);
		}
		else
		{
			result = details::appmodel_string(&::GetCurrentPackagePath);
		}
		result[0] = std::towupper(result[0]);

		return result;
	}

	inline std::wstring current_application_user_model_id()
	{
		return details::appmodel_string(&::GetCurrentApplicationUserModelId);
	}

	inline std::wstring application_id_from_application_user_model_id(const std::wstring& aumid)
	{
		// NOTE: The app id (PRAID) is the string after the '!'
		auto pos = aumid.find(L'!');
		if (pos == std::wstring::npos)
		{
			throw_win32(APPMODEL_ERROR_PACKAGE_IDENTITY_CORRUPT);
		}

		assert(aumid[pos + 1] != '\0');
		return aumid.substr(pos + 1);
	}

	inline std::wstring current_application_id()
	{
		auto aumid = current_application_user_model_id();
		return application_id_from_application_user_model_id(aumid);
	}

	inline bool is_packaged() noexcept
	{
		UINT32 length = 0;
		return ::GetCurrentPackageFamilyName(&length, nullptr) != APPMODEL_ERROR_NO_PACKAGE;
	}

	inline std::filesystem::path get_final_path_name(const std::filesystem::path& filePath)
	{
		std::wstring result;
		HANDLE file = CreateFile(filePath.native().c_str(), FILE_READ_EA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (file)
		{
			DWORD flags = 0;
			DWORD length = GetFinalPathNameByHandle(file, nullptr, 0, flags);
			if (length == 0)
			{
				flags = VOLUME_NAME_GUID;
				length = GetFinalPathNameByHandle(file, nullptr, 0, flags);
			}

			if (length > 0)
			{
				result.resize(length - 1);
				length = GetFinalPathNameByHandle(file, result.data(), length, flags);
				// In success case the returned length does contain the value of the null terminator
				assert(length <= result.size());
			}

			CloseHandle(file);
		}
		return result;
	}
}
