#define CONSOLIDATE
#define DONTREDIRECTIFPARENTNOTINPACKAGE

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <regex>
#include <vector>

#include <known_folders.h>
#include <objbase.h>
#include <psf_framework.h>
#include <psf_logging.h>
#include <utilities.h>

#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <TraceLoggingProvider.h>
#include "Telemetry.h"
#include "RemovePII.h"


#if _DEBUG
#define MOREDEBUG 1
#endif

using namespace std::literals;

extern std::filesystem::path g_packageRootPath;
extern std::filesystem::path g_packageVfsRootPath;
extern std::filesystem::path g_redirectRootPath;
extern std::filesystem::path g_writablePackageRootPath;
extern std::filesystem::path g_finalPackageRootPath;

extern std::vector<vfs_folder_mapping> g_vfsFolderMappings;
extern std::vector<path_redirection_spec> g_redirectionSpecs;

#pragma region NormalizeV2
void LogNormalizedPathV2(normalized_pathV2 np2, std::wstring desc, [[maybe_unused]] DWORD instance)
{
    Log(L"[%d]\tNormalized_path %ls Type=%x, Orig=%ls, Full=%ls, Abs=%ls",
        instance, desc.c_str(), (int)np2.path_type, np2.original_path.c_str(),
        np2.full_path.c_str(), np2.drive_absolute_path.c_str());
}


normalized_pathV2 NormalizePathV2Impl(const wchar_t* path, [[maybe_unused]] DWORD inst)
{
    normalized_pathV2 result;
    ///Log(L"[%d]\t\t\tNormailizePath2Impl",inst);
    result.original_path = path;
    result.path_type = psf::path_type(path);

    std::wstring new_wstring = StripFileColonSlash(result.original_path.c_str());        // removes "file:\\" from start of path if present
    new_wstring = ReplaceSlashBackwardOnly(new_wstring);      // Ensure all slashes are backslashes
    new_wstring = UrlDecode(new_wstring);
    result.full_path = new_wstring;

    ///Log(L"[%d]\t\t\tNormailizePath2Impl post initial decodes.",inst);

    std::filesystem::path cwd;

    switch (result.path_type)
    {
    case psf::dos_path_type::unc_absolute:  // E.g. "\\servername\share\path\to\file"
        result.drive_absolute_path = result.full_path.data();
        break;
    case psf::dos_path_type::drive_absolute:  // E.g. "C:\path\to\file"
        result.drive_absolute_path = result.full_path.data();
        break;
    case psf::dos_path_type::drive_relative:  // E.g. "C:path\to\file"
        cwd = std::filesystem::current_path();
        cwd.append(L"\\");
        cwd.append(result.full_path.data() + 2);
        result.drive_absolute_path = cwd.native();
        break;
    case psf::dos_path_type::rooted:   // E.g. "\path\to\file"
        cwd = std::filesystem::current_path();
        cwd = cwd.root_name();
        cwd /= result.full_path.data();
        result.drive_absolute_path = cwd.native();
        break;
    case psf::dos_path_type::relative:  // E.g. "path\to\file"
        cwd = std::filesystem::current_path();
        cwd /= result.full_path.data();
        result.drive_absolute_path = cwd.native();
        break;
    case psf::dos_path_type::local_device:  // E.g. "\\.\C:\path\to\file"
        result.drive_absolute_path = result.full_path.substr(4); 
        break;
    case psf::dos_path_type::root_local_device:   // E.g. "\\?\C:\path\to\file"
        result.drive_absolute_path = result.full_path.substr(4);
        break;
    case psf::dos_path_type::unknown:
    default:
        result.drive_absolute_path = result.full_path.data();
        break;
    }

#if _DEBUG
    LogString(inst, L"\t\tNormailizePath2Impl orig",result.original_path.data());
    LogString(inst, L"\t\tNormailizePath2Impl full", result.full_path.c_str());
    LogString(inst, L"\t\tNormailizePath2Impl abs", result.drive_absolute_path.c_str());
#endif
    return result;
}

normalized_pathV2 NormalizePathV2(const char* path, DWORD inst)
{
    //Log(L"[%d]\t\tNormalizePathV2 A",inst);
    normalized_pathV2 n2;

    if (path != NULL && path[0] != 0)
    {
        n2.original_path = widen(path).c_str();
        if (IsColonColonGuid(path))
        {
            //Log(L"[%d]\t\t\tNormalizePathV2 A: Guid avoidance.",inst);            
            n2.path_type = psf::dos_path_type::unknown;
            n2.full_path = widen(path).c_str();
            n2.drive_absolute_path = n2.full_path;
            n2.original_path = n2.full_path;
        }
        else if (IsBlobColon(path))  // blob:hexstring has been seen, believed to be associated with writing encrypted data,  Just pass it through as it is not a real file.
        {
            //Log(L"[%d]\t\t\tNormalizePathV2 A: Blob avoidance.",inst);  
            n2.path_type = psf::dos_path_type::unknown;
            n2.full_path = widen(path).c_str();
            n2.drive_absolute_path = n2.full_path;
            n2.original_path = n2.full_path;
        }
        else
        {
            n2 = NormalizePathV2Impl(n2.original_path.c_str(),inst);
        }
    }
    else
    {
        n2 = NormalizePathV2Impl(std::filesystem::current_path().c_str(),inst);
        n2.original_path = nullptr;
    }
    return n2;
}

