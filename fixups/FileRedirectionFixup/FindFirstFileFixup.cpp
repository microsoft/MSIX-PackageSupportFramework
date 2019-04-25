//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// The general design is roughly as follows. Redirection is set up as a base path and some pattern to match file name/
// sub path. It would be fairly tricky to try and merge that pattern with the patterns that FindFirstFile supports, so
// we instead: (1) enumerate files in the redirected directory, if the directory exists, then (2) enumerate the files in
// the non-redirected directory, ignoring files that exist in the redirected directory. Note that this order is
// important since it could be the case that a file gets copied to the redirected directory in the middle of enumeration
// and we could otherwise return the same file twice.
// NOTE: If we ever address the "delete package file" problem, we'll need to address that here, too

#include <array>

#include <dos_paths.h>
#include <fancy_handle.h>
#include <psf_framework.h>

#include "FunctionImplementations.h"
#include "PathRedirection.h"

struct find_deleter
{
    using pointer = psf::fancy_handle;

    void operator()(const pointer& findHandle) noexcept
    {
        if (findHandle)
        {
            [[maybe_unused]] auto result = impl::FindClose(findHandle);
            assert(result);
        }
    }
};
using unique_find_handle = std::unique_ptr<void, find_deleter>;

struct find_data
{
    // Redirected path directory so that we can avoid returning duplicate filenames. This value will be empty if the
    // path does not exist/match any existing files at the start of the enumeration, for a slight optimization
    std::wstring redirect_path;

    // The first value is the find handle for the redirected path. The second is the find handle for the non-redirected
    // path. The values are set to INVALID_HANDLE_VALUE as enumeration completes.
    unique_find_handle find_handles[2];

    // We need to hold on to the results of FindFirstFile for find_handles[1]
    WIN32_FIND_DATAW cached_data;
};

template <typename CharT>
using win32_find_data_t = std::conditional_t<psf::is_ansi<CharT>, WIN32_FIND_DATAA, WIN32_FIND_DATAW>;

DWORD copy_find_data(const WIN32_FIND_DATAW& from, WIN32_FIND_DATAA& to) noexcept
{
    to.dwFileAttributes = from.dwFileAttributes;
    to.ftCreationTime = from.ftCreationTime;
    to.ftLastAccessTime = from.ftLastAccessTime;
    to.ftLastWriteTime = from.ftLastWriteTime;
    to.nFileSizeHigh = from.nFileSizeHigh;
    to.nFileSizeLow = from.nFileSizeLow;
    to.dwReserved0 = from.dwReserved0;
    to.dwReserved1 = from.dwReserved1;

    if (auto len = ::WideCharToMultiByte(CP_ACP, 0, from.cFileName, -1, to.cFileName, sizeof(to.cFileName), nullptr, nullptr);
        !len || (len > sizeof(to.cFileName)))
    {
        return ::GetLastError();
    }

    if (auto len = ::WideCharToMultiByte(CP_ACP, 0, from.cAlternateFileName, -1, to.cAlternateFileName, sizeof(to.cAlternateFileName), nullptr, nullptr);
        !len || (len > sizeof(to.cAlternateFileName)))
    {
        return ::GetLastError();
    }

    return ERROR_SUCCESS;
}

DWORD copy_find_data(const WIN32_FIND_DATAW& from, WIN32_FIND_DATAW& to) noexcept
{
    to = from;
    return ERROR_SUCCESS;
}

