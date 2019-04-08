//-------------------------------------------------------------------------------------------------------    
// Copyright (C) Microsoft Corporation. All rights reserved.    
// Licensed under the MIT license. See LICENSE file in the project root for full license information.    
//-------------------------------------------------------------------------------------------------------    
    
#include <regex>    
#include <vector>    
    
#include <known_folders.h>    
#include <objbase.h>    
#include <psf_framework.h>    
#include <utilities.h>    
    
#include "FunctionImplementations.h"    
#include "PathRedirection.h"    
    
using namespace std::literals;    
    
std::filesystem::path g_packageRootPath;    
std::filesystem::path g_packageVfsRootPath;    
std::filesystem::path g_redirectRootPath;    
std::filesystem::path g_writablePackageRootPath;    
    
struct vfs_folder_mapping    
{    
    std::filesystem::path path;    
    std::filesystem::path package_vfs_relative_path; // E.g. "Windows"    
};    
std::vector<vfs_folder_mapping> g_vfsFolderMappings;    
    
void InitializePaths()    
{    
    // For path comparison's sake - and the fact that std::filesystem::path doesn't handle (root-)local device paths all    
    // that well - ensure that these paths are drive-absolute    
    auto packageRootPath = ::PSFQueryPackageRootPath();    
    if (auto pathType = psf::path_type(packageRootPath);    
        (pathType == psf::dos_path_type::root_local_device) || (pathType == psf::dos_path_type::local_device))    
    {    
        packageRootPath += 4;    
    }    
    assert(psf::path_type(packageRootPath) == psf::dos_path_type::drive_absolute);    
    g_packageRootPath = psf::remove_trailing_path_separators(packageRootPath);    
    g_packageVfsRootPath = g_packageRootPath / L"VFS";    
    
    // Ensure that the redirected root path exists    
    g_redirectRootPath = psf::known_folder(FOLDERID_LocalAppData) / L"VFS";    
    impl::CreateDirectory(g_redirectRootPath.c_str(), nullptr);    
    
    g_writablePackageRootPath = psf::known_folder(FOLDERID_LocalAppData) / L"Packages" / psf::current_package_family_name() / LR"(LocalCache\Local\Microsoft\WritablePackageRoot)";    
    impl::CreateDirectory(g_writablePackageRootPath.c_str(), nullptr);    
    
    // Folder IDs and their desktop bridge packaged VFS location equivalents. Taken from:    
    // https://docs.microsoft.com/en-us/windows/uwp/porting/desktop-to-uwp-behind-the-scenes    
    //      System Location                 Redirected Location (Under [PackageRoot]\VFS)   Valid on architectures    
    //      FOLDERID_SystemX86              SystemX86                                       x86, amd64    
    //      FOLDERID_System                 SystemX64                                       amd64    
    //      FOLDERID_ProgramFilesX86        ProgramFilesX86                                 x86, amd6    
    //      FOLDERID_ProgramFilesX64        ProgramFilesX64                                 amd64    
    //      FOLDERID_ProgramFilesCommonX86  ProgramFilesCommonX86                           x86, amd64    
    //      FOLDERID_ProgramFilesCommonX64  ProgramFilesCommonX64                           amd64    
    //      FOLDERID_Windows                Windows                                         x86, amd64    
    //      FOLDERID_ProgramData            Common AppData                                  x86, amd64    
    //      FOLDERID_System\catroot         AppVSystem32Catroot                             x86, amd64    
    //      FOLDERID_System\catroot2        AppVSystem32Catroot2                            x86, amd64    
    //      FOLDERID_System\drivers\etc     AppVSystem32DriversEtc                          x86, amd64    
    //      FOLDERID_System\driverstore     AppVSystem32Driverstore                         x86, amd64    
    //      FOLDERID_System\logfiles        AppVSystem32Logfiles                            x86, amd64    
    //      FOLDERID_System\spool           AppVSystem32Spool                               x86, amd64    
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_SystemX86), LR"(SystemX86)"sv });    
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_ProgramFilesX86), LR"(ProgramFilesX86)"sv });    
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_ProgramFilesCommonX86), LR"(ProgramFilesCommonX86)"sv });    
#if !_M_IX86    
    // FUTURE: We may want to consider the possibility of a 32-bit application trying to reference "%windir%\sysnative\"    
    //         in which case we'll have to get smarter about how we resolve paths    
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_System), LR"(SystemX64)"sv });    
    // FOLDERID_ProgramFilesX64* not supported for 32-bit applications    
    // FUTURE: We may want to consider the possibility of a 32-bit process trying to access this path anyway. E.g. a    
    //         32-bit child process of a 64-bit process that set the current directory    
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_ProgramFilesX64), LR"(ProgramFilesX64)"sv });    
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_ProgramFilesCommonX64), LR"(ProgramFilesCommonX64)"sv });    
#endif    
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_Windows), LR"(Windows)"sv });    
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_ProgramData), LR"(Common AppData)"sv });    
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_System) / LR"(catroot)"sv, LR"(AppVSystem32Catroot)"sv });    
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_System) / LR"(catroot2)"sv, LR"(AppVSystem32Catroot2)"sv });    
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_System) / LR"(drivers\etc)"sv, LR"(AppVSystem32DriversEtc)"sv });    
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_System) / LR"(driverstore)"sv, LR"(AppVSystem32Driverstore)"sv });    
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_System) / LR"(logfiles)"sv, LR"(AppVSystem32Logfiles)"sv });    
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_System) / LR"(spool)"sv, LR"(AppVSystem32Spool)"sv });    
}    
    
