//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <psf_framework.h>

#include "Config.h"
#include "FunctionImplementations.h"
#include "Logging.h"
#include "PreserveError.h"

auto AddDllDirectoryImpl = &::AddDllDirectory;
DLL_DIRECTORY_COOKIE __stdcall AddDllDirectoryFixup(_In_ PCWSTR newDirectory)
{
    auto entry = LogFunctionEntry();
    auto result = AddDllDirectoryImpl(newDirectory);

    auto functionResult = from_win32_bool(result != 0);
    if (auto lock = acquire_output_lock(function_type::dynamic_link_library, functionResult))
    {
        Log("AddDllDirectory:\n");
        LogString("New Directory", newDirectory);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogLastError();
        }
        else
        {
            Log("\tCookie=%d\n", result);
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_FIXUP(AddDllDirectoryImpl, AddDllDirectoryFixup);

auto LoadLibraryImpl = psf::detoured_string_function(&::LoadLibraryA, &::LoadLibraryW);
template <typename CharT>
HMODULE __stdcall LoadLibraryFixup(_In_ const CharT* libFileName)
{
    auto entry = LogFunctionEntry();
    auto result = LoadLibraryImpl(libFileName);

    auto functionResult = from_win32_bool(result != NULL);
    if (auto lock = acquire_output_lock(function_type::dynamic_link_library, functionResult))
    {
        Log("LoadLibrary:\n");
        LogString("File Name", libFileName);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogLastError();
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_STRING_FIXUP(LoadLibraryImpl, LoadLibraryFixup);

auto LoadLibraryExImpl = psf::detoured_string_function(&::LoadLibraryExA, &::LoadLibraryExW);
template <typename CharT>
HMODULE __stdcall LoadLibraryExFixup(_In_ const CharT* libFileName, _Reserved_ HANDLE file, _In_ DWORD flags)
{
    auto entry = LogFunctionEntry();
    auto result = LoadLibraryExImpl(libFileName, file, flags);

    auto functionResult = from_win32_bool(result != NULL);
    if (auto lock = acquire_output_lock(function_type::dynamic_link_library, functionResult))
    {
        Log("LoadLibraryEx:\n");
        LogString("File Name", libFileName);
        LogLoadLibraryFlags(flags);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogLastError();
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_STRING_FIXUP(LoadLibraryExImpl, LoadLibraryExFixup);

auto LoadModuleImpl = &::LoadModule;
DWORD __stdcall LoadModuleFixup(_In_ LPCSTR moduleName, _In_ LPVOID parameterBlock)
{
    auto entry = LogFunctionEntry();
    auto result = LoadModuleImpl(moduleName, parameterBlock);

    auto functionResult = (result > 31) ? function_result::success : (result == 0) ? function_result::failure : from_win32(result);
    if (auto lock = acquire_output_lock(function_type::dynamic_link_library, functionResult))
    {
        Log("LoadModule:\n");
        LogString("Module Name", moduleName);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogLastError();
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_FIXUP(LoadModuleImpl, LoadModuleFixup);

auto LoadPackagedLibraryImpl = &::LoadPackagedLibrary;
HMODULE __stdcall LoadPackagedLibraryFixup(_In_ LPCWSTR libFileName, _Reserved_ DWORD reserved)
{
    auto entry = LogFunctionEntry();
    auto result = LoadPackagedLibraryImpl(libFileName, reserved);

    auto functionResult = from_win32_bool(result != NULL);
    if (auto lock = acquire_output_lock(function_type::dynamic_link_library, functionResult))
    {
        Log("LoadPackagedLibrary:\n");
        LogString("File Name", libFileName);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogLastError();
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_FIXUP(LoadPackagedLibraryImpl, LoadPackagedLibraryFixup);

auto RemoveDllDirectoryImpl = &::RemoveDllDirectory;
BOOL __stdcall RemoveDllDirectoryFixup(_In_ DLL_DIRECTORY_COOKIE cookie)
{
    auto entry = LogFunctionEntry();
    auto result = RemoveDllDirectoryImpl(cookie);

    auto functionResult = from_win32_bool(result);
    if (auto lock = acquire_output_lock(function_type::dynamic_link_library, functionResult))
    {
        Log("RemoveDllDirectory:\n");
        Log("\tCookie=%d\n", cookie);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogLastError();
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_FIXUP(RemoveDllDirectoryImpl, RemoveDllDirectoryFixup);

auto SetDefaultDllDirectoriesImpl = &::SetDefaultDllDirectories;
BOOL __stdcall SetDefaultDllDirectoriesFixup(_In_ DWORD directoryFlags)
{
    auto entry = LogFunctionEntry();
    auto result = SetDefaultDllDirectoriesImpl(directoryFlags);

    auto functionResult = from_win32_bool(result);
    if (auto lock = acquire_output_lock(function_type::dynamic_link_library, functionResult))
    {
        Log("SetDefaultDllDirectories:\n");
        LogLoadLibraryFlags(directoryFlags, "Directory Flags");
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogLastError();
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_FIXUP(SetDefaultDllDirectoriesImpl, SetDefaultDllDirectoriesFixup);

auto SetDllDirectoryImpl = psf::detoured_string_function(&::SetDllDirectoryA, &::SetDllDirectoryW);
template <typename CharT>
BOOL __stdcall SetDllDirectoryFixup(_In_opt_ const CharT* pathName)
{
    auto entry = LogFunctionEntry();
    auto result = SetDllDirectoryImpl(pathName);

    auto functionResult = from_win32_bool(result);
    if (auto lock = acquire_output_lock(function_type::dynamic_link_library, functionResult))
    {
        Log("SetDllDirectory:\n");
        LogString("Path Name", pathName);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogLastError();
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_STRING_FIXUP(SetDllDirectoryImpl, SetDllDirectoryFixup);

// NOTE: The following is a list of functions taken from https://msdn.microsoft.com/en-us/library/windows/desktop/ms682599(v=vs.85).aspx
//       that are _not_ present above. This is just a convenient collection of what's missing; it is not a collection of
//       future work.
//
// DisableThreadLibraryCalls
// DllMain
// FreeLibrary
// FreeLibraryAndExitThread
// GetDllDirectory
// GetModuleFileName
// GetModuleFileNameEx
// GetModuleHandle
// GetModuleHandleEx
// GetProcAddress
// QueryOptionalDelayLoadedAPI