template <typename CharT>
HANDLE __stdcall FindFirstFileExFixup(
    _In_ const CharT* fileName,
    _In_ FINDEX_INFO_LEVELS infoLevelId,
    _Out_writes_bytes_(sizeof(win32_find_data_t<CharT>)) LPVOID findFileData,
    _In_ FINDEX_SEARCH_OPS searchOp,
    _Reserved_ LPVOID searchFilter,
    _In_ DWORD additionalFlags) noexcept try
{
    auto guard = g_reentrancyGuard.enter();
    if (!guard)
    {
        return impl::FindFirstFileEx(fileName, infoLevelId, findFileData, searchOp, searchFilter, additionalFlags);
    }

#if _DEBUG
    Log("FindFirstFile with %s", fileName);
#endif

    // Split the input into directory and pattern
    auto path = widen(fileName, CP_ACP);
    normalized_path dir;
    const wchar_t* pattern = nullptr;
    if (auto dirPos = path.find_last_of(LR"(\/)"); dirPos != std::wstring::npos)
    {
        // Special case for single separator at beginning of the path "/foo.txt"
        if (dirPos == 0)
        {
            auto nextChar = std::exchange(path[dirPos + 1], L'\0');
            dir = NormalizePath(path.c_str());
            path[dirPos + 1] = nextChar;
        }
        else
        {
            auto separator = std::exchange(path[dirPos], L'\0');
            dir = NormalizePath(path.c_str());
            path[dirPos] = separator;
        }
        pattern = path.c_str() + dirPos + 1;
    }
    else
    {
        dir = NormalizePath(L".");
        pattern = path.c_str();
    }

    dir = DeVirtualizePath(std::move(dir));

    auto result = std::make_unique<find_data>();
    result->redirect_path = RedirectedPath(dir);
    if (result->redirect_path.back() != L'\\')
    {
        result->redirect_path.push_back(L'\\');
    }

#if _DEBUG
    Log("FindFirstFile redirected to %ls", result->redirect_path.c_str());
#endif

    [[maybe_unused]] auto ansiData = reinterpret_cast<WIN32_FIND_DATAA*>(findFileData);
    [[maybe_unused]] auto wideData = reinterpret_cast<WIN32_FIND_DATAW*>(findFileData);
    WIN32_FIND_DATAW* findData = psf::is_ansi<CharT> ? &result->cached_data : wideData;

    // Open the redirected find handle
    auto revertSize = result->redirect_path.length();
    result->redirect_path += pattern;
    result->find_handles[0].reset(impl::FindFirstFileEx(result->redirect_path.c_str(), infoLevelId, findData, searchOp, searchFilter, additionalFlags));
    result->redirect_path.resize(revertSize);

    // Some applications really care about the failure reason. Try and make this the best that we can, preferring
    // something like "file not found" over "path does not exist"
    auto initialFindError = ::GetLastError();

    if (result->find_handles[0])
    {
        if constexpr (psf::is_ansi<CharT>)
        {
            if (copy_find_data(*findData, *ansiData))
            {
                // NOTE: Last error set by caller
                return INVALID_HANDLE_VALUE;
            }
        }
        else
        {
            // No need to copy since we wrote directly into the output buffer
            assert(findData == wideData);
        }
#if _DEBUG
        Log("FindFirstFile redirected (from redirected): had results");
#endif
    }
    else
    {
        // Path doesn't exist or match any files. We can safely get away without the redirected file exists check
        result->redirect_path.clear();
#if _DEBUG
        Log("FindFirstFile redirected (from redirected): no results");
#endif    
    }

    findData = (result->find_handles[0] || psf::is_ansi<CharT>) ? &result->cached_data : wideData;

    // Open the non-redirected find handle
    result->find_handles[1].reset(impl::FindFirstFileEx(path.c_str(), infoLevelId, findData, searchOp, searchFilter, additionalFlags));
#if _DEBUG
    if (result->find_handles[1])
    {
        Log("FindFirstFile (from redirected): had results");
    }
    else
    {
        Log("FindFirstFile (from redirected): no results");
    }
#endif
    if (!result->find_handles[0])
    {
        if (!result->find_handles[1])
        {
            // Neither path exists. The last error should still be set by FindFirstFileEx, but prefer the initial error
            // if it indicates that the redirected directory structure exists
            if (initialFindError == ERROR_FILE_NOT_FOUND)
            {
                ::SetLastError(initialFindError);
            }

            return INVALID_HANDLE_VALUE;
        }
        else
        {
            if constexpr (psf::is_ansi<CharT>)
            {
                if (copy_find_data(*findData, *ansiData))
                {
                    // NOTE: Last error set by caller
                    return INVALID_HANDLE_VALUE;
                }
            }
            else
            {
                // No need to copy since we wrote directly into the output buffer
                assert(findData == wideData);
            }
#if _DEBUG
            Log("FindFirstFile (from redirected): had results");
#endif
        }
    }

    ::SetLastError(ERROR_SUCCESS);
    return reinterpret_cast<HANDLE>(result.release());
}
catch (...)
{
    // NOTE: Since we allocate our own "find handle" memory, we can't just forward on to the implementation
    ::SetLastError(win32_from_caught_exception());
    return INVALID_HANDLE_VALUE;
}
DECLARE_STRING_FIXUP(impl::FindFirstFileEx, FindFirstFileExFixup);

