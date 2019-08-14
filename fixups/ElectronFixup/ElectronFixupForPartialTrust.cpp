#include "ElectronFixupForPartialTrustImpls.h"

#include "psf_framework.h"
#include "reentrancy_guard.h"
#include "utilities.h"
#include "dos_paths.h"

#include <Windows.h>
#include <fileapifromapp.h>

///
// detectPipe
// Given an input string, determines if the string could be a pipe.
// - Checks for form: \\.\pipe\<token>
// - Will only allow local pipes
// - Will not look for local pipes using localhost or 127.0.0.1
static bool detectPipe(std::wstring_view pipeName);
static bool detectPipe(std::string_view  pipeName);

///
// getLocalPipeName
// Given an input string, prefixs the pipe path with a UWP accepted pipe path
//  - Converts path: \\.\pipe\<token>
//               to: \\.\pipe\LOCAL\<token>
static wide_argument_string_with_buffer getLocalPipeName(LPCWSTR pipeName);
static wide_argument_string_with_buffer getLocalPipeName(LPCSTR  pipeName);

template <typename CharT>
HANDLE WINAPI CreateFileFixup(
    _In_     const CharT* lpFileName,
    _In_     DWORD    dwDesiredAccess,
    _In_     DWORD    dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_     DWORD    dwCreationDisposition,
    _In_     DWORD    dwFlagsAndAttributes,
    _In_opt_ HANDLE   hTemplateFile) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    if (guard) 
    {
        // CreateFile also services pipes. Check if the input is a pipe first
        auto newPipeName = getLocalPipeName(lpFileName);
        if (newPipeName.c_str())
        {
            // Pipes must use the native API
            return impl::CreateFile(newPipeName.c_str(), dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
        }

        return CreateFileFromAppW(widen_argument(lpFileName).c_str(), dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    }

    // If we have already fixed this function, fall back on the native call
    return impl::CreateFile(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}
DECLARE_STRING_FIXUP(impl::CreateFile, CreateFileFixup);

template <typename CharT>
BOOL WINAPI CreateDirectoryFixup(
    _In_     const CharT* lpPathName,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    auto guard = g_reentrancyGuard.enter();
    if (guard) 
    {
        return CreateDirectoryFromAppW(widen_argument(lpPathName).c_str(), lpSecurityAttributes);
    }

    // If we have already fixed this function, fall back on the native call
    return impl::CreateDirectory(lpPathName, lpSecurityAttributes);
}
DECLARE_STRING_FIXUP(impl::CreateDirectory, CreateDirectoryFixup);

template <typename CharT>
BOOL WINAPI CopyFileFixup(
    _In_ const CharT* lpExistingFileName,
    _In_ const CharT* lpNewFileName,
    _In_ BOOL         bFailIfExists)
{
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        return CopyFileFromAppW(widen_argument(lpExistingFileName).c_str(), widen_argument(lpNewFileName).c_str(), bFailIfExists);
    }

    // If we have already fixed this function, fall back on the native call
    return impl::CopyFile(lpExistingFileName, lpNewFileName, bFailIfExists);
}
DECLARE_STRING_FIXUP(impl::CopyFile, CopyFileFixup);

template <typename CharT>
BOOL WINAPI DeleteFileFixup(
    _In_ const CharT* lpFileName)
{
    auto guard = g_reentrancyGuard.enter();
    if (guard) 
    {
        return DeleteFileFromAppW(widen_argument(lpFileName).c_str());
    }

    // If we have already fixed this function, fall back on the native call
    return impl::DeleteFile(lpFileName);
}
DECLARE_STRING_FIXUP(impl::DeleteFile, DeleteFileFixup);

template <typename CharT>
using win32_find_data_t = std::conditional_t<psf::is_ansi<CharT>, WIN32_FIND_DATAA, WIN32_FIND_DATAW>;

template <typename CharT>
HANDLE WINAPI FindFirstFileExFixup(
    _In_ const CharT*       lpFileName,
    _In_ FINDEX_INFO_LEVELS fInfoLevelId,
    _Out_writes_bytes_(sizeof(win32_find_data_t)) LPVOID lpFindFileData,
    _In_ FINDEX_SEARCH_OPS  fSearchOp,
    _Reserved_ LPVOID       lpSearchFilter,
    _In_ DWORD              dwAdditionalFlags)
{
    auto guard = g_reentrancyGuard.enter();
    if (guard) 
    {
        WIN32_FIND_DATAW* findData = nullptr;
        [[maybe_unused]] WIN32_FIND_DATAW  wideData;
        if (lpFindFileData != nullptr)
        {
            findData = psf::is_ansi<CharT> ? &wideData : reinterpret_cast<WIN32_FIND_DATAW*>(lpFindFileData);
        }

        // If lpFindFileData was nullptr, we carry forward that error.
        auto status = FindFirstFileExFromAppW(widen_argument(lpFileName).c_str(), fInfoLevelId, findData, fSearchOp, lpSearchFilter, dwAdditionalFlags);

        // Do the conversion from wide to narrow struct if required
        if ((lpFindFileData != nullptr) && (psf::is_ansi<CharT>) && status)
        {
            WIN32_FIND_DATAA* workingData = reinterpret_cast<WIN32_FIND_DATAA*>(lpFindFileData);
            workingData->dwFileAttributes = findData->dwFileAttributes;
            workingData->ftCreationTime   = findData->ftCreationTime;
            workingData->ftLastAccessTime = findData->ftLastAccessTime;
            workingData->ftLastWriteTime  = findData->ftLastWriteTime;
            workingData->nFileSizeHigh    = findData->nFileSizeHigh;
            workingData->nFileSizeLow     = findData->nFileSizeLow;
            workingData->dwReserved0      = findData->dwReserved0;
            workingData->dwReserved1      = findData->dwReserved1;
            // Assume loss of 'large' characters
            wcstombs(workingData->cFileName, findData->cFileName, sizeof(workingData->cFileName));
            wcstombs(workingData->cAlternateFileName, findData->cAlternateFileName, sizeof(workingData->cAlternateFileName));
        }

        return status;
    }

    // If we have already fixed this function, fall back on the native call
    return impl::FindFirstFileEx(lpFileName, fInfoLevelId, lpFindFileData, fSearchOp, lpSearchFilter, dwAdditionalFlags);;
}
DECLARE_STRING_FIXUP(impl::FindFirstFileEx, FindFirstFileExFixup);

template <typename CharT>
DWORD WINAPI GetFileAttributesFixup(
    _In_ const CharT* lpFileName)
{
    auto guard = g_reentrancyGuard.enter();
    if (guard) 
    {
        WIN32_FILE_ATTRIBUTE_DATA data;
        GetFileAttributesExFromAppW(widen_argument(lpFileName).c_str(), GET_FILEEX_INFO_LEVELS::GetFileExInfoStandard, &data);

        return data.dwFileAttributes;
    }

    // If we have already fixed this function, fall back on the native call
    return impl::GetFileAttributes(lpFileName);
}
DECLARE_STRING_FIXUP(impl::GetFileAttributes, GetFileAttributesFixup);

template <typename CharT>
BOOL WINAPI GetFileAttributesExFixup(
    _In_ const CharT*           lpFileName,
    _In_ GET_FILEEX_INFO_LEVELS fInfoLevelId,
    _Out_writes_bytes_(sizeof(WIN32_FILE_ATTRIBUTE_DATA)) LPVOID lpFileInformation)
{
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        return GetFileAttributesExFromAppW(widen_argument(lpFileName).c_str(), fInfoLevelId, lpFileInformation);
    }

    // If we have already fixed this function, fall back on the native call
    return impl::GetFileAttributesEx(lpFileName, fInfoLevelId, lpFileInformation);
}
DECLARE_STRING_FIXUP(impl::GetFileAttributesEx, GetFileAttributesExFixup);