std::filesystem::path path_from_known_folder_string(std::wstring_view str)    
{    
    KNOWNFOLDERID id;    
    
    if (str == L"SystemX86"sv)    
    {    
        id = FOLDERID_SystemX86;    
    }    
    else if (str == L"System"sv)    
    {    
        id = FOLDERID_System;    
    }    
    else if (str == L"ProgramFilesX86"sv)    
    {    
        id = FOLDERID_ProgramFilesX86;    
    }    
    else if (str == L"ProgramFilesCommonX86"sv)    
    {    
        id = FOLDERID_ProgramFilesCommonX86;    
    }    
#if _M_IX86    
    else if ((str == L"ProgramFilesX64"sv) || (str == L"ProgramFilesCommonX64"sv))    
    {    
        return {};    
    }    
#else    
    else if (str == L"ProgramFilesX64"sv)    
    {    
        id = FOLDERID_ProgramFilesX64;    
    }    
    else if (str == L"ProgramFilesCommonX64"sv)    
    {    
        id = FOLDERID_ProgramFilesCommonX64;    
    }    
#endif    
    else if (str == L"Windows"sv)    
    {    
        id = FOLDERID_Windows;    
    }    
    else if (str == L"ProgramData"sv)    
    {    
        id = FOLDERID_ProgramData;    
    }    
    else if ((str.length() >= 38) && (str[0] == '{'))    
    {    
        if (FAILED(::IIDFromString(str.data(), &id)))    
        {    
            return {};    
        }    
    }    
    else    
    {    
        // Unknown    
        return {};    
    }    
    
    return psf::known_folder(id);    
}    
    
struct path_redirection_spec    
{    
    std::filesystem::path base_path;    
    std::wregex pattern;    
};    
    
std::vector<path_redirection_spec> g_redirectionSpecs;    
    
void InitializeConfiguration()    
{    
    if (auto rootConfig = ::PSFQueryCurrentDllConfig())    
    {    
        auto& rootObject = rootConfig->as_object();    
        if (auto pathsValue = rootObject.try_get("redirectedPaths"))    
        {    
            auto& redirectedPathsObject = pathsValue->as_object();    
            auto initializeRedirection = [](const std::filesystem::path& basePath, const psf::json_array& specs)    
            {    
                for (auto& spec : specs)    
                {    
                    auto& specObject = spec.as_object();    
                    auto path = psf::remove_trailing_path_separators(basePath / specObject.get("base").as_string().wstring());    
                    for (auto& pattern : specObject.get("patterns").as_array())    
                    {    
                        auto patternString = pattern.as_string().wstring();    
    
                        g_redirectionSpecs.emplace_back();    
                        g_redirectionSpecs.back().base_path = path;    
                        g_redirectionSpecs.back().pattern.assign(patternString.data(), patternString.length());    
                    }    
                }    
            };    
    
            if (auto packageRelativeValue = redirectedPathsObject.try_get("packageRelative"))    
            {    
                initializeRedirection(g_packageRootPath, packageRelativeValue->as_array());    
            }    
    
            if (auto packageDriveRelativeValue = redirectedPathsObject.try_get("packageDriveRelative"))    
            {    
                initializeRedirection(g_packageRootPath.root_name(), packageDriveRelativeValue->as_array());    
            }    
    
            if (auto knownFoldersValue = redirectedPathsObject.try_get("knownFolders"))    
            {    
                for (auto& knownFolderValue : knownFoldersValue->as_array())    
                {    
                    auto& knownFolderObject = knownFolderValue.as_object();    
                    auto path = path_from_known_folder_string(knownFolderObject.get("id").as_string().wstring());    
                    if (!path.empty())    
                    {    
                        initializeRedirection(path, knownFolderObject.get("relativePaths").as_array());    
                    }    
                }    
            }    
        }    
    }    
}    
    
