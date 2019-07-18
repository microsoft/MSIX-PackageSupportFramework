//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// NOTE: For the functions here, the declaration/documentation was taken from the MSDN page for the Zw*** equivalent
//       functions. E.g. NtCreateDirectoryObject from ZwCreateDirectoryObject, etc. The intention of these fixups is to
//       serve as a kind of "catch all" to try and identify failures that occur from calling a not-otherwise-fixed
//       function. E.g. you may see several calls to NtCreateFile that appear to come out of nowhere. In general, not
//       knowing the cause is not of significant concern. Only when application failures occur around the same time as
//       one of these failures should you try and dig deeper into the root cause

#include <psf_framework.h>
#include <utilities.h>

#include "Config.h"
#include "FunctionImplementations.h"
#include "PreserveError.h"
#include "WinternlLogging.h"

using namespace std::literals;

static inline bool should_ignore(POBJECT_ATTRIBUTES objectAttributes)
{
    if (!ignore_dll_load)
    {
        return false;
    }

    constexpr iwstring_view ignoreFiles[] =
    {
        L".dll"_isv,
        L".dll.mui"_isv,
    };

    iwstring_view path(objectAttributes->ObjectName->Buffer, objectAttributes->ObjectName->Length / sizeof(wchar_t));
    for (auto& ignoreFile : ignoreFiles)
    {
        if ((path.length() >= ignoreFile.length()) && (path.substr(path.length() - ignoreFile.length()) == ignoreFile))
        {
            return true;
        }
    }

    return false;
}

auto NtCreateFileImpl = WINTERNL_FUNCTION(NtCreateFile);
NTSTATUS __stdcall NtCreateFileFixup(
    OUT PHANDLE fileHandle,
    IN ACCESS_MASK desiredAccess,
    IN POBJECT_ATTRIBUTES objectAttributes,
    OUT PIO_STATUS_BLOCK ioStatusBlock,
    IN PLARGE_INTEGER allocationSize OPTIONAL,
    IN ULONG fileAttributes,
    IN ULONG shareAccess,
    IN ULONG createDisposition,
    IN ULONG createOptions,
    IN PVOID eaBuffer OPTIONAL,
    IN ULONG eaLength)
{
    if (should_ignore(objectAttributes))
    {
        return NtCreateFileImpl(
            fileHandle,
            desiredAccess,
            objectAttributes,
            ioStatusBlock,
            allocationSize,
            fileAttributes,
            shareAccess,
            createDisposition,
            createOptions,
            eaBuffer,
            eaLength);
    }

    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();
    auto result = NtCreateFileImpl(
        fileHandle,
        desiredAccess,
        objectAttributes,
        ioStatusBlock,
        allocationSize,
        fileAttributes,
        shareAccess,
        createDisposition,
        createOptions,
        eaBuffer,
        eaLength);
    QueryPerformanceCounter(&TickEnd);

    auto functionResult = from_ntstatus(result);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
    {
        if (output_method == trace_method::eventlog)
        {
            std::string inputs = "";
            std::string outputs = "";
            std::string results = "";

            inputs =  InterpretObjectAttributes(objectAttributes);

            inputs += "\n" + InterpretFileCreateOptions(createOptions);
            if (createOptions & FILE_DIRECTORY_FILE)
            {
                inputs += "\n" + InterpretDirectoryAccess(desiredAccess);
            }
            else if (createOptions & FILE_NON_DIRECTORY_FILE)
            {
                inputs += "\n" + InterpretFileAccess(desiredAccess);
            }
            else
            {
                inputs += "\n" + InterpretGenericAccess(desiredAccess);
            }
            outputs =  InterpretFileAttributes(fileAttributes, "File Attributes");
            outputs += "\n" + InterpretShareMode(shareAccess);
            outputs += "\n" + InterpretCreationDispositionInternal(createDisposition);

            results = InterpretReturnNT(functionResult, result).c_str();
            if (function_failed(functionResult))
            {
                outputs += "\n" + InterpretNTStatus(result);
            }
            else
            {
                outputs += "\n" + InterpretAsHex("Handle", fileHandle);
            }

            std::ostringstream sout;
            InterpretCallingModulePart1()
                sout << InterpretCallingModulePart2()
                InterpretCallingModulePart3()
                std::string cm = sout.str();

            Log_ETW_PostMsgOperationA("NtCreateFile", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);
        }
        else
        {
        Log("NtCreateFile:\n");
        LogObjectAttributes(objectAttributes);
        if (createOptions & FILE_DIRECTORY_FILE)
        {
            LogDirectoryAccess(desiredAccess);
        }
        else if (createOptions & FILE_NON_DIRECTORY_FILE)
        {
            LogFileAccess(desiredAccess);
        }
        else
        {
            LogGenericAccess(desiredAccess);
        }
        LogFileAttributes(fileAttributes, "File Attributes");
        LogShareMode(shareAccess);
        LogCreationDispositionInternal(createDisposition);
        LogFileCreateOptions(createOptions);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogNTStatus(result);
        }
        LogCallingModule();
    }
    }

    return result;
}
DECLARE_FIXUP(NtCreateFileImpl, NtCreateFileFixup);