normalized_pathV2 NormalizePathV2(const wchar_t* path, DWORD inst)
{
    //Log(L"[%d]\t\tNormalizePathV2 W", inst);
    normalized_pathV2 n2;
    if (path != NULL && path[0] != 0)
    {
        n2.original_path = path;
        if (IsColonColonGuid(path))
        {
#if _DEBUG
            Log(L"[%d]\t\t\tNormalizePathV2 W: Guid avoidance.",inst);
#endif
            n2.path_type = psf::dos_path_type::unknown;
            n2.original_path = path;
            n2.full_path = path;
            n2.drive_absolute_path = path;
        }
        else if (IsBlobColon(path))  // blog:hexstring has been seen, believed to be associated with writing encrypted data,  Just pass it through as it is not a real file.
        {
#if _DEBUG
            Log(L"[%d]\t\t\tNormalizePathV2 W: Blob avoidance.",inst);
#endif
            n2.path_type = psf::dos_path_type::unknown;
            n2.full_path = path;
            n2.full_path = path;
            n2.drive_absolute_path = path;
        }
        else
        {
            n2 = NormalizePathV2Impl(n2.original_path.c_str(),inst);
        }
    }
    else
    {
#if _DEBUG
        Log(L"[%d]\t\tNormalizePathV2 W: NULL Use CWD.",inst);
#endif
        n2 = NormalizePathV2Impl(std::filesystem::current_path().c_str(),inst);
        n2.original_path = path;
    }
    return n2;
}

#pragma endregion

#pragma region DevirtualizeV2
void LogDeVirtualizedPathV2(normalized_pathV2 np2, std::wstring desc, [[maybe_unused]] DWORD instance)
{
#if _DEBUG
    Log(L"[%d]\tDeVirtualized_path %ls Type=%x, Orig=%ls, Full=%ls, Abs=%ls", instance, desc.c_str(), (int)np2.path_type, np2.original_path.c_str(), np2.full_path.c_str(), np2.drive_absolute_path.c_str());
#endif
}

std::wstring DeVirtualizePathV2(normalized_pathV2 path)
{
    std::wstring dvPath = L"";

    if (path.path_type == psf::dos_path_type::unknown ||
        path.path_type == psf::dos_path_type::unc_absolute)
    {
        ///Log(L"[%d]\t\tDeVirtualizePath: unknown input type, not devirtualizable.", impl);
    }
    else
    {
        if (path_relative_to(path.drive_absolute_path.c_str(), g_packageVfsRootPath))
        {
            auto packageRelativePath = path.drive_absolute_path.data() + g_packageVfsRootPath.native().length();
            if (psf::is_path_separator(packageRelativePath[0]))
            {
                ++packageRelativePath;
                for (auto& mapping : g_vfsFolderMappings)
                {
                    if (path_relative_to(packageRelativePath, mapping.package_vfs_relative_path))
                    {
                        auto vfsRelativePath = packageRelativePath + mapping.package_vfs_relative_path.native().length();
                        if (psf::is_path_separator(vfsRelativePath[0]))
                        {
                            ++vfsRelativePath;
                        }
                        else if (vfsRelativePath[0])
                        {
                            // E.g. AppVSystem32Catroot2, but matched with AppVSystem32Catroot. This is not the match we are
                            // looking for
                            continue;
                        }

                        // NOTE: We should have already validated that mapping.path is drive-absolute
                        dvPath = (mapping.path / vfsRelativePath).native();
                        break;
                    }
                }
            }
            // Otherwise a directory/file named something like "VFSx" for some non-path separator/null terminator 'x'
        }
    }
    return dvPath;
}
#pragma endregion

