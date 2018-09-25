//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "FunctionImplementations.h"
#include "Logging.h"

inline void LogObjectAttributes(ULONG attributes)
{
    Log("\tObject Attributes=%08X", attributes);
    if (attributes)
    {
        const char* prefix = "";
        Log(" (");
        LogIfFlagSet(attributes, OBJ_INHERIT);
        LogIfFlagSet(attributes, OBJ_PERMANENT);
        LogIfFlagSet(attributes, OBJ_EXCLUSIVE);
        LogIfFlagSet(attributes, OBJ_CASE_INSENSITIVE);
        LogIfFlagSet(attributes, OBJ_OPENIF);
        LogIfFlagSet(attributes, OBJ_OPENLINK);
        LogIfFlagSet(attributes, OBJ_KERNEL_HANDLE);
        LogIfFlagSet(attributes, OBJ_FORCE_ACCESS_CHECK);
        LogIfFlagSet(attributes, OBJ_IGNORE_IMPERSONATED_DEVICEMAP);
        LogIfFlagSet(attributes, OBJ_DONT_REPARSE);
        Log(")");
    }

    Log("\n");
}

inline void LogCreationDispositionInternal(ULONG creationDisposition)
{
    Log("\tCreation Disposition=%d (", creationDisposition);
    LogIfEqual(creationDisposition, FILE_SUPERSEDE)
    else LogIfEqual(creationDisposition, FILE_CREATE)
    else LogIfEqual(creationDisposition, FILE_OPEN)
    else LogIfEqual(creationDisposition, FILE_OPEN_IF)
    else LogIfEqual(creationDisposition, FILE_OVERWRITE)
    else LogIfEqual(creationDisposition, FILE_OVERWRITE_IF)
    else Log("Unknown");
    Log(")\n");
}

inline void LogFileCreateOptions(ULONG options)
{
    Log("\tOptions=%08X", options);
    if (options)
    {
        const char* prefix = "";
        Log(" (");
        LogIfFlagSet(options, FILE_DIRECTORY_FILE);
        LogIfFlagSet(options, FILE_WRITE_THROUGH);
        LogIfFlagSet(options, FILE_SEQUENTIAL_ONLY);
        LogIfFlagSet(options, FILE_NO_INTERMEDIATE_BUFFERING);
        LogIfFlagSet(options, FILE_SYNCHRONOUS_IO_ALERT);
        LogIfFlagSet(options, FILE_SYNCHRONOUS_IO_NONALERT);
        LogIfFlagSet(options, FILE_NON_DIRECTORY_FILE);
        LogIfFlagSet(options, FILE_CREATE_TREE_CONNECTION);
        LogIfFlagSet(options, FILE_COMPLETE_IF_OPLOCKED);
        LogIfFlagSet(options, FILE_NO_EA_KNOWLEDGE);
        LogIfFlagSet(options, FILE_OPEN_REMOTE_INSTANCE);
        LogIfFlagSet(options, FILE_RANDOM_ACCESS);
        LogIfFlagSet(options, FILE_DELETE_ON_CLOSE);
        LogIfFlagSet(options, FILE_OPEN_BY_FILE_ID);
        LogIfFlagSet(options, FILE_OPEN_FOR_BACKUP_INTENT);
        LogIfFlagSet(options, FILE_NO_COMPRESSION);
        LogIfFlagSet(options, FILE_OPEN_REQUIRING_OPLOCK);
        LogIfFlagSet(options, FILE_RESERVE_OPFILTER);
        LogIfFlagSet(options, FILE_OPEN_REPARSE_POINT);
        LogIfFlagSet(options, FILE_OPEN_NO_RECALL);
        LogIfFlagSet(options, FILE_OPEN_FOR_FREE_SPACE_QUERY);
        Log(")");
    }

    Log("\n");
}

inline void LogDirectoryAccessInternal(ACCESS_MASK access, const char* msg = "Access")
{
    Log("\t%s=%08X", msg, access);
    if (access)
    {
        // NOTE: Only documented; definitions don't exist anywhere
        constexpr ACCESS_MASK DIRECTORY_QUERY = 0x0001;
        constexpr ACCESS_MASK DIRECTORY_TRAVERSE = 0x0002;
        constexpr ACCESS_MASK DIRECTORY_CREATE_OBJECT = 0x0004;
        constexpr ACCESS_MASK DIRECTORY_CREATE_SUBDIRECTORY = 0x0008;

        const char* prefix = "";
        Log(" (");

        // Specific rights
        LogIfFlagSet(access, DIRECTORY_QUERY);
        LogIfFlagSet(access, DIRECTORY_TRAVERSE);
        LogIfFlagSet(access, DIRECTORY_CREATE_OBJECT);
        LogIfFlagSet(access, DIRECTORY_CREATE_SUBDIRECTORY);

        LogCommonAccess(access, prefix);

        Log(")");
    }

    Log("\n");
}

inline void LogUnicodeString(const char* msg, const PUNICODE_STRING string)
{
    Log("\t%s=", msg);
    if (string->Buffer)
    {
        Log("%.*ls", string->Length / 2, string->Buffer);
    }

    Log("\n");
}

inline bool TryGetDirectoryPath(HANDLE dir, std::wstring& result)
{
    result.clear();

    if (auto size = ::GetFinalPathNameByHandleW(dir, nullptr, 0, 0))
    {
        result.resize(size - 1, '\0');
        size = ::GetFinalPathNameByHandleW(dir, result.data(), size, 0);
        assert(size == result.length());
    }

    return !result.empty();
}

inline bool TryGetKeyPath(HANDLE key, std::wstring& result)
{
    result.clear();

    ULONG size;
    if (auto status = impl::NtQueryKey(key, winternl::KeyNameInformation, nullptr, 0, &size);
        (status == STATUS_BUFFER_TOO_SMALL) || (status == STATUS_BUFFER_OVERFLOW))
    {
        auto buffer = std::make_unique<std::uint8_t[]>(size);
        if (NT_SUCCESS(impl::NtQueryKey(key, winternl::KeyNameInformation, buffer.get(), size, &size)))
        {
            auto info = reinterpret_cast<winternl::PKEY_NAME_INFORMATION>(buffer.get());
            result.assign(info->Name, info->NameLength / 2);
        }
    }

    return !result.empty();
}

inline void LogObjectAttributes(POBJECT_ATTRIBUTES objectAttributes)
{
    // Get the root directory/registry/etc. path that the ObjectName is relative to
    std::wstring rootDir;
    if (objectAttributes->RootDirectory && (objectAttributes->RootDirectory != INVALID_HANDLE_VALUE))
    {
        TryGetDirectoryPath(objectAttributes->RootDirectory, rootDir) ||
        TryGetKeyPath(objectAttributes->RootDirectory, rootDir);
    }

    LogUnicodeString("Path", objectAttributes->ObjectName);
    if (!rootDir.empty()) LogString("Root", rootDir.c_str());
    LogObjectAttributes(objectAttributes->Attributes);
}