auto NtOpenFileImpl = WINTERNL_FUNCTION(NtOpenFile);
NTSTATUS __stdcall NtOpenFileFixup(
    OUT PHANDLE fileHandle,
    IN ACCESS_MASK desiredAccess,
    IN POBJECT_ATTRIBUTES objectAttributes,
    OUT PIO_STATUS_BLOCK ioStatusBlock,
    IN ULONG shareAccess,
    IN ULONG openOptions)
{
    if (should_ignore(objectAttributes))
    {
        return NtOpenFileImpl(fileHandle, desiredAccess, objectAttributes, ioStatusBlock, shareAccess, openOptions);
    }

    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();
    auto result = NtOpenFileImpl(fileHandle, desiredAccess, objectAttributes, ioStatusBlock, shareAccess, openOptions);

    auto functionResult = from_ntstatus(result);
    QueryPerformanceCounter(&TickEnd);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
    {
        if (output_method == trace_method::eventlog)
        {
            std::string inputs = "";
            std::string outputs = "";
            std::string results = "";

            inputs = InterpretObjectAttributes(objectAttributes);

            if (openOptions & FILE_DIRECTORY_FILE)
            {
                inputs += "\n" + InterpretDirectoryAccess(desiredAccess);
            }
            else if (openOptions & FILE_NON_DIRECTORY_FILE)
            {
                inputs += "\n" + InterpretFileAccess(desiredAccess);
            }
            else
            {
                inputs += "\n" + InterpretGenericAccess(desiredAccess);
            }
            outputs  =        InterpretShareMode(shareAccess);
            outputs += "\n" + InterpretFileCreateOptions(openOptions);

            results = InterpretReturnNT(functionResult, result).c_str();
            if (function_failed(functionResult))
            {
                outputs += "\n" + InterpretNTStatus(result);
            }
            else
            {
                outputs += "\n" + InterpretAsHex("Handle", fileHandle);
            }

            std::ostringstream sout;
            InterpretCallingModulePart1()
                sout << InterpretCallingModulePart2()
                InterpretCallingModulePart3()
                std::string cm = sout.str();

            Log_ETW_PostMsgOperationA("NtOpenFile", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);
        }
        else
        {
        Log("NtOpenFile:\n");
        LogObjectAttributes(objectAttributes);
        if (openOptions & FILE_DIRECTORY_FILE)
        {
            LogDirectoryAccess(desiredAccess);
        }
        else if (openOptions & FILE_NON_DIRECTORY_FILE)
        {
            LogFileAccess(desiredAccess);
        }
        else
        {
            LogGenericAccess(desiredAccess);
        }
        LogShareMode(shareAccess);
        LogFileCreateOptions(openOptions);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogNTStatus(result);
        }
        LogCallingModule();
    }
    }

    return result;
}
DECLARE_FIXUP(NtOpenFileImpl, NtOpenFileFixup);

// NOTE: NtCreateDirectoryObject is only documented; it has no declaration
NTSTATUS WINAPI NtCreateDirectoryObject(
    _Out_ PHANDLE DirectoryHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes);
