//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <shim_framework.h>

#include "Config.h"
#include "Logging.h"
#include "PreserveError.h"

#pragma comment(lib, "Lz32.lib")

using namespace std::literals;

auto CreateFileImpl = shims::detoured_string_function(&::CreateFileA, &::CreateFileW);
template <typename CharT>
HANDLE __stdcall CreateFileShim(
    _In_ const CharT* fileName,
    _In_ DWORD desiredAccess,
    _In_ DWORD shareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES securityAttributes,
    _In_ DWORD creationDisposition,
    _In_ DWORD flagsAndAttributes,
    _In_opt_ HANDLE templateFile)
{
    auto entry = LogFunctionEntry();
    auto result = CreateFileImpl(fileName, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(result != INVALID_HANDLE_VALUE);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
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

    return result;
}
DECLARE_STRING_SHIM(CreateFileImpl, CreateFileShim);

auto CreateFile2Impl = &::CreateFile2;
HANDLE __stdcall CreateFile2Shim(
    _In_ LPCWSTR fileName,
    _In_ DWORD desiredAccess,
    _In_ DWORD shareMode,
    _In_ DWORD creationDisposition,
    _In_opt_ LPCREATEFILE2_EXTENDED_PARAMETERS createExParams)
{
    auto entry = LogFunctionEntry();
    auto result = CreateFile2Impl(fileName, desiredAccess, shareMode, creationDisposition, createExParams);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(result != INVALID_HANDLE_VALUE);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
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

    return result;
}
DECLARE_SHIM(CreateFile2Impl, CreateFile2Shim);

auto CopyFileImpl = shims::detoured_string_function(&::CopyFileA, &::CopyFileW);
template <typename CharT>
BOOL __stdcall CopyFileShim(_In_ const CharT* existingFileName, _In_ const CharT* newFileName, _In_ BOOL failIfExists)
{
    auto entry = LogFunctionEntry();
    auto result = CopyFileImpl(existingFileName, newFileName, failIfExists);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(result);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
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

    return result;
}
DECLARE_STRING_SHIM(CopyFileImpl, CopyFileShim);

auto CopyFile2Impl = &::CopyFile2;
HRESULT __stdcall CopyFile2Shim(
    _In_ PCWSTR existingFileName,
    _In_ PCWSTR newFileName,
    _In_opt_ COPYFILE2_EXTENDED_PARAMETERS* extendedParameters)
{
    auto entry = LogFunctionEntry();
    auto result = CopyFile2Impl(existingFileName, newFileName, extendedParameters);

    auto functionResult = from_hresult(result);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
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

    return result;
}
DECLARE_SHIM(CopyFile2Impl, CopyFile2Shim);

auto CopyFileExImpl = shims::detoured_string_function(&::CopyFileExA, &::CopyFileExW);
template <typename CharT>
BOOL __stdcall CopyFileExShim(
    _In_ const CharT* existingFileName,
    _In_ const CharT* newFileName,
    _In_opt_ LPPROGRESS_ROUTINE progressRoutine,
    _In_opt_ LPVOID data,
    _In_opt_ LPBOOL cancel,
    _In_ DWORD copyFlags)
{
    auto entry = LogFunctionEntry();
    auto result = CopyFileExImpl(existingFileName, newFileName, progressRoutine, data, cancel, copyFlags);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(result);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
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

    return result;
}
DECLARE_STRING_SHIM(CopyFileExImpl, CopyFileExShim);

auto CreateHardLinkImpl = shims::detoured_string_function(&::CreateHardLinkA, &::CreateHardLinkW);
template <typename CharT>
BOOL __stdcall CreateHardLinkShim(
    _In_ const CharT* fileName,
    _In_ const CharT* existingFileName,
    _Reserved_ LPSECURITY_ATTRIBUTES securityAttributes)
{
    auto entry = LogFunctionEntry();
    auto result = CreateHardLinkImpl(fileName, existingFileName, securityAttributes);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(result);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
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

    return result;
}
DECLARE_STRING_SHIM(CreateHardLinkImpl, CreateHardLinkShim);

auto CreateSymbolicLinkImpl = shims::detoured_string_function(&::CreateSymbolicLinkA, &::CreateSymbolicLinkW);
template <typename CharT>
BOOLEAN __stdcall CreateSymbolicLinkShim(
    _In_ const CharT* symlinkFileName,
    _In_ const CharT* targetFileName,
    _In_ DWORD flags)
{
    auto entry = LogFunctionEntry();
    auto result = CreateSymbolicLinkImpl(symlinkFileName, targetFileName, flags);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(result);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
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

    return result;
}
DECLARE_STRING_SHIM(CreateSymbolicLinkImpl, CreateSymbolicLinkShim);

auto DeleteFileImpl = shims::detoured_string_function(&::DeleteFileA, &::DeleteFileW);
template <typename CharT>
BOOL __stdcall DeleteFileShim(_In_ const CharT* fileName)
{
    auto entry = LogFunctionEntry();
    auto result = DeleteFileImpl(fileName);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(result);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
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

    return result;
}
DECLARE_STRING_SHIM(DeleteFileImpl, DeleteFileShim);

auto MoveFileImpl = shims::detoured_string_function(&::MoveFileA, &::MoveFileW);
template <typename CharT>
BOOL __stdcall MoveFileShim(_In_ const CharT* existingFileName, _In_ const CharT* newFileName)
{
    auto entry = LogFunctionEntry();
    auto result = MoveFileImpl(existingFileName, newFileName);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(result);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
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

    return result;
}
DECLARE_STRING_SHIM(MoveFileImpl, MoveFileShim);

auto MoveFileExImpl = shims::detoured_string_function(&::MoveFileExA, &::MoveFileExW);
template <typename CharT>
BOOL __stdcall MoveFileExShim(_In_ const CharT* existingFileName, _In_opt_ const CharT* newFileName, _In_ DWORD flags)
{
    auto entry = LogFunctionEntry();
    auto result = MoveFileExImpl(existingFileName, newFileName, flags);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(result);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
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

    return result;
}
DECLARE_STRING_SHIM(MoveFileExImpl, MoveFileExShim);

auto ReplaceFileImpl = shims::detoured_string_function(&::ReplaceFileA, &::ReplaceFileW);
template <typename CharT>
BOOL __stdcall ReplaceFileShim(
    _In_ const CharT* replacedFileName,
    _In_ const CharT* replacementFileName,
    _In_opt_ const CharT* backupFileName,
    _In_ DWORD replaceFlags,
    _Reserved_ LPVOID exclude,
    _Reserved_ LPVOID reserved)
{
    auto entry = LogFunctionEntry();
    auto result = ReplaceFileImpl(replacedFileName, replacementFileName, backupFileName, replaceFlags, exclude, reserved);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(result);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
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

    return result;
}
DECLARE_STRING_SHIM(ReplaceFileImpl, ReplaceFileShim);

template <typename CharT>
using win32_find_data_t = std::conditional_t<shims::is_ansi<CharT>, WIN32_FIND_DATAA, WIN32_FIND_DATAW>;

auto FindFirstFileImpl = shims::detoured_string_function(&::FindFirstFileA, &FindFirstFileW);
template <typename CharT>
HANDLE __stdcall FindFirstFileShim(
    _In_ const CharT* fileName,
    _Out_ win32_find_data_t<CharT>* findFileData)
{
    auto entry = LogFunctionEntry();
    auto result = FindFirstFileImpl(fileName, findFileData);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(result != INVALID_HANDLE_VALUE);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
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

    return result;
}
DECLARE_STRING_SHIM(FindFirstFileImpl, FindFirstFileShim);

auto FindFirstFileExImpl = shims::detoured_string_function(&::FindFirstFileExA, &::FindFirstFileExW);
template <typename CharT>
HANDLE __stdcall FindFirstFileExShim(
    _In_ const CharT* fileName,
    _In_ FINDEX_INFO_LEVELS infoLevelId,
    _Out_writes_bytes_(sizeof(win32_find_data_t<CharT>)) LPVOID findFileData,
    _In_ FINDEX_SEARCH_OPS searchOp,
    _Reserved_ LPVOID searchFilter,
    _In_ DWORD additionalFlags)
{
    auto entry = LogFunctionEntry();
    auto result = FindFirstFileExImpl(fileName, infoLevelId, findFileData, searchOp, searchFilter, additionalFlags);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(result != INVALID_HANDLE_VALUE);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
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

    return result;
}
DECLARE_STRING_SHIM(FindFirstFileExImpl, FindFirstFileExShim);

auto FindNextFileImpl = shims::detoured_string_function(&::FindNextFileA, &::FindNextFileW);
template <typename CharT>
BOOL __stdcall FindNextFileShim(_In_ HANDLE findFile, _Out_ win32_find_data_t<CharT>* findFileData)
{
    auto entry = LogFunctionEntry();
    auto result = FindNextFileImpl(findFile, findFileData);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(result || (::GetLastError() == ERROR_NO_MORE_FILES));
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
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

    return result;
}
DECLARE_STRING_SHIM(FindNextFileImpl, FindNextFileShim);

auto FindCloseImpl = &::FindClose;
BOOL __stdcall FindCloseShim(_Inout_ HANDLE findFile)
{
    auto entry = LogFunctionEntry();
    auto result = FindCloseImpl(findFile);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(result);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
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

    return result;
}
DECLARE_SHIM(FindCloseImpl, FindCloseShim);

auto CreateDirectoryImpl = shims::detoured_string_function(&::CreateDirectoryA, &::CreateDirectoryW);
template <typename CharT>
BOOL __stdcall CreateDirectoryShim(_In_ const CharT* pathName, _In_opt_ LPSECURITY_ATTRIBUTES securityAttributes)
{
    auto entry = LogFunctionEntry();
    auto result = CreateDirectoryImpl(pathName, securityAttributes);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(result);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
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

    return result;
}
DECLARE_STRING_SHIM(CreateDirectoryImpl, CreateDirectoryShim);

auto CreateDirectoryExImpl = shims::detoured_string_function(&::CreateDirectoryExA, &::CreateDirectoryExW);
template <typename CharT>
BOOL __stdcall CreateDirectoryExShim(
    _In_ const CharT* templateDirectory,
    _In_ const CharT* newDirectory,
    _In_opt_ LPSECURITY_ATTRIBUTES securityAttributes)
{
    auto entry = LogFunctionEntry();
    auto result = CreateDirectoryExImpl(templateDirectory, newDirectory, securityAttributes);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(result);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
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

    return result;
}
DECLARE_STRING_SHIM(CreateDirectoryExImpl, CreateDirectoryExShim);

auto RemoveDirectoryImpl = shims::detoured_string_function(&::RemoveDirectoryA, &::RemoveDirectoryW);
template <typename CharT>
BOOL __stdcall RemoveDirectoryShim(_In_ const CharT* pathName)
{
    auto entry = LogFunctionEntry();
    auto result = RemoveDirectoryImpl(pathName);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(result);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
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

    return result;
}
DECLARE_STRING_SHIM(RemoveDirectoryImpl, RemoveDirectoryShim);

auto SetCurrentDirectoryImpl = shims::detoured_string_function(&::SetCurrentDirectoryA, &::SetCurrentDirectoryW);
template <typename CharT>
BOOL __stdcall SetCurrentDirectoryShim(_In_ const CharT* pathName)
{
    auto entry = LogFunctionEntry();
    auto result = SetCurrentDirectoryImpl(pathName);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(result);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
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

    return result;
}
DECLARE_STRING_SHIM(SetCurrentDirectoryImpl, SetCurrentDirectoryShim);

auto GetCurrentDirectoryImpl = shims::detoured_string_function(&::GetCurrentDirectoryA, &::GetCurrentDirectoryW);
template <typename CharT>
DWORD __stdcall GetCurrentDirectoryShim(
    _In_ DWORD bufferLength,
    _Out_writes_to_opt_(bufferLength, return +1) CharT* buffer)
{
    auto entry = LogFunctionEntry();
    auto result = GetCurrentDirectoryImpl(bufferLength, buffer);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(result != 0);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
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

    return result;
}
DECLARE_STRING_SHIM(GetCurrentDirectoryImpl, GetCurrentDirectoryShim);

auto GetFileAttributesImpl = shims::detoured_string_function(&::GetFileAttributesA, &::GetFileAttributesW);
template <typename CharT>
DWORD __stdcall GetFileAttributesShim(_In_ const CharT* fileName)
{
    auto entry = LogFunctionEntry();
    auto result = GetFileAttributesImpl(fileName);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(result != INVALID_FILE_ATTRIBUTES);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
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

    return result;
}
DECLARE_STRING_SHIM(GetFileAttributesImpl, GetFileAttributesShim);

auto SetFileAttributesImpl = shims::detoured_string_function(&::SetFileAttributesA, &::SetFileAttributesW);
template <typename CharT>
BOOL __stdcall SetFileAttributesShim(_In_ const CharT* fileName, _In_ DWORD fileAttributes)
{
    auto entry = LogFunctionEntry();
    auto result = SetFileAttributesImpl(fileName, fileAttributes);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(result);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
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

    return result;
}
DECLARE_STRING_SHIM(SetFileAttributesImpl, SetFileAttributesShim);

auto GetFileAttributesExImpl = shims::detoured_string_function(&::GetFileAttributesExA, &::GetFileAttributesExW);
template <typename CharT>
BOOL __stdcall GetFileAttributesExShim(
    _In_ const CharT* fileName,
    _In_ GET_FILEEX_INFO_LEVELS infoLevelId,
    _Out_writes_bytes_(sizeof(WIN32_FILE_ATTRIBUTE_DATA)) LPVOID fileInformation)
{
    auto entry = LogFunctionEntry();
    auto result = GetFileAttributesExImpl(fileName, infoLevelId, fileInformation);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(result);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
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

    return result;
}
DECLARE_STRING_SHIM(GetFileAttributesExImpl, GetFileAttributesExShim);

auto LZOpenFileImpl = shims::detoured_string_function(&::LZOpenFileA, &::LZOpenFileW);
template <typename CharT>
INT __stdcall LZOpenFileShim(_In_ CharT* fileName, _Inout_ LPOFSTRUCT reOpenBuf, _In_ WORD style)
{
    auto entry = LogFunctionEntry();
    auto result = LZOpenFileImpl(fileName, reOpenBuf, style);

    auto functionResult = from_lzerror(result);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
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

    return result;
}
DECLARE_STRING_SHIM(LZOpenFileImpl, LZOpenFileShim);

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
