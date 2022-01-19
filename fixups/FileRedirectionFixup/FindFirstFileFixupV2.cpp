//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP.  All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// This implementation of FindFileFirst/Next is an attempt at a clean and more complete version than the original fixup.
// 
// The general design is roughly as follows. 
// When a find is requested, it is possible that in addition to the location requested, there may be files that we should find in three other locations.
// First, we develope a "clean" version of the requested path that looks like C:\whatever to aid in creating the alternate paths:
// A) An area where package file changes may have been previously redirected to.
// B) A VFS path in the package (when the location requested is outside of the package boundary).
// C) The normalized path as requested.
// D) A devirtualized native path (when the location requested is inside a package VFS folder, this is the native location).
// E) A deredirected package path (when the requested location is in the writablepackageroot area).
// 
// So we calculate all five possible paths (or deterine the path is not possible/applicable),
// Then we build up results in a logical order, presenting in order of: Redirection, VFS, requested, and devirtualized native, deredirected package.
//
// NOTE: If we ever address the "delete package file" problem, we'll need to address that here, too.
// NOTE2: If successful, this replacement form might be a model needed in other file operations.

#include <array>

#include <dos_paths.h>
#include <fancy_handle.h>
#include <psf_framework.h>

#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <psf_logging.h>


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


struct find_data2
{
    // Redirected path directory so that we can avoid returning duplicate filenames. This value will be empty if the
    // path does not exist/match any existing files at the start of the enumeration, for a slight optimization
    std::wstring redirect_path;
    std::wstring package_vfs_path;
    std::wstring requested_path;
    std::wstring package_devfs_path;
    std::wstring package_deredirect_path;
    std::list<std::wstring> already_returned_list = {};

    // There are five different locations where we might find things, stored in the find_handles array and used in the following order:
    //    The first (optional) value is the find handle for the "redirected path" (typically to the user's profile). Set when original is a package path.
    //    The second (optional) value is for a "package VFS" location equivalent to the requested information when native AppData and LocalAppdata are used in the request,.
    //    The third is the find handle for the non-redirected path as asked for by the app.
    //    The fourth (optional) value is for a native location equivalent to the requested package VFS location.
    //    The fifth (optional) value is for a package location equivalent when a redirected path was asked for.
    // Some of these values will be not used (empty) when not appropriate for the requested type of request.

    // The values are set to INVALID_HANDLE_VALUE as enumeration completes.

    // Notes: When the original request was in the path of the package, the second handle is NULL,.
    //        When the original request was a path outside of package path, second handle is NULL if no equivalent VFS exists in package.
    //        When the original request was in a path including VFS in the package path, the fourth handle is used, otherwise NULL
    //        First handle is NULL when redirection has not happened yet.
    //        We need the second handle only for VFS/AppData and VFS/LocalAppdata in the package because the underlying runtime doesn't layer those folders in when we use %Appdata% and %LocalAppData% for the third handle.
    unique_find_handle find_handles[5];

    // We need to hold on to the results of FindFirstFile for find_handles[0/1/2/3/4] 
    WIN32_FIND_DATAW cached_data;

    DWORD RememberedInstance;
};

extern std::filesystem::path g_writablePackageRootPath;

#if USE_FINDFIXUP2
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
DWORD copy_find_data(const WIN32_FIND_DATAA& from, WIN32_FIND_DATAW& to) noexcept
{
    to.dwFileAttributes = from.dwFileAttributes;
    to.ftCreationTime = from.ftCreationTime;
    to.ftLastAccessTime = from.ftLastAccessTime;
    to.ftLastWriteTime = from.ftLastWriteTime;
    to.nFileSizeHigh = from.nFileSizeHigh;
    to.nFileSizeLow = from.nFileSizeLow;
    to.dwReserved0 = from.dwReserved0;
    to.dwReserved1 = from.dwReserved1;

    if (auto len = ::MultiByteToWideChar(CP_ACP, 0, from.cFileName, -1, to.cFileName, sizeof(to.cFileName));
        !len || (len > sizeof(to.cFileName)))
    {
        return ::GetLastError();
    }

    if (auto len = ::MultiByteToWideChar(CP_ACP, 0, from.cAlternateFileName, -1, to.cAlternateFileName, sizeof(to.cAlternateFileName));
        !len || (len > sizeof(to.cAlternateFileName)))
    {
        return ::GetLastError();
    }

    return ERROR_SUCCESS;
}

void LogNormalizedPath(normalized_path np,  std::wstring desc, DWORD instance)
{
    Log(L"[%d]\tNormalized_path %ls Type=%x, Full=%ls, Abs=%ls", instance, desc.c_str(), (int)np.path_type, np.full_path.c_str(), np.drive_absolute_path);
}


template <typename CharT>
HANDLE __stdcall FindFirstFileExFixupV2(
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
#if _DEBUG
        LogString(L"\tFindFirstFileExFixupV2 (unguarded): for fileName", fileName);
#endif
        return impl::FindFirstFileEx(fileName, infoLevelId, findFileData, searchOp, searchFilter, additionalFlags);
    }

    DWORD FindFirstFileExInstanceV2 = ++g_FileIntceptInstance;
    
    std::wstring wFileName = widen(fileName);