auto NtCreateDirectoryObjectImpl = WINTERNL_FUNCTION(NtCreateDirectoryObject);
NTSTATUS __stdcall NtCreateDirectoryObjectFixup(
    _Out_ PHANDLE directoryHandle,
    _In_ ACCESS_MASK desiredAccess,
    _In_ POBJECT_ATTRIBUTES objectAttributes)
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();
    auto result = NtCreateDirectoryObjectImpl(directoryHandle, desiredAccess, objectAttributes);

    auto functionResult = from_ntstatus(result);
    QueryPerformanceCounter(&TickEnd);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
    {
        if (output_method == trace_method::eventlog)
        {
            std::string inputs = "";
            std::string outputs = "";
            std::string results = "";

            inputs = InterpretObjectAttributes(objectAttributes);
            inputs += "\n" + InterpretDirectoryAccess(desiredAccess);
            
            results = InterpretReturnNT(functionResult, result).c_str();
            if (function_failed(functionResult))
            {
                outputs +=  InterpretNTStatus(result);
            }
            else
            {
                outputs +=  InterpretAsHex("Handle", directoryHandle);
            }

            std::ostringstream sout;
            InterpretCallingModulePart1()
                sout << InterpretCallingModulePart2()
                InterpretCallingModulePart3()
                std::string cm = sout.str();

            Log_ETW_PostMsgOperationA("NtCreateDirectoryObject", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);
        }
        else
        {
        Log("NtCreateDirectoryObject:\n");
        LogObjectAttributes(objectAttributes);
        LogDirectoryAccess(desiredAccess);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogNTStatus(result);
        }
        LogCallingModule();
    }
    }

    return result;
}
DECLARE_FIXUP(NtCreateDirectoryObjectImpl, NtCreateDirectoryObjectFixup);

// NOTE: NtOpenDirectoryObject is only documented; it has no declaration
NTSTATUS WINAPI NtOpenDirectoryObject(
    _Out_ PHANDLE DirectoryHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes);
auto NtOpenDirectoryObjectImpl = WINTERNL_FUNCTION(NtOpenDirectoryObject);
NTSTATUS __stdcall NtOpenDirectoryObjectFixup(
    _Out_ PHANDLE directoryHandle,
    _In_ ACCESS_MASK desiredAccess,
    _In_ POBJECT_ATTRIBUTES objectAttributes)
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();
    auto result = NtOpenDirectoryObjectImpl(directoryHandle, desiredAccess, objectAttributes);

    auto functionResult = from_ntstatus(result);
    QueryPerformanceCounter(&TickEnd);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
    {
        if (output_method == trace_method::eventlog)
        {
            std::string inputs = "";
            std::string outputs = "";
            std::string results = "";

            inputs = InterpretObjectAttributes(objectAttributes);
            inputs += "\n" + InterpretDirectoryAccessInternal(desiredAccess);

            results = InterpretReturnNT(functionResult, result).c_str();
            if (function_failed(functionResult))
            {
                outputs +=  InterpretNTStatus(result);
            }
            else
            {
                outputs +=  InterpretAsHex("Handle", directoryHandle);
            }

            std::ostringstream sout;
            InterpretCallingModulePart1()
                sout << InterpretCallingModulePart2()
                InterpretCallingModulePart3()
                std::string cm = sout.str();

            Log_ETW_PostMsgOperationA("NtOpenDirectoryObject", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);
        }
        else
        {
        Log("NtOpenDirectoryObject:\n");
        LogObjectAttributes(objectAttributes);
        LogDirectoryAccessInternal(desiredAccess);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogNTStatus(result);
        }
        LogCallingModule();
    }
    }

    return result;
}
DECLARE_FIXUP(NtOpenDirectoryObjectImpl, NtOpenDirectoryObjectFixup);

// NOTE: NtQueryDirectoryObject is only documented; it has no declaration
NTSTATUS WINAPI NtQueryDirectoryObject(
    _In_ HANDLE DirectoryHandle,
    _Out_opt_ PVOID Buffer,
    _In_ ULONG Length,
    _In_ BOOLEAN ReturnSingleEntry,
    _In_ BOOLEAN RestartScan,
    _Inout_ PULONG Context,
    _Out_opt_ PULONG ReturnLength);