#pragma region VirtualizeV2
// If the input path is a physical path outside of the package (e.g. "C:\Windows\System32\foo.txt"),
// this returns what the package VFS equivalent would be (e.g "C:\Program Files\WindowsApps\Packagename\VFS\SystemX64\foo.txt");
// NOTE: Does not check if package has this virtualized path.
std::wstring VirtualizePathV2(normalized_pathV2 path, [[maybe_unused]] DWORD impl)
{
#ifdef MOREDEBUG
    //Log(L"[%d]\t\tVirtualizePathV2: Input original_path %ls", impl, path.original_path.c_str());
    //Log(L"[%d]\t\tVirtualizePathV2: Input full_path %ls", impl, path.full_path.c_str());
    //Log(L"[%d]\t\tVirtualizePathV2: Input drive_absolute_path %ls", impl, path.drive_absolute_path.c_str());
#endif

    std::wstring vPath = L"";

    if (path.path_type == psf::dos_path_type::unknown)
    {
        //Log(L"[%d]\t\tVirtualizePathV2: unknown input type, not virtualizable.", impl);
    }
    else
    {
        if (path_relative_to(path.drive_absolute_path.c_str(), g_packageRootPath))
        {
#ifdef MOREDEBUG
            //Log(L"[%d]\t\tVirtualizePathV2: input, is in package", impl);
#endif
        }
        else
        {
            //I think this iteration must be forward order, otherwise C:\Windows\SysWOW64 matches to VFS\Windows\SysWOW64 instead of VFS\System32
            //for (std::vector<vfs_folder_mapping>::reverse_iterator iter = g_vfsFolderMappings.rbegin(); iter != g_vfsFolderMappings.rend(); ++iter)
            for (std::vector<vfs_folder_mapping>::iterator iter = g_vfsFolderMappings.begin(); iter != g_vfsFolderMappings.end(); ++iter)
            {
                auto& mapping = *iter;
                if (path_relative_to(path.drive_absolute_path.c_str(), mapping.path))
                {
#ifdef MOREDEBUG
                    //LogString(impl, L"\t\t\t mapping entry match on path", mapping.path.wstring().c_str());
                    //LogString(impl, L"\t\t\t package_vfs_relative_path", mapping.package_vfs_relative_path.native().c_str());
                    //Log(L"[%d]\t\t\t rel length =%d, %d", impl, mapping.path.native().length(), mapping.package_vfs_relative_path.native().length());
#endif
                    auto vfsRelativePath = path.drive_absolute_path.data() + mapping.path.native().length();
                    if (psf::is_path_separator(vfsRelativePath[0]))
                    {
                        ++vfsRelativePath;
                    }
#ifdef MOREDEBUG
                    //LogString(impl, L"\t\t\t VfsRelativePath", vfsRelativePath);
#endif
                    vPath = (g_packageVfsRootPath / mapping.package_vfs_relative_path / vfsRelativePath).native();
                    return vPath;
                }
            }
        }
    }

    vPath = g_packageVfsRootPath;
    vPath.push_back(L'\\');
    vPath.push_back(path.drive_absolute_path[0]);
    vPath.push_back('$');  // Replace ':' with '$'
    auto remainingLength = wcslen(path.drive_absolute_path.c_str());
    remainingLength -= 2;
    vPath.append(path.drive_absolute_path.data() + 2, remainingLength);
#ifdef MOREDEBUG
    //Log(L"[%d]\t\tVirtualizePathV2: Output path %ls", impl, vPath.c_str());
#endif
    return vPath;
}
#pragma endregion