bool path_relative_to(const wchar_t* path, const std::filesystem::path& basePath)    
{    
    return std::equal(basePath.native().begin(), basePath.native().end(), path, psf::path_compare{});    
}    
    
template <typename CharT>    
normalized_path NormalizePathImpl(const CharT* path)    
{    
    normalized_path result;    
    
    auto pathType = psf::path_type(path);    
    if (pathType == psf::dos_path_type::root_local_device)    
    {    
        // Root-local device paths are a direct escape into the object manager, so don't normalize them    
        result.full_path = widen(path);    
    }    
    else if (pathType != psf::dos_path_type::unknown)    
    {    
        result.full_path = widen(psf::full_path(path));    
        pathType = psf::path_type(result.full_path.c_str());    
    }    
    else // unknown    
    {    
        return result;    
    }    
    
    if (pathType == psf::dos_path_type::drive_absolute)    
    {    
        result.drive_absolute_path = result.full_path.data();    
    }    
    else if ((pathType == psf::dos_path_type::local_device) || (pathType == psf::dos_path_type::root_local_device))    
    {    
        auto candidatePath = result.full_path.data() + 4;    
        if (psf::path_type(candidatePath) == psf::dos_path_type::drive_absolute)    
        {    
            result.drive_absolute_path = candidatePath;    
        }    
    }    
    else if (pathType == psf::dos_path_type::unc_absolute)    
    {    
        // We assume that UNC paths will never reference a path that we need to redirect. Note that this isn't perfect.    
        // E.g. "\\localhost\C$\foo\bar.txt" is the same path as "C:\foo\bar.txt"; we shall defer solving this problem    
        return result;    
    }    
    else    
    {    
        // GetFullPathName did something odd...    
        assert(false);    
        return {};    
    }    
    
    return result;    
}    
    
normalized_path NormalizePath(const char* path)    
{    
    return NormalizePathImpl(path);    
}    
    
normalized_path NormalizePath(const wchar_t* path)    
{    
    return NormalizePathImpl(path);    
}    
    
normalized_path DeVirtualizePath(normalized_path path)    
{    
    if (path.drive_absolute_path && path_relative_to(path.drive_absolute_path, g_packageVfsRootPath))    
    {    
        auto packageRelativePath = path.drive_absolute_path + g_packageVfsRootPath.native().length();    
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
                    path.full_path = (mapping.path / vfsRelativePath).native();    
                    path.drive_absolute_path = path.full_path.data();    
                    break;    
                }    
            }    
        }    
        // Otherwise a directory/file named something like "VFSx" for some non-path separator/null terminator 'x'    
    }    
    
    return path;    
}    
    
std::wstring GenerateRedirectedPath(std::wstring_view relativePath, bool ensureDirectoryStructure, std::wstring result)    
{    
    if (ensureDirectoryStructure)    
    {    
        for (std::size_t pos = 0; pos < relativePath.length(); )    
        {    
            [[maybe_unused]] auto dirResult = impl::CreateDirectory(result.c_str(), nullptr);    
#if _DEBUG    
            auto err = ::GetLastError();    
            assert(dirResult || (err == ERROR_ALREADY_EXISTS));    
#endif    
            auto nextPos = relativePath.find_first_of(LR"(\/)", pos + 1);    
            if (nextPos == relativePath.length())    
            {    
                // Ignore trailing path separators. E.g. if the call is to CreateDirectory, we don't want it to "fail"    
                // with an "already exists" error    
                nextPos = std::wstring_view::npos;    
            }    
    
            result += relativePath.substr(pos, nextPos - pos);    
            pos = nextPos;    
        }    
    }    
    else    
    {    
        result += relativePath;    
    }    
    
    return result;    
}    
    