auto NtQueryDirectoryObjectImpl = WINTERNL_FUNCTION(NtQueryDirectoryObject);
NTSTATUS __stdcall NtQueryDirectoryObjectFixup(
    _In_ HANDLE directoryHandle,
    _Out_opt_ PVOID buffer,
    _In_ ULONG length,
    _In_ BOOLEAN returnSingleEntry,
    _In_ BOOLEAN restartScan,
    _Inout_ PULONG context,
    _Out_opt_ PULONG returnLength)
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();
    auto result = NtQueryDirectoryObjectImpl(directoryHandle, buffer, length, returnSingleEntry, restartScan, context, returnLength);

    auto functionResult = from_ntstatus(result);
    QueryPerformanceCounter(&TickEnd);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
    {
        if (output_method == trace_method::eventlog)
        {
            std::string inputs = "";
            std::string outputs = "";
            std::string results = "";

            inputs =  InterpretAsHex("Handle", directoryHandle);
            inputs += "\n" +    InterpretBool("Return Single Entry",returnSingleEntry);
            inputs += "\n" + InterpretBool("Restart Scan", restartScan);

            results = InterpretReturnNT(functionResult, result).c_str();
            if (function_failed(functionResult))
            {
                outputs +=  InterpretNTStatus(result);
            }
            else
            {
                struct OBJECT_DIRECTORY_INFORMATION
                {
                    UNICODE_STRING Name;
                    UNICODE_STRING TypeName;
                };

                std::ostringstream sout2;
                sout2 << "Result(s):";
                auto info = reinterpret_cast<const OBJECT_DIRECTORY_INFORMATION*>(buffer);
                for (std::size_t i = 0; info[i].Name.Length || info[i].TypeName.Length; ++i)
                {
                    sout2 << "\tBuffer[" << std::setw(length) << i;
                    sout2 << "]:";
                    if (info[i].Name.Length)
                    {
                        sout2 << InterpretCountedString(" Name", info[i].Name.Buffer, info[i].Name.Length/2);
                    }
                    if (info[i].TypeName.Length)
                    {
                        sout2 << InterpretCountedString(" tTypeName", info[i].TypeName.Buffer, info[i].TypeName.Length / 2);
                    }
                    sout2 << "\n";
                }
                outputs += "\n" + sout2.str();
            }

            std::ostringstream sout;
            InterpretCallingModulePart1()
                sout << InterpretCallingModulePart2()
                InterpretCallingModulePart3()
                std::string cm = sout.str();

            Log_ETW_PostMsgOperationA("NtQueryDirectoryObject", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);
        }
        else
        {
        Log("NtQueryDirectoryObject:\n");
        LogBool("Return Single Entry", returnSingleEntry);
        LogBool("Restart Scan", restartScan);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogNTStatus(result);
        }
        else if (buffer)
        {
            // NOTE: Only documented
            struct OBJECT_DIRECTORY_INFORMATION
            {
                UNICODE_STRING Name;
                UNICODE_STRING TypeName;
            };

            auto info = reinterpret_cast<const OBJECT_DIRECTORY_INFORMATION*>(buffer);
            for (std::size_t i = 0; info[i].Name.Length || info[i].TypeName.Length; ++i)
            {
                Log("\tBuffer[%d]:\n", i);
                if (info[i].Name.Length)
                {
                    Log("\t\tName=%.*ls\n", info[i].Name.Length / 2, info[i].Name.Buffer);
                }
                if (info[i].TypeName.Length)
                {
                    Log("\t\tTypeName=%.*ls\n", info[i].TypeName.Length / 2, info[i].TypeName.Buffer);
                }
            }
        }
        LogCallingModule();
    }
    }

    return result;
}
DECLARE_FIXUP(NtQueryDirectoryObjectImpl, NtQueryDirectoryObjectFixup);

// NOTE: NtOpenSymbolicLinkObject is only documented; it has no declaration
NTSTATUS WINAPI NtOpenSymbolicLinkObject(
    _Out_ PHANDLE LinkHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes);
auto NtOpenSymbolicLinkObjectImpl = WINTERNL_FUNCTION(NtOpenSymbolicLinkObject);
NTSTATUS __stdcall NtOpenSymbolicLinkObjectFixup(
    _Out_ PHANDLE linkHandle,
    _In_ ACCESS_MASK desiredAccess,
    _In_ POBJECT_ATTRIBUTES objectAttributes)
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();
    auto result = NtOpenSymbolicLinkObjectImpl(linkHandle, desiredAccess, objectAttributes);

    auto functionResult = from_ntstatus(result);
    QueryPerformanceCounter(&TickEnd);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
    {
        if (output_method == trace_method::eventlog)
        {
            std::string inputs = "";
            std::string outputs = "";
            std::string results = "";

            inputs = InterpretObjectAttributes(objectAttributes);
            inputs += "\n" + InterpretGenericAccess(desiredAccess);

            results = InterpretReturnNT(functionResult, result).c_str();
            if (function_failed(functionResult))
            {
                outputs +=  InterpretNTStatus(result);
            }
            else
            {
                outputs += InterpretAsHex("LinkHandle", linkHandle);
            }

            std::ostringstream sout;
            InterpretCallingModulePart1()
                sout << InterpretCallingModulePart2()
                InterpretCallingModulePart3()
                std::string cm = sout.str();

            Log_ETW_PostMsgOperationA("NtOpenSymbolicLinkObject", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);
        }
        else
        {
        Log("NtOpenSymbolicLinkObject:\n");
        LogObjectAttributes(objectAttributes);
        LogGenericAccess(desiredAccess);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogNTStatus(result);
        }
        LogCallingModule();
    }
    }

    return result;
}
DECLARE_FIXUP(NtOpenSymbolicLinkObjectImpl, NtOpenSymbolicLinkObjectFixup);