#if _DEBUG
    LogString(FindFirstFileExInstanceV2, L"\tFindFirstFileExFixupV2: for fileName", wFileName.c_str());
    if (psf::is_ansi<CharT>)
        Log(L"[%d]DEBUG IsAnsi call", FindFirstFileExInstanceV2);
    else
        Log(L"[%d]DEBUG IsWide call", FindFirstFileExInstanceV2);

    switch (searchOp)
    {
    case FindExSearchNameMatch:
        Log(L"[%d]SearchOp FindExSearchNameMatch",FindFirstFileExInstanceV2);
            break;
    case FindExSearchLimitToDirectories:
        Log(L"[%d]SearchOp FindExSearchLimitToDirectories", FindFirstFileExInstanceV2);
        break;
    case FindExSearchLimitToDevices:
        Log(L"[%d]SearchOp FindExSearchLimitToDevices", FindFirstFileExInstanceV2);
        break;
    case FindExSearchMaxSearchOp:
        Log(L"[%d]SearchOp FindExSearchMaxSearchOp", FindFirstFileExInstanceV2);
        break;
    default:
        Log(L"[%d]SearchOp Unknown=0x%x", FindFirstFileExInstanceV2,searchOp);
        break;
    }
#endif
    if (wFileName.length() > 3)
    {
        if (wFileName.substr(wFileName.length() - 3).compare(L"\\\\*")==0)
        {
            // This case is an invalid find request that accidentally works in normal find calls, but our redirection gets messed up.
#if _DEBUG
            Log(L"[%d]DEBUG TEST: Tweek bad call", FindFirstFileExInstanceV2);
#endif
            wFileName[wFileName.length()-2] = wFileName[wFileName.length()-1];
            wFileName.resize(wFileName.length() - 1);
        }
    }

    normalized_pathV2 pathAsRequestedNormalized = NormalizePathV2(wFileName.c_str(), FindFirstFileExInstanceV2);
    std::wstring pathVirtualized = L"";
    if (pathAsRequestedNormalized.path_type != psf::dos_path_type::unknown &&
        pathAsRequestedNormalized.path_type != psf::dos_path_type::unc_absolute &&
        !IsUnderPackageRoot(pathAsRequestedNormalized.drive_absolute_path.c_str()) &&
        !IsUnderUserPackageWritablePackageRoot(pathAsRequestedNormalized.drive_absolute_path.c_str()))
    {
        pathVirtualized = VirtualizePathV2(pathAsRequestedNormalized);
    }
    std::wstring pathDeVirtualized = DeVirtualizePathV2(pathAsRequestedNormalized);
    std::wstring pathRedirected = RedirectedPathV2(pathAsRequestedNormalized, false, g_writablePackageRootPath.native(), FindFirstFileExInstanceV2);
    std::wstring pathDeRedirected = ReverseRedirectedToPackage(pathAsRequestedNormalized.original_path.c_str());

    auto result = std::make_unique<find_data2>();
    result->RememberedInstance = FindFirstFileExInstanceV2;
    result->redirect_path = pathRedirected;
    result->package_vfs_path = pathVirtualized;
    result->requested_path = wFileName.c_str();// widen(fileName).c_str();
    result->package_devfs_path = pathDeVirtualized;
    result->package_deredirect_path = pathDeRedirected;

#if _DEBUG
    Log(L"[%d]FindFirstFileExFixupV2 redirect_path        for [0] (from redir)   is %ls", FindFirstFileExInstanceV2, result->redirect_path.c_str());
    Log(L"[%d]FindFirstFileExFixupV2 package_vfs_path     for [1] (from VFS)     is %ls", FindFirstFileExInstanceV2, result->package_vfs_path.c_str());
    Log(L"[%d]FindFirstFileExFixupV2 requested_path       for [2] (from req)     is %ls", FindFirstFileExInstanceV2, result->requested_path.c_str());
    Log(L"[%d]FindFirstFileExFixupV2 package_devfs_path   for [3] (from DeVFS)   is %ls", FindFirstFileExInstanceV2, result->package_devfs_path.c_str());
    Log(L"[%d]FindFirstFileExFixupV2 package_deredir_path for [4] (from DeRedir) is %ls", FindFirstFileExInstanceV2, result->package_deredirect_path.c_str());
#endif

    [[maybe_unused]] auto ansiData = reinterpret_cast<WIN32_FIND_DATAA*>(findFileData);
    [[maybe_unused]] auto wideData = reinterpret_cast<WIN32_FIND_DATAW*>(findFileData);
    WIN32_FIND_DATAW* findData = psf::is_ansi<CharT> ? &result->cached_data : wideData; 
    DWORD initialFindError = ERROR_PATH_NOT_FOUND;
    bool AnyValidResult = false;
    bool AnyValidPath = false;
    //
    // [0] Open the redirected find handle
    if (!pathRedirected.empty())
    {
        result->find_handles[0].reset(impl::FindFirstFileEx(result->redirect_path.c_str(), infoLevelId, findData, searchOp, searchFilter, additionalFlags));
        // Some applications really care about the failure reason. Try and make this the best that we can, preferring
        // something like "file not found" over "path does not exist"
        initialFindError = ::GetLastError();

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
                copy_find_data(*wideData, result->cached_data);
            }