#pragma region RedirectedPathV2
/// <summary>
/// Figures out the absolute path to redirect to.
/// </summary>
/// <param name="pathAsRequestedNormalized">The original path from the app</param>
/// <param name="ensureDirectoryStructure">If true, the pathAsRequestedNormalized will be appended to the allowed write location</param>
/// <returns>The new absolute path.</returns>
std::wstring RedirectedPathV2(const normalized_pathV2& pathAsRequestedNormalized, bool ensureDirectoryStructure, std::filesystem::path destinationTargetBase, DWORD inst)
{
    std::wstring result;

    std::wstring basePath;
    std::wstring relativePath;

    bool shouldredirectToPackageRoot = false;
    auto deVirtualizedFullPath = pathAsRequestedNormalized.drive_absolute_path;
    if (pathAsRequestedNormalized.drive_absolute_path.empty())
    {
        deVirtualizedFullPath = pathAsRequestedNormalized.full_path;
    }

    // The package root path is not redirected by this code
    if (IsPackageRoot(deVirtualizedFullPath.c_str()))
        return L"";

    ///if (_wcsicmp(destinationTargetBase.c_str(), g_redirectRootPath.c_str()) == 0)
    if (_wcsicmp(destinationTargetBase.c_str(), g_writablePackageRootPath.c_str()) == 0)
    {
        // PSF defaulted destination target.
        basePath = LR"(\\?\)" + g_writablePackageRootPath.native();
    }
    else
    {
        std::filesystem::path destNoTrailer = psf::remove_trailing_path_separators(destinationTargetBase);
        basePath = LR"(\\?\)" + destNoTrailer.wstring();
    }

    //Lowercase the full path because .find is case-sensitive.
    transform(deVirtualizedFullPath.begin(), deVirtualizedFullPath.end(), deVirtualizedFullPath.begin(), towlower);

    if (deVirtualizedFullPath.find(g_packageRootPath) != std::wstring::npos && deVirtualizedFullPath.length() >= g_packageRootPath.wstring().length())
    {
        //Log(L"[%d]\t\t\tcase: target in package.", inst);
        //LogString(inst, L"      destinationTargetBase:     ", destinationTargetBase.c_str());
        //LogString(inst, L"      g_writablePackageRootPath: ", g_writablePackageRootPath.c_str());

        size_t lengthPackageRootPath = 0;
        auto pathType = psf::path_type(deVirtualizedFullPath.c_str());

        if (pathType != psf::dos_path_type::local_device &&
            pathType != psf::dos_path_type::root_local_device)
        {
            lengthPackageRootPath = g_packageRootPath.native().length();
            //Log(L"[%d] dap length to remove=%d", inst, lengthPackageRootPath);
        }
        else
        {
            lengthPackageRootPath = g_finalPackageRootPath.native().length();
            //Log(L"[%d] !dap length to remove=%d", inst, lengthPackageRootPath);
        }

        if (_wcsicmp(destinationTargetBase.c_str(), g_writablePackageRootPath.c_str()) == 0)
        {
            //Log(L"[%d]\t\t\tsubcase: redirect to default.", inst);
            // PSF defaulted destination target.
            shouldredirectToPackageRoot = true;
            //Log("DAP before");
            relativePath = deVirtualizedFullPath.data() + lengthPackageRootPath;
            //Log("DAP after");
            ///auto stringToTurnIntoAStringView = pathAsRequestedNormalized.full_path.substr(lengthPackageRootPath);
            ///relativePath = std::wstring_view(stringToTurnIntoAStringView);
        }
        else
        {
            Log(L"[%d]\t\t\tsubcase: redirect specified.", inst);
            // PSF configured destination target: probably a home drive.
            std::wstring temprel = deVirtualizedFullPath.data() + lengthPackageRootPath;
            relativePath = L"\\PackageCache\\" + psf::current_package_family_name() + L"\\" + temprel;
        }
    }
    else
    {
#if _DEBUG
        ///Log(L"[%d]\t\t\tcase: target not in package.", inst);
#endif
        //if ( _wcsicmp(deVirtualizedFullPath.substr(0,2).c_str(), L"\\\\")==0)   this test was incorrect ue to local_device and root_local_device cases
        auto pathType = psf::path_type(deVirtualizedFullPath.c_str());
        if (pathType == psf::dos_path_type::unc_absolute)
        {
            // Clearly we should never redirect files from a share
#if _DEBUG
            ///Log("[%d]RedirectedPath: File share case should not be redirected ever.", inst);
#endif
            return pathAsRequestedNormalized.full_path;
        }
        else
        {
            // input location was not in package path but not on share.
            // This function doesn't cause redirection, just specifies where it would have to be.
            // Therefore it is OK to return answers that wouldn't be used. 
                 // TODO: Currently, this code redirects always.  We probably don't want to do that!
                 //       Ideally, we should look closer at the request; the text below is an example of what might be needed.
                 //       If the user asked for a native path and we aren't VFSing close to that path, and it's just a read, we probably shouldn't redirect.
                 //       But let's say it was a write, then probably still don't redirect and let the chips fall where they may.
                 //       But if we have a VFS folder in the package (such as VFS\AppDataCommon\Vendor) with files and the app tries to add a new file using native pathing, then we probably want to redirect.
                 //       There are probably more situations to consider.
                 // To avoid redirecting everything with the current implementation, the configuration spec should be as specific as possible so that we never get here.
            //LogString(inst, L"      destinationTargetBase: ", destinationTargetBase.c_str());
            //LogString(inst, L"      g_redirectRootPath:    ", g_redirectRootPath.c_str());  // don't think we should use this!!
            if (_wcsicmp(destinationTargetBase.c_str(), g_redirectRootPath.c_str()) == 0)
            {
#if _DEBUG
                ///Log(L"[%d]\t\t\tsubcase: redirect to default.", inst);
#endif
                // PSF defaulted destination target.
                relativePath = L"\\";
            }
            else
            {
#if _DEBUG
                ///Log(L"[%d]\t\t\tsubcase: redirect for outside file specified.", inst);
#endif
                // PSF  configured destination target: probably a home drive.
                //relativePath = L"\\PackageCache\\" + psf::current_package_family_name() + +L"\\VFS\\PackageDrive";
                // Not sure what that code in the 2 lines above was, but probably not for the general case of a file outside of the package
                ///relativePath = L"\\PackageCache\\" + psf::current_package_family_name() + +L"\\Microsoft\\WritablePackageRoot\\VFS\\";
                std::wstring virtualized = VirtualizePathV2(pathAsRequestedNormalized, 0);
                /////relativePath = virtualized.c_str() + g_packageVfsRootPath.native().length();
                relativePath = virtualized.c_str() + g_packageRootPath.native().length();
#if _DEBUG
                ///LogString(inst,L"\t\t\tsubcase: virtualized", virtualized.c_str());
                ///LogString(inst,L"\t\t\tsubcase: relativePath", relativePath.c_str());
#endif
            }

            // NTFS doesn't allow colons in filenames, so simplest thing is to just substitute something in; use a dollar sign
            // similar to what's done for UNC paths
            ///// Note: assert here is normal it ignore for FindFileEx with file input "*" case which becomes a different type.
            //LogString(L"[%d]\t\t\tdVirtualized.drive_absolute_path.c_str()", pathAsRequestedNormalized.drive_absolute_path.c_str());
            //assert(psf::path_type(pathAsRequestedNormalized.drive_absolute_path.c_str()) == psf::dos_path_type::drive_absolute);
            //relativePath.push_back(L'\\');
            //relativePath.push_back(pathAsRequestedNormalized.drive_absolute_path[0]);
            //relativePath.push_back('$');
            //auto remainingLength = wcslen(pathAsRequestedNormalized.drive_absolute_path.c_str());
            //remainingLength -= 2;
            //relativePath.append(pathAsRequestedNormalized.drive_absolute_path.data() + 2, remainingLength);
        }
    }

    ////Log(L"\tFRF devirt.full_path %ls", pathAsRequestedNormalized.full_path.c_str());
    ////Log(L"\tFRF devirt.da_path %ls", pathAsRequestedNormalized.drive_absolute_path);
    //LogString(inst, L"\tFRF initial basePath", basePath.c_str());
    //LogString(inst, L"\tFRF initial relative", relativePath.c_str());

    // Create folder structure, if needed
    if (impl::PathExists((basePath + relativePath).c_str()))
    {
        result = basePath + relativePath;
#if _DEBUG
        ///Log(L"[%d]\t\tFRF Found that a copy exists in the redirected area so we skip the folder creation.", inst);
#endif
    }
    else
    {
        std::wstring_view relativePathview = std::wstring_view(relativePath);

        if (shouldredirectToPackageRoot)
        {
            result = GenerateRedirectedPath(relativePath, ensureDirectoryStructure, basePath, inst);
            //Log(L"[%d]\t\tFRF shouldredirectToPackageRoot case returns result", inst);
            //Log(result.c_str());
        }
        else
        {
            result = GenerateRedirectedPath(relativePath, ensureDirectoryStructure, basePath, inst);
            //Log(L"[%d]\t\tFRF not to PackageRoot case returns result", inst);
            //Log(result.c_str());
        }

    }
    return result;
}

