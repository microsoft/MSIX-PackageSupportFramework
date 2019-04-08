//-------------------------------------------------------------------------------------------------------    
// Copyright (C) Microsoft Corporation. All rights reserved.    
// Licensed under the MIT license. See LICENSE file in the project root for full license information.    
//-------------------------------------------------------------------------------------------------------    
    
#include <psf_framework.h>    
    
#include "Config.h"    
#include "Logging.h"    
#include "PreserveError.h"    
    
#pragma comment(lib, "Lz32.lib")    
    
using namespace std::literals;    
    
auto CreateFileImpl = psf::detoured_string_function(&::CreateFileA, &::CreateFileW);    
template <typename CharT>    
HANDLE __stdcall CreateFileFixup(    
    _In_ const CharT* fileName,    
    _In_ DWORD desiredAccess,    
    _In_ DWORD shareMode,    
    _In_opt_ LPSECURITY_ATTRIBUTES securityAttributes,    
    _In_ DWORD creationDisposition,    
    _In_ DWORD flagsAndAttributes,    
    _In_opt_ HANDLE templateFile)    
{    
    LARGE_INTEGER TickStart, TickEnd;    
    QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = CreateFileImpl(fileName, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile);    
    QueryPerformanceCounter(&TickEnd);    
    preserve_last_error preserveError;    
    
    auto functionResult = from_win32_bool(result != INVALID_HANDLE_VALUE);    
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))    
    {    
        if (output_method == trace_method::eventlog)    
        {    
            std::string inputs = "";    
            std::string outputs = "";    
            std::string results = "";    
    
            inputs = "Path=" + InterpretStringA(fileName);    
            inputs += "\n" + InterpretGenericAccess(desiredAccess);    
            inputs += "\n" + InterpretShareMode(shareMode);    
            if (securityAttributes)    
                inputs += "\nSecurityAttributes (unparsed)";    
            inputs += "\n" + InterpretCreationDisposition(creationDisposition);    
            inputs += "\n" + InterpretFileFlagsAndAttributes(flagsAndAttributes);    
            if (templateFile)    
                inputs += "\n" + InterpretAsHex("TemplateFile Handle", templateFile);    
    
            results = InterpretReturn(functionResult, result != INVALID_HANDLE_VALUE).c_str();    
            if (function_failed(functionResult))    
            {    
                outputs += InterpretLastError();    
            }    
            else    
                outputs += InterpretAsHex("Handle", result);    
    
            std::ostringstream sout;    
            InterpretCallingModulePart1()    
                sout << InterpretCallingModulePart2()    
                InterpretCallingModulePart3()    
                std::string cm = sout.str();    
    
            Log_ETW_PostMsgOperationA("CreateFile", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
        }    
        else    
        {    
        Log("CreateFile:\n");    
        LogString("Path", fileName);    
        LogGenericAccess(desiredAccess);    
        LogShareMode(shareMode);    
        LogCreationDisposition(creationDisposition);    
        LogFileFlagsAndAttributes(flagsAndAttributes);    
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
DECLARE_STRING_FIXUP(CreateFileImpl, CreateFileFixup);    
    
auto CreateFile2Impl = &::CreateFile2;    
HANDLE __stdcall CreateFile2Fixup(    
    _In_ LPCWSTR fileName,    
    _In_ DWORD desiredAccess,    
    _In_ DWORD shareMode,    
    _In_ DWORD creationDisposition,    
    _In_opt_ LPCREATEFILE2_EXTENDED_PARAMETERS createExParams)    
{    
    LARGE_INTEGER TickStart, TickEnd;    
    QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = CreateFile2Impl(fileName, desiredAccess, shareMode, creationDisposition, createExParams);    
    preserve_last_error preserveError;    
    QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32_bool(result != INVALID_HANDLE_VALUE);    
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))    
    {    
        if (output_method == trace_method::eventlog)    
        {    
            std::string inputs = "";    
            std::string outputs = "";    
            std::string results = "";    
                
            inputs = "Path=" + InterpretStringA(fileName);    
            inputs += "\n" + InterpretGenericAccess(desiredAccess);    
            inputs += "\n" + InterpretShareMode(shareMode);    
            inputs += "\n" + InterpretCreationDisposition(creationDisposition);    
            if (createExParams)    
            {    
                inputs += "\n" + InterpretFileAttributes(createExParams->dwFileAttributes);    
                inputs += "\n" + InterpretFileFlags(createExParams->dwFileFlags);    
                inputs += "\n" + InterpretSQOS(createExParams->dwSecurityQosFlags);    
                if (createExParams->lpSecurityAttributes)    
                    inputs += "\nSecurity (unparsed)";    
            }    
    
            results = InterpretReturn(functionResult, result != INVALID_HANDLE_VALUE).c_str();    
            if (function_failed(functionResult))    
            {    
                outputs +=  InterpretLastError();    
            }    
            else    
                outputs += InterpretAsHex("Handle", result);    
    
            std::ostringstream sout;    
            InterpretCallingModulePart1()    
                sout << InterpretCallingModulePart2()    
                InterpretCallingModulePart3()    
                std::string cm = sout.str();    
    
            Log_ETW_PostMsgOperationA("CreateFile2", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
        }    
        else    
        {    
        Log("CreateFile2:\n");    
        LogString("Path", fileName);    
        LogGenericAccess(desiredAccess);    
        LogShareMode(shareMode);    
        LogCreationDisposition(creationDisposition);    
        if (createExParams) LogFileAttributes(createExParams->dwFileAttributes);    
        if (createExParams) LogFileFlags(createExParams->dwFileFlags);    
        if (createExParams) LogSQOS(createExParams->dwSecurityQosFlags);    
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
DECLARE_FIXUP(CreateFile2Impl, CreateFile2Fixup);    
    
auto CopyFileImpl = psf::detoured_string_function(&::CopyFileA, &::CopyFileW);    
template <typename CharT>    
BOOL __stdcall CopyFileFixup(_In_ const CharT* existingFileName, _In_ const CharT* newFileName, _In_ BOOL failIfExists)    
{    
    LARGE_INTEGER TickStart, TickEnd;    
    QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = CopyFileImpl(existingFileName, newFileName, failIfExists);    
    preserve_last_error preserveError;    
    QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32_bool(result);    
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))    
    {    
        if (output_method == trace_method::eventlog)    
        {    
            std::string inputs = "";    
            std::string outputs = "";    
            std::string results = "";    
    
            inputs = "Source=" + InterpretStringA(existingFileName);    
            inputs += "\nDestination=" + InterpretStringA(newFileName);    
            inputs += "\nFailIfExists=";     
            inputs += bool_to_string(failIfExists);    
    
            results = InterpretReturn(functionResult, result).c_str();    
            if (function_failed(functionResult))    
            {    
                outputs += InterpretLastError();    
            }    
    
            std::ostringstream sout;    
            InterpretCallingModulePart1()    
                sout <<  InterpretCallingModulePart2()    
                InterpretCallingModulePart3()    
                std::string cm = sout.str();    
    
            Log_ETW_PostMsgOperationA("CopyFile", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
        }    
        else    
        {    
        Log("CopyFile:\n");    
        LogString("Source", existingFileName);    
        LogString("Destination", newFileName);    
        LogBool( "Fail If Exists", failIfExists);    
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
DECLARE_STRING_FIXUP(CopyFileImpl, CopyFileFixup);    
    
auto CopyFile2Impl = &::CopyFile2;    
HRESULT __stdcall CopyFile2Fixup(    
    _In_ PCWSTR existingFileName,    
    _In_ PCWSTR newFileName,    
    _In_opt_ COPYFILE2_EXTENDED_PARAMETERS* extendedParameters)    
{    
    LARGE_INTEGER TickStart, TickEnd;    
    QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = CopyFile2Impl(existingFileName, newFileName, extendedParameters);    
    QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_hresult(result);    
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))    
    {    
        if (output_method == trace_method::eventlog)    
        {    
            std::string inputs = "";    
            std::string outputs = "";    
            std::string results = "";    
    
            inputs = "Source=" + InterpretStringA(existingFileName);    
            inputs += "\nDestination=" + InterpretStringA(newFileName);    
            if (extendedParameters)    
                inputs += InterpretCopyFlags(extendedParameters->dwCopyFlags);    
    
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
    
            Log_ETW_PostMsgOperationA("CopyFile2", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
        }    
        else    
        {    
        Log("CopyFile2:\n");    
        LogString("Source", existingFileName);    
        LogString("Destination", newFileName);    
        if (extendedParameters) LogCopyFlags(extendedParameters->dwCopyFlags);    
        LogFunctionResult(functionResult);    
        if (function_failed(functionResult))    
        {    
            LogHResult(result);    
        }    
        LogCallingModule();    
    }    
    }    
    
    return result;    
}    
DECLARE_FIXUP(CopyFile2Impl, CopyFile2Fixup);    
    
auto CopyFileExImpl = psf::detoured_string_function(&::CopyFileExA, &::CopyFileExW);    
template <typename CharT>    
BOOL __stdcall CopyFileExFixup(    
    _In_ const CharT* existingFileName,    
    _In_ const CharT* newFileName,    
    _In_opt_ LPPROGRESS_ROUTINE progressRoutine,    
    _In_opt_ LPVOID data,    
    _In_opt_ LPBOOL cancel,    
    _In_ DWORD copyFlags)    
{    
    LARGE_INTEGER TickStart, TickEnd;    
    QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = CopyFileExImpl(existingFileName, newFileName, progressRoutine, data, cancel, copyFlags);    
    preserve_last_error preserveError;    
    QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32_bool(result);    
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))    
    {    
        if (output_method == trace_method::eventlog)    
        {    
            std::string inputs = "";    
            std::string outputs = "";    
            std::string results = "";    
    
            inputs = "Source=" + InterpretStringA(existingFileName);    
            inputs += "\nDestination=" + InterpretStringA(newFileName);    
            inputs += "\n" + InterpretCopyFlags(copyFlags);    
    
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
    
            Log_ETW_PostMsgOperationA("CopyFileEx", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
        }    
        else    
        {    
        Log("CopyFileEx:\n");    
        LogString("Source", existingFileName);    
        LogString("Destination", newFileName);    
        LogCopyFlags(copyFlags);    
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
DECLARE_STRING_FIXUP(CopyFileExImpl, CopyFileExFixup);    
    
auto CreateHardLinkImpl = psf::detoured_string_function(&::CreateHardLinkA, &::CreateHardLinkW);    
template <typename CharT>    
BOOL __stdcall CreateHardLinkFixup(    
    _In_ const CharT* fileName,    
    _In_ const CharT* existingFileName,    
    _Reserved_ LPSECURITY_ATTRIBUTES securityAttributes)    
{    
    LARGE_INTEGER TickStart, TickEnd;    
    QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = CreateHardLinkImpl(fileName, existingFileName, securityAttributes);    
    preserve_last_error preserveError;    
    QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32_bool(result);    
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))    
    {    
        if (output_method == trace_method::eventlog)    
        {    
            std::string inputs = "";    
            std::string outputs = "";    
            std::string results = "";    
    
            inputs = "Path=" + InterpretStringA(fileName);    
            inputs += "\nTarget=" + InterpretStringA(existingFileName);    
        
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
    
            Log_ETW_PostMsgOperationA("CreateHardLink", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
        }    
        else    
        {    
        Log("CreateHardLink:\n");    
        LogString("Path", fileName);    
        LogString("Existing File", existingFileName);    
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
DECLARE_STRING_FIXUP(CreateHardLinkImpl, CreateHardLinkFixup);    
    
auto CreateSymbolicLinkImpl = psf::detoured_string_function(&::CreateSymbolicLinkA, &::CreateSymbolicLinkW);    
template <typename CharT>    
BOOLEAN __stdcall CreateSymbolicLinkFixup(    
    _In_ const CharT* symlinkFileName,    
    _In_ const CharT* targetFileName,    
    _In_ DWORD flags)    
{    
    LARGE_INTEGER TickStart, TickEnd;    
    QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = CreateSymbolicLinkImpl(symlinkFileName, targetFileName, flags);    
    preserve_last_error preserveError;    
    QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32_bool(result);    
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))    
    {    
        if (output_method == trace_method::eventlog)    
        {    
            std::string inputs = "";    
            std::string outputs = "";    
            std::string results = "";    
    
            inputs = "Symlink=" + InterpretStringA(symlinkFileName);    
            inputs += "\nTarget=" + InterpretStringA(targetFileName);    
            inputs += "\n" + InterpretSymlinkFlags(flags);    
    
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
    
            Log_ETW_PostMsgOperationA("CreateSymbolicLink", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
        }    
        else    
        {    
        Log("CreateSymbolicLink:\n");    
        LogString("Symlink", symlinkFileName);    
        LogString("Target", targetFileName);    
        LogSymlinkFlags(flags);    
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
DECLARE_STRING_FIXUP(CreateSymbolicLinkImpl, CreateSymbolicLinkFixup);    
    
auto DeleteFileImpl = psf::detoured_string_function(&::DeleteFileA, &::DeleteFileW);    
template <typename CharT>    
BOOL __stdcall DeleteFileFixup(_In_ const CharT* fileName)    
{    
    LARGE_INTEGER TickStart, TickEnd;    
    QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = DeleteFileImpl(fileName);    
    preserve_last_error preserveError;    
    QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32_bool(result);    
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))    
    {    
        if (output_method == trace_method::eventlog)    
        {    
            std::string inputs = "";    
            std::string outputs = "";    
            std::string results = "";    
    
            inputs = "Path" + InterpretStringA(fileName);;    
    
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
    
            Log_ETW_PostMsgOperationA("DeleteFile", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
        }    
        else    
        {    
        Log("DeleteFile:\n");    
        LogString("Path", fileName);    
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
DECLARE_STRING_FIXUP(DeleteFileImpl, DeleteFileFixup);    
    
auto MoveFileImpl = psf::detoured_string_function(&::MoveFileA, &::MoveFileW);    
template <typename CharT>    
BOOL __stdcall MoveFileFixup(_In_ const CharT* existingFileName, _In_ const CharT* newFileName)    
{    
    LARGE_INTEGER TickStart, TickEnd;    
    QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = MoveFileImpl(existingFileName, newFileName);    
    preserve_last_error preserveError;    
    QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32_bool(result);    
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))    
    {    
        if (output_method == trace_method::eventlog)    
        {    
            std::string inputs = "";    
            std::string outputs = "";    
            std::string results = "";    
    
            inputs = "From=" + InterpretStringA(existingFileName);    
            inputs += "\nTo=" + InterpretStringA(newFileName);    
    
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
    
            Log_ETW_PostMsgOperationA("MoveFile", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
        }    
        else    
        {    
        Log("MoveFile:\n");    
        LogString("From", existingFileName);    
        LogString("To", newFileName);    
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
DECLARE_STRING_FIXUP(MoveFileImpl, MoveFileFixup);    
    
auto MoveFileExImpl = psf::detoured_string_function(&::MoveFileExA, &::MoveFileExW);    
template <typename CharT>    
BOOL __stdcall MoveFileExFixup(_In_ const CharT* existingFileName, _In_opt_ const CharT* newFileName, _In_ DWORD flags)    
{    
    LARGE_INTEGER TickStart, TickEnd;    
    QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = MoveFileExImpl(existingFileName, newFileName, flags);    
    preserve_last_error preserveError;    
    QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32_bool(result);    
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))    
    {    
        if (output_method == trace_method::eventlog)    
        {    
            std::string inputs = "";    
            std::string outputs = "";    
            std::string results = "";    
    
            inputs = "From=" + InterpretStringA(existingFileName);    
            inputs += "\nTo=" + InterpretStringA(newFileName);    
            inputs += "\n" +  InterpretMoveFileFlags(flags);    
    
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
    
            Log_ETW_PostMsgOperationA("MoveFileEx", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
        }    
        else    
        {    
        Log("MoveFile:\n");    
        LogString("From", existingFileName);    
        if (newFileName) LogString("To", newFileName);    
        LogMoveFileFlags(flags);    
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
DECLARE_STRING_FIXUP(MoveFileExImpl, MoveFileExFixup);    
    
auto ReplaceFileImpl = psf::detoured_string_function(&::ReplaceFileA, &::ReplaceFileW);    
template <typename CharT>    
BOOL __stdcall ReplaceFileFixup(    
    _In_ const CharT* replacedFileName,    
    _In_ const CharT* replacementFileName,    
    _In_opt_ const CharT* backupFileName,    
    _In_ DWORD replaceFlags,    
    _Reserved_ LPVOID exclude,    
    _Reserved_ LPVOID reserved)    
{    
    LARGE_INTEGER TickStart, TickEnd;    
    QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = ReplaceFileImpl(replacedFileName, replacementFileName, backupFileName, replaceFlags, exclude, reserved);    
    preserve_last_error preserveError;    
    QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32_bool(result);    
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))    
    {    
        if (output_method == trace_method::eventlog)    
        {    
            std::string inputs = "";    
            std::string outputs = "";    
            std::string results = "";    
    
            inputs = "Replaced File=" + InterpretStringA(replacedFileName);    
            inputs += "\nReplacement File=" + InterpretStringA(replacementFileName);    
            if (backupFileName)    
                inputs += "\nBackup File=" + InterpretStringA(backupFileName);    
            inputs += "\n" + InterpretReplaceFileFlags(replaceFlags);    
    
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
    
            Log_ETW_PostMsgOperationA("ReplaceFile", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
        }    
        else    
        {    
        Log("ReplaceFile:\n");    
        LogString("Replaced File", replacedFileName);    
        LogString("Replacement File", replacementFileName);    
        if (backupFileName) LogString("Backup File", backupFileName);    
        LogReplaceFileFlags(replaceFlags);    
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
DECLARE_STRING_FIXUP(ReplaceFileImpl, ReplaceFileFixup);    
    
template <typename CharT>    
using win32_find_data_t = std::conditional_t<psf::is_ansi<CharT>, WIN32_FIND_DATAA, WIN32_FIND_DATAW>;    
    
auto FindFirstFileImpl = psf::detoured_string_function(&::FindFirstFileA, &FindFirstFileW);    
template <typename CharT>    
HANDLE __stdcall FindFirstFileFixup(    
    _In_ const CharT* fileName,    
    _Out_ win32_find_data_t<CharT>* findFileData)    
{    
    LARGE_INTEGER TickStart, TickEnd;    
    QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = FindFirstFileImpl(fileName, findFileData);    
    preserve_last_error preserveError;    
    QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32_bool(result != INVALID_HANDLE_VALUE);    
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))    
    {    
        if (output_method == trace_method::eventlog)    
        {    
            std::string inputs = "";    
            std::string outputs = "";    
            std::string results = "";    
    
            inputs = "Search Path=" + InterpretStringA(fileName);    
    
            results = InterpretReturn(functionResult, result != INVALID_HANDLE_VALUE).c_str();    
            if (function_failed(functionResult))    
            {    
                outputs +=  InterpretLastError();    
            }    
            else    
            {    
                if (result != INVALID_HANDLE_VALUE)    
                {    
                    outputs +=  InterpretAsHex("Handle", result);     
                    outputs += "\nFirst File=" + InterpretStringA(findFileData->cFileName);    
                }    
            }    
    
            std::ostringstream sout;    
            InterpretCallingModulePart1()    
                sout << InterpretCallingModulePart2()    
                InterpretCallingModulePart3()    
                std::string cm = sout.str();    
    
            Log_ETW_PostMsgOperationA("FindFirstFile", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
        }    
        else    
        {    
        Log("FindFirstFile:\n");    
        LogString("Search Path", fileName);    
        LogFunctionResult(functionResult);    
        if (function_failed(functionResult))    
        {    
            LogLastError();    
        }    
        else    
        {    
            if (result != INVALID_HANDLE_VALUE)    
            {    
                Log("\tHandle=%p\n", result);    
                LogString("First File", findFileData->cFileName);    
            }    
        }    
        LogCallingModule();    
    }    
    }    
    
    return result;    
}    
DECLARE_STRING_FIXUP(FindFirstFileImpl, FindFirstFileFixup);    
    
auto FindFirstFileExImpl = psf::detoured_string_function(&::FindFirstFileExA, &::FindFirstFileExW);    
template <typename CharT>    
HANDLE __stdcall FindFirstFileExFixup(    
    _In_ const CharT* fileName,    
    _In_ FINDEX_INFO_LEVELS infoLevelId,    
    _Out_writes_bytes_(sizeof(win32_find_data_t<CharT>)) LPVOID findFileData,    
    _In_ FINDEX_SEARCH_OPS searchOp,    
    _Reserved_ LPVOID searchFilter,    
    _In_ DWORD additionalFlags)    
{    
    LARGE_INTEGER TickStart, TickEnd;    
    QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = FindFirstFileExImpl(fileName, infoLevelId, findFileData, searchOp, searchFilter, additionalFlags);    
    preserve_last_error preserveError;    
    QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32_bool(result != INVALID_HANDLE_VALUE);    
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))    
    {    
        if (output_method == trace_method::eventlog)    
        {    
            std::string inputs = "";    
            std::string outputs = "";    
            std::string results = "";    
    
            inputs = "Search Path=" + InterpretStringA(fileName);    
            inputs += "\n" + InterpretInfoLevelId(infoLevelId);    
            inputs += "\n" + InterpretSearchOp(searchOp);    
            inputs += "\n" + InterpretFindFirstFileExFlags(additionalFlags);    
    
            results = InterpretReturn(functionResult, result != INVALID_HANDLE_VALUE).c_str();    
            if (function_failed(functionResult))    
            {    
                outputs +=  InterpretLastError();    
            }    
            else    
            {    
                if (result != INVALID_HANDLE_VALUE)    
                {    
                    outputs +=  InterpretAsHex("Handle", result);    
                    auto findData = reinterpret_cast<win32_find_data_t<CharT>*>(findFileData);    
                    outputs += "\nFirst File=" + InterpretStringA(findData->cFileName);    
                }    
            }    
    
            std::ostringstream sout;    
            InterpretCallingModulePart1()    
                sout << InterpretCallingModulePart2()    
                InterpretCallingModulePart3()    
                std::string cm = sout.str();    
    
            Log_ETW_PostMsgOperationA("FindFirstFileEx", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
        }    
        else    
        {    
        Log("FindFirstFileEx:\n");    
        LogString("Search Path", fileName);    
        LogInfoLevelId(infoLevelId);    
        LogSearchOp(searchOp);    
        LogFindFirstFileExFlags(additionalFlags);    
        LogFunctionResult(functionResult);    
        if (function_failed(functionResult))    
        {    
            LogLastError();    
        }    
        else    
        {    
            if (result != INVALID_HANDLE_VALUE)    
            {    
                auto findData = reinterpret_cast<win32_find_data_t<CharT>*>(findFileData);    
                Log("\tHandle=%p\n", result);    
                LogString("First File", findData->cFileName);    
            }    
        }    
        LogCallingModule();    
    }    
    }    
    
    return result;    
}    
DECLARE_STRING_FIXUP(FindFirstFileExImpl, FindFirstFileExFixup);    
    
auto FindNextFileImpl = psf::detoured_string_function(&::FindNextFileA, &::FindNextFileW);    
template <typename CharT>    
BOOL __stdcall FindNextFileFixup(_In_ HANDLE findFile, _Out_ win32_find_data_t<CharT>* findFileData)    
{    
    LARGE_INTEGER TickStart, TickEnd;    
    QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = FindNextFileImpl(findFile, findFileData);    
    preserve_last_error preserveError;    
    QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32_bool(result || (::GetLastError() == ERROR_NO_MORE_FILES));    
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))    
    {    
        if (output_method == trace_method::eventlog)    
        {    
    
            std::string inputs = "";    
            std::string outputs = "";    
            std::string results = "";    
    
            inputs = InterpretAsHex("Handle", findFile);    
    
            results = InterpretReturn(functionResult, result || (::GetLastError() == ERROR_NO_MORE_FILES)).c_str();    
            if (function_failed(functionResult))    
            {    
                outputs +=  InterpretLastError();    
            }    
            else    
            {    
                if (result)    
                {    
                    outputs += "Next File=" + InterpretStringA(findFileData->cFileName);    
                }    
            }    
    
            std::ostringstream sout;    
            InterpretCallingModulePart1()    
                sout <<  InterpretCallingModulePart2()    
                InterpretCallingModulePart3()    
                std::string cm = sout.str();    
    
            Log_ETW_PostMsgOperationA("FindNextFile", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
        }    
        else    
        {    
        Log("FindNextFile:\n");    
        Log("\tHandle=%p\n", findFile);    
        LogFunctionResult(functionResult);    
        if (function_failed(functionResult))    
        {    
            LogLastError();    
        }    
        else if (result) // I.e. not ERROR_NO_MORE_FILES    
        {    
            LogString("Next File", findFileData->cFileName);    
        }    
        LogCallingModule();    
    }    
    }    
    
    return result;    
}    
DECLARE_STRING_FIXUP(FindNextFileImpl, FindNextFileFixup);    
    
auto FindCloseImpl = &::FindClose;    
BOOL __stdcall FindCloseFixup(_Inout_ HANDLE findFile)    
{    
    LARGE_INTEGER TickStart, TickEnd;    
    QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = FindCloseImpl(findFile);    
    preserve_last_error preserveError;    
    QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32_bool(result);    
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))    
    {    
        if (output_method == trace_method::eventlog)    
        {    
            std::string inputs = "";    
            std::string outputs = "";    
            std::string results = "";    
    
            inputs = InterpretAsHex("Handle", findFile);    
    
    
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
    
            Log_ETW_PostMsgOperationA("FindClose", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
        }    
        else    
        {    
        Log("FindClose:\n");    
        Log("\tHandle=%p\n", findFile);    
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
DECLARE_FIXUP(FindCloseImpl, FindCloseFixup);    
    
auto CreateDirectoryImpl = psf::detoured_string_function(&::CreateDirectoryA, &::CreateDirectoryW);    
template <typename CharT>    
BOOL __stdcall CreateDirectoryFixup(_In_ const CharT* pathName, _In_opt_ LPSECURITY_ATTRIBUTES securityAttributes)    
{    
    LARGE_INTEGER TickStart, TickEnd;    
    QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = CreateDirectoryImpl(pathName, securityAttributes);    
    preserve_last_error preserveError;    
    QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32_bool(result);    
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))    
    {    
        if (output_method == trace_method::eventlog)    
        {    
            std::string inputs = "";    
            std::string outputs = "";    
            std::string results = "";    
    
            inputs = "Path=" + InterpretStringA(pathName);    
            if (securityAttributes)    
                inputs += "\nSecurityAttributes=(unparsed)";    
    
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
    
            Log_ETW_PostMsgOperationA("CreateDirectory", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
        }    
        else    
        {    
        Log("CreateDirectory:\n");    
        LogString("Path", pathName);    
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
DECLARE_STRING_FIXUP(CreateDirectoryImpl, CreateDirectoryFixup);    
    
auto CreateDirectoryExImpl = psf::detoured_string_function(&::CreateDirectoryExA, &::CreateDirectoryExW);    
template <typename CharT>    
BOOL __stdcall CreateDirectoryExFixup(    
    _In_ const CharT* templateDirectory,    
    _In_ const CharT* newDirectory,    
    _In_opt_ LPSECURITY_ATTRIBUTES securityAttributes)    
{    
    LARGE_INTEGER TickStart, TickEnd;    
    QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = CreateDirectoryExImpl(templateDirectory, newDirectory, securityAttributes);    
    preserve_last_error preserveError;    
    QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32_bool(result);    
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))    
    {    
        if (output_method == trace_method::eventlog)    
        {    
            std::string inputs = "";    
            std::string outputs = "";    
            std::string results = "";    
    
            inputs = "TemplateDirectory=" + InterpretStringA(templateDirectory);    
            inputs += "\nNewDirectory=" + InterpretStringA(newDirectory);    
            if (securityAttributes)    
                inputs += "\nSecurityAttributes=(unparsed)";    
    
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
    
            Log_ETW_PostMsgOperationA("CreateDirectoryEx", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
        }    
        else    
        {    
        Log("CreateDirectoryEx:\n");    
        LogString("Template", templateDirectory);    
        LogString("Path", newDirectory);    
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
DECLARE_STRING_FIXUP(CreateDirectoryExImpl, CreateDirectoryExFixup);    
    
auto RemoveDirectoryImpl = psf::detoured_string_function(&::RemoveDirectoryA, &::RemoveDirectoryW);    
template <typename CharT>    
BOOL __stdcall RemoveDirectoryFixup(_In_ const CharT* pathName)    
{    
    LARGE_INTEGER TickStart, TickEnd;    
    QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = RemoveDirectoryImpl(pathName);    
    preserve_last_error preserveError;    
    QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32_bool(result);    
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))    
    {    
        if (output_method == trace_method::eventlog)    
        {    
            std::string inputs = "";    
            std::string outputs = "";    
            std::string results = "";    
    
            inputs = "Path=" + InterpretStringA(pathName);    
    
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
    
            Log_ETW_PostMsgOperationA("RemoveDirectory", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
        }    
        else    
        {    
        Log("RemoveDirectory:\n");    
        LogString("Path", pathName);    
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
DECLARE_STRING_FIXUP(RemoveDirectoryImpl, RemoveDirectoryFixup);    
    
auto SetCurrentDirectoryImpl = psf::detoured_string_function(&::SetCurrentDirectoryA, &::SetCurrentDirectoryW);    
template <typename CharT>    
BOOL __stdcall SetCurrentDirectoryFixup(_In_ const CharT* pathName)    
{    
    LARGE_INTEGER TickStart, TickEnd;    
    QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = SetCurrentDirectoryImpl(pathName);    
    preserve_last_error preserveError;    
    QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32_bool(result);    
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))    
    {    
        if (output_method == trace_method::eventlog)    
        {    
            std::string inputs = "";    
            std::string outputs = "";    
            std::string results = "";    
    
            inputs = "Path=" + InterpretStringA(pathName);    
    
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
    
            Log_ETW_PostMsgOperationA("SetCurrentDirectory", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
        }    
        else    
        {    
        Log("SetCurrentDirectory:\n");    
        LogString("Path", pathName);    
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
DECLARE_STRING_FIXUP(SetCurrentDirectoryImpl, SetCurrentDirectoryFixup);    
    
auto GetCurrentDirectoryImpl = psf::detoured_string_function(&::GetCurrentDirectoryA, &::GetCurrentDirectoryW);    
template <typename CharT>    
DWORD __stdcall GetCurrentDirectoryFixup(    
    _In_ DWORD bufferLength,    
    _Out_writes_to_opt_(bufferLength, return +1) CharT* buffer)    
{    
    LARGE_INTEGER TickStart, TickEnd;    
    QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = GetCurrentDirectoryImpl(bufferLength, buffer);    
    preserve_last_error preserveError;    
    QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32_bool(result != 0);    
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))    
    {    
        if (output_method == trace_method::eventlog)    
        {    
            std::string inputs = "";    
            std::string outputs = "";    
            std::string results = "";    
    
            results = InterpretReturn(functionResult, result != 0).c_str();    
            if (function_failed(functionResult))    
            {    
                outputs += InterpretLastError();    
            }    
            else if (buffer)    
            {    
                outputs += "Path=" + InterpretStringA(buffer);    
            }    
            std::ostringstream sout;    
            InterpretCallingModulePart1()    
                sout <<  InterpretCallingModulePart2()    
                InterpretCallingModulePart3()    
                std::string cm = sout.str();    
    
    
            Log_ETW_PostMsgOperationA("GetCurrentDirectory", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
        }    
        else    
        {    
        Log("GetCurrentDirectory:\n");    
        LogFunctionResult(functionResult);    
        if (function_failed(functionResult))    
        {    
            LogLastError();    
        }    
        else if (buffer)    
        {    
            LogString("Path", buffer);    
        }    
        LogCallingModule();    
    }    
    }    
    
    return result;    
}    
DECLARE_STRING_FIXUP(GetCurrentDirectoryImpl, GetCurrentDirectoryFixup);    
    
auto GetFileAttributesImpl = psf::detoured_string_function(&::GetFileAttributesA, &::GetFileAttributesW);    
template <typename CharT>    
DWORD __stdcall GetFileAttributesFixup(_In_ const CharT* fileName)    
{    
    LARGE_INTEGER TickStart, TickEnd;    
    QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = GetFileAttributesImpl(fileName);    
    preserve_last_error preserveError;    
    QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32_bool(result != INVALID_FILE_ATTRIBUTES);    
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))    
    {    
        if (output_method == trace_method::eventlog)    
        {    
            std::string inputs = "";    
            std::string outputs = "";    
            std::string results = "";    
    
            inputs = "Path=" + InterpretStringA(fileName);    
    
            results = InterpretReturn(functionResult, result != INVALID_FILE_ATTRIBUTES).c_str();    
            if (function_failed(functionResult))    
            {    
                outputs +=  InterpretLastError();    
            }    
            else    
            {    
                outputs += InterpretFileAttributes(result);    
            }    
    
            std::ostringstream sout;    
            InterpretCallingModulePart1()    
                sout << InterpretCallingModulePart2()    
                InterpretCallingModulePart3()    
                std::string cm = sout.str();    
    
            Log_ETW_PostMsgOperationA("GetFileAttributes", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
        }    
        else    
        {    
        Log("GetFileAttributes:\n");    
        LogString("Path", fileName);    
        LogFunctionResult(functionResult);    
        if (function_failed(functionResult))    
        {    
            LogLastError();    
        }    
        else    
        {    
            LogFileAttributes(result);    
        }    
        LogCallingModule();    
    }    
    }    
    
    return result;    
}    
DECLARE_STRING_FIXUP(GetFileAttributesImpl, GetFileAttributesFixup);    
    
auto SetFileAttributesImpl = psf::detoured_string_function(&::SetFileAttributesA, &::SetFileAttributesW);    
template <typename CharT>    
BOOL __stdcall SetFileAttributesFixup(_In_ const CharT* fileName, _In_ DWORD fileAttributes)    
{    
    LARGE_INTEGER TickStart, TickEnd;    
    QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = SetFileAttributesImpl(fileName, fileAttributes);    
    preserve_last_error preserveError;    
    QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32_bool(result);    
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))    
    {    
        if (output_method == trace_method::eventlog)    
        {    
            std::string inputs = "";    
            std::string outputs = "";    
            std::string results = "";    
    
            inputs = "Path=" + InterpretStringA(fileName);    
            inputs += "\n" + InterpretFileAttributes(fileAttributes);    
    
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
    
            Log_ETW_PostMsgOperationA("SetFileAttributes", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
        }    
        else    
        {    
        Log("GetFileAttributes:\n");    
        LogString("Path", fileName);    
        LogFileAttributes(fileAttributes);    
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
DECLARE_STRING_FIXUP(SetFileAttributesImpl, SetFileAttributesFixup);    
    
auto GetFileAttributesExImpl = psf::detoured_string_function(&::GetFileAttributesExA, &::GetFileAttributesExW);    
template <typename CharT>    
BOOL __stdcall GetFileAttributesExFixup(    
    _In_ const CharT* fileName,    
    _In_ GET_FILEEX_INFO_LEVELS infoLevelId,    
    _Out_writes_bytes_(sizeof(WIN32_FILE_ATTRIBUTE_DATA)) LPVOID fileInformation)    
{    
    LARGE_INTEGER TickStart, TickEnd;    
    QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = GetFileAttributesExImpl(fileName, infoLevelId, fileInformation);    
    preserve_last_error preserveError;    
    QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32_bool(result);    
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))    
    {    
        if (output_method == trace_method::eventlog)    
        {    
            std::string inputs = "";    
            std::string outputs = "";    
            std::string results = "";    
    
            inputs = "Path=" + InterpretStringA(fileName);    
            inputs += "\n" + InterpretInfoLevelId(infoLevelId);    
    
            results = InterpretReturn(functionResult, result).c_str();    
            if (function_failed(functionResult))    
            {    
                outputs +=  InterpretLastError();    
            }    
            else    
            {    
                auto data = reinterpret_cast<WIN32_FILE_ATTRIBUTE_DATA*>(fileInformation);    
                outputs +=  InterpretFileAttributes(data->dwFileAttributes);    
                // TODO: Dump out other elements of data...    
            }    
    
            std::ostringstream sout;    
            InterpretCallingModulePart1()    
                sout << InterpretCallingModulePart2()    
                InterpretCallingModulePart3()    
                std::string cm = sout.str();    
    
            Log_ETW_PostMsgOperationA("GetFileAttributesEx", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
        }    
        else    
        {    
        Log("GetFileAttributesEx:\n");    
        LogString("Path", fileName);    
        LogInfoLevelId(infoLevelId);    
        LogFunctionResult(functionResult);    
        if (function_failed(functionResult))    
        {    
            LogLastError();    
        }    
        else    
        {    
            auto data = reinterpret_cast<WIN32_FILE_ATTRIBUTE_DATA*>(fileInformation);    
            LogFileAttributes(data->dwFileAttributes);    
        }    
        LogCallingModule();    
    }    
    }    
    
    return result;    
}    
DECLARE_STRING_FIXUP(GetFileAttributesExImpl, GetFileAttributesExFixup);    
    
auto LZOpenFileImpl = psf::detoured_string_function(&::LZOpenFileA, &::LZOpenFileW);    
template <typename CharT>    
INT __stdcall LZOpenFileFixup(_In_ CharT* fileName, _Inout_ LPOFSTRUCT reOpenBuf, _In_ WORD style)    
{    
    LARGE_INTEGER TickStart, TickEnd;    
    QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = LZOpenFileImpl(fileName, reOpenBuf, style);    
    QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_lzerror(result);    
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))    
    {    
        if (output_method == trace_method::eventlog)    
        {    
            std::string inputs = "";    
            std::string outputs = "";    
            std::string results = "";    
    
            inputs = "Path=" + InterpretStringA(fileName);    
            inputs += "\n" + InterpretOpenFileStyle(style);    
    
            results = InterpretReturn(functionResult, result).c_str();    
            if (function_failed(functionResult))    
            {    
                outputs +=  InterpretLZError(result);    
            }    
            else    
            {    
                outputs += InterpretAsHex("Handle", (DWORD)result);    
            }    
    
            std::ostringstream sout;    
            InterpretCallingModulePart1()    
                sout << InterpretCallingModulePart2()    
                InterpretCallingModulePart3()    
                std::string cm = sout.str();    
    
            Log_ETW_PostMsgOperationA("LZOpenFile", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
        }    
        else    
        {    
        Log("LZOpenFile:\n");    
        LogString("Path", fileName);    
        LogOpenFileStyle(style);    
        LogFunctionResult(functionResult);    
        if (function_failed(functionResult))    
        {    
            LogLZError(result);    
        }    
        LogCallingModule();    
    }    
    }    
    
    return result;    
}    
DECLARE_STRING_FIXUP(LZOpenFileImpl, LZOpenFileFixup);    
    
    
// Detours seems to only handle string pairs.  Need to figure out how to do something like this...    
#ifdef DETOURSWITHOUTSTRINGS    
    
auto LZCopyImpl = shims::detoured_string_function(&::LZCopy, &::LZCopy);    
template <typename CharT>    
INT __stdcall LZCopyShim(_In_ INT hfSource, _In_ INT hfDest)    
{    
    LARGE_INTEGER TickStart, TickEnd;    
    QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = LZCopyImpl(hfSource, hfDest);    
    QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_lzerror(result);    
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))    
    {    
        if (output_method == trace_method::eventlog)    
        {    
            std::string inputs = "";    
            std::string outputs = "";    
            std::string results = "";    
    
            inputs = InterpretAsHex("HfSource", hfSource);    
            inputs += "\n" + InterpretAsHex("HfDest", hfDest);    
    
            results = InterpretReturn(functionResult, result).c_str();    
            if (function_failed(functionResult))    
            {    
                outputs +=  InterpretLZError(result);    
            }    
    
            std::ostringstream sout;    
            InterpretCallingModulePart1()    
                sout << InterpretCallingModulePart2()    
                InterpretCallingModulePart3()    
                std::string cm = sout.str();    
    
            Log_ETW_PostMsgOperationA("LZCopy", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
        }    
        else    
        {    
            Log("LZCopy:\n");    
            LogFunctionResult(functionResult);    
            if (function_failed(functionResult))    
            {    
                LogLZError(result);    
            }    
            LogCallingModule();    
        }    
    }    
    
    return result;    
}    
DECLARE_STRING_SHIM(LZCopyImpl, LZCopyShim);    
#endif    
    
// NOTE: The following is a list of functions taken from https://msdn.microsoft.com/en-us/library/windows/desktop/aa364232(v=vs.85).aspx    
//       and https://msdn.microsoft.com/en-us/library/windows/desktop/aa363950(v=vs.85).aspx that are _not_ present    
//       above. This is just a convenient collection of what's missing; it is not a collection of future work. Also note    
//       that functions that don't deal with individual file(s)/paths have been removed    
//    
//  The following deal with file paths:    
//      AddUsersToEncryptedFile    
//      CheckNameLegalDOS8Dot3    
//      CopyFileTransacted    
//      CreateDirectoryTransacted    
//      CreateFileMapping    
//      CreateFileTransacted    
//      CreateHardLinkTransacted    
//      CreateSymbolicLinkTransacted    
//      DecryptFile    
//      DeleteFileTransacted    
//      DuplicateEncryptionInfoFile    
//      EncryptFile    
//      EncryptionDisable    
//      FileEncryptionStatus    
//      FindFirstChangeNotification    
//      FindFirstFileNameTransactedW    
//      FindFirstFileNameW    
//      FindFirstFileTransacted    
//      FindFirstStreamTransactedW    
//      FindFirstStreamW    
//      FindNextFileNameW    
//      FindNextStreamW    
//      GetBinaryType    
//      GetCompressedFileSize    
//      GetCompressedFileSizeTransacted    
//      GetExpandedName                             ?????    
//      GetFileAttributesTransacted    
//      GetFullPathName    
//      GetFullPathNameTransacted    
//      GetLongPathName    
//      GetLongPathNameTransacted    
//      GetShortPathName    
//      GetTempFileName    
//      MoveFileTransacted    
//      MoveFileWithProgress    
//      OpenEncryptedFileRaw    
//      OpenFile    
//      QueryRecoveryAgentsOnEncryptedFile    
//      QueryUsersOnEncryptedFile    
//      RemoveDirectoryTransacted    
//      RemoveUsersFromEncryptedFile    
//      SearchPath    
//      SetFileAttributesTransacted    
//      WofEnumEntries    
//      WofFileEnumFiles    
//      WofIsExternalFile    
//      WofShouldCompressBinaries    
//      WofWimAddEntry    
//      WofWimEnumFiles    
//      WofWimRemoveEntry    
//      WofWimSuspendEntry    
//      WofWimUpdateEntry    
//    
//  The following deal with open file handles/contexts:    
//      CancelIo    
//      CancelIoEx    
//      CloseEncryptedFileRaw    
//      CreateIoCompletionPort    
//      FindCloseChangeNotification    
//      FindNextChangeNotification    
//      FlushFileBuffers    
//      GetFileBandwidthReservation    
//      GetFileInformationByHandle    
//      GetFileInformationByHandleEx (*)    
//      GetFileSize    
//      GetFileSizeEx    
//      GetFileType    
//      GetFinalPathNameByHandle (*)    
//      GetQueuedCompletionStatus    
//      GetQueuedCompletionStatusEx    
//      LockFile    
//      LockFileEx    
//      LZClose    
//      LZCopy    
//      LZInit    
//      LZRead    
//      LZSeek    
//      OpenFileById    
//      PostQueuedCompletionStatus    
//      ReadDirectoryChangesExW    
//      ReadDirectoryChangesW    
//      ReadEncryptedFileRaw    
//      ReadFile    
//      ReadFileEx    
//      ReadFileScatter    
//      ReOpenFile    
//      SetEndOfFile    
//      SetFileBandwidthReservation    
//      SetFileCompletionNotificationModes    
//      SetFileInformationByHandle    
//      SetFileIoOverlappedRange    
//      SetFilePointer    
//      SetFilePointerEx    
//      SetFileShortName (*)    
//      SetFileValidData    
//      UnlockFile    
//      UnlockFileEx    
//      WofGetDriverVersion    
//      WofSetFileDataLocation    
//      WriteEncryptedFileRaw    
//      WriteFile    
//      WriteFileEx    
//      WriteFileGather