#if _DEBUG
            Log(L"[%d]FindFirstFileExFixupV2[0] (from redirected): had results %ls", FindFirstFileExInstanceV2,findData->cFileName);
#endif
            AnyValidPath = true;
            AnyValidResult = true;
        }
        else
        {
            if (initialFindError == ERROR_FILE_NOT_FOUND)
                AnyValidPath = true;

            // Path doesn't exist or match any files. We can safely get away without the redirected file exists check
            result->redirect_path.clear();
#if _DEBUG
            Log(L"[%d]FindFirstFileExFixupV2[0] (from redirected): no results", FindFirstFileExInstanceV2);
#endif
        }

        // reset for next level???
        findData = (result->find_handles[0] || psf::is_ansi<CharT>) ? &result->cached_data : wideData;
    }
    else
    {
#if _DEBUG
        Log(L"[%d]FindFirstFileExFixupV2[0] (from redirected): no results possible", FindFirstFileExInstanceV2);
#endif
    }

    //
    // [1] Open the package_vfsP_path handle if AppData or LocalAppData (letting the runtime handle other VFSs)
    // runtime not doing what we want...
    //if (IsUnderUserAppDataLocal(wPath.c_str()) || IsUnderUserAppDataRoaming(wPath.c_str()))
    if (!pathVirtualized.empty())
    {
        ///auto vfspathSize = result->package_vfs_path.length();
        ///result->package_vfs_path += pattern;
        result->find_handles[1].reset(impl::FindFirstFileEx(result->package_vfs_path.c_str(), infoLevelId, findData, searchOp, searchFilter, additionalFlags));
        ///result->package_vfs_path.resize(vfspathSize);
        if (result->find_handles[1])
        {
#if _DEBUG
            Log(L"[%d]FindFirstFileExFixupV2[1] (from vfs_path):   had results", FindFirstFileExInstanceV2);
#endif
            AnyValidPath = true;
            AnyValidResult = true;
            initialFindError = ERROR_SUCCESS;
        }
        else
        {
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
                AnyValidPath = true;
            result->package_vfs_path.clear();
#if _DEBUG
            Log(L"[%d]FindFirstFileExFixupV2[1] (from vfs_path):   no results", FindFirstFileExInstanceV2);
#endif
        }
        if (!result->find_handles[0])
        {
            if (result->find_handles[1])
            {
                if constexpr (psf::is_ansi<CharT>)
                {
                    if (copy_find_data(*findData, *ansiData))
                    {
#if _DEBUG
                        Log(L"[%d]FindFirstFile error set by caller", FindFirstFileExInstanceV2);
#endif
                        // NOTE: Last error set by caller
                        return INVALID_HANDLE_VALUE;
                    }
                }
                else
                {
                    // No need to copy since we wrote directly into the output buffer
                    assert(findData == wideData);
                    copy_find_data(*wideData, result->cached_data);
                }
            }
        }

        // reset for next level???
        findData = (result->find_handles[0] || result->find_handles[1] || psf::is_ansi<CharT>) ? &result->cached_data : wideData;
    }
    else
    {
#if _DEBUG
        Log(L"[%d]FindFirstFileExFixupV2[1] (from vfs_path):   no results possible", FindFirstFileExInstanceV2);
#endif
    }

    //
    // [2] Open the non-redirected find handle
    result->find_handles[2].reset(impl::FindFirstFileEx(result->requested_path.c_str(), infoLevelId, findData, searchOp, searchFilter, additionalFlags));
    if (result->find_handles[2])
    {
#if _DEBUG
        Log(L"[%d]FindFirstFileExFixupV2[2] (from origial):    had results", FindFirstFileExInstanceV2);
#endif
        AnyValidPath = true; 
        AnyValidResult = true;
        initialFindError = ERROR_SUCCESS;
    }
    else
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
            AnyValidPath = true;
        result->requested_path.clear();
#if _DEBUG
        Log(L"[%d]FindFirstFileExFixupV2[2] (from original):   no results", FindFirstFileExInstanceV2);
