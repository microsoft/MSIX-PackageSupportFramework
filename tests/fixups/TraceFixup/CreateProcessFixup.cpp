//-------------------------------------------------------------------------------------------------------    
// Copyright (C) Microsoft Corporation. All rights reserved.    
// Licensed under the MIT license. See LICENSE file in the project root for full license information.    
//-------------------------------------------------------------------------------------------------------    
    
#include <psf_framework.h>    
    
#include "Config.h"    
#include "Logging.h"    
#include "PreserveError.h"    
    
template <typename CharT>    
using startupinfo_t = std::conditional_t<psf::is_ansi<CharT>, LPSTARTUPINFOA, LPSTARTUPINFOW>;    
    
auto CreateProcessImpl = psf::detoured_string_function(&::CreateProcessA, &::CreateProcessW);    
template <typename CharT>    
BOOL __stdcall CreateProcessFixup(    
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
    LARGE_INTEGER TickStart, TickEnd;    
    QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = CreateProcessImpl(applicationName, commandLine, processAttributes, threadAttributes, inheritHandles, creationFlags, environment, currentDirectory, startupInfo, processInformation);    
    preserve_last_error preserveError;    
    QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32_bool(result);    
    if (auto lock = acquire_output_lock(function_type::process_and_thread, functionResult))    
    {    
        if (output_method == trace_method::eventlog)    
        {    
            std::string inputs = "Application Name=" + InterpretStringA(applicationName) +    
                "\nCommand Line=" + InterpretStringA(commandLine);    
            std::string outputs = "Working Directory=" + InterpretStringA(currentDirectory)    
                + "\nInheritHandles-" + bool_to_string(inheritHandles)    
                + "\n" + InterpretProcessCreationFlags(creationFlags);    
            std::string results = "";    
            if (processAttributes)    
                outputs += "ProcessAttributes present.\n"; // cheap way out for now.    
            if (environment)    
            {    
                outputs += "Environment:\n";    
                for (auto ptr = reinterpret_cast<const CharT*>(environment); *ptr; ptr += std::char_traits<CharT>::length(ptr) + 1)    
                {    
                    outputs += "\t" + InterpretStringA(ptr) + "\n";    
                }    
            }    
    
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
    
            Log_ETW_PostMsgOperationA("CreateProcess", inputs.c_str(), results.c_str() , outputs.c_str(),cm.c_str(), TickStart, TickEnd);    
        }    
        else    
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
                    if constexpr (psf::is_ansi<CharT>)    
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
    }    
    
    return result;    
}    
DECLARE_STRING_FIXUP(CreateProcessImpl, CreateProcessFixup);    
    
auto CreateProcessAsUserImpl = psf::detoured_string_function(&::CreateProcessAsUserA, &::CreateProcessAsUserW);    
template <typename CharT>    
BOOL __stdcall CreateProcessAsUserFixup(    
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
    LARGE_INTEGER TickStart, TickEnd;    
    QueryPerformanceCounter(&TickStart);                
    auto entry = LogFunctionEntry();    
    auto result = CreateProcessAsUserImpl(token, applicationName, commandLine, processAttributes, threadAttributes, inheritHandles, creationFlags, environment, currentDirectory, startupInfo, processInformation);    
    QueryPerformanceCounter(&TickEnd);    
    preserve_last_error preserveError;    
    
    auto functionResult = from_win32_bool(result);    
    if (auto lock = acquire_output_lock(function_type::process_and_thread, functionResult))    
    {    
        if (output_method == trace_method::eventlog)    
        {    
            std::string inputs = "Application Name=" + InterpretStringA(applicationName) +    
                "\nCommand Line=" + InterpretStringA(commandLine);    
            std::string outputs = "Working Directory=" + InterpretStringA(currentDirectory)    
                    + "\nInheritHandles-" + bool_to_string(inheritHandles)    
                    + "\n" + InterpretProcessCreationFlags(creationFlags);    
            std::string results = "";    
            if (processAttributes)    
                outputs += "\nProcessAttributes present."; // cheap way out for now.    
            if (environment)    
            {    
                outputs += "\nEnvironment:";    
                for (auto ptr = reinterpret_cast<const CharT*>(environment); *ptr; ptr += std::char_traits<CharT>::length(ptr) + 1)    
                {    
                    outputs += "\n\t" + InterpretStringA(ptr);    
                }    
            }    
            std::ostringstream sout1;    
            sout1 << "\nToken=0x" << std::uppercase << std::setfill('0') << std::setw(16) << std::hex << token;    
            outputs += sout1.str();    
    
            results = InterpretReturn(functionResult, result).c_str();    
            if (function_failed(functionResult))    
            {    
                outputs += "\n" + InterpretLastError();    
            }    
    
            std::ostringstream sout;    
            InterpretCallingModulePart1()    
                sout << InterpretCallingModulePart2()    
                InterpretCallingModulePart3()    
                std::string cm = sout.str();    
    
            Log_ETW_PostMsgOperationA("CreateProcessAsUser", inputs.c_str(), results.c_str(), outputs.c_str(),cm.c_str(), TickStart, TickEnd);    
        }    
        else    
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
                    if constexpr (psf::is_ansi<CharT>)    
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
    }    
    
    return result;    
}    
DECLARE_STRING_FIXUP(CreateProcessAsUserImpl, CreateProcessAsUserFixup);    
