//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <shim_framework.h>

#include "Config.h"
#include "Logging.h"
#include "PreserveError.h"

template <typename CharT>
using startupinfo_t = std::conditional_t<shims::is_ansi<CharT>, LPSTARTUPINFOA, LPSTARTUPINFOW>;

auto CreateProcessImpl = shims::detoured_string_function(&::CreateProcessA, &::CreateProcessW);
template <typename CharT>
BOOL __stdcall CreateProcessShim(
    _In_opt_ const CharT* applicationName,
    _Inout_opt_ CharT* commandLine,
    _In_opt_ LPSECURITY_ATTRIBUTES processAttributes,
    _In_opt_ LPSECURITY_ATTRIBUTES threadAttributes,
    _In_ BOOL inheritHandles,
    _In_ DWORD creationFlags,
    _In_opt_ LPVOID environment,
    _In_opt_ const CharT* currentDirectory,
    _In_ startupinfo_t<CharT> startupInfo,
    _Out_ LPPROCESS_INFORMATION processInformation)
{
    auto entry = LogFunctionEntry();
    auto result = CreateProcessImpl(applicationName, commandLine, processAttributes, threadAttributes, inheritHandles, creationFlags, environment, currentDirectory, startupInfo, processInformation);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(result);
    if (auto lock = acquire_output_lock(function_type::process_and_thread, functionResult))
    {
        Log("CreateProcess:\n");
        if (applicationName) LogString("Application Name", applicationName);
        if (commandLine) LogString("Command Line", commandLine);
        if (currentDirectory) LogString("Working Directory", currentDirectory);
        LogBool("Inherit Handles", inheritHandles);
        LogProcessCreationFlags(creationFlags);
        if (environment)
        {
            Log("\tEnvironment:\n");
            for (auto ptr = reinterpret_cast<const CharT*>(environment); *ptr; ptr += std::char_traits<CharT>::length(ptr) + 1)
            {
                if constexpr (shims::is_ansi<CharT>)
                {
                    Log("\t\t%s\n", ptr);
                }
                else
                {
                    Log("\t\t%ls\n", ptr);
                }
            }
        }
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogLastError();
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_STRING_SHIM(CreateProcessImpl, CreateProcessShim);

auto CreateProcessAsUserImpl = shims::detoured_string_function(&::CreateProcessAsUserA, &::CreateProcessAsUserW);
template <typename CharT>
BOOL __stdcall CreateProcessAsUserShim(
    _In_opt_ HANDLE token,
    _In_opt_ const CharT* applicationName,
    _Inout_opt_ CharT* commandLine,
    _In_opt_ LPSECURITY_ATTRIBUTES processAttributes,
    _In_opt_ LPSECURITY_ATTRIBUTES threadAttributes,
    _In_ BOOL inheritHandles,
    _In_ DWORD creationFlags,
    _In_opt_ LPVOID environment,
    _In_opt_ const CharT* currentDirectory,
    _In_ startupinfo_t<CharT> startupInfo,
    _Out_ LPPROCESS_INFORMATION processInformation)
{
    auto entry = LogFunctionEntry();
    auto result = CreateProcessAsUserImpl(token, applicationName, commandLine, processAttributes, threadAttributes, inheritHandles, creationFlags, environment, currentDirectory, startupInfo, processInformation);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(result);
    if (auto lock = acquire_output_lock(function_type::process_and_thread, functionResult))
    {
        Log("CreateProcessAsUser:\n");
        if (applicationName) LogString("Application Name", applicationName);
        if (commandLine) LogString("Command Line", commandLine);
        if (currentDirectory) LogString("Working Directory", currentDirectory);
        Log("\tToken=%p\n", token);
        LogBool("Inherit Handles", inheritHandles);
        LogProcessCreationFlags(creationFlags);
        if (environment)
        {
            Log("\tEnvironment:\n");
            for (auto ptr = reinterpret_cast<const CharT*>(environment); *ptr; ptr += std::char_traits<CharT>::length(ptr) + 1)
            {
                if constexpr (shims::is_ansi<CharT>)
                {
                    Log("\t\t%s\n", ptr);
                }
                else
                {
                    Log("\t\t%ls\n", ptr);
                }
            }
        }
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogLastError();
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_STRING_SHIM(CreateProcessAsUserImpl, CreateProcessAsUserShim);