// NOTE: NtQuerySymbolicLinkObject is only documented; it has no declaration
NTSTATUS WINAPI NtQuerySymbolicLinkObject(
    _In_ HANDLE LinkHandle,
    _Inout_ PUNICODE_STRING LinkTarget,
    _Out_opt_ PULONG ReturnedLength);
auto NtQuerySymbolicLinkObjectImpl = WINTERNL_FUNCTION(NtQuerySymbolicLinkObject);
NTSTATUS WINAPI NtQuerySymbolicLinkObjectFixup(
    _In_ HANDLE linkHandle,
    _Inout_ PUNICODE_STRING linkTarget,
    _Out_opt_ PULONG returnedLength)
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();
    auto result = NtQuerySymbolicLinkObjectImpl(linkHandle, linkTarget, returnedLength);

    auto functionResult = from_ntstatus(result);
    QueryPerformanceCounter(&TickEnd);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
    {
        if (output_method == trace_method::eventlog)
        {
            std::string inputs = "";
            std::string outputs = "";
            std::string results = "";

            inputs = InterpretAsHex("LinkHandle", linkHandle);
            inputs += "\n" + InterpretUnicodeString("LinkTarget", linkTarget);

            results = InterpretReturnNT(functionResult, result).c_str();
            if (function_failed(functionResult))
            {
                outputs += InterpretNTStatus(result);
                if (returnedLength && (result == STATUS_BUFFER_TOO_SMALL))
                {
                    outputs += InterpretAsHex("\nRequired Length", *returnedLength);
                }
            }
            else
            {
                outputs +=  InterpretUnicodeString("Target", linkTarget);
            }

            std::ostringstream sout;
            InterpretCallingModulePart1()
                sout << InterpretCallingModulePart2()
                InterpretCallingModulePart3()
                std::string cm = sout.str();

            Log_ETW_PostMsgOperationA("NtQuerySymbolicLinkObject", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);
        }
        else
        {
        Log("NtQuerySymbolicLinkObject:\n");
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogNTStatus(result);
            if (returnedLength && (result == STATUS_BUFFER_TOO_SMALL))
            {
                Log("\tRequired Length=%d\n", *returnedLength);
            }
        }
        else
        {
            LogUnicodeString("Target", linkTarget);
        }
        LogCallingModule();
    }
    }

    return result;
}
DECLARE_FIXUP(NtQuerySymbolicLinkObjectImpl, NtQuerySymbolicLinkObjectFixup);

// NOTE: NtCreateKey is only documented; it has no declaration
NTSTATUS WINAPI NtCreateKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG TitleIndex,
    _In_ PUNICODE_STRING Class,
    _In_ ULONG CreateOptions,
    _Out_ PULONG Disposition);
auto NtCreateKeyImpl = WINTERNL_FUNCTION(NtCreateKey);
NTSTATUS __stdcall NtCreateKeyFixup(
    _Out_ PHANDLE keyHandle,
    _In_ ACCESS_MASK desiredAccess,
    _In_ POBJECT_ATTRIBUTES objectAttributes,
    _In_ ULONG titleIndex,
    _In_ PUNICODE_STRING objectClass,
    _In_ ULONG createOptions,
    _Out_ PULONG disposition)
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();
    auto result = NtCreateKeyImpl(keyHandle, desiredAccess, objectAttributes, titleIndex, objectClass, createOptions, disposition);

    auto functionResult = from_ntstatus(result);
    QueryPerformanceCounter(&TickEnd);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        if (output_method == trace_method::eventlog)
        {
            std::string inputs = "";
            std::string outputs = "";
            std::string results = "";

            inputs = InterpretUnicodeString("Class", objectClass); 
            inputs += "\n" + InterpretObjectAttributes(objectAttributes);
            inputs += "\n" + InterpretRegKeyAccess(desiredAccess);
            inputs += "\n" + InterpretRegKeyFlags(createOptions);

            results = InterpretReturnNT(functionResult, result).c_str();
            if (function_failed(functionResult))
            {
                outputs +=  InterpretNTStatus(result);                
            }
            else if (disposition)
            {
                outputs +=  InterpretRegKeyDisposition(*disposition);
            }

            std::ostringstream sout;
            InterpretCallingModulePart1()
                sout << InterpretCallingModulePart2()
                InterpretCallingModulePart3()
                std::string cm = sout.str();

            Log_ETW_PostMsgOperationA("NtCreateKey", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);
        }
        else
        {
        Log("NtCreateKey:\n");
        LogObjectAttributes(objectAttributes);
        LogRegKeyAccess(desiredAccess);
        LogUnicodeString("Class", objectClass);
        LogRegKeyFlags(createOptions);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogNTStatus(result);
        }
        else if (disposition)
        {
            LogRegKeyDisposition(*disposition);
        }
        LogCallingModule();
    }
    }

    return result;
}
DECLARE_FIXUP(NtCreateKeyImpl, NtCreateKeyFixup);

