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
inline std::string InterpretObjectAttributes(ULONG attributes)    
{    
	std::ostringstream sout;    
	sout << InterpretAsHex("Object Attributes", attributes);    
    
	if (attributes)    
	{    
		sout << " (";    
		const char* prefix = "";    
		    
		if (IsFlagSet(attributes, OBJ_INHERIT))    
		{    
			sout << prefix << "OBJ_INHERIT";    
			prefix = " | ";    
		}    
		if (IsFlagSet(attributes, OBJ_PERMANENT))    
		{    
			sout << prefix << "OBJ_PERMANENT";    
			prefix = " | ";    
		}    
		if (IsFlagSet(attributes, OBJ_EXCLUSIVE))    
		{    
			sout << prefix << "OBJ_EXCLUSIVE";    
			prefix = " | ";    
		}    
		if (IsFlagSet(attributes, OBJ_CASE_INSENSITIVE))    
		{    
			sout << prefix << "OBJ_CASE_INSENSITIVE";    
			prefix = " | ";    
		}    
		if (IsFlagSet(attributes, OBJ_OPENIF))    
		{    
			sout << prefix << "OBJ_OPENIF";    
			prefix = " | ";    
		}    
		if (IsFlagSet(attributes, OBJ_OPENLINK))    
		{    
			sout << prefix << "OBJ_OPENLINK";    
			prefix = " | ";    
		}    
		if (IsFlagSet(attributes, OBJ_KERNEL_HANDLE))    
		{    
			sout << prefix << "OBJ_KERNEL_HANDLE";    
			prefix = " | ";    
		}    
		if (IsFlagSet(attributes, OBJ_FORCE_ACCESS_CHECK))    
		{    
			sout << prefix << "OBJ_FORCE_ACCESS_CHECK";    
			prefix = " | ";    
		}    
		if (IsFlagSet(attributes, OBJ_IGNORE_IMPERSONATED_DEVICEMAP))    
		{    
			sout << prefix << "OBJ_IGNORE_IMPERSONATED_DEVICEMAP";    
			prefix = " | ";    
		}    
		if (IsFlagSet(attributes, OBJ_DONT_REPARSE))    
		{    
			sout << prefix << "OBJ_DONT_REPARSE";    
			prefix = " | ";    
		}    
		sout << ")";    
	}    
	return sout.str();    
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
inline std::string InterpretCreationDispositionInternal(ULONG creationDisposition)    
{    
	std::ostringstream sout;    
	sout << InterpretAsHex("Creation Disposition",creationDisposition);    
	sout << " (";    
    
	if (IsFlagEqual(creationDisposition, FILE_SUPERSEDE))    
		sout << "FILE_SUPERSEDE";    
	else if (IsFlagEqual(creationDisposition, FILE_CREATE))    
		sout << "FILE_CREATE";    
	else if (IsFlagEqual(creationDisposition, FILE_OPEN))    
		sout << "FILE_OPEN";    
	else if (IsFlagEqual(creationDisposition, FILE_OPEN_IF))    
		sout << "FILE_OPEN_IF";    
	else if (IsFlagEqual(creationDisposition, FILE_OVERWRITE))    
		sout << "FILE_OVERWRITE";    
	else if (IsFlagEqual(creationDisposition, FILE_OVERWRITE_IF))    
		sout << "FILE_OVERWRITE_IF";    
	else    
		sout << "Unknown";    
	sout << ")";    
	return sout.str();    
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
inline std::string InterpretFileCreateOptions(ULONG options)    
{    
	std::ostringstream sout;    
	sout << InterpretAsHex("Options", options);    
	if (options)    
	{    
		sout << " (";    
		const char* prefix = "";    
    
		if (IsFlagSet(options, FILE_DIRECTORY_FILE))    
		{    
			sout << prefix << "FILE_DIRECTORY_FILE";    
			prefix = " | ";    
		}    
		if (IsFlagSet(options, FILE_NON_DIRECTORY_FILE))    
		{    
			sout << prefix << "FILE_NON_DIRECTORY_FILE";    
			prefix = " | ";    
		}    
		if (IsFlagSet(options, FILE_WRITE_THROUGH))    
		{    
			sout << prefix << "FILE_WRITE_THROUGH";    
			prefix = " | ";    
		}    
		if (IsFlagSet(options, FILE_SEQUENTIAL_ONLY))    
		{    
			sout << prefix << "FILE_SEQUENTIAL_ONLY";    
			prefix = " | ";    
		}    
		if (IsFlagSet(options, FILE_NO_INTERMEDIATE_BUFFERING))    
		{    
			sout << prefix << "FILE_NO_INTERMEDIATE_BUFFERING";    
			prefix = " | ";    
		}    
		if (IsFlagSet(options, FILE_SYNCHRONOUS_IO_ALERT))    
		{    
			sout << prefix << "FILE_SYNCHRONOUS_IO_ALERT";    
			prefix = " | ";    
		}    
		if (IsFlagSet(options, FILE_SYNCHRONOUS_IO_NONALERT))    
		{    
			sout << prefix << "FILE_SYNCHRONOUS_IO_NONALERT";    
			prefix = " | ";    
		}    
		if (IsFlagSet(options, FILE_CREATE_TREE_CONNECTION))    
		{    
			sout << prefix << "FILE_CREATE_TREE_CONNECTION";    
			prefix = " | ";    
		}    
		if (IsFlagSet(options, FILE_COMPLETE_IF_OPLOCKED))    
		{    
			sout << prefix << "FILE_COMPLETE_IF_OPLOCKED";    
			prefix = " | ";    
		}    
		if (IsFlagSet(options, FILE_NO_EA_KNOWLEDGE))    
		{    
			sout << prefix << "FILE_NO_EA_KNOWLEDGE";    
			prefix = " | ";    
		}    
		if (IsFlagSet(options, FILE_OPEN_REMOTE_INSTANCE))    
		{    
			sout << prefix << "FILE_OPEN_REMOTE_INSTANCE";    
			prefix = " | ";    
		}    
		if (IsFlagSet(options, FILE_RANDOM_ACCESS))    
		{    
			sout << prefix << "FILE_RANDOM_ACCESS";    
			prefix = " | ";    
		}    
		if (IsFlagSet(options, FILE_DELETE_ON_CLOSE))    
		{    
			sout << prefix << "FILE_DELETE_ON_CLOSE";    
			prefix = " | ";    
		}    
		if (IsFlagSet(options, FILE_OPEN_BY_FILE_ID))    
		{    
			sout << prefix << "FILE_OPEN_BY_FILE_ID";    
			prefix = " | ";    
		};    
		if (IsFlagSet(options, FILE_OPEN_FOR_BACKUP_INTENT))    
		{    
			sout << prefix << "FILE_OPEN_FOR_BACKUP_INTENT";    
			prefix = " | ";    
		}    
		if (IsFlagSet(options, FILE_NO_COMPRESSION))    
		{    
			sout << prefix << "FILE_NO_COMPRESSION";    
			prefix = " | ";    
		}    
		if (IsFlagSet(options, FILE_OPEN_REQUIRING_OPLOCK))    
		{    
			sout << prefix << "FILE_OPEN_REQUIRING_OPLOCK";    
			prefix = " | ";    
		}    
		if (IsFlagSet(options, FILE_SUPPORTS_ENCRYPTION))    
		{    
			sout << prefix << "FILE_SUPPORTS_ENCRYPTION";    
			prefix = " | ";    
		}    
		if (IsFlagSet(options, FILE_NAMED_STREAMS))    
		{    
			sout << prefix << "FILE_NAMED_STREAMS";    
			prefix = " | ";    
		}    
		if (IsFlagSet(options, FILE_READ_ONLY_VOLUME))    
		{    
			sout << prefix << "FILE_READ_ONLY_VOLUME";    
			prefix = " | ";    
		}    
    
		if (IsFlagSet(options, FILE_RESERVE_OPFILTER))    
		{    
			sout << prefix << "FILE_RESERVE_OPFILTER";    
			prefix = " | ";    
		}    
		if (IsFlagSet(options, FILE_OPEN_REPARSE_POINT))    
		{    
			sout << prefix << "FILE_OPEN_REPARSE_POINT";    
			prefix = " | ";    
		}    
		if (IsFlagSet(options, FILE_OPEN_NO_RECALL))    
		{    
			sout << prefix << "FILE_OPEN_NO_RECALL";    
			prefix = " | ";    
		}    
		if (IsFlagSet(options, FILE_OPEN_FOR_FREE_SPACE_QUERY))    
		{    
			sout << prefix << "FILE_OPEN_FOR_FREE_SPACE_QUERY";    
			prefix = " | ";    
		}    
    
		sout << ")";    
	}    
	return sout.str();    
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
inline std::string InterpretDirectoryAccessInternal(ACCESS_MASK access, const char* msg = "Access")    
{    
	std::ostringstream sout;    
	sout << InterpretAsHex(msg, access);    
	if (access)    
	{    
		// NOTE: Only documented; definitions don't exist anywhere    
		constexpr ACCESS_MASK DIRECTORY_QUERY = 0x0001;    
		constexpr ACCESS_MASK DIRECTORY_TRAVERSE = 0x0002;    
		constexpr ACCESS_MASK DIRECTORY_CREATE_OBJECT = 0x0004;    
		constexpr ACCESS_MASK DIRECTORY_CREATE_SUBDIRECTORY = 0x0008;    
    
		const char* prefix = "";    
		sout << " (";    
    
		// Specific rights    
		if (IsFlagSet(access, DIRECTORY_QUERY))    
		{    
			sout << prefix << "DIRECTORY_QUERY";    
			prefix = " | ";    
		}    
		if (IsFlagSet(access, DIRECTORY_TRAVERSE))    
		{    
			sout << prefix << "DIRECTORY_TRAVERSE";    
			prefix = " | ";    
		}    
		if (IsFlagSet(access, DIRECTORY_CREATE_OBJECT))    
		{    
			sout << prefix << "DIRECTORY_CREATE_OBJECT";    
			prefix = " | ";    
		}    
		if (IsFlagSet(access, DIRECTORY_CREATE_SUBDIRECTORY))    
		{    
			sout << prefix << "DIRECTORY_CREATE_SUBDIRECTORY";    
			prefix = " | ";    
		}    
    
		sout << InterpretCommonAccess(access, prefix);    
    
		sout << ")";    
	}    
    
	return sout.str();    
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
inline std::string InterpretUnicodeString(const char* msg, const PUNICODE_STRING string)    
{    
	std::ostringstream olog;    
	    
	if (string->Buffer)    
		//olog << msg << "=" << string->Buffer;     
	    olog << InterpretCountedString(msg, string->Buffer, string->Length / 2);    
	else    
		olog << msg << "=";    
	return olog.str();    
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
inline std::string InterpretObjectAttributes(POBJECT_ATTRIBUTES objectAttributes)    
{    
	std::ostringstream olog;    
    
	// Get the root directory/registry/etc. path that the ObjectName is relative to    
	std::wstring rootDir;    
	if (objectAttributes->RootDirectory && (objectAttributes->RootDirectory != INVALID_HANDLE_VALUE))    
	{    
		TryGetDirectoryPath(objectAttributes->RootDirectory, rootDir) ||    
			TryGetKeyPath(objectAttributes->RootDirectory, rootDir);    
	}    
    
	olog << InterpretUnicodeString("Path", objectAttributes->ObjectName);    
	if (!rootDir.empty())     
		olog << "\nRoot=" << InterpretStringA(rootDir.c_str());    
	olog << "\n" << InterpretObjectAttributes(objectAttributes->Attributes);    
	return olog.str();    
}    
    
inline std::string InterpretKeyValueInformationClass(winternl::KEY_VALUE_INFORMATION_CLASS infoclass)    
{    
	switch (infoclass)    
	{    
	case winternl::KeyValueBasicInformation:    
		return "KeyValueBasicInformation";    
	case winternl::KeyValueFullInformation:    
		return "KeyValueFullInformation";    
	case winternl::KeyValueFullInformationAlign64:    
		return "KeyValueFullInformationAlign64";    
	case winternl::KeyValuePartialInformation:    
		return "KeyValuePartialInformation";    
	case winternl::KeyValuePartialInformationAlign64:    
		return "KeyValuePartialInformationAlign64";    
	default:    
		return "unknown";    
	}    
}