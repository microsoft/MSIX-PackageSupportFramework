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
std::filesystem::path g_redirectRootPathDefault;

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
    g_redirectRootPathDefault = psf::known_folder(FOLDERID_LocalAppData) / L"VFS";
    impl::CreateDirectory(g_redirectRootPathDefault.c_str(), nullptr);

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
    else if (str == L"LocalAppData"sv)
    {
        id = FOLDERID_LocalAppData;
    }
    else if (str == L"RoamingAppData"sv)
    {
        id = FOLDERID_RoamingAppData;
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
    std::filesystem::path redirect_targetbase;
    bool isExclusion;
    bool isReadOnly;
    bool isUnmanagedRetarget;
};

std::vector<path_redirection_spec> g_redirectionSpecs;



#ifdef LOGIT
void Log(const char* fmt, ...)
{
    std::string str;
    str.resize(256);

    va_list args;
    va_start(args, fmt);
    std::size_t count = std::vsnprintf(str.data(), str.size() + 1, fmt, args);
    assert(count >= 0);
    va_end(args);

    if (count > str.size())
    {
        str.resize(count);

        va_list args2;
        va_start(args2, fmt);
        count = std::vsnprintf(str.data(), str.size() + 1, fmt, args2);
        assert(count >= 0);
        va_end(args2);
    }

    str.resize(count);

    ::OutputDebugStringA(str.c_str());
}
#endif

template <typename CharT>
bool IsUnderUserAppDataLocalImpl(_In_ const CharT* fileName)
{
    return path_relative_to(fileName, psf::known_folder(FOLDERID_LocalAppData));
}
bool IsUnderUserAppDataLocal(_In_ const wchar_t* fileName)
{
    return IsUnderUserAppDataLocalImpl(fileName);
}
bool IsUnderUserAppDataLocal(_In_ const char* fileName)
{
    return IsUnderUserAppDataLocalImpl(fileName);
}

template <typename CharT>
bool IsUnderUserAppDataRoamingImpl(_In_ const CharT* fileName)
{
    return path_relative_to(fileName, psf::known_folder(FOLDERID_RoamingAppData));
}
bool IsUnderUserAppDataRoaming(_In_ const wchar_t* fileName)
{
    return IsUnderUserAppDataRoamingImpl(fileName);
}
bool IsUnderUserAppDataRoaming(_In_ const char* fileName)
{
    return IsUnderUserAppDataRoamingImpl(fileName);
}

template <typename CharT>
std::filesystem::path GetPackageVFSPathImpl(const CharT* fileName)
{
    if (IsUnderUserAppDataLocal(fileName))
    {
        auto lad = psf::known_folder(FOLDERID_LocalAppData);
        std::filesystem::path foo = fileName; 
        auto testLad = g_packageVfsRootPath / L"Local AppData" / foo.wstring().substr(wcslen(lad.c_str())+1).c_str();
        return testLad;
    }
    else if (IsUnderUserAppDataRoaming(fileName))
    {
        auto rad = psf::known_folder(FOLDERID_RoamingAppData);
        std::filesystem::path foo = fileName;
        auto testRad = g_packageVfsRootPath / L"AppData" / foo.wstring().substr(wcslen(rad.c_str())+1).c_str();
        return testRad;
    }
    return L"";
}
std::filesystem::path GetPackageVFSPath(const wchar_t* fileName)
{
    return GetPackageVFSPathImpl(fileName);
}
std::filesystem::path GetPackageVFSPath(const char* fileName)
{
    return GetPackageVFSPathImpl(fileName);
}

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
                    std::filesystem::path redirectTargetBaseValue = g_redirectRootPathDefault;
                    if (auto redirectTargetBase = specObject.try_get("redirectTargetBase"))
                    {
                        redirectTargetBaseValue = specObject.get("redirectTargetBase").as_string().wstring();	
                    }
                    bool IsExclusionValue = false;
                    if (auto IsExclusion = specObject.try_get("isExclusion"))
                    {
                        IsExclusionValue = specObject.get("isExclusion").as_boolean().get();
                    }
                    bool IsReadOnlyValue = false;
                    if (auto IsReadOnly = specObject.try_get("isReadOnly"))
                    {
                        IsReadOnlyValue = specObject.get("isReadOnly").as_boolean().get();
                    }

                    bool IsUnmanagedRetargetValue = false;
                    if (auto IsUnmanagedRetarget = specObject.try_get("isUnmanagedRetarget"))
                    {
                        IsUnmanagedRetargetValue = specObject.get("isUnmanagedRetarget").as_boolean().get();
                    }
                    for (auto& pattern : specObject.get("patterns").as_array())
                    {
                        auto patternString = pattern.as_string().wstring();

                        g_redirectionSpecs.emplace_back();
                        g_redirectionSpecs.back().base_path = path;
                        g_redirectionSpecs.back().pattern.assign(patternString.data(), patternString.length());
                        g_redirectionSpecs.back().redirect_targetbase = redirectTargetBaseValue;
                        g_redirectionSpecs.back().isExclusion = IsExclusionValue;
                        g_redirectionSpecs.back().isReadOnly = IsReadOnlyValue;
                        g_redirectionSpecs.back().isUnmanagedRetarget = IsUnmanagedRetargetValue;
                    }
