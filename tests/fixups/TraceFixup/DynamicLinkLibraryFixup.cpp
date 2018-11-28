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
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();
    auto result = AddDllDirectoryImpl(newDirectory);
    QueryPerformanceCounter(&TickEnd);

    auto functionResult = from_win32_bool(result != 0);
    if (auto lock = acquire_output_lock(function_type::dynamic_link_library, functionResult))
    {
        if (output_method == trace_method::eventlog)
        {
            std::string inputs = "";
            std::string outputs = "";
            std::string results = "";
            
            inputs = "Directory=" + InterpretStringA(newDirectory);

            results = InterpretReturn(functionResult, result!=0).c_str();
            if (function_failed(functionResult))
            {
                outputs +=  InterpretLastError();
            }

            std::ostringstream sout;
            InterpretCallingModulePart1()
            sout << InterpretCallingModulePart2()
            InterpretCallingModulePart3()
            std::string cm = sout.str();

            Log_ETW_PostMsgOperationA("AddDllDirectory", inputs.c_str(), results.c_str(), outputs.c_str(),cm.c_str(), TickStart, TickEnd);
        }
        else
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
    }

    return result;
}
DECLARE_FIXUP(AddDllDirectoryImpl, AddDllDirectoryFixup);

auto LoadLibraryImpl = psf::detoured_string_function(&::LoadLibraryA, &::LoadLibraryW);
template <typename CharT>
HMODULE __stdcall LoadLibraryFixup(_In_ const CharT* libFileName)
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();
    auto result = LoadLibraryImpl(libFileName);
    QueryPerformanceCounter(&TickEnd);

    auto functionResult = from_win32_bool(result != NULL);
    if (auto lock = acquire_output_lock(function_type::dynamic_link_library, functionResult))
    {
        if (output_method == trace_method::eventlog)
        {
            std::string inputs = "";
            std::string outputs = "";
            std::string results = "";

            inputs = "File Name=" + InterpretStringA(libFileName);

            results = InterpretReturn(functionResult, result).c_str();
            if (function_failed(functionResult))
            {
                outputs +=  InterpretLastError();
            }

            std::ostringstream sout;
            InterpretCallingModulePart1()
                sout << InterpretCallingModulePart2()
                InterpretCallingModulePart3()
                std::string cm = sout.str();

            Log_ETW_PostMsgOperationA("LoadLibrary", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);
        }
        else
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
    }

    return result;
}
DECLARE_STRING_FIXUP(LoadLibraryImpl, LoadLibraryFixup);

auto LoadLibraryExImpl = psf::detoured_string_function(&::LoadLibraryExA, &::LoadLibraryExW);
template <typename CharT>
HMODULE __stdcall LoadLibraryExFixup(_In_ const CharT* libFileName, _Reserved_ HANDLE file, _In_ DWORD flags)
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();
    auto result = LoadLibraryExImpl(libFileName, file, flags);
    QueryPerformanceCounter(&TickEnd);

    auto functionResult = from_win32_bool(result != NULL);
    if (auto lock = acquire_output_lock(function_type::dynamic_link_library, functionResult))
    {
        if (output_method == trace_method::eventlog)
        {
            std::string inputs = "";
            std::string outputs = "";
            std::string results = "";

            inputs = "File Name=" + InterpretStringA(libFileName);
            inputs += "\n" + InterpretLoadLibraryFlags(flags);

            results = InterpretReturn(functionResult, result).c_str();
            if (function_failed(functionResult))
            {
                outputs += InterpretLastError();
            }

            std::ostringstream sout;
            InterpretCallingModulePart1()
                sout << InterpretCallingModulePart2()
                InterpretCallingModulePart3()
                std::string cm = sout.str();

            Log_ETW_PostMsgOperationA("LoadLibraryEx", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);
        }
        else
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
    }

    return result;
}
DECLARE_STRING_FIXUP(LoadLibraryExImpl, LoadLibraryExFixup);

auto LoadModuleImpl = &::LoadModule;
DWORD __stdcall LoadModuleFixup(_In_ LPCSTR moduleName, _In_ LPVOID parameterBlock)
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();
    auto result = LoadModuleImpl(moduleName, parameterBlock);
    QueryPerformanceCounter(&TickEnd);

    auto functionResult = (result > 31) ? function_result::success : (result == 0) ? function_result::failure : from_win32(result);
    if (auto lock = acquire_output_lock(function_type::dynamic_link_library, functionResult))
    {
        if (output_method == trace_method::eventlog)
        {
            std::string inputs = "";
            std::string outputs = "";
            std::string results = "";

            inputs = "Module Name=" + InterpretStringA(moduleName);

            results = InterpretReturn(functionResult, result).c_str();
            if (function_failed(functionResult))
            {
                outputs +=  InterpretLastError();
            }

            std::ostringstream sout;
            InterpretCallingModulePart1()
                sout <<  InterpretCallingModulePart2()
                InterpretCallingModulePart3()
                std::string cm = sout.str();

            Log_ETW_PostMsgOperationA("LoadModule", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);
        }
        else
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
    }

    return result;
}
DECLARE_FIXUP(LoadModuleImpl, LoadModuleFixup);

