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

#if USE_FINDFIXUP2
#else
struct find_data
{
    // Redirected path directory so that we can avoid returning duplicate filenames. This value will be empty if the
    // path does not exist/match any existing files at the start of the enumeration, for a slight optimization
    std::wstring redirect_path;
    std::wstring package_vfs_path;
    std::wstring requested_path;

    // The first value is the find handle for the "redirected path" (typically to the user's profile). 
    // The second value is for a "package VFS" location equivalent to the requested information when native AppData and LocalAppdata are used in the request.
    // The third is the find handle for the non-redirected path as asked for by the app.
    // The values are set to INVALID_HANDLE_VALUE as enumeration completes.
    
    // Note: When the original request was in the path of the package, the second handle is NULL.
    //       When the original request was a path outside of package path, second handle is NULL if no equivalent VFS exists in package.
    //       First handle is NULL when redirection has not happened yet.
    //       We need the second handle only for VFS/AppData and VFS/LocalAppdata in the package because the underlying runtime doesn't layer those folders in when we use %Appdata% and %LocalAppData% for the third handle.
    unique_find_handle find_handles[3];

    // We need to hold on to the results of FindFirstFile for find_handles[1/2] 
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

void LogNormalizedPath(normalized_path np, std::wstring desc, DWORD instance)
{
    Log(L"[%d]\tNormalized_path %ls Type=%x, Full=%ls, Abs=%ls", instance, desc.c_str(), (int)np.path_type, np.full_path.c_str(), np.drive_absolute_path);
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
        LogString(L"\tFindFirstFileExFixup: for fileName", fileName);

        return impl::FindFirstFileEx(fileName, infoLevelId, findFileData, searchOp, searchFilter, additionalFlags);
    }
    DWORD FindFirstFileExInstance = ++g_FileIntceptInstance;


    // Split the input into directory and pattern
    auto wPath = widen(fileName);
    auto wfileName = widen(fileName);
    LogString(FindFirstFileExInstance,L"\tFindFirstFileEx: for fileName", wfileName.c_str());
    Log(L"[%d]\tFindFirstFileEx: InfoLevel=0x%x searchOp=0x%x addtionalFlags=0x%x ", FindFirstFileExInstance, infoLevelId, searchOp, additionalFlags);