// NOTE: NtOpenKey is only documented; it has no declaration
NTSTATUS WINAPI NtOpenKey(_Out_ PHANDLE KeyHandle, _In_ ACCESS_MASK DesiredAccess, _In_ POBJECT_ATTRIBUTES ObjectAttributes);
auto NtOpenKeyImpl = WINTERNL_FUNCTION(NtOpenKey);
NTSTATUS __stdcall NtOpenKeyFixup(_Out_ PHANDLE keyHandle, _In_ ACCESS_MASK desiredAccess, _In_ POBJECT_ATTRIBUTES objectAttributes)
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();
    auto result = NtOpenKeyImpl(keyHandle, desiredAccess, objectAttributes);

    auto functionResult = from_ntstatus(result);
    QueryPerformanceCounter(&TickEnd);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        if (output_method == trace_method::eventlog)
        {
            std::string inputs = "";
            std::string outputs = "";
            std::string results = "";

            inputs = InterpretObjectAttributes(objectAttributes);
            inputs += "\n" + InterpretRegKeyAccess(desiredAccess);

            results = InterpretReturnNT(functionResult, result).c_str();
            if (function_failed(functionResult))
            {
                outputs += InterpretNTStatus(result);
            }
            else 
            {
                outputs += InterpretAsHex("Handle", keyHandle);
            }

            std::ostringstream sout;
            InterpretCallingModulePart1()
                sout << InterpretCallingModulePart2()
                InterpretCallingModulePart3()
                std::string cm = sout.str();

            Log_ETW_PostMsgOperationA("NtOpenKey", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);
        }
        else
        {
        Log("NtOpenKey:\n");
        LogObjectAttributes(objectAttributes);
        LogRegKeyAccess(desiredAccess);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogNTStatus(result);
        }
        LogCallingModule();
    }
    }

    return result;
}
DECLARE_FIXUP(NtOpenKeyImpl, NtOpenKeyFixup);

// NOTE: NtOpenKeyEx is only documented; it has no declaration
NTSTATUS WINAPI NtOpenKeyEx(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG OpenOptions);
auto NtOpenKeyExImpl = WINTERNL_FUNCTION(NtOpenKeyEx);
NTSTATUS __stdcall NtOpenKeyExFixup(
    _Out_ PHANDLE keyHandle,
    _In_ ACCESS_MASK desiredAccess,
    _In_ POBJECT_ATTRIBUTES objectAttributes,
    _In_ ULONG openOptions)
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();
    auto result = NtOpenKeyExImpl(keyHandle, desiredAccess, objectAttributes, openOptions);

    auto functionResult = from_ntstatus(result);
    QueryPerformanceCounter(&TickEnd);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        if (output_method == trace_method::eventlog)
        {
            std::string inputs = "";
            std::string outputs = "";
            std::string results = "";

            inputs = InterpretObjectAttributes(objectAttributes);
            inputs += "\n" + InterpretRegKeyAccess(desiredAccess);
            inputs += "\n" + InterpretRegKeyFlags(openOptions);

            results = InterpretReturnNT(functionResult, result).c_str();
            if (function_failed(functionResult))
            {
                outputs += InterpretNTStatus(result);
            }
            else
            {
                outputs += InterpretAsHex("Handle", keyHandle);
            }

            std::ostringstream sout;
            InterpretCallingModulePart1()
                sout << InterpretCallingModulePart2()
                InterpretCallingModulePart3()
                std::string cm = sout.str();

            Log_ETW_PostMsgOperationA("NtOpenKeyEx", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);
        }
        else
        {
        Log("NtOpenKey:\n");
        LogObjectAttributes(objectAttributes);
        LogRegKeyAccess(desiredAccess);
        LogRegKeyFlags(openOptions);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogNTStatus(result);
        }
        LogCallingModule();
    }
    }

    return result;
}
DECLARE_FIXUP(NtOpenKeyExImpl, NtOpenKeyExFixup);