auto LoadPackagedLibraryImpl = &::LoadPackagedLibrary;
HMODULE __stdcall LoadPackagedLibraryFixup(_In_ LPCWSTR libFileName, _Reserved_ DWORD reserved)
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();
    auto result = LoadPackagedLibraryImpl(libFileName, reserved);
    QueryPerformanceCounter(&TickEnd);

    auto functionResult = from_win32_bool(result != NULL);
    if (auto lock = acquire_output_lock(function_type::dynamic_link_library, functionResult))
    {
        if (output_method == trace_method::eventlog)
        {
            std::string inputs = "";
            std::string outputs = "";
            std::string results = "";

            inputs = "File Name=" + InterpretStringA(libFileName);

            results = InterpretReturn(functionResult, result).c_str();
            if (function_failed(functionResult))
            {
                outputs +=  InterpretLastError();
            }

            std::ostringstream sout;
            InterpretCallingModulePart1()
                sout <<  InterpretCallingModulePart2()
                InterpretCallingModulePart3()
                std::string cm = sout.str();

            Log_ETW_PostMsgOperationA("LoadPackagedLibrary", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);
        }
        else
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
    }

    return result;
}
DECLARE_FIXUP(LoadPackagedLibraryImpl, LoadPackagedLibraryFixup);

auto RemoveDllDirectoryImpl = &::RemoveDllDirectory;
BOOL __stdcall RemoveDllDirectoryFixup(_In_ DLL_DIRECTORY_COOKIE cookie)
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();
    auto result = RemoveDllDirectoryImpl(cookie);
    QueryPerformanceCounter(&TickEnd);

    auto functionResult = from_win32_bool(result);
    if (auto lock = acquire_output_lock(function_type::dynamic_link_library, functionResult))
    {
        if (output_method == trace_method::eventlog)
        {
            std::string inputs = "";
            std::string outputs = "";
            std::string results = "";

            inputs = InterpretAsHex("Cookie", cookie);

            results = InterpretReturn(functionResult, result).c_str();
            if (function_failed(functionResult))
            {
                outputs +=  InterpretLastError();
            }

            std::ostringstream sout;
            InterpretCallingModulePart1()
                sout <<  InterpretCallingModulePart2()
                InterpretCallingModulePart3()
                std::string cm = sout.str();

            Log_ETW_PostMsgOperationA("RemoveDllDirectory", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);
        }
        else
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
    }

    return result;
}
DECLARE_FIXUP(RemoveDllDirectoryImpl, RemoveDllDirectoryFixup);

auto SetDefaultDllDirectoriesImpl = &::SetDefaultDllDirectories;
BOOL __stdcall SetDefaultDllDirectoriesFixup(_In_ DWORD directoryFlags)
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();
    auto result = SetDefaultDllDirectoriesImpl(directoryFlags);
    QueryPerformanceCounter(&TickEnd);

    auto functionResult = from_win32_bool(result);
    if (auto lock = acquire_output_lock(function_type::dynamic_link_library, functionResult))
    {
        if (output_method == trace_method::eventlog)
        {
            std::string inputs = "";
            std::string outputs = "";
            std::string results = "";

            inputs = InterpretLoadLibraryFlags(directoryFlags, "Directory Flags");

            results = InterpretReturn(functionResult, result).c_str();
            if (function_failed(functionResult))
            {
                outputs +=  InterpretLastError();
            }

            std::ostringstream sout;
            InterpretCallingModulePart1()
                sout <<  InterpretCallingModulePart2()
                InterpretCallingModulePart3()
                std::string cm = sout.str();
            
            Log_ETW_PostMsgOperationA("SetDefaultDllDirectories", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);
        }
        else
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
    }

    return result;
}
DECLARE_FIXUP(SetDefaultDllDirectoriesImpl, SetDefaultDllDirectoriesFixup);

auto SetDllDirectoryImpl = psf::detoured_string_function(&::SetDllDirectoryA, &::SetDllDirectoryW);
template <typename CharT>
BOOL __stdcall SetDllDirectoryFixup(_In_opt_ const CharT* pathName)
{
    auto entry = LogFunctionEntry();
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto result = SetDllDirectoryImpl(pathName);
    QueryPerformanceCounter(&TickEnd);

    auto functionResult = from_win32_bool(result);
    if (auto lock = acquire_output_lock(function_type::dynamic_link_library, functionResult))
    {
        if (output_method == trace_method::eventlog)
        {
            std::string inputs = "";
            std::string outputs = "";
            std::string results = "";

            inputs = "Path Name" + InterpretStringA(pathName);

            results = InterpretReturn(functionResult, result).c_str();
            if (function_failed(functionResult))
            {
                outputs +=  InterpretLastError();
            }

            std::ostringstream sout;
            InterpretCallingModulePart1()
                sout <<  InterpretCallingModulePart2()
                InterpretCallingModulePart3()
                std::string cm = sout.str();

            Log_ETW_PostMsgOperationA("SetDllDirectory", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);
        }
        else
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