template <typename CharT>
BOOL WINAPI MoveFileFixup(
    _In_ const CharT* lpExistingFileName,
    _In_ const CharT* lpNewFileName)
{
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        return MoveFileFromAppW(widen_argument(lpExistingFileName).c_str(), widen_argument(lpNewFileName).c_str());
    }

    // If we have already fixed this function, fall back on the native call
    return impl::MoveFile(lpExistingFileName, lpNewFileName);
}
DECLARE_STRING_FIXUP(impl::MoveFile, MoveFileFixup);

template <typename CharT>
BOOL WINAPI MoveFileExFixup(
    _In_ const CharT* lpExistingFileName,
    _In_ const CharT* lpNewFileName,
    _In_ DWORD dwFlags)
{
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        // Try the native implementation first
        if (!impl::MoveFileEx(lpExistingFileName, lpNewFileName, dwFlags))
        {
            // Attempt the move ignoring the dwFlags
            return MoveFileFromAppW(widen_argument(lpExistingFileName).c_str(), widen_argument(lpNewFileName).c_str());
        }

        // If we get here, the native implementation succeeded
        return TRUE;
    }

    // If we have already fixed this function, fall back on the native call
    return impl::MoveFileEx(lpExistingFileName, lpNewFileName, dwFlags);
}
DECLARE_STRING_FIXUP(impl::MoveFileEx, MoveFileExFixup);

template <typename CharT>
BOOL WINAPI RemoveDirectoryFixup(
    _In_ const CharT* lpPathName)
{
    auto guard = g_reentrancyGuard.enter();
    if (guard) 
    {
        return RemoveDirectoryFromAppW(widen_argument(lpPathName).c_str());
    }

    // If we have already fixed this function, fall back on the native call
    return impl::RemoveDirectory(lpPathName);
}
DECLARE_STRING_FIXUP(impl::RemoveDirectory, RemoveDirectoryFixup);

template <typename CharT>
BOOL WINAPI ReplaceFileFixup(
    _In_       const CharT* lpReplacedFileName,
    _In_       const CharT* lpReplacementFileName,
    _In_opt_   const CharT* lpBackupFileName,
    _In_       DWORD  dwReplaceFlags,
    _Reserved_ LPVOID lpExclude,
    _Reserved_ LPVOID lpReserved)
{
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        // lpBackupFileName is not always included
        if (lpBackupFileName)
        {
            return ReplaceFileFromAppW(widen_argument(lpReplacedFileName).c_str(), widen_argument(lpReplacementFileName).c_str(), widen_argument(lpBackupFileName).c_str(), dwReplaceFlags, lpExclude, lpReserved);
        }

        return ReplaceFileFromAppW(widen_argument(lpReplacedFileName).c_str(), widen_argument(lpReplacementFileName).c_str(), nullptr, dwReplaceFlags, lpExclude, lpReserved);
    }

    // If we have already fixed this function, fall back on the native call
    return impl::ReplaceFile(lpReplacedFileName, lpReplacementFileName, lpBackupFileName, dwReplaceFlags, lpExclude, lpReserved);
}
DECLARE_STRING_FIXUP(impl::ReplaceFile, ReplaceFileFixup);

template <typename CharT>
BOOL WINAPI SetFileAttributesFixup(
    _In_ const CharT* lpFileName,
    _In_ DWORD        dwFileAttributes)
{
    auto guard = g_reentrancyGuard.enter();
    if (guard) 
    {
        return SetFileAttributesFromAppW(widen_argument(lpFileName).c_str(), dwFileAttributes);
    }

    // If we have already fixed this function, fall back on the native call
    return impl::SetFileAttributes(lpFileName, dwFileAttributes);
}
DECLARE_STRING_FIXUP(impl::SetFileAttributes, SetFileAttributesFixup);

template <typename CharT>
HANDLE WINAPI CreateNamedPipeFixup(
    _In_ const CharT* lpName,
    _In_ DWORD        dwOpenMode,
    _In_ DWORD        dwPipeMode,
    _In_ DWORD        nMaxInstances,
    _In_ DWORD        nOutBufferSize,
    _In_ DWORD        nInBufferSize,
    _In_ DWORD        nDefaultTimeOut,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    return impl::CreateNamedPipe(getLocalPipeName(lpName).c_str(), dwOpenMode, dwPipeMode, nMaxInstances, nOutBufferSize, nInBufferSize, nDefaultTimeOut, lpSecurityAttributes);
}
DECLARE_STRING_FIXUP(impl::CreateNamedPipe, CreateNamedPipeFixup);

template <typename CharT>
BOOL WINAPI CallNamedPipeFixup(
    _In_  const CharT* lpNamedPipeName,
    _In_reads_bytes_opt_(nInBufferSize) LPVOID  lpInBuffer,
    _In_  DWORD   nInBufferSize,
    _Out_writes_bytes_to_opt_(nOutBufferSize, *lpBytesRead) LPVOID  lpOutBuffer,
    _In_  DWORD   nOutBufferSize,
    _Out_ LPDWORD lpBytesRead,
    _In_  DWORD   nTimeOut)
{
    return impl::CallNamedPipe(getLocalPipeName(lpNamedPipeName).c_str(), lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesRead, nTimeOut);
}
DECLARE_STRING_FIXUP(impl::CallNamedPipe, CallNamedPipeFixup);

template <typename CharT>
BOOL WINAPI WaitNamedPipeFixup(
    _In_ const CharT* lpNamedPipeName,
    _In_ DWORD        nTimeOut)
{
    return impl::WaitNamedPipe(getLocalPipeName(lpNamedPipeName).c_str(), nTimeOut);
}
DECLARE_STRING_FIXUP(impl::WaitNamedPipe, WaitNamedPipeFixup);

const static int uwpPipePrefixLen = 9;
inline static wide_argument_string_with_buffer getLocalPipeName(LPCSTR pipeName)
{
    if (pipeName)
    {
        return getLocalPipeName(widen_argument(pipeName).c_str());
    }
    return wide_argument_string_with_buffer{};
}

static wide_argument_string_with_buffer getLocalPipeName(LPCWSTR pipeName)
{
    if (pipeName)
    {
        if (detectPipe(pipeName))
        {
            std::wstring_view name(pipeName);
            std::wstring formattedPipe{ LR"(\\.\pipe\LOCAL\)" };
            // Stomp on the server name. In an app container, pipe name must be as follows
            formattedPipe.append(name.substr(uwpPipePrefixLen));
            return wide_argument_string_with_buffer{ formattedPipe };
        }
    }

    // Not a valid pipe input so return empty string
    return wide_argument_string_with_buffer();
}

inline static bool detectPipe(std::string_view pipeName)
{
    return detectPipe(widen(pipeName));
}

static bool detectPipe(std::wstring_view pipeName)
{
    // Verify that the name can even fit
    if (pipeName.size() <= uwpPipePrefixLen)
    {
        return false;
    }

    // Verify pipe path, makes assumptions on the input path as follows
    auto pathType = psf::path_type(pipeName.data());
    if ((pathType != psf::dos_path_type::local_device) &&
        (pathType != psf::dos_path_type::root_local_device))
    {
        return false;
    }

    // Verify that the name after the path root is pipe/
    if (((pipeName[4] != 'p') && (pipeName[4] != 'P')) ||
        ((pipeName[5] != 'i') && (pipeName[5] != 'I')) ||
        ((pipeName[6] != 'p') && (pipeName[6] != 'P')) ||
        ((pipeName[7] != 'e') && (pipeName[7] != 'E')) ||
        !psf::is_path_separator(pipeName[8]))
    {
        return false;
    }

    return true;
}