std::wstring RedirectedPathV2(const normalized_pathV2& pathAsRequestedNormalized, bool ensureDirectoryStructure, DWORD inst)
{
    // Only until all code paths use the new version of the interface...
    return RedirectedPathV2(pathAsRequestedNormalized, ensureDirectoryStructure, g_writablePackageRootPath.native(), inst);
}
#pragma endregion


#pragma region ShouldRedirectV2

template <typename CharT>
static path_redirect_info ShouldRedirectV2Impl(const CharT* path, redirect_flags flags, DWORD inst)
{
    path_redirect_info result;

    if (!path)
    {
        return result;
    }
#if _DEBUG
    LogString(inst, L"\tFRFShouldRedirectV2: called for path", widen(path).c_str());
#endif
    try
    {

#if _DEBUG
        bool c_presense = flag_set(flags, redirect_flags::check_file_presence);
        bool c_copy = flag_set(flags, redirect_flags::copy_file);
        bool c_ensure = flag_set(flags, redirect_flags::ensure_directory_structure);
        Log(L"[%d]\t\tFRFShouldRedirectV2: flags  CheckPresense:%d  CopyFile:%d  EnsureDirectory:%d", inst, c_presense, c_copy, c_ensure);
#endif

        std::wstring reversedWritablePathWstring = ReverseRedirectedToPackage(widen(path).c_str());
        if (reversedWritablePathWstring.length() > 0)
        {
            // We were provided a path that is in the redirection area. We probably want to use this path,
            // but maybe the area it was redirected from.
#ifdef MOREDEBUG
            LogString(inst, L"\t\tFRFShouldRedirectV2: ReverseRedirection", reversedWritablePathWstring.c_str());
#endif
            // For now, let's return this path and let the caller deal with it.
#if _DEBUG
            LogString(inst, L"\tFRFShouldRedirectV2: Prevent redundant redirection.", widen(path).c_str());
#endif
            return result;
        }

        // normalizedPathv2 represents the requested path, redirected to the external system if relevant, or just as requested if not.
        // pathVirtualizedV2 represents this as a package relative path
        normalized_pathV2 normalizedPathV2 = NormalizePathV2(path, inst);
        std::filesystem::path destinationTargetBase;

        if (normalizedPathV2.path_type == psf::dos_path_type::local_device)
        {
#if _DEBUG
            LogString(L"\tFRFShouldRedirectV2: Path is of type local device so FRF should ignore.", widen(path).c_str());
#endif
            return result;
        }

        if (normalizedPathV2.drive_absolute_path.empty())
        {
            // FUTURE: We could do better about canonicalising paths, but the cost/benefit doesn't make it worth it right now
#if _DEBUG
            Log(L"[%d] ***Normalized has no drive_absolute_path", inst);
#endif
            return result;
        }

#ifdef MOREDEBUG
        LogString(inst, L"\t\tFRFShouldRedirectV2: NormalizedV2.DAP", normalizedPathV2.drive_absolute_path.c_str());
#endif

#ifdef DODEVIRT
        // To be consistent in where we redirect files, we need to map VFS paths to their non-package-relative equivalent,
        // but maybe only for findfile and not these
        std::wstring DeVirtualizedV2 = DeVirtualizePathV2(std::move(normalizedPathV2));
#ifdef MOREDEBUG
        LogString(inst, L"\t\tFRFShouldRedirectV2: DeVirtualizedV2", DeVirtualizedV2.c_str());
#endif
#endif

        // If you change the below logic, or
        // you you change what goes into RedirectedPath
        // you need to mirror all changes in FindFirstFileFixup.cpp

        // Basically, what goes into RedirectedPath here also needs to go into 
        // FindFirstFile/NextFixup.cpp
        std::wstring pathVirtualizedV2 = L"";
        if (normalizedPathV2.path_type != psf::dos_path_type::unknown &&
            normalizedPathV2.path_type != psf::dos_path_type::unc_absolute &&
            !IsUnderPackageRoot(normalizedPathV2.drive_absolute_path.c_str()) &&
            !IsUnderUserPackageWritablePackageRoot(normalizedPathV2.drive_absolute_path.c_str()))
        {
            pathVirtualizedV2 = VirtualizePathV2(normalizedPathV2);
        }
        if (pathVirtualizedV2.length() == 0)
            pathVirtualizedV2 = normalizedPathV2.full_path.c_str();

        auto vfspathV2 = NormalizePathV2(pathVirtualizedV2.c_str(), inst);
        if (!vfspathV2.drive_absolute_path.empty())
        {
#ifdef MOREDEBUG
            LogString(inst, L"\t\tFRFShouldRedirectV2: VirtualizedV2", pathVirtualizedV2.c_str());
#endif
        }


        // Figure out if this VFS path is something we need to redirect
        for (auto& redirectSpec : g_redirectionSpecs)
        {
#ifdef MOREDEBUG
            //LogString(inst, L"\t\tFRFShouldRedirect: Check against: base", redirectSpec.base_path.c_str());
#endif
            if (path_relative_to(pathVirtualizedV2.c_str(), redirectSpec.base_path))
            {
#ifdef MOREDEBUG
                //LogString(inst, L"\t\tFRFShouldRedirect: In ball park of base", redirectSpec.base_path.c_str());
#endif
                auto relativePath = pathVirtualizedV2.c_str() + redirectSpec.base_path.native().length();
                if (psf::is_path_separator(relativePath[0]))
                {
                    ++relativePath;
                }
                else if (relativePath[0])
                {
                    // Otherwise, just a substring match (e.g. we're trying to match against 'foo' but input was 'foobar')
                    continue;
                }
                // Otherwise exact match. Assume an implicit directory separator at the end (e.g. for matches to satisfy the
                // first call to CreateDirectory
#ifdef MOREDEBUG
            //LogString(inst, L"\t\t\tFRFShouldRedirect: relativePath",relativePath);
#endif
                if (std::regex_match(relativePath, redirectSpec.pattern))
                {
                    if (redirectSpec.isExclusion)
                    {
                        // The impact on isExclusion is that redirection is not needed.
                        result.should_redirect = false;
#ifdef MOREDEBUG
                        LogString(inst, L"\t\tFRFShouldRedirectV2 CASE:Exclusion for path", widen(path).c_str());
#endif
                    }
                    else
                    {
                        result.should_redirect = true;
                        result.shouldReadonly = (redirectSpec.isReadOnly == true);

                        // Check if file exists as VFS path in the package
                        std::wstring rldPath = TurnPathIntoRootLocalDevice(pathVirtualizedV2.c_str());
                        if (impl::PathExists(rldPath.c_str()))
                        {
#ifdef MOREDEBUG
                            Log(L"[%d]\t\t\tFRFShouldRedirectV2 CASE:match, existing in package.", inst);
#endif
                            destinationTargetBase = redirectSpec.redirect_targetbase;

#ifdef MOREDEBUG
                            //LogString(inst, L"\t\tFRFShouldRedirect isWide for", pathVirtualizedV2.c_str());
                            //LogString(inst, L"\t\tFRFShouldRedirect isWide redir", destinationTargetBase.c_str());
#endif
                            result.redirect_path = RedirectedPathV2(vfspathV2, flag_set(flags, redirect_flags::ensure_directory_structure), destinationTargetBase, inst);

                        }
                        else
                        {
#ifdef MOREDEBUG
                            Log(L"[%d]\t\t\tFRFShouldRedirectV2 CASE:match, not existing in package.", inst);
#endif
                            // If the folder above it exists, we might want to redirect anyway?
                            //  EX: Folder has VFS\AppData\Vendor
                            //  Request:       ...\AppData\Roaming                      Redirect yes (found above)
                            //  Request:       ...\AppData\Roaming\Vendor               Redirect yes (found above)
                            //  Request:       ...\AppData\Roaming\Vendor\foo           Redirect yes
                            //  Request:       ...\AppData\Roaming\Vendor\foo\bar       Redirect currently yes, was no
                            //  Request:       ...\AppData\Roaming\Vendor\foo\bar\now   Redirect currently yes, was no
                            std::filesystem::path abs = pathVirtualizedV2.c_str();
                            std::filesystem::path abs2vfsvarfolder = trim_absvfs2varfolder(abs);
#ifdef MOREDEBUG
                            LogString(inst, L"\t\t\tFRFShouldRedirectV2 check if VFS var-folder is in package?", abs2vfsvarfolder.c_str());
#endif
                            std::wstring rldPPath = TurnPathIntoRootLocalDevice(abs2vfsvarfolder.c_str());
                            rldPPath = rldPPath.substr(0, rldPPath.find_last_of(L"\\"));
                            if (impl::PathExists(rldPPath.c_str()))
                            {
#ifdef MOREDEBUG
                                Log(L"[%d]\t\t\tFRFShouldRedirectV2 SUBCASE: parent-folder is in package.", inst);
#endif
                                destinationTargetBase = redirectSpec.redirect_targetbase;
                                result.redirect_path = RedirectedPathV2(vfspathV2, flag_set(flags, redirect_flags::ensure_directory_structure), destinationTargetBase, inst);
                            }
                            else
                            {

#ifdef DONTREDIRECTIFPARENTNOTINPACKAGE
                                psf::dos_path_type origType = psf::path_type(path);
#ifdef MOREDEBUG
                                Log(L"[%d]\t\t\tFRFShouldRedirectV2 Orig Type=0%x", inst, (int)origType);
#endif
                                if (origType == psf::dos_path_type::relative)
                                {
#ifdef MOREDEBUG
                                    Log(L"[%d]\t\t\tFRFShouldRedirectV2 SUBCASE: VFS var-folder is also not in package, but req was relative path, we should redirect.", inst);
#endif
                                    destinationTargetBase = redirectSpec.redirect_targetbase;
                                    result.redirect_path = RedirectedPathV2(vfspathV2, flag_set(flags, redirect_flags::ensure_directory_structure), destinationTargetBase, inst);
                                }
                                else
                                {
#ifdef MOREDEBUG
                                    Log(L"[%d]\t\t\tFRFShouldRedirectV2 SUBCASE: VFS var-folder is also not in package, therefore we should NOT redirect.", inst);
#endif
                                    result.should_redirect = false;
                                }
#else
#ifdef MOREDEBUG
                                Log(L"[%d]\t\t\tFRFShouldRedirectV2 SUBCASE: VFS var-folder is also not in package, but since rule exists we should redirect.", inst);
#endif
                                destinationTargetBase = redirectSpec.redirect_targetbase;
                                result.redirect_path = RedirectedPath(vfspath, flag_set(flags, redirect_flags::ensure_directory_structure), destinationTargetBase, inst);
#endif
                            }
                        }
                        if (result.should_redirect)
                        {
#ifdef MOREDEBUG
                            LogString(inst, L"\t\tFRFShouldRedirectV2 CASE:match on redirect_path", result.redirect_path.c_str());
#endif
                        }
                    }
                    break;
                }
                else
                {
#ifdef MOREDEBUG
                    //LogString(inst, L"\t\tFRFShouldRedirectV2: no match on parse relativePath", relativePath);
#endif
                }
            }
            else
            {
#ifdef MOREDEBUG
                //LogString(inst, L"\t\tFRFShouldRedirectV2: Not in ball park of base", redirectSpec.base_path.c_str());
#endif
            }
        }

#ifdef MOREDEBUG
        Log(L"[%d]\t\tFRFShouldRedirectV2 post check 1", inst);
#endif


        if (!result.should_redirect)
        {
#if _DEBUG
            LogString(inst, L"\tFRFShouldRedirectV2: no redirect rule for path", widen(path).c_str());
#endif
            return result;
        }

#ifdef MOREDEBUG
        Log(L"[%d]\t\tFRFShouldRedirectV2 post check 2", inst);
        LogString(inst, L"\t\tFRFShouldRedirectV2 redir path currently", result.redirect_path.c_str());
#endif

        if (flag_set(flags, redirect_flags::check_file_presence))
        {
            std::wstring rldRedir = TurnPathIntoRootLocalDevice(widen(result.redirect_path).c_str());
            std::wstring rldVedir = TurnPathIntoRootLocalDevice(widen(pathVirtualizedV2.c_str()).c_str());
            std::wstring rldNdir = TurnPathIntoRootLocalDevice(widen(normalizedPathV2.drive_absolute_path.c_str()).c_str());
            if (!impl::PathExists(rldRedir.c_str()) &&
                !impl::PathExists(rldVedir.c_str()) &&
                !impl::PathExists(rldNdir.c_str()))
            {
                result.should_redirect = false;
                result.redirect_path.clear();

#if _DEBUG
                LogString(inst, L"\tFRFShouldRedirectV2: skipped (redirected not present check failed) for path", widen(path).c_str());
#endif
                return result;
            }
        }

#ifdef MOREDEBUG
        Log(L"[%d]\t\tFRFShouldRedirectV2 post check 3", inst);
#endif

        if (flag_set(flags, redirect_flags::copy_file))
        {
#ifdef MOREDEBUG
            Log(L"[%d]\t\tFRFShouldRedirectV2: copy_file flag is set", inst);
#endif
            [[maybe_unused]] BOOL copyResult = false;
            if (impl::PathExists(TurnPathIntoRootLocalDevice(widen(result.redirect_path).c_str()).c_str()))
            {
#if _DEBUG
                Log(L"[%d]\t\tFRFShouldRedirectV2: Found that a copy exists in the redirected area so we skip the folder creation.", inst);
#endif
            }
            else
            {
                std::filesystem::path CopySource = normalizedPathV2.drive_absolute_path.c_str();
                if (impl::PathExists( TurnPathIntoRootLocalDevice(pathVirtualizedV2.c_str()).c_str() ))
                {
                    CopySource = pathVirtualizedV2.c_str();
                }


                auto attr = impl::GetFileAttributes(TurnPathIntoRootLocalDevice(CopySource.c_str()).c_str()); //normalizedPath.drive_absolute_path);
#ifdef MOREDEBUG
                Log(L"[%d]\t\tFRFShouldRedirectV2 source %ls attributes=0x%x", inst, CopySource.c_str(), attr);
#endif
                if (attr != INVALID_FILE_ATTRIBUTES)
                {
                    if ((attr & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
                    {
#ifdef MOREDEBUG
                        Log(L"[%d]\tFRFShouldRedirectV2 we have a file to be copied to %ls", inst, result.redirect_path.c_str());
#endif
                        copyResult = impl::CopyFileEx(
                            CopySource.c_str(), //normalizedPath.drive_absolute_path,
                            result.redirect_path.c_str(),
                            nullptr,
                            nullptr,
                            nullptr,
                            COPY_FILE_FAIL_IF_EXISTS | COPY_FILE_NO_BUFFERING);
                        if (copyResult)
                        {
#if _DEBUG
                            LogString(inst, L"\t\tFRFShouldRedirectV2 CopyFile Success From", CopySource.c_str());
                            LogString(inst, L"\t\tFRFShouldRedirectV2 CopyFile Success To", result.redirect_path.c_str());
#endif
                        }
                        else
                        {
                            //0x72 = ERROR_INVALID_TARGET_HANDLE
                            auto err = ::GetLastError();
#if _DEBUG
                            Log("[%d]\t\tFRFShouldRedirectV2 CopyFile Fail=0x%x", inst, err);
                            LogString(inst, L"\t\tFRFShouldRedirectV2 CopyFile Fail From", CopySource.c_str());
                            LogString(inst, L"\t\tFRFShouldRedirectV2 CopyFile Fail To", result.redirect_path.c_str());
#endif
                            switch (err)
                            {
                            case ERROR_FILE_EXISTS:
#if _DEBUG
                                Log(L"[%d]\t\tFRFShouldRedirectV2  was ERROR_FILE_EXISTS", inst);
#endif
                                break;
                            case ERROR_PATH_NOT_FOUND:
#if _DEBUG
                                Log(L"[%d]\t\tFRFShouldRedirectV2  was ERROR_PATH_NOT_FOUND", inst);
#endif
                                break;
                            case ERROR_FILE_NOT_FOUND:
#if _DEBUG
                                Log(L"[%d]\t\tFRFShouldRedirectV2  was ERROR_FILE_NOT_FOUND", inst);
#endif
                                break;
                            case ERROR_ALREADY_EXISTS:
#if _DEBUG
                                Log(L"[%d]\t\tFRFShouldRedirectV2  was ERROR_ALREADY_EXISTS", inst);
#endif
                                break;
                            default:
#if _DEBUG
                                Log(L"[%d]\t\tFRFShouldRedirectV2 was 0x%x", inst, err);
#endif
                                break;
                            }
                        }
                    }
                    else
                    {
#ifdef MOREDEBUG
                        Log(L"[%d]\tFRFShouldRedirectV2 we have a directory to be copied to %ls.", inst, result.redirect_path.c_str());
#endif
                        copyResult = impl::CreateDirectoryEx(CopySource.c_str(), result.redirect_path.c_str(), nullptr);
                        if (copyResult)
                        {
#if _DEBUG
                            LogString(inst, L"\t\tFRFShouldRedirectV2 CreateDir Success From", CopySource.c_str());
                            LogString(inst, L"\t\tFRFShouldRedirectV2 CreateDir Success To", result.redirect_path.c_str());
#endif
                        }
                        else
                        {
#if _DEBUG
                            Log("[%d]\t\tFRFShouldRedirectV2 CreateDir Fail=0x%x", inst, ::GetLastError());
                            LogString(inst, L"\t\tFRFShouldRedirectV2 CreateDir Fail From", CopySource.c_str());
                            LogString(inst, L"\t\tFRFShouldRedirectV2 CreateDir Fail To", result.redirect_path.c_str());
#endif
                        }
#if _DEBUG
                        /////auto err = ::GetLastError();  // saw this happen with a redirected iconccache_32.db file.  Let's not crash!
                        /////assert(copyResult || (err == ERROR_FILE_EXISTS) || (err == ERROR_PATH_NOT_FOUND) || (err == ERROR_FILE_NOT_FOUND) || (err == ERROR_ALREADY_EXISTS));
#endif
                    }
                }
                else
                {
                    //There is no source to copy, so we just want to allow it to be created in the redirected area.
#if _DEBUG
                    LogString(inst,L"\t\tFRFShouldRedirectV2 there is no package file to be copied to",  result.redirect_path.c_str());
#endif 
                }
            }
        }
#if _DEBUG
        LogString(inst, L"\tFRFShouldRedirectV2: returns with result", result.redirect_path.c_str());
#endif
    }
    catch (...)
    {
        Log(L"[%d]*****FRFShouldRedirectV2 Exeption!!", inst);
        result.should_redirect = false;  // What else to do???
    }
    return result;
}

path_redirect_info ShouldRedirectV2(const char* path, redirect_flags flags, DWORD inst)
{
#ifdef MOREDEBUG
    Log(L"[%d]\t\tFRFShouldRedirectV2 A", inst);
#endif
    return ShouldRedirectV2Impl(path, flags, inst);
}

path_redirect_info ShouldRedirectV2(const wchar_t* path, redirect_flags flags, DWORD inst)
{
#ifdef MOREDEBUG
    Log(L"[%d]\t\tFRFShouldRedirectV2 W", inst);
#endif
    return ShouldRedirectV2Impl(path, flags, inst);
}

#pragma endregion