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
        inline std::wstring appmodel_string(LONG (__stdcall *AppModelFunc)(UINT32*, wchar_t*))
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

    inline std::filesystem::path current_package_path()
    {
        auto result = details::appmodel_string(&::GetCurrentPackagePath);
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
}