#endif
    }
    if (!result->find_handles[0] &&
        !result->find_handles[1])
    {
        if (result->find_handles[2])
        {
            if constexpr (psf::is_ansi<CharT>)
            {
                if (copy_find_data(*findData, *ansiData))
                {
#if _DEBUG
                    Log(L"[%d]FindFirstFileExFixupV2 error set by caller", FindFirstFileExInstanceV2);
#endif
                    // NOTE: Last error set by caller
                    return INVALID_HANDLE_VALUE;
                }
            }
            else
            {
                // No need to copy since we wrote directly into the output buffer
                assert(findData == wideData);
                copy_find_data(*wideData, result->cached_data);
            }
        }
    }
    // reset for next level???
    findData = (result->find_handles[0] || result->find_handles[1] || result->find_handles[2] || psf::is_ansi<CharT>) ? &result->cached_data : wideData;

    //
    // [3] Open the devitrualized find handle
    if (!pathDeVirtualized.empty())
    {
        result->find_handles[3].reset(impl::FindFirstFileEx(result->package_devfs_path.c_str(), infoLevelId, findData, searchOp, searchFilter, additionalFlags));
        if (result->find_handles[3])
        {
#if _DEBUG
            Log(L"[%d]FindFirstFileExFixupV2[3] (from devirt):     had results", FindFirstFileExInstanceV2);
#endif
            AnyValidPath = true; 
            AnyValidResult = true;
            initialFindError = ERROR_SUCCESS;
        }
        else
        {
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
                AnyValidPath = true;
            result->requested_path.clear();
#if _DEBUG
            Log(L"[%d]FindFirstFileExFixupV2[3] (from devirt):     no results", FindFirstFileExInstanceV2);
#endif
        }
        if (!result->find_handles[0] &&
            !result->find_handles[1] &&
            !result->find_handles[2])
        {
            if (result->find_handles[3])
            {
                if constexpr (psf::is_ansi<CharT>)
                {
                    if (copy_find_data(*findData, *ansiData))
                    {
#if _DEBUG
                        Log(L"[%d]FindFirstFileExFixupV2 error set by caller", FindFirstFileExInstanceV2);
#endif
                        // NOTE: Last error set by caller
                        return INVALID_HANDLE_VALUE;
                    }
                }
                else
                {
                    // No need to copy since we wrote directly into the output buffer
                    assert(findData == wideData);
                    copy_find_data(*wideData, result->cached_data);
                }
            }
        }
        // reset for next level???
        findData = (result->find_handles[0] || result->find_handles[1] || result->find_handles[2] || result->find_handles[3] || psf::is_ansi<CharT>) ? &result->cached_data : wideData;
    }
    else
    {
#if _DEBUG
        Log(L"[%d]FindFirstFileExFixupV2[3] (from devirt):     no results possible", FindFirstFileExInstanceV2);
#endif
    }

    //
    // [4] Open the deredirected find handle
    if (!pathDeRedirected.empty())
    {
        result->find_handles[4].reset(impl::FindFirstFileEx(result->package_deredirect_path.c_str(), infoLevelId, findData, searchOp, searchFilter, additionalFlags));
        if (result->find_handles[4])
        {
#if _DEBUG
            Log(L"[%d]FindFirstFileExFixupV2[4] (from deredir):    had results", FindFirstFileExInstanceV2);
#endif
            AnyValidPath = true;
            AnyValidResult = true;
            initialFindError = ERROR_SUCCESS;
        }
        else
        {
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
                AnyValidPath = true;
            result->requested_path.clear();
#if _DEBUG
            Log(L"[%d]FindFirstFileExFixupV2[4] (from deredir):   no results", FindFirstFileExInstanceV2);
#endif
        }
        if (!result->find_handles[0] &&
            !result->find_handles[1] &&
            !result->find_handles[2] &&
            !result->find_handles[3] )
        {
            if (result->find_handles[4])
            {
                if constexpr (psf::is_ansi<CharT>)
                {
                    if (copy_find_data(*findData, *ansiData))
                    {
#if _DEBUG
                        Log(L"[%d]FindFirstFile2 error set by caller", FindFirstFileExInstanceV2);
#endif
                        // NOTE: Last error set by caller
                        return INVALID_HANDLE_VALUE;
                    }
                }
                else
                {
                    // No need to copy since we wrote directly into the output buffer
                    assert(findData == wideData);
                    copy_find_data(*wideData, result->cached_data);
                }
            }
        }
    }
    else
    {
#if _DEBUG
        Log(L"[%d]FindFirstFileExFixupV2[4] (from deredir):    no results possible", FindFirstFileExInstanceV2);
#endif
    }

            

    if (AnyValidResult)
    {
        if constexpr (psf::is_ansi<CharT>)
        {
            // return first result found
            copy_find_data(*ansiData, result->cached_data);
            //result->cached_data = ansiData;  
        }
#if _DEBUG
        Log(L"[%d]FindFirstFileExFixupV2 returns %ls", FindFirstFileExInstanceV2, result->cached_data.cFileName);
#endif
        result->already_returned_list.push_back(result->cached_data.cFileName);
        ::SetLastError(ERROR_SUCCESS);
    }
    else
    {
        if (AnyValidPath)
        {
            // If in any of the potential locations, the directory exists but no files found,
            // we prefer to return file not found.
            initialFindError = ERROR_FILE_NOT_FOUND;
        }
#if _DEBUG
        Log(L"[%d]FindFirstFileExFixupV2 returns 0x%x", FindFirstFileExInstanceV2, initialFindError);
#endif
        ::SetLastError(initialFindError);
        return INVALID_HANDLE_VALUE;
    }
    return reinterpret_cast<HANDLE>(result.release());

}
catch (...)
{
    // NOTE: Since we allocate our own "find handle" memory, we can't just forward on to the implementation
    ::SetLastError(win32_from_caught_exception());
    Log(L"***FindFirstFileExFixupV2 Exception***");
    return INVALID_HANDLE_VALUE;
}
DECLARE_STRING_FIXUP(impl::FindFirstFileEx, FindFirstFileExFixupV2);