// NOTE: NtSetValueKey is only documented; it has no declaration
NTSTATUS WINAPI NtSetValueKey(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING ValueName,
    _In_ ULONG TitleIndex,
    _In_ ULONG Type,
    _In_ PVOID Data,
    _In_ ULONG DataSize);
auto NtSetValueKeyImpl = WINTERNL_FUNCTION(NtSetValueKey);
NTSTATUS __stdcall NtSetValueKeyFixup(
    _In_ HANDLE keyHandle,
    _In_ PUNICODE_STRING valueName,
    _In_ ULONG titleIndex,
    _In_ ULONG type,
    _In_ PVOID data,
    _In_ ULONG dataSize)
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();
    auto result = NtSetValueKeyImpl(keyHandle, valueName, titleIndex, type, data, dataSize);

    auto functionResult = from_ntstatus(result);
    QueryPerformanceCounter(&TickEnd);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        if (output_method == trace_method::eventlog)
        {
            std::string inputs = "";
            std::string outputs = "";
            std::string results = "";

            inputs = InterpretAsHex("Handle", keyHandle); 
            inputs += "\n" + InterpretUnicodeString("Value Name", valueName); 
            inputs += "\n" + InterpretRegKeyType(type);
            inputs += "\n" + InterpretRegValueA<char>(type, data, dataSize);

            results = InterpretReturnNT(functionResult, result).c_str();
            if (function_failed(functionResult))
            {
                outputs += InterpretNTStatus(result);
            }

            std::ostringstream sout;
            InterpretCallingModulePart1()
                sout << InterpretCallingModulePart2()
                InterpretCallingModulePart3()
                std::string cm = sout.str();

            Log_ETW_PostMsgOperationA("NtSetValueKey", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);
        }
        else
        {
        Log("NtSetValueKey:\n");
        // TODO: Translate keyHandle to a name?
        LogUnicodeString("Value Name", valueName);
        LogRegKeyType(type);
        LogRegValue<wchar_t>(type, data, dataSize);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogNTStatus(result);
        }
        LogCallingModule();
    }
    }

    return result;
}
DECLARE_FIXUP(NtSetValueKeyImpl, NtSetValueKeyFixup);