#ifdef LOGIT
                    Log("\t\tFRF RULE: Path=%ls retarget=%ls", path.c_str(), redirectTargetBaseValue.c_str());
#endif
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

template <typename CharT>
bool path_relative_toImpl(const CharT* path, const std::filesystem::path& basePath)
{
    return std::equal(basePath.native().begin(), basePath.native().end(), path, psf::path_compare{});
}
bool path_relative_to(const wchar_t* path, const std::filesystem::path& basePath)
{
    return path_relative_toImpl(path, basePath);
}
bool path_relative_to(const char* path, const std::filesystem::path& basePath)
{
    return path_relative_toImpl(path, basePath);
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
std::wstring RedirectedPath(const normalized_path& deVirtualizedPath, bool ensureDirectoryStructure, std::filesystem::path destinationTargetBase, bool isUnmanagedRetarget )
{
    std::wstring result;
    if (isUnmanagedRetarget)
    {
        result = LR"(\\?\)" + psf::remove_trailing_path_separators(destinationTargetBase).wstring();
        size_t offset = result.length();
        result += L"\\PackageCache\\";
        result += ::PSFQueryPackageFullName();
        //result = psf::remove_trailing_path_separators(destinationTargetBase).wstring() + L"\\PackageCache\\" + ::PSFQueryPackageFullName();
        if (IsUnderUserAppDataLocal(deVirtualizedPath.full_path.c_str()))
        {
            result.push_back(L'\\');
            result.append(L"VFS\\");
            result.append(L"LocalAppData");
            result.append(deVirtualizedPath.full_path.substr(psf::known_folder(FOLDERID_LocalAppData).wstring().length()).c_str());
        }
        else if (IsUnderUserAppDataRoaming(deVirtualizedPath.full_path.c_str()))
        {
            result += L"\\VFS\\RoamingAppData" + deVirtualizedPath.full_path.substr(psf::known_folder(FOLDERID_RoamingAppData).wstring().length());
        }
        else
        {
            result += deVirtualizedPath.full_path.substr(g_packageRootPath.wstring().length());
        }

        //g_packageRootPath;
#ifdef LOGIT
        Log("\tFRF devirt.full_path %ls", deVirtualizedPath.full_path.c_str());
        Log("\tFRF devirt.da_path %ls", deVirtualizedPath.drive_absolute_path);
        Log("\tFRF unmanged %ls", result.c_str());
#endif
        // Create folder structure, if needed
        if (impl::PathExists(result.c_str()))
        {
#ifdef LOGIT
            Log("\t\tFRF Found that a copy exists in the redirected area so we skip the folder creation.");
#endif
        }
        else
        {
            if (ensureDirectoryStructure)
            {
                while (offset < result.length())
                {
                    size_t newoffset = result.find('\\', offset);
                    if (newoffset == -1)
                        break;
#ifdef LOGIT2
                    Log("\t\tFRF find at %d", newoffset);
#endif			
                    if (newoffset > 2)  // skip the C:\\ 
                    {
                        std::wstring partialdir = result.substr(0, newoffset);
                        [[maybe_unused]] auto dirResultU = impl::CreateDirectory(partialdir.c_str(), nullptr);
#ifdef LOGIT
                        std::wstring res = L"false";
                        if (dirResultU)
                            res = L"true";
                        Log("\t\tFRF Create Unmanaged folder %ls RES=%ls", partialdir.c_str(), res.c_str());
#endif
                    }
#ifdef LOGIT2
                    else
                    {
                        Log("\t\tFRF skip %d", newoffset);
                    }
#endif
                    offset = newoffset + 1;
                }
            }
        }
        return result;
    }
    else
    {
        
        //To prevent apps breaking on an upgrade we redirect writes to the package root path to
        //a path that contains the package family name and not the package full name.
        bool shouldredirectToPackageRoot = false;
        if (deVirtualizedPath.full_path.find(g_packageRootPath) != std::wstring::npos)
        {
            //Maybe result = LR"(\\?\)" + psf::remove_trailing_path_separators(destinationTargetBase).wstring();
            result = LR"(\\?\)" + g_writablePackageRootPath.native();
            shouldredirectToPackageRoot = true;
        }
        else
        {
            //Maybe result = LR"(\\?\)" + psf::remove_trailing_path_separators(destinationTargetBase).wstring();
            result = LR"(\\?\)" + g_redirectRootPath.native();
        }

        //std::wstring_view relativePath;
        //if (shouldredirectToPackageRoot)
        //{
        //    auto lengthPackageRootPath = g_packageRootPath.native().length();
        //    auto stringToTurnIntoAStringView = deVirtualizedPath.full_path.substr(lengthPackageRootPath);
        //    relativePath = std::wstring_view(stringToTurnIntoAStringView);
        //
        //    return GenerateRedirectedPath(relativePath, ensureDirectoryStructure, result);
        //}

        // NTFS doesn't allow colons in filenames, so simplest thing is to just substitute something in; use a dollar sign
        // similar to what's done for UNC paths
        assert(psf::path_type(deVirtualizedPath.drive_absolute_path) == psf::dos_path_type::drive_absolute);
        result.push_back(L'\\');
        result.push_back(deVirtualizedPath.drive_absolute_path[0]);
        result.push_back('$');

        auto remainingLength = deVirtualizedPath.full_path.length();
        remainingLength -= (deVirtualizedPath.drive_absolute_path - deVirtualizedPath.full_path.c_str());
        remainingLength -= 2;
        std::wstring_view relativePath = std::wstring_view(deVirtualizedPath.drive_absolute_path + 2, remainingLength);
    
    #ifdef LOGIT
            Log("\tFRF relativepath = %d", relativePath.length());
    #endif

        if (ensureDirectoryStructure)
        {
            for (std::size_t pos = 0; pos < relativePath.length(); )
            {
                [[maybe_unused]] auto dirResult = impl::CreateDirectory(result.c_str(), nullptr);
#if _DEBUG
auto err = ::GetLastError();
                assert(dirResult || (err == ERROR_ALREADY_EXISTS));
#endif
#ifdef LOGIT
                std::wstring res = L"false";
                if (dirResult)
                    res = L"true";
                Log("\t\tFRF Create Managed folder %ls RES=%ls", result.c_str(), res.c_str());
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
    return result; ///GenerateRedirectedPath(relativePath, ensureDirectoryStructure, result);
    }
}

std::wstring RedirectedPath(const normalized_path& deVirtualizedPath, bool ensureDirectoryStructure)
{
    // Only until all code paths use the new version of the interface...
    return RedirectedPath(deVirtualizedPath, ensureDirectoryStructure, g_redirectRootPathDefault.native(),  false);
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
    std::filesystem::path destinationTargetBase;

    if (!normalizedPath.drive_absolute_path)
    {
        // FUTURE: We could do better about canonicalising paths, but the cost/benefit doesn't make it worth it right now
        return result;
    }

    // To be consistent in where we redirect files, we need to map VFS paths to their non-package-relative equivalent
    normalizedPath = DeVirtualizePath(std::move(normalizedPath));

#ifdef LOGIT
    Log("\tFRF Should: for %ls", path);
    if (normalizedPath.drive_absolute_path != NULL)
    {
        Log("\t\tFRF Normalized&DeVirtualized=%ls", normalizedPath.drive_absolute_path);
    }
#endif

    // Figure out if this is something we need to redirect
    for (auto& redirectSpec : g_redirectionSpecs)
    {
        if (path_relative_to(normalizedPath.drive_absolute_path, redirectSpec.base_path))
        {
#ifdef LOGIT
            Log("\t\tFRF In ball park of base %ls", redirectSpec.base_path.c_str());
#endif   
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
#ifdef LOGIT
            Log("\t\tFRF relativePath=%ls",relativePath);
#endif
            if (std::regex_match(relativePath, redirectSpec.pattern))
            {
                if (redirectSpec.isExclusion)
                {
                    // The impact on isExclusion is that redirection is not needed.
                    result.should_redirect = false;
#ifdef LOGIT
                    Log("\t\tFRF CASE:Exclusion for %ls", path); 
#endif
                }
                else
                {
                    result.should_redirect = true;
                    result.shouldReadonly = (redirectSpec.isReadOnly == true);
                    if (redirectSpec.isUnmanagedRetarget == false)
                    {
#ifdef LOGIT
                        Log("\t\tFRF CASE: Managed for %ls", path);
#endif						
                        destinationTargetBase = redirectSpec.redirect_targetbase;
                        result.redirect_path = RedirectedPath(normalizedPath, flag_set(flags, redirect_flags::ensure_directory_structure), destinationTargetBase,false);
                    }
                    else
                    {
#ifdef LOGIT
                        Log("\t\tFRF CASE: Unmanaged for %ls", path);
#endif						
                        destinationTargetBase = redirectSpec.redirect_targetbase;
                        result.redirect_path = RedirectedPath(normalizedPath, flag_set(flags, redirect_flags::ensure_directory_structure), destinationTargetBase, true);
                    }
                }
                break;
            }
            else
            {
#ifdef LOGIT
                /////Log("\t\tFRF no match on parse %ls against $ls", relativePath.c_str(), redirectSpec.pattern.());
#endif
            }
        }
        else
        {
#ifdef LOGIT
            Log("\t\tFRF Not in ball park of base %ls", redirectSpec.base_path.c_str());
#endif
        }
    }

#ifdef LOGIT2
    Log("\t\tFRF post check 1");
#endif

    if (!result.should_redirect)
    {
#ifdef LOGIT
        Log("\tFRF no redirect rule for %ls", path);
#endif
        return result;
    }

#ifdef LOGIT2
    Log("\t\tFRF post check 2");
#endif
    //result.redirect_path = RedirectedPath(normalizedPath, flag_set(flags, redirect_flags::ensure_directory_structure));

    if (flag_set(flags, redirect_flags::check_file_presence) && !impl::PathExists(result.redirect_path.c_str()))
    {
        result.should_redirect = false;
        result.redirect_path.clear();
#ifdef LOGIT
        Log("\tFRF skipped (redirected not present check failed) for %ls", path);
#endif        
        return result;
    }

#ifdef LOGIT2
    Log("\t\tFRF post check 3");
#endif

    if (flag_set(flags, redirect_flags::copy_file))
    {
#ifdef LOGIT2
        Log("\t\tFRF post check 4");
#endif

        // Special case for the AppData's as VFS Layering doesn't apply in the Runtime
        std::filesystem::path CopySource = normalizedPath.drive_absolute_path;
        if (IsUnderUserAppDataLocal(normalizedPath.drive_absolute_path) ||
            IsUnderUserAppDataRoaming(normalizedPath.drive_absolute_path) )
        {
            std::filesystem::path PotentialCopySource = GetPackageVFSPath(normalizedPath.drive_absolute_path);
#ifdef LOGIT
            Log("\t\tFRF Looking for Potential Source=%ls", PotentialCopySource.c_str());
#endif			
            if (impl::PathExists(PotentialCopySource.c_str()))
            {
                CopySource = PotentialCopySource;
#ifdef LOGIT
                Log("\t\tFRF Found a copy in Package Appdata Folders");
#endif
            }
        }
        if (impl::PathExists(result.redirect_path.c_str()))
        {
#ifdef LOGIT
            Log("\t\tFRF Found that a copy exists in the redirected area so we skip the folder creation.");
#endif
        }
        else
        {
            [[maybe_unused]] BOOL copyResult;
            auto attr = impl::GetFileAttributes(CopySource.c_str()); //normalizedPath.drive_absolute_path);
#ifdef LOGIT
            Log("\t\tFRF source attributes=0x%x", attr);
#endif
            if ((attr & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
            {
                copyResult = impl::CopyFileEx(
                    CopySource.c_str(), //normalizedPath.drive_absolute_path,
                    result.redirect_path.c_str(),
                    nullptr,
                    nullptr,
                    nullptr,
                    COPY_FILE_FAIL_IF_EXISTS | COPY_FILE_NO_BUFFERING);
#ifdef LOGIT
                if (copyResult)
                {
                    Log("\t\tFRF CopyFile Success %ls %ls", CopySource.c_str(), result.redirect_path.c_str());
                }
                else
                {
                    Log("\t\tFRF CopyFile Fail=0x%x %ls %ls", ::GetLastError(), CopySource.c_str(), result.redirect_path.c_str());
                    auto err = ::GetLastError();
                    switch (err)
                    {
                    case ERROR_FILE_EXISTS:
                        Log("\t\tFRF  was ERROR_FILE_EXISTS");
                        break;
                    case ERROR_PATH_NOT_FOUND:
                        Log("\t\tFRF  was ERROR_PATH_NOT_FOUND");
                        break;
                    case ERROR_FILE_NOT_FOUND:
                        Log("\t\tFRF  was ERROR_FILE_NOT_FOUND");
                        break;
                    case ERROR_ALREADY_EXISTS:
                        Log("\t\tFRF  was ERROR_ALREADY_EXISTS");
                        break;
                    default:
                        break;
                    }
                }
#endif
            }
            else
            {
                copyResult = impl::CreateDirectoryEx(CopySource.c_str(), result.redirect_path.c_str(), nullptr);
#ifdef LOGIT
                if (copyResult)
                    Log("\t\tFRF CreateDir Success %ls %ls", CopySource.c_str(), result.redirect_path.c_str());
                else
                    Log("\t\tFRF CreateDir Fail=0x%x %ls %ls", ::GetLastError(), CopySource.c_str(), result.redirect_path.c_str());
#endif       
            }
        }
#ifdef LOGIT2
        Log("\t\tFRF post check 6");
#endif
#if _DEBUG
        auto err = ::GetLastError();
        assert(copyResult || (err == ERROR_FILE_EXISTS) || (err == ERROR_PATH_NOT_FOUND) || (err == ERROR_FILE_NOT_FOUND) || (err == ERROR_ALREADY_EXISTS));
#endif
    }

#ifdef LOGIT2
    Log("\t\tFRF post check 7");
#endif
#ifdef LOGIT
    //Log("\tFRF Redirect from %ls", path);
    Log("\tFRF Should: Redirect to %ls", result.redirect_path.c_str());
#endif
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