    normalized_path dir;
    const wchar_t* pattern = nullptr;
    wchar_t dal[512];
    if (auto dirPos = wPath.find_last_of(LR"(\/)"); dirPos != std::wstring::npos)
    {
        Log("[%d]\tFileFirstFileEx: has slash", FindFirstFileExInstance);
        // Special case for single separator at beginning of the wPath "/foo.txt"
        if (dirPos == 0)
        {
            auto nextChar = std::exchange(wPath[dirPos + 1], L'\0');
            Log("[%d]\tdir comes from single char case =%ls", FindFirstFileExInstance,  wPath.c_str() );
            dir = NormalizePath(wPath.c_str());
            wPath[dirPos + 1] = nextChar;
        }
        else if (std::iswalpha(wPath[0]) && (wPath[1] == ':'))
        {
            // Ensure dir path_type is absolute drive and not drive relative
            auto nextChar = std::exchange(wPath[dirPos + 1], L'\0');
            Log("[%d]\tdir comes from driveroot case=%ls", FindFirstFileExInstance, wPath.c_str() );
            dir = NormalizePath(wPath.c_str());
            // Avoid bug in NormalizePath causing absolute_path to fail.
            Log(L"[%d]\t\tnormalabs prefix=%ls", FindFirstFileExInstance, dir.drive_absolute_path);
            if (dir.drive_absolute_path == nullptr)
            {
                size_t dirlen = wcslen(wPath.c_str());
                if (dirlen < 511)
                {
                    wcsncpy_s(dal, wPath.c_str(), dirlen);
                    dal[(int)dirlen] = L'\0';   /* null character manually added */
                    dir.drive_absolute_path = (wchar_t*)dal;
                }
            }
            wPath[dirPos + 1] = nextChar;
            Log(L"[%d]\t\tnormalabs postfix=%ls", FindFirstFileExInstance, dir.drive_absolute_path);
        }
        else
        {
            auto separator = std::exchange(wPath[dirPos], L'\0');
            Log("[%d]\tdir comes from =%ls", FindFirstFileExInstance, wPath.c_str() );
            dir = NormalizePath(wPath.c_str());
            wPath[dirPos] = separator;
        }
        pattern = wPath.c_str() + dirPos + 1;
    }
    else
    {
        Log("[%d]\tFileFirstFileEx: no slash, assume cwd based.", FindFirstFileExInstance);
        // TODO: This is a messy situation and the code I am replacing doen't handle it well and can crash the app later on in PathRedirection.
        // While FindFirstFileEx is usually passed a regular (unique) filepath in the first parameter, 
        // it is permissible for the caller to use wildcards to select multiple subfolders or files to be searched.
        // And if they don't give a full path (we end up here and) we should assume it means the wildcards are relative to the current working directory.
        // For example, the app I'm looking at puts "*" in this field. This currently causes NormalizePath to return a normailzed path that is type relative but with a null entry for the path.
        // The right solution might be to first find subfolders matching the path pattern and make individual layered calls on the rest, but that seems too ugly.
        // Here is the code being replaced, but while it fixes the "*" I'm not comforatble that the replacement is right in all cases, 
        // so this comment is here to try to explain.
        //     dir = NormalizePath(L".");
        //     pattern = path.c_str();
        std::filesystem::path cwd = std::filesystem::current_path();
        Log("[%d]\tFileFirstFileEx: swap to cwd: %ls", FindFirstFileExInstance,cwd.c_str());
        dir = NormalizePath(cwd.c_str()); 
        pattern = wPath.c_str();
        Log("[%d]\tFileFirstFileEx: no slash, assumed cwd based type=x%x dap=%ls", FindFirstFileExInstance, psf::path_type(cwd.c_str()),dir.drive_absolute_path);
    }
    Log("[%d]\tpattern=%ls wPath=%ls", FindFirstFileExInstance, pattern, wPath.c_str());

    // If you change the below logic, or
	// you you change what goes into RedirectedPath
	// you need to mirror all changes to ShouldRedirectImpl
	// in PathRedirection.cpp
 
    //Basically, what goes into RedirectedPath here also needs to go into 
    // RedirectedPath in PathRedirection.cpp
    LogNormalizedPath(dir, L"dir before NP", FindFirstFileExInstance);
    if (dir.path_type == psf::dos_path_type::local_device || dir.path_type == psf::dos_path_type::root_local_device)
    {
        dir = NormalizePath(dir.drive_absolute_path);
        LogNormalizedPath(dir, L"dir mid NP", FindFirstFileExInstance);
    }
    if (dir.full_path.back() != L'\\')
    {
        dir.full_path.push_back(L'\\');
        dir.full_path.append(pattern);  // not really sure why we used the directory and then have to put this back on...
        if (dir.drive_absolute_path != nullptr)
        {
            std::wstring tmp = dir.drive_absolute_path;
            tmp.append(L"'\\");
            tmp.append(pattern);
            dir.drive_absolute_path =tmp.data();
        }
    }
    LogNormalizedPath(dir, L"dir before VP", FindFirstFileExInstance);
    dir = VirtualizePath(std::move(dir), FindFirstFileExInstance);
    LogNormalizedPath(dir, L"dir after VP", FindFirstFileExInstance);
    
    auto result = std::make_unique<find_data>();

    result->requested_path = wPath.c_str(); 
    /////result->requested_path = wfileName.c_str();
    //if (result->requested_path.back() != L'\\')
    //{
    //   result->requested_path.push_back(L'\\');
    //}
    Log(L"[%d]FindFirstFile requested_path for [2] (from original) is", FindFirstFileExInstance);
    Log(result->requested_path.c_str());

    result->redirect_path = RedirectedPath(dir,false, FindFirstFileExInstance);
    //if (result->redirect_path.back() != L'\\')
    //{
    //    result->redirect_path.push_back(L'\\');
    //}