/// <summary>    
/// Figures out the absolute path to redirect to.    
/// </summary>    
/// <param name="deVirtualizedPath">The original path from the app</param>    
/// <param name="ensureDirectoryStructure">If true, the deVirtualizedPath will be appended to the allowed write location</param>    
/// <returns>The new absolute path.</returns>    
std::wstring RedirectedPath(const normalized_path& deVirtualizedPath, bool ensureDirectoryStructure)    
{    
    /*    
        To prevent apps breaking on an upgrade we redirect writes to the package root path to    
        a path that contains the package family name and not the package full name.    
    */    
    std::wstring result;    
    bool shouldredirectToPackageRoot = false;    
    if (deVirtualizedPath.full_path.find(g_packageRootPath) != std::wstring::npos)    
    {    
        result = LR"(\\?\)" + g_writablePackageRootPath.native();    
        shouldredirectToPackageRoot = true;    
    }    
    else    
    {    
        //TEST    
        //MessageBoxEx(NULL, L"Redirecting to VFS", L"Redirecting to VFS", 0, 0);    
        result = LR"(\\?\)" + g_redirectRootPath.native();    
    }    
    
    std::wstring_view relativePath;    
    if (shouldredirectToPackageRoot)    
    {    
        auto lengthPackageRootPath = g_packageRootPath.native().length();    
        auto stringToTurnIntoAStringView = deVirtualizedPath.full_path.substr(lengthPackageRootPath);    
        relativePath = std::wstring_view(stringToTurnIntoAStringView);    
    
        return GenerateRedirectedPath(relativePath, ensureDirectoryStructure, result);    
    }    
    
    
    
    // NTFS doesn't allow colons in filenames, so simplest thing is to just substitute something in; use a dollar sign    
    // similar to what's done for UNC paths    
    assert(psf::path_type(deVirtualizedPath.drive_absolute_path) == psf::dos_path_type::drive_absolute);    
    result.push_back(L'\\');    
    result.push_back(deVirtualizedPath.drive_absolute_path[0]);    
    result.push_back('$');    
    
    auto remainingLength = deVirtualizedPath.full_path.length();    
    remainingLength -= (deVirtualizedPath.drive_absolute_path - deVirtualizedPath.full_path.c_str());    
    remainingLength -= 2;    
    relativePath = std::wstring_view(deVirtualizedPath.drive_absolute_path + 2, remainingLength);    
    
    return GenerateRedirectedPath(relativePath, ensureDirectoryStructure, result);    
    
    
    
}    
    
template <typename CharT>    
static path_redirect_info ShouldRedirectImpl(const CharT* path, redirect_flags flags)    
{    
    path_redirect_info result;    
    
    if (!path)    
    {    
        return result;    
    }    
    
    auto normalizedPath = NormalizePath(path);    
    
    if (!normalizedPath.drive_absolute_path)    
    {    
        // FUTURE: We could do better about canonicalising paths, but the cost/benefit doesn't make it worth it right now    
        return result;    
    }    
    
    // To be consistent in where we redirect files, we need to map VFS paths to their non-package-relative equivalent    
    normalizedPath = DeVirtualizePath(std::move(normalizedPath));    
    
    // Figure out if this is something we need to redirect    
    for (auto& redirectSpec : g_redirectionSpecs)    
    {    
        if (path_relative_to(normalizedPath.drive_absolute_path, redirectSpec.base_path))    
        {    
            auto relativePath = normalizedPath.drive_absolute_path + redirectSpec.base_path.native().length();    
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
    
            if (std::regex_match(relativePath, redirectSpec.pattern))    
            {    
                result.should_redirect = true;    
                break;    
            }    
        }    
    }    
    
    if (!result.should_redirect)    
    {    
        return result;    
    }    
    
    result.redirect_path = RedirectedPath(normalizedPath, flag_set(flags, redirect_flags::ensure_directory_structure));    
    
    if (flag_set(flags, redirect_flags::check_file_presence) && !impl::PathExists(result.redirect_path.c_str()))    
    {    
        result.should_redirect = false;    
        result.redirect_path.clear();    
        return result;    
    }    
    
    if (flag_set(flags, redirect_flags::copy_file))    
    {    
        [[maybe_unused]] BOOL copyResult;    
        auto attr = impl::GetFileAttributes(normalizedPath.drive_absolute_path);    
        if ((attr & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)    
        {    
            copyResult = impl::CopyFileEx(    
                normalizedPath.drive_absolute_path,    
                result.redirect_path.c_str(),    
                nullptr,    
                nullptr,    
                nullptr,    
                COPY_FILE_FAIL_IF_EXISTS | COPY_FILE_NO_BUFFERING);    
        }    
        else    
        {    
            copyResult = impl::CreateDirectoryEx(normalizedPath.drive_absolute_path, result.redirect_path.c_str(), nullptr);    
        }    
#if _DEBUG    
        auto err = ::GetLastError();    
        assert(copyResult || (err == ERROR_FILE_EXISTS) || (err == ERROR_PATH_NOT_FOUND) || (err == ERROR_FILE_NOT_FOUND) || (err == ERROR_ALREADY_EXISTS));    
#endif    
    }    
    
    return result;    
}    
    
path_redirect_info ShouldRedirect(const char* path, redirect_flags flags)    
{    
    return ShouldRedirectImpl(path, flags);    
}    
    
path_redirect_info ShouldRedirect(const wchar_t* path, redirect_flags flags)    
{    
    return ShouldRedirectImpl(path, flags);    
}    