// NOTE: NtQueryValueKey is only documented; it has no declaration
NTSTATUS __stdcall NtQueryValueKeyFixup(
    _In_ HANDLE keyHandle,
    _In_ PUNICODE_STRING valueName,
    _In_ winternl::KEY_VALUE_INFORMATION_CLASS keyValueInformationClass,
    _Out_ PVOID keyValueInformation,
    _In_ ULONG length,
    _Out_ PULONG resultLength)
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();
    auto result = impl::NtQueryValueKey(keyHandle, valueName, keyValueInformationClass, keyValueInformation, length, resultLength);

    auto functionResult = from_ntstatus(result);
    QueryPerformanceCounter(&TickEnd);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        if (output_method == trace_method::eventlog)
        {
            std::string inputs = "";
            std::string outputs = "";
            std::string results = "";

            inputs = InterpretAsHex("Handle", keyHandle);
            inputs += "\n" + InterpretUnicodeString("Value Name", valueName);
            inputs += "\nKeyValueInformationClass" + InterpretKeyValueInformationClass(keyValueInformationClass);
            inputs += "(" + InterpretAsHex("", (DWORD)keyValueInformationClass) + ")";

            results = InterpretReturnNT(functionResult, result).c_str();
            if (function_failed(functionResult))
            {
                outputs +=  InterpretNTStatus(result);
                if ((result == STATUS_BUFFER_OVERFLOW) || (result == STATUS_BUFFER_TOO_SMALL))
                {
                    outputs += "\n" + InterpretAsHex("Required Length", *resultLength);
                }
            }
            else
            {
                auto align64 = [](auto ptr)
                {
                    auto ptrValue = reinterpret_cast<std::uintptr_t>(ptr);
                    ptrValue += 63;
                    ptrValue &= ~0x3F;
                    return reinterpret_cast<decltype(ptr)>(ptr);
                };

                std::wstring_view name;
                ULONG type = 0;
                const void* data = nullptr;
                std::size_t dataSize = 0;

                switch (keyValueInformationClass)
                {
                case winternl::KeyValueBasicInformation:
                {
                    auto info = reinterpret_cast<const winternl::KEY_VALUE_BASIC_INFORMATION*>(keyValueInformation);
                    name = { info->Name, info->NameLength / 2 };
                    type = info->Type;
                } break;

                case winternl::KeyValueFullInformation:
                case winternl::KeyValueFullInformationAlign64:
                {
                    auto info = reinterpret_cast<const winternl::KEY_VALUE_FULL_INFORMATION*>(keyValueInformation);
                    if (keyValueInformationClass == winternl::KeyValueFullInformationAlign64)
                    {
                        info = align64(info);
                    }
                    name = { info->Name, info->NameLength / 2 };
                    type = info->Type;
                    data = reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(info) + info->DataOffset);
                    dataSize = info->DataLength;
                } break;

                case winternl::KeyValuePartialInformation:
                case winternl::KeyValuePartialInformationAlign64:
                {
                    auto info = reinterpret_cast<const winternl::KEY_VALUE_PARTIAL_INFORMATION*>(keyValueInformation);
                    if (keyValueInformationClass == winternl::KeyValuePartialInformationAlign64)
                    {
                        info = align64(info);
                    }
                    type = info->Type;
                    data = info->Data;
                    dataSize = info->DataLength;
                } break;

                default: // Invalid or undocumented
                    break;
                }

                if (!name.empty()) 
                    outputs +=  InterpretCountedString("Name", name.data(), name.length()) + "\n";
                outputs += InterpretRegKeyType(type);
                if (data)
                {
                    outputs += "\n" + InterpretRegValueA<wchar_t>(type, data, dataSize); 

                }
            }

            std::ostringstream sout;
            InterpretCallingModulePart1()
                sout << InterpretCallingModulePart2()
                InterpretCallingModulePart3()
                std::string cm = sout.str();

            Log_ETW_PostMsgOperationA("NtQueryValueKey", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);
        }
        else
        {
        Log("NtQueryValueKey:\n");
        LogUnicodeString("Value Name", valueName);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogNTStatus(result);
            if ((result == STATUS_BUFFER_OVERFLOW) || (result == STATUS_BUFFER_TOO_SMALL))
            {
                Log("\tRequired Length=%d\n", *resultLength);
            }
        }
        else
        {
            auto align64 = [](auto ptr)
            {
                auto ptrValue = reinterpret_cast<std::uintptr_t>(ptr);
                ptrValue += 63;
                ptrValue &= ~0x3F;
                return reinterpret_cast<decltype(ptr)>(ptr);
            };

            std::wstring_view name;
            ULONG type = 0;
            const void* data = nullptr;
            std::size_t dataSize = 0;

            switch (keyValueInformationClass)
            {
            case winternl::KeyValueBasicInformation:
            {
                auto info = reinterpret_cast<const winternl::KEY_VALUE_BASIC_INFORMATION*>(keyValueInformation);
                name = { info->Name, info->NameLength / 2 };
                type = info->Type;
            } break;

            case winternl::KeyValueFullInformation:
            case winternl::KeyValueFullInformationAlign64:
            {
                auto info = reinterpret_cast<const winternl::KEY_VALUE_FULL_INFORMATION*>(keyValueInformation);
                if (keyValueInformationClass == winternl::KeyValueFullInformationAlign64)
                {
                    info = align64(info);
                }
                name = { info->Name, info->NameLength / 2 };
                type = info->Type;
                data = reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(info) + info->DataOffset);
                dataSize = info->DataLength;
            } break;

            case winternl::KeyValuePartialInformation:
            case winternl::KeyValuePartialInformationAlign64:
            {
                auto info = reinterpret_cast<const winternl::KEY_VALUE_PARTIAL_INFORMATION*>(keyValueInformation);
                if (keyValueInformationClass == winternl::KeyValuePartialInformationAlign64)
                {
                    info = align64(info);
                }
                type = info->Type;
                data = info->Data;
                dataSize = info->DataLength;
            } break;

            default: // Invalid or undocumented
                break;
            }

            if (!name.empty()) LogCountedString("Name", name.data(), name.length());
            LogRegKeyType(type);
            if (data)
            {
                LogRegValue<wchar_t>(type, data, dataSize);
            }
        }
        LogCallingModule();
    }
    }

    return result;
}
DECLARE_FIXUP(impl::NtQueryValueKey, NtQueryValueKeyFixup);