    Log(L"[%d]FindFirstFile redirected_path for [0] (from redirected) is", FindFirstFileExInstance);
    Log(result->redirect_path.c_str());
    
    size_t foundWA = wPath.find(L"\\WindowsApps");
    size_t foundVFS = wPath.find(L"\\VFS");
    if (foundWA == std::wstring::npos || foundVFS == std::wstring::npos)
    {
        Log(L"[%d]FindFirstFile wPath not in package.", FindFirstFileExInstance);
        std::filesystem::path vfspath = GetPackageVFSPath(wPath.c_str());
        Log(L"[%d]debug FindFirstFile wPath after GetPackageVFSPath.", FindFirstFileExInstance);
        if (wcslen(vfspath.c_str()) > 0)
        {
            result->package_vfs_path = vfspath.c_str();
            Log(L"[%d]FindFirstFile package_vfs_path for [1] (from vfs_path) is", FindFirstFileExInstance);
            Log(result->package_vfs_path.c_str());
        }
        else
        {
            result->package_vfs_path = result->redirect_path; //dir.full_path.c_str();
            Log(L"[%d]FindFirstFile package_vfs_path for [1] (from vfs_path) is (non AppData) so not applicable", FindFirstFileExInstance);
            Log(result->package_vfs_path.c_str());
        }
        

    }
    else
    {
        // input path was the VFS path so no need to look here 
        //result->package_vfs_path =  dir.full_path.c_str();
        //if (result->package_vfs_path.back() != L'\\')
        //{
        //    result->package_vfs_path.push_back(L'\\');
        //}
        Log(L"[%d]FindFirstFile package_vfs_path for [1] (from vfs_path) is not applicable", FindFirstFileExInstance);
    }

    [[maybe_unused]] auto ansiData = reinterpret_cast<WIN32_FIND_DATAA*>(findFileData);
    [[maybe_unused]] auto wideData = reinterpret_cast<WIN32_FIND_DATAW*>(findFileData);
    WIN32_FIND_DATAW* findData = psf::is_ansi<CharT> ? &result->cached_data : wideData;