template <typename CharT>
HANDLE __stdcall FindFirstFileFixupV2(_In_ const CharT* fileName, _Out_ win32_find_data_t<CharT>* findFileData) noexcept
{
    // For simplicity, just do what the OS appears to be doing and forward arguments on to FindFirstFileEx
#if _DEBUG
    if constexpr (psf::is_ansi<CharT>)
    {
        Log(L"FileFirstFile A redirected to FindFirstFileExFixupV2");
    }
    else
    {
        Log(L"FileFirstFile W redirected to FindFirstFileExFixupV2");
    }
#endif
    return FindFirstFileExFixupV2(fileName, FindExInfoStandard, findFileData, FindExSearchNameMatch, nullptr, 0);
}
DECLARE_STRING_FIXUP(impl::FindFirstFile, FindFirstFileFixupV2);




template <typename CharT>
BOOL __stdcall FindNextFileFixupV2(_In_ HANDLE findFile, _Out_ win32_find_data_t<CharT>* findFileData) noexcept try
{
    auto guard = g_reentrancyGuard.enter();
    if (!guard)
    {
#if _DEBUG
        Log(L"FindNextFileFixupV2 (unguarded) for file.");
#endif
        return impl::FindNextFile(findFile, findFileData);
    }

#if _DEBUG
        DWORD FindNextFileInstance2 = ++g_FileIntceptInstance;
#endif



    if (findFile == INVALID_HANDLE_VALUE)
    {
#if _DEBUG
        Log(L"[%d]FindNextFileFixupV2 invaid handle.", FindNextFileInstance2);
#endif
        ::SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    auto data = reinterpret_cast<find_data2*>(findFile);

#if _DEBUG
    //Log(L"[%d][%d]FindNextFileFixupV2 is against redir    =%ls", data->RememberedInstance, FindNextFileInstance2, data->redirect_path.c_str());
    //Log(L"[%d][%d]FindNextFileFixupV2 is against pkgVfs   =%ls", data->RememberedInstance, FindNextFileInstance2, data->package_vfs_path.c_str());
    Log(L"[%d][%d]FindNextFileFixupV2 is against original request=%ls", data->RememberedInstance, FindNextFileInstance2, data->requested_path.c_str());
    //Log(L[%d]"[%d]FindNextFileFixupV2 is against deVfs    =%ls", data->RememberedInstance, FindNextFileInstance2, data->package_devfs_path.c_str());
    //Log(L"[%d][%d]FindNextFileFixupV2 is against deRedir  =%ls", data->RememberedInstance, FindNextFileInstance2, data->package_deredirect_path.c_str());
#endif

#ifdef OBSOLETE
    auto redirectedFileExists = [&](auto filename)
    {
        if (data->redirect_path.empty())
        {
            Log(L"[%d]\tFindNextFileV2 redirectedFileExists returns false.", FindNextFileInstance2);
            return false;  
        }
        LogString(FindNextFileInstance2, L"\tFindNextFileV2 redirectedFileExists versus", filename);

        auto revertSize = data->redirect_path.length();

        // NOTE: 'is_ansi' evaluation not inline due to the bug:
        //       https://developercommunity.visualstudio.com/content/problem/324366/illegal-indirection-error-when-the-evaluation-of-i.html
        constexpr bool is_ansi = psf::is_ansi<std::decay_t<decltype(*filename)>>;
        if constexpr (is_ansi)
        {
            std::wstring wFilename = widen(filename);
            if (wcscmp(wFilename.c_str(), L"..") == 0 ||
                wcscmp(wFilename.c_str(), L".") == 0)
            {
                // assume previously returned as part of requesteds
                Log(L"[%d]\tFindNextFileV2 A redirectedFileExists returns true %ls", FindNextFileInstance2, wFilename.c_str());
                return true;
            }
            data->redirect_path += wFilename;
        }
        else
        {
            if (wcscmp(filename, L"..") == 0 ||
                wcscmp(filename, L".") == 0)
            {
                // assume previously returned as part of requesteds
                Log(L"[%d]\tFindNextFileV2 W redirectedFileExists returns true %ls", FindNextFileInstance2, filename);
                return true;
            }
            data->redirect_path += filename;
        }

        auto result = impl::PathExists(data->redirect_path.c_str());
        data->redirect_path.resize(revertSize);
        Log(L"[%d]\tFindNextFileV2 redirectedFileExists returns 0x%x %ls", FindNextFileInstance2, (int)result, data->redirect_path.c_str());
        return result;
    };
    auto vfspathFileExists = [&](auto filename)
    {
        if (data->package_vfs_path.empty())
        {
            Log(L"[%d]\tFindNextFileV2 vfspathFileExists returns false.", FindNextFileInstance2);
            return false;
        }
        //LogString(FindNextFileInstance2, L"\tFindNextFileV2 vfspathFileExists versus", filename);

        auto revertSize = data->package_vfs_path.length();

        // NOTE: 'is_ansi' evaluation not inline due to the bug:
        //       https://developercommunity.visualstudio.com/content/problem/324366/illegal-indirection-error-when-the-evaluation-of-i.html
        constexpr bool is_ansi = psf::is_ansi<std::decay_t<decltype(*filename)>>;
        if constexpr (is_ansi)
        {
            std::wstring wFilename = widen(filename);
            if (wcscmp(wFilename.c_str(), L"..") == 0 ||
                wcscmp(wFilename.c_str(), L".") == 0)
            {
                // assume previously returned as part of requesteds
                Log(L"[%d]\tFindNextFileV2 A vfspathFileExists returns true %ls", FindNextFileInstance2, wFilename.c_str());
                return true;
            }
            data->package_vfs_path += wFilename;
        }
        else
        {
            if (wcscmp(filename, L"..") == 0 ||
                wcscmp(filename, L".") == 0)
            {
                // assume previously returned as part of requesteds
                Log(L"[%d]\tFindNextFileV2 W vfspathFileExists returns true %ls", FindNextFileInstance2, filename);
                return true;
            }
            data->package_vfs_path += filename;
        }

        auto result = impl::PathExists(data->package_vfs_path.c_str());
        data->package_vfs_path.resize(revertSize);
        Log(L"[%d]\tFindNextFileV2 vfspathFileExists returns 0x%x %ls", FindNextFileInstance2, (int)result, data->package_vfs_path.c_str());
        return result;
    };
    auto requestedFileExists = [&](auto filename)
    {
        if (data->requested_path.empty())
        {
            Log(L"[%d]\tFindNextFileV2 requestedFileExists returns false.", FindNextFileInstance2);
            return false;
        }
        LogString(FindNextFileInstance2,L"\tFindNextFileV2 requestedFileExists versus", filename);

        auto revertSize = data->requested_path.length();

        // NOTE: 'is_ansi' evaluation not inline due to the bug:
        //       https://developercommunity.visualstudio.com/content/problem/324366/illegal-indirection-error-when-the-evaluation-of-i.html
        constexpr bool is_ansi = psf::is_ansi<std::decay_t<decltype(*filename)>>;
        if constexpr (is_ansi)
        {
            std::wstring wFilename = widen(filename);
            if (wcscmp(wFilename.c_str(), L"..") == 0 ||
                wcscmp(wFilename.c_str(), L".") == 0)
            {
                // assume previously returned as part of requesteds
                Log(L"[%d]\tFindNextFileV2 A requestedFileExists returns true %ls", FindNextFileInstance2, wFilename.c_str());
                return true;
            }
            data->requested_path += wFilename;
        }
        else
        {
            if (wcscmp(filename, L"..") == 0 ||
                wcscmp(filename, L".") == 0)
            {
                // assume previously returned as part of requesteds
                Log(L"[%d]\tFindNextFileV2 W requestedFileExists returns true %ls", FindNextFileInstance2, filename);
                return true;
            }
            data->requested_path += filename;
        }

        auto result = impl::PathExists(data->requested_path.c_str());
        data->requested_path.resize(revertSize);
        Log(L"[%d]\tFindNextFileV2 requestedFileExists returns 0x%x %ls", FindNextFileInstance2, (int)result, data->requested_path.c_str());
        return result;
    };
#endif
    auto wasFileAlreadyProvided = [&](std::wstring findrequest, auto filename)
    {
#if _DEBUG
        LogString(FindNextFileInstance2, L"\tFindNextFileV2 wasFileAlreadyProvided versus", filename);
#endif

        if (data->already_returned_list.empty())
        {
#if _DEBUG
            Log(L"[%d]\tFindNextFileV2 wasFileAlreadyProvided returns false.", FindNextFileInstance2);
#endif
            return false;
        }
        
        std::wstring wFilename;
        // NOTE: 'is_ansi' evaluation not inline due to the bug:
        //       https://developercommunity.visualstudio.com/content/problem/324366/illegal-indirection-error-when-the-evaluation-of-i.html
        constexpr bool is_ansi = psf::is_ansi<std::decay_t<decltype(*filename)>>;
        if constexpr (is_ansi)
        {
            wFilename = widen(filename);
        }
        else
        {
            wFilename = filename;
        }

#if NOREMOVE
        if (wcscmp(wFilename.c_str(), L"..") == 0 ||
            wcscmp(wFilename.c_str(), L".") == 0)
        {
            // assume previously returned as part of requesteds
#if _DEBUG
            Log(L"[%d]\tFindNextFileV2 A wasFileAlreadyProvided returns true %ls", FindNextFileInstance2, wFilename.c_str());
#endif
            return true;
        }
#endif

// always return false on directories as these are always considered merged.
        std::filesystem::path fullpath = findrequest.c_str();
        fullpath  = fullpath.parent_path() / wFilename.c_str();
#if WasntABadIdea
        // Maybe not.  The caller only needs the folder once, it doesn't see the path that succeeded.
        // Subsequent calls to other file APIs will redirect as needed.
#if _DEBUG
        Log(L"[%d]\tDEBUG FindNextFileV2 constructed path %ls", FindNextFileInstance2, fullpath.c_str());
#endif
        if (std::filesystem::is_directory(fullpath))
        {
#if _DEBUG
            Log(L"[%d]\tFindNextFileV2 Is a directory so wasFileAlreadyProvided returns false %ls", FindNextFileInstance2, wFilename.c_str());
#endif
            return false;
        }
#endif

        _locale_t locale = _wcreate_locale(LC_ALL,L"");
        for (std::wstring check : data->already_returned_list)
        {
//            if (check.compare(wFilename.c_str()) == 0)
            if (_wcsicmp_l(check.c_str(),wFilename.c_str(),locale) == 0)
            {
#if _DEBUG
                Log(L"[%d][%d]\tFindNextFileV2 A wasFileAlreadyProvided returns true %ls", data->RememberedInstance, FindNextFileInstance2, wFilename.c_str());
#endif
                _free_locale(locale);
                return true;
            }
        }
        _free_locale(locale);

#if _DEBUG
        Log(L"[%d][%d]\tFindNextFileV2 wasFileAlreadyProvided returns false", data->RememberedInstance, FindNextFileInstance2);
#endif
        return false;
    };

    if (!data->redirect_path.empty())
    {
        if (data->find_handles[0])
        {
            //Log(L"[%d]FindNextFileV2[0] to be checked.", FindNextFileInstance2);
            if (impl::FindNextFile(data->find_handles[0].get(), findFileData))
            {
#if _DEBUG
                Log(L"[%d][%d]FindNextFileV2[0] returns TRUE: %ls", data->RememberedInstance, FindNextFileInstance2, widen(findFileData->cFileName).c_str());
#endif
                data->already_returned_list.push_back(widen(findFileData->cFileName));
                return TRUE;
            }
            else if (::GetLastError() == ERROR_NO_MORE_FILES)
            {
                ///Log(L"[%d][%d]FindNextFileV2[0] had FALSE with ERROR_NO_MORE_FILES.", data->RememberedInstance, FindNextFileInstance2);
                data->find_handles[0].reset();
                ::SetLastError(ERROR_NO_MORE_FILES);
                // now check[1]
            }
            else
            {
                // Error due to something other than reaching the end 
#if _DEBUG
                Log(L"[%d][%d]FindNextFileV2[0] returns FALSE 0x%x", data->RememberedInstance, FindNextFileInstance2, ::GetLastError());
#endif
                return FALSE;
            }
        }
    }

    if (!data->package_vfs_path.empty())
    {
        while (data->find_handles[1])
        {
            if (impl::FindNextFile(data->find_handles[1].get(), findFileData))
            {
                // Skip the file if the name was previously used, unless it is a directory
                if (!wasFileAlreadyProvided(data->package_vfs_path,findFileData->cFileName))
                {
#if _DEBUG
                    Log(L"[%d][%d] FindNextFileV2[1] returns TRUE with ERROR_SUCCESS and file %ls", data->RememberedInstance, FindNextFileInstance2, widen(findFileData->cFileName).c_str());
#endif
                    data->already_returned_list.push_back(widen(findFileData->cFileName));
                    ::SetLastError(ERROR_SUCCESS);
                    return TRUE;
                }
                else
                {
                    // Otherwise, skip this file and check the next one
#if _DEBUG
                    Log(L"[%d][%d] FindNextFileV2[1] skips file %ls", data->RememberedInstance, FindNextFileInstance2, widen(findFileData->cFileName).c_str());
#endif
                }
            }
            else if (::GetLastError() == ERROR_NO_MORE_FILES)
            {
                ///Log(L"[%d][%d]FindNextFileV2[1] had FALSE with ERROR_NO_MORE_FILES.", data->RememberedInstance, FindNextFileInstance2);
                data->find_handles[1].reset();
                ::SetLastError(ERROR_NO_MORE_FILES);
                // now check [2]
            }
            else
            {
#if _DEBUG
                Log(L"[%d][%d]FindNextFileV2[1] returns FALSE 0x%x", data->RememberedInstance, FindNextFileInstance2, ::GetLastError());
#endif
                // Error due to something other than reaching the end
                return FALSE;
            }
        }
    }

    if (!data->requested_path.empty())
    {
        while (data->find_handles[2])
        {
            if (impl::FindNextFile(data->find_handles[2].get(), findFileData))
            {
                if (wcscmp(widen(findFileData->cFileName).c_str(), L"*") == 0)
                {
                    // Skip this entry as artifact
                    ///Log(L"[%d][%d] FindNextFileV2[2] returns TRUE with ERROR_SUCCESS and artifact file *", data->RememberedInstance, FindNextFileInstance2);
                    // If this works, spread to other cases too...
                }
                else
                {
                    // Skip the file if the name was previously used, unless it is a directory
                    if (!wasFileAlreadyProvided(data->requested_path, findFileData->cFileName))
                    {
#if _DEBUG
                        Log(L"[%d][%d] FindNextFileV2[2] returns TRUE with ERROR_SUCCESS and file %ls", data->RememberedInstance, FindNextFileInstance2, widen(findFileData->cFileName).c_str());
#endif
                        data->already_returned_list.push_back(widen(findFileData->cFileName));
                        ::SetLastError(ERROR_SUCCESS);
                        return TRUE;
                    }
                    else
                    {
                        // Otherwise, skip this file and check the next one
#if _DEBUG
                        Log(L"[%d][%d] FindNextFileV2[2] skips file %ls", data->RememberedInstance, FindNextFileInstance2, widen(findFileData->cFileName).c_str());
#endif
                    }
                }
            }
            else if (::GetLastError() == ERROR_NO_MORE_FILES)
            {
                ///Log(L"[%d][%d]FindNextFileV2[2] had FALSE with ERROR_NO_MORE_FILES.", data->RememberedInstance, FindNextFileInstance2);
                data->find_handles[2].reset();
                ::SetLastError(ERROR_NO_MORE_FILES);
                ///now check [3]
            }
            else
            {
#if _DEBUG
                Log(L"[%d][%d]FindNextFileV2[2] returns FALSE 0x%x", data->RememberedInstance, FindNextFileInstance2, ::GetLastError());
#endif
                // Error due to something other than reaching the end
                return FALSE;
            }
        }
    }


    if (!data->package_devfs_path.empty())
    {
        while (data->find_handles[3])
        {
            if (impl::FindNextFile(data->find_handles[3].get(), findFileData))
            {
                // Skip the file if the name was previously used, unless it is a directory
                if (!wasFileAlreadyProvided(data->package_devfs_path, findFileData->cFileName))
                {
#if _DEBUG
                    Log(L"[%d][%d] FindNextFileV2[3] returns TRUE with ERROR_SUCCESS and file %ls", data->RememberedInstance, FindNextFileInstance2, widen(findFileData->cFileName).c_str());
#endif
                    data->already_returned_list.push_back(widen(findFileData->cFileName));
                    ::SetLastError(ERROR_SUCCESS);
                    return TRUE;
                }
                else
                {
                    // Otherwise, skip this file and check the next one
#if _DEBUG
                    Log(L"[%d][%d] FindNextFileV2[3] skips file %ls", data->RememberedInstance, FindNextFileInstance2, widen(findFileData->cFileName).c_str());
#endif
                }
            }
            else if (::GetLastError() == ERROR_NO_MORE_FILES)
            {
#if _DEBUG
                Log(L"[%d][%d]FindNextFileV2[3] returns FALSE with ERROR_NO_MORE_FILES.", data->RememberedInstance, FindNextFileInstance2);
#endif
                data->find_handles[3].reset();
                ::SetLastError(ERROR_NO_MORE_FILES);
                ///now check [4];
            }
            else
            {
#if _DEBUG
                Log(L"[%d][%d]FindNextFileV2[3] returns FALSE 0x%x", data->RememberedInstance, FindNextFileInstance2, ::GetLastError());
#endif
                // Error due to something other than reaching the end
                // return existing error code
                return FALSE;
            }
        }
    }


    if (!data->package_deredirect_path.empty())
    {
        while (data->find_handles[4])
        {
            if (impl::FindNextFile(data->find_handles[4].get(), findFileData))
            {
                // Skip the file if the name was previously used, unless it is a directory
                if (!wasFileAlreadyProvided(data->package_deredirect_path, findFileData->cFileName))
                {
#if _DEBUG
                    Log(L"[%d][%d] FindNextFileV2[4] returns TRUE with ERROR_SUCCESS and file %ls", data->RememberedInstance, FindNextFileInstance2, widen(findFileData->cFileName).c_str());
#endif
                    data->already_returned_list.push_back(widen(findFileData->cFileName));
                    ::SetLastError(ERROR_SUCCESS);
                    return TRUE;
                }
                else
                {
                    // Otherwise, skip this file and check the next one
#if _DEBUG
                    Log(L"[%d][%d] FindNextFileV2[4] skips file %ls", data->RememberedInstance, FindNextFileInstance2, widen(findFileData->cFileName).c_str());
#endif
                }
            }
            else if (::GetLastError() == ERROR_NO_MORE_FILES)
            {
#if _DEBUG
                Log(L"[%d][%d]FindNextFileV2[4] returns FALSE with ERROR_NO_MORE_FILES.", data->RememberedInstance, FindNextFileInstance2);
#endif
                data->find_handles[4].reset();
                ::SetLastError(ERROR_NO_MORE_FILES);
                return FALSE;
            }
            else
            {
#if _DEBUG
                Log(L"[%d][%d]FindNextFileV2[4] returns FALSE 0x%x", data->RememberedInstance, FindNextFileInstance2, ::GetLastError());
#endif
                // Error due to something other than reaching the end
                // return existing error code
                return FALSE;
            }
        }
    }


    // We ran out of data either on a previous call, or by ignoring files that have been redirected
#if _DEBUG
    Log(L"[%d][%d]FindNextFileV2 returns FALSE with ERROR_NO_MORE_FILES.", data->RememberedInstance, FindNextFileInstance2);
#endif
    ::SetLastError(ERROR_NO_MORE_FILES);
    return FALSE;

}
catch (...)
{
    ::SetLastError(win32_from_caught_exception());
    return FALSE;
}
DECLARE_STRING_FIXUP(impl::FindNextFile, FindNextFileFixupV2);

BOOL __stdcall FindCloseFixupV2(_Inout_ HANDLE findHandle) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    if (!guard)
    {
#if _DEBUG
        Log(L"FindCloseFixupV2");
#endif
        return impl::FindClose(findHandle);
    }

#if _DEBUG
    DWORD FindCloseInstance = ++g_FileIntceptInstance;
#endif
    if (findHandle == INVALID_HANDLE_VALUE)
    {
        ::SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

#if _DEBUG
    auto data = reinterpret_cast<find_data2*>(findHandle);
    Log(L"[%d][%d]FindCloseFixupV2.", data->RememberedInstance, FindCloseInstance);
#endif

    delete reinterpret_cast<find_data2*>(findHandle);
    ::SetLastError(ERROR_SUCCESS);
    return TRUE;
}
DECLARE_FIXUP(impl::FindClose, FindCloseFixupV2);

#endif