template <typename CharT>
HANDLE __stdcall FindFirstFileFixup(_In_ const CharT* fileName, _Out_ win32_find_data_t<CharT>* findFileData) noexcept
{
    // For simplicity, just do what the OS appears to be doing and forward arguments on to FindFirstFileEx
    return FindFirstFileExFixup(fileName, FindExInfoStandard, findFileData, FindExSearchNameMatch, nullptr, 0);
}
DECLARE_STRING_FIXUP(impl::FindFirstFile, FindFirstFileFixup);

template <typename CharT>
BOOL __stdcall FindNextFileFixup(_In_ HANDLE findFile, _Out_ win32_find_data_t<CharT>* findFileData) noexcept try
{
    auto guard = g_reentrancyGuard.enter();
    if (!guard)
    {
        return impl::FindNextFile(findFile, findFileData);
    }

    if (findFile == INVALID_HANDLE_VALUE)
    {
        ::SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    auto data = reinterpret_cast<find_data*>(findFile);
    auto redirectedFileExists = [&](auto filename)
    {
        if (data->redirect_path.empty())
        {
            return false;
        }

        auto revertSize = data->redirect_path.length();

        // NOTE: 'is_ansi' evaluation not inline due to the bug:
        //       https://developercommunity.visualstudio.com/content/problem/324366/illegal-indirection-error-when-the-evaluation-of-i.html
        constexpr bool is_ansi = psf::is_ansi<std::decay_t<decltype(*filename)>>;
        if constexpr (is_ansi)
        {
            data->redirect_path += widen(filename);
        }
        else
        {
            data->redirect_path += filename;
        }

        auto result = impl::PathExists(data->redirect_path.c_str());
        data->redirect_path.resize(revertSize);
        return result;
    };

    if (data->find_handles[0])
    {
        if (impl::FindNextFile(data->find_handles[0].get(), findFileData))
        {
            return TRUE;
        }
        else if (::GetLastError() == ERROR_NO_MORE_FILES)
        {
            data->find_handles[0].reset();
            if (!data->find_handles[1])
            {
                // NOTE: Last error scribbled over by closing find_handles[0]
                ::SetLastError(ERROR_NO_MORE_FILES);
                return FALSE;
            }

            if (!redirectedFileExists(data->cached_data.cFileName))
            {
                if (copy_find_data(data->cached_data, *findFileData))
                {
                    // NOTE: Last error set by caller
                    return FALSE;
                }

                ::SetLastError(ERROR_SUCCESS);
                return TRUE;
            }
        }
        else
        {
            // Error due to something other than reaching the end
            return FALSE;
        }
    }

    while (data->find_handles[1])
    {
        if (impl::FindNextFile(data->find_handles[1].get(), findFileData))
        {
            // Skip the file if it exists in the redirected path
            if (!redirectedFileExists(findFileData->cFileName))
            {
                ::SetLastError(ERROR_SUCCESS);
                return TRUE;
            }
            // Otherwise, skip this file and check the next one
        }
        else if (::GetLastError() == ERROR_NO_MORE_FILES)
        {
            data->find_handles[1].reset();
            ::SetLastError(ERROR_NO_MORE_FILES);
            return FALSE;
        }
        else
        {
            // Error due to something other than reaching the end
            return FALSE;
        }
    }

    // We ran out of data either on a previous call, or by ignoring files that have been redirected
    ::SetLastError(ERROR_NO_MORE_FILES);
    return FALSE;
}
catch (...)
{
    ::SetLastError(win32_from_caught_exception());
    return FALSE;
}
DECLARE_STRING_FIXUP(impl::FindNextFile, FindNextFileFixup);

BOOL __stdcall FindCloseFixup(_Inout_ HANDLE findHandle) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    if (!guard)
    {
        return impl::FindClose(findHandle);
    }

    if (findHandle == INVALID_HANDLE_VALUE)
    {
        ::SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    delete reinterpret_cast<find_data*>(findHandle);
    ::SetLastError(ERROR_SUCCESS);
    return TRUE;
}
DECLARE_FIXUP(impl::FindClose, FindCloseFixup);