    //
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
        Log(L"[%d]FindFirstFile[0] (from redirected): had results", FindFirstFileExInstance);
    }
    else
    {
        // Path doesn't exist or match any files. We can safely get away without the redirected file exists check
        result->redirect_path.clear();
        Log(L"[%d]FindFirstFile[0] (from redirected): no results", FindFirstFileExInstance);
    }

    findData = (result->find_handles[0] || psf::is_ansi<CharT>) ? &result->cached_data : wideData;

    //
    // Open the package_vfsP_path handle if AppData or LocalAppData (letting the runtime handle other VFSs)
    // runtime not doing what we want...
    //if (IsUnderUserAppDataLocal(wPath.c_str()) || IsUnderUserAppDataRoaming(wPath.c_str()))
    if (foundVFS == std::wstring::npos)
    {
        ///auto vfspathSize = result->package_vfs_path.length();
        ///result->package_vfs_path += pattern;
        result->find_handles[1].reset(impl::FindFirstFileEx(result->package_vfs_path.c_str(), infoLevelId, findData, searchOp, searchFilter, additionalFlags));
        ///result->package_vfs_path.resize(vfspathSize);
        if (result->find_handles[1])
        {
            Log(L"[%d]FindFirstFile[1] (from vfs_path):   had results", FindFirstFileExInstance);
        }
        else
        {
            result->package_vfs_path.clear();
            Log(L"[%d]FindFirstFile[1] (from vfs_path):   no results", FindFirstFileExInstance);
        }
        if (!result->find_handles[0])
        {
            if constexpr (psf::is_ansi<CharT>)
            {
                if (copy_find_data(*findData, *ansiData))
                {
                    Log(L"[%d]FindFirstFile error set by caller", FindFirstFileExInstance);
                    // NOTE: Last error set by caller
                    return INVALID_HANDLE_VALUE;
                }
            }
            else
            {
                // No need to copy since we wrote directly into the output buffer
                assert(findData == wideData);
            }
        }
    }

    //
    // Open the non-redirected find handle
    result->find_handles[2].reset(impl::FindFirstFileEx(result->requested_path.c_str(), infoLevelId, findData, searchOp, searchFilter, additionalFlags));
    if (result->find_handles[2])
    {
        Log(L"[%d]FindFirstFile[2] (from origial):    had results", FindFirstFileExInstance);
    }
    else
    {
        result->requested_path.clear();
        Log(L"[%d]FindFirstFile[2] (from original):   no results", FindFirstFileExInstance);
    }
    if (!result->find_handles[0] &&
        !result->find_handles[1])
    {
        if (!result->find_handles[2])
        {
            // Neither path exists. The last error should still be set by FindFirstFileEx, but prefer the initial error
            // if it indicates that the redirected directory structure exists
            if (initialFindError == ERROR_FILE_NOT_FOUND)
            {
                Log(L"[%d]FindFirstFile error 0x%x", FindFirstFileExInstance, initialFindError);
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
                    Log(L"[%d]FindFirstFile error set by caller", FindFirstFileExInstance);
                    // NOTE: Last error set by caller
                    return INVALID_HANDLE_VALUE;
                }
            }
            else
            {
                // No need to copy since we wrote directly into the output buffer
                assert(findData == wideData);
            }
        }
    }

    Log(L"[%d]FindFirstFile returns %ls", FindFirstFileExInstance, result->cached_data.cFileName);
    ::SetLastError(ERROR_SUCCESS);
    return reinterpret_cast<HANDLE>(result.release());

}
catch (...)
{
    // NOTE: Since we allocate our own "find handle" memory, we can't just forward on to the implementation
    ::SetLastError(win32_from_caught_exception());
    Log(L"***FindDirstFileExFixup Exception***");
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
        Log(L"FindNextFileFixup for file.");

        return impl::FindNextFile(findFile, findFileData);
    }

    DWORD FindNextFileInstance = ++g_FileIntceptInstance;

    


    if (findFile == INVALID_HANDLE_VALUE)
    {
        ::SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    auto data = reinterpret_cast<find_data*>(findFile);

    Log(L"[%d]FindNextFileFixup is against redir  =%ls", FindNextFileInstance, data->redirect_path.c_str());
    Log(L"[%d]FindNextFileFixup is against pkgVfs =%ls", FindNextFileInstance, data->package_vfs_path.c_str());
    Log(L"[%d]FindNextFileFixup is against request=%ls", FindNextFileInstance, data->requested_path.c_str());


    auto redirectedFileExists = [&](auto filename)
    {
        if (data->redirect_path.empty())
        {
            Log(L"[%d]FindNextFile redirectedFileExists returns false.", FindNextFileInstance);
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
        Log(L"[%d]FindNextFile redirectedFileExists returns %ls", FindNextFileInstance, data->redirect_path.c_str());
        return result;
    };
    auto vfspathFileExists = [&](auto filename)
    {
        if (data->package_vfs_path.empty())
        {
            Log(L"[%d]FindNextFile vfspathFileExists returns false.", FindNextFileInstance);
            return false;
        }

        auto revertSize = data->package_vfs_path.length();

        // NOTE: 'is_ansi' evaluation not inline due to the bug:
        //       https://developercommunity.visualstudio.com/content/problem/324366/illegal-indirection-error-when-the-evaluation-of-i.html
        constexpr bool is_ansi = psf::is_ansi<std::decay_t<decltype(*filename)>>;
        if constexpr (is_ansi)
        {
            data->package_vfs_path += widen(filename);
        }
        else
        {
            data->package_vfs_path += filename;
        }

        auto result = impl::PathExists(data->package_vfs_path.c_str());
        data->package_vfs_path.resize(revertSize);
        Log(L"[%d]FindNextFile vfspathFileExists returns %ls", FindNextFileInstance, data->package_vfs_path.c_str());
        return result;
    };

    if (!data->redirect_path.empty())
    {
        if (data->find_handles[0])
        {
            Log(L"[%d]FindNextFile[0] to be checked.", FindNextFileInstance);
            if (impl::FindNextFile(data->find_handles[0].get(), findFileData))
            {
                Log(L"[%d]FindNextFile[0] returns TRUE: %ls", FindNextFileInstance, data->cached_data.cFileName);
                return TRUE;
            }
            else if (::GetLastError() == ERROR_NO_MORE_FILES)
            {
                Log(L"[%d]FindNextFile[0] had FALSE with ERROR_NO_MORE_FILES.", FindNextFileInstance);
                data->find_handles[0].reset();

                if (data->package_vfs_path.empty() || !data->find_handles[1])
                {
                    Log(L"[%d]FindNextFile[1] not in use.", FindNextFileInstance);
                    if (data->requested_path.empty() || !data->find_handles[2])
                    {
                        Log(L"[%d]FindNextFile[2] not in use, so return ERROR_NO_MORE_FILES.", FindNextFileInstance);
                        // NOTE: Last error scribbled over by closing find_handles[0]
                        ::SetLastError(ERROR_NO_MORE_FILES);
                        return FALSE;
                    }
                    // else check[1]
                }

                if (!redirectedFileExists(data->cached_data.cFileName))
                {
                    if (copy_find_data(data->cached_data, *findFileData))
                    {
                        Log(L"[%d]FindNextFile[0] returns FALSE with last error set by caller", FindNextFileInstance);
                        // NOTE: Last error set by caller
                        return FALSE;
                    }

                    Log( L"[%x] FindNextFile[0] returns TRUE with ERROR_SUCCESS and file %ls", FindNextFileInstance, data->cached_data.cFileName);
                    ::SetLastError(ERROR_SUCCESS);
                    return TRUE;
                }
            }
            else
            {
                // Error due to something other than reaching the end 
                Log(L"[%d]FindNextFile[0] returns FALSE", FindNextFileInstance);
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
                // Skip the file if it exists in the redirected path
                if (!redirectedFileExists(findFileData->cFileName))
                {
                    Log(L"[%x] FindNextFile[1] returns TRUE with ERROR_SUCCESS and file %ls", FindNextFileInstance, findFileData->cFileName);
                    ::SetLastError(ERROR_SUCCESS);
                    return TRUE;
                }
                // Otherwise, skip this file and check the next one
            }
            else if (::GetLastError() == ERROR_NO_MORE_FILES)
            {
                Log(L"[%d]FindNextFile[1] returns FALSE with ERROR_NO_MORE_FILES_FOUND.", FindNextFileInstance);
                data->find_handles[1].reset();
                // now check [2]
            }
            else
            {
                Log(L"[%d]FindNextFile[1] returns FALSE", FindNextFileInstance);
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
                // Skip the file if it exists in the redirected path
                if (!redirectedFileExists(findFileData->cFileName) &&
                    !vfspathFileExists(findFileData->cFileName))
                {
                    Log(L"[%x] FindNextFile[2] returns TRUE with ERROR_SUCCESS and file %ls", FindNextFileInstance, findFileData->cFileName);
                    ::SetLastError(ERROR_SUCCESS);
                    return TRUE;
                }
                // Otherwise, skip this file and check the next one
            }
            else if (::GetLastError() == ERROR_NO_MORE_FILES)
            {
                Log(L"[%d]FindNextFile[2] returns FALSE with ERROR_NO_MORE_FILES_FOUND.", FindNextFileInstance);
                data->find_handles[2].reset();
                ::SetLastError(ERROR_NO_MORE_FILES);
                return FALSE;
            }
            else
            {
                Log(L"[%d]FindNextFile[2] returns FALSE", FindNextFileInstance);
                // Error due to something other than reaching the end
                return FALSE;
            }
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
        Log(L"FindCloseFixup");

        return impl::FindClose(findHandle);
    }

//    DWORD FindCloseInstance = ++g_FileIntceptInstance;
    Log(L"FindCloseFixup.");
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

#endif