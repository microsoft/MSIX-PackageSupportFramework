#define CONSOLIDATE
#define DONTREDIRECTIFPARENTNOTINPACKAGE

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
#include <TraceLoggingProvider.h>
#include "Telemetry.h"
#include "RemovePII.h"
#include <psf_logging.h>


#if _DEBUG
#define MOREDEBUG 1
#endif

TRACELOGGING_DECLARE_PROVIDER(g_Log_ETW_ComponentProvider);
TRACELOGGING_DEFINE_PROVIDER(
    g_Log_ETW_ComponentProvider,
    "Microsoft.Windows.PSFRuntime",
    (0xf7f4e8c4, 0x9981, 0x5221, 0xe6, 0xfb, 0xff, 0x9d, 0xd1, 0xcd, 0xa4, 0xe1),
    TraceLoggingOptionMicrosoftTelemetry());

using namespace std::literals;

std::filesystem::path g_packageRootPath;
std::filesystem::path g_packageVfsRootPath;
std::filesystem::path g_redirectRootPath;
std::filesystem::path g_writablePackageRootPath;
std::filesystem::path g_finalPackageRootPath;

DWORD g_FileIntceptInstance = 0;

std::vector<vfs_folder_mapping> g_vfsFolderMappings;

void InitializePaths()
{
    // For path comparison's sake - and the fact that std::filesystem::path doesn't handle (root-)local device paths all
    // that well - ensure that these paths are drive-absolute
    auto packageRootPath = std::wstring(::PSFQueryPackageRootPath());
    auto pathType = psf::path_type(packageRootPath.c_str());
    if (pathType == psf::dos_path_type::root_local_device || (pathType == psf::dos_path_type::local_device))
    {
        packageRootPath += 4;
    }
    assert(psf::path_type(packageRootPath.c_str()) == psf::dos_path_type::drive_absolute);
    transform(packageRootPath.begin(), packageRootPath.end(), packageRootPath.begin(), towlower);
    g_packageRootPath = psf::remove_trailing_path_separators(packageRootPath);

    g_packageVfsRootPath = g_packageRootPath / L"VFS";

	auto finalPackageRootPath = std::wstring(::PSFQueryFinalPackageRootPath());
	g_finalPackageRootPath = psf::remove_trailing_path_separators(finalPackageRootPath);  // has \\?\ prepended to PackageRootPath
    
    // Ensure that the redirected root path exists
    g_redirectRootPath = psf::known_folder(FOLDERID_LocalAppData) / std::filesystem::path(L"Packages") / psf::current_package_family_name() / LR"(LocalCache\Local\VFS)";
    std::filesystem::create_directories(g_redirectRootPath);

    g_writablePackageRootPath = psf::known_folder(FOLDERID_LocalAppData) /std::filesystem::path(L"Packages") / psf::current_package_family_name() / LR"(LocalCache\Local\Microsoft\WritablePackageRoot)";
    std::filesystem::create_directories(g_writablePackageRootPath);

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
    ///// NOTE:
    ///// It is critical that the ordering in this list causes a more specific path to match before the more general one.
    ///// So FOLDERID_SystemX86 is before FOLDERID_Windows and common folders before their parent, etc
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_System) / LR"(catroot2)"sv,    LR"(AppVSystem32Catroot2)"sv });
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_System) / LR"(catroot)"sv,     LR"(AppVSystem32Catroot)"sv });
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_System) / LR"(drivers\etc)"sv, LR"(AppVSystem32DriversEtc)"sv });
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_System) / LR"(driverstore)"sv, LR"(AppVSystem32Driverstore)"sv });
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_System) / LR"(logfiles)"sv,    LR"(AppVSystem32Logfiles)"sv });
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_System) / LR"(spool)"sv,       LR"(AppVSystem32Spool)"sv });

    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_SystemX86),                   LR"(SystemX86)"sv });
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_ProgramFilesCommonX86),       LR"(ProgramFilesCommonX86)"sv });
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_ProgramFilesX86),             LR"(ProgramFilesX86)"sv });
#if !_M_IX86
    // FUTURE: We may want to consider the possibility of a 32-bit application trying to reference "%windir%\sysnative\"
    //         in which case we'll have to get smarter about how we resolve paths
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_System), LR"(SystemX64)"sv });
    // FOLDERID_ProgramFilesX64* not supported for 32-bit applications
    // FUTURE: We may want to consider the possibility of a 32-bit process trying to access this path anyway. E.g. a
    //         32-bit child process of a 64-bit process that set the current directory
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_ProgramFilesCommonX64), LR"(ProgramFilesCommonX64)"sv });
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_ProgramFilesX64), LR"(ProgramFilesX64)"sv });
#endif
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_System),                       LR"(System)"sv });
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_Fonts),                        LR"(Fonts)"sv });
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_Windows),                      LR"(Windows)"sv });

    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_ProgramData),                  LR"(Common AppData)"sv });

    // These are additional folders that may appear in MSIX packages and need help
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_LocalAppData),                 LR"(Local AppData)"sv });
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_RoamingAppData),               LR"(AppData)"sv });

    //These are additional folders seen from App-V packages converted into MSIX (still looking for an official list)
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_PublicDesktop),                LR"(Common Desktop)"sv });
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_CommonPrograms),               LR"(Common Programs)"sv });
    g_vfsFolderMappings.push_back(vfs_folder_mapping{ psf::known_folder(FOLDERID_LocalAppDataLow),              LR"(LOCALAPPDATALOW)"sv });
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

std::vector<path_redirection_spec> g_redirectionSpecs;



template <typename CharT>
std::filesystem::path GetPackageVFSPathImpl(const CharT* fileName)
{
    if (fileName != NULL)
    {
        constexpr wchar_t root_local_device_prefix[] = LR"(\\?\)";
        constexpr wchar_t root_local_device_prefix_dot[] = LR"(\\.\)";

        try
        {
            if (IsUnderUserAppDataLocal(fileName))
            {
                auto lad = psf::known_folder(FOLDERID_LocalAppData);
                std::filesystem::path foo;
                if (std::equal(root_local_device_prefix, root_local_device_prefix + 4, fileName))
                {
                    foo = fileName + 4;
                }
                else if (std::equal(root_local_device_prefix_dot, root_local_device_prefix_dot + 4, fileName))
                {
                    foo = fileName + 4;
                }
                else
                {
                    foo = fileName;
                }
                auto testLad = g_packageVfsRootPath / L"Local AppData" / foo.wstring().substr(wcslen(lad.c_str()) + 1).c_str();
                return testLad;
            }
            else if (IsUnderUserAppDataRoaming(fileName))
            {
                auto rad = psf::known_folder(FOLDERID_RoamingAppData);
                std::filesystem::path foo;
                if (std::equal(root_local_device_prefix, root_local_device_prefix + 4, fileName))
                {
                    foo = fileName + 4;
                }
                else
                {
                    foo = fileName;
                }
                auto testRad = g_packageVfsRootPath / L"AppData" / foo.wstring().substr(wcslen(rad.c_str()) + 1).c_str();
                return testRad;
            }
        }
        catch (...)
        {
            return L"";
        }
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
    TraceLoggingRegister(g_Log_ETW_ComponentProvider);
    std::wstringstream traceDataStream;

    if (auto rootConfig = ::PSFQueryCurrentDllConfig())
    {
        auto& rootObject = rootConfig->as_object();
        traceDataStream << " config:\n";
        if (auto pathsValue = rootObject.try_get("redirectedPaths"))
        {
            traceDataStream << " redirectedPaths:\n";
            auto& redirectedPathsObject = pathsValue->as_object();
            auto initializeRedirection = [&traceDataStream](const std::filesystem::path & basePath, const psf::json_array & specs, bool traceOnly = false)
            {
                for (auto& spec : specs)
                {
                    auto& specObject = spec.as_object();
                    auto path = psf::remove_trailing_path_separators(basePath / specObject.get("base").as_string().wstring());
                    std::filesystem::path redirectTargetBaseValue = g_writablePackageRootPath;
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
                  
                    traceDataStream << " base:" << RemovePIIfromFilePath(specObject.get("base").as_string().wide()) << " ;";
                    traceDataStream << " patterns:";
                    for (auto& pattern : specObject.get("patterns").as_array())
                    {
                        auto patternString = pattern.as_string().wstring();
                        traceDataStream << pattern.as_string().wide() << " ;";                      
                        if (!traceOnly)
                        {
                          g_redirectionSpecs.emplace_back();
                          g_redirectionSpecs.back().base_path = path;
                          g_redirectionSpecs.back().pattern.assign(patternString.data(), patternString.length());
                          g_redirectionSpecs.back().redirect_targetbase = redirectTargetBaseValue;
                          g_redirectionSpecs.back().isExclusion = IsExclusionValue;
                          g_redirectionSpecs.back().isReadOnly = IsReadOnlyValue;
                        }
                    }
#if _DEBUG
                    if (IsExclusionValue)
                        Log("\t\tFRF EXCLUSION: Path=%ls", path.c_str());
                    else
                        Log("\t\tFRF RULE: Path=%ls retarget=%ls", path.c_str(), redirectTargetBaseValue.c_str());
#endif
                }
            };

            if (auto packageRelativeValue = redirectedPathsObject.try_get("packageRelative"))
            {
                traceDataStream << " packageRelative:\n";
                initializeRedirection(g_packageRootPath, packageRelativeValue->as_array());
            }

            if (auto packageDriveRelativeValue = redirectedPathsObject.try_get("packageDriveRelative"))
            {
                traceDataStream << " packageDriveRelative:\n";
                initializeRedirection(g_packageRootPath.root_name(), packageDriveRelativeValue->as_array());
            }

            if (auto knownFoldersValue = redirectedPathsObject.try_get("knownFolders"))
            {
                traceDataStream << " knownFolders:\n";
                for (auto& knownFolderValue : knownFoldersValue->as_array())
                {
                    auto& knownFolderObject = knownFolderValue.as_object();
                    auto path = path_from_known_folder_string(knownFolderObject.get("id").as_string().wstring());
                    traceDataStream << " id:" << knownFolderObject.get("id").as_string().wide() << " ;";

                    traceDataStream << " relativePaths:\n";
                    initializeRedirection(path, knownFolderObject.get("relativePaths").as_array(), path.empty());
                }
            }
        }

        TraceLoggingWrite(
            g_Log_ETW_ComponentProvider,
            "FileRedirectionFixupConfigdata",
            TraceLoggingWideString(traceDataStream.str().c_str(), "FileRedirectionFixupConfig"),
            TraceLoggingBoolean(TRUE, "UTCReplace_AppSessionGuid"),
            TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
    }

    TraceLoggingUnregister(g_Log_ETW_ComponentProvider);
}


#pragma region Normalize
template <typename CharT>
normalized_path NormalizePathImpl(const CharT* path, DWORD inst)
{
    normalized_path result;

    result.path_type = psf::path_type(path);
    if (result.path_type == psf::dos_path_type::root_local_device)
    {
        // Root-local device paths are a direct escape into the object manager, so don't normalize them
#ifdef MOREDEBUG
        Log(L"[%d]\t\tNormalizePathImpl: root_local_device",inst);
#endif
        result.full_path = widen(path);
    }
    else if (result.path_type == psf::dos_path_type::local_device)
    {
        // these are a direct escape, but for devices.
        ///Log(L"[%d]\t\tNormalizePathImpl: local_device",inst);
        result.full_path = widen(path); // widen(path + 4);
    }
    else if (result.path_type == psf::dos_path_type::drive_absolute)
    {
        ///Log(L"[%d]\t\tNormalizePathImpl: drive_absolute",inst);
        result.full_path = widen(path);
    }
    else if (result.path_type != psf::dos_path_type::unknown)
    {
        ///Log(L"[%d]\t\tNormalizePathImpl: other",inst);
        result.full_path = widen(psf::full_path(path));
        result.path_type = psf::path_type(result.full_path.c_str());
    }
    else // unknown
    {
        ///Log(L"[%d]\t\tNormalizePathImpl: unknown",inst);
        result.full_path = widen(path);
        //return result;
    }

    if (result.path_type == psf::dos_path_type::drive_absolute)
    {
        result.drive_absolute_path = result.full_path.data();
        ///LogString(inst,L"\t\tNormalizePathImpl driveabs", result.drive_absolute_path);
    }
    else if ((result.path_type == psf::dos_path_type::local_device) || (result.path_type == psf::dos_path_type::root_local_device))
    {
#ifdef MOREDEBUG
        Log(L"[%d]***\t\t\tNormalizePathImpl: Path is local_device or root_local_device so adjust for drive_absolute_path",inst);
#endif
        //auto trunc = psf::full_path(result.full_path.c_str() + 4);
        //result.drive_absolute_path = trunc.data();
        result.drive_absolute_path = result.full_path.data() + 4;
#ifdef MOREDEBUG
        Log(L"[%d]***\t\t\tNormalizePathImpl: dap set",inst);
#endif
    }
    else if (result.path_type == psf::dos_path_type::unc_absolute)
    {
        // We assume that UNC paths will never reference a path that we need to redirect. Note that this isn't perfect.
        // E.g. "\\localhost\C$\foo\bar.txt" is the same path as "C:\foo\bar.txt"; we shall defer solving this problem
#ifdef MOREDEBUG
        Log(L"[%d]***\t\t\tNormalizePathImpl: Path is UNC so no absolute path",inst);
#endif
        return result;
    }
    else
    {
        // GetFullPathName did something odd...
        LogString(inst,L"\t\tFRF Error: Path type not supported", path);
        Log(L"[%d]\t\tFRF Error: Path type: 0x%x", inst, result.path_type);

        assert(false);
        return {};
    }

    return result;
}

normalized_path NormalizePath(const char* path, DWORD inst)
{
    if (path != NULL && path[0] != 0)
    {
        std::string new_string = UrlDecode(path);       // replaces things like %3a with :
        if (IsColonColonGuid(path))
        {
            ///Log(L"NormalizePath A: Guid avoidance");
            normalized_path npath;
            npath.full_path = widen(new_string);
            return npath;
        }
        if (IsBlobColon(new_string))  // blog:hexstring has been seen, believed to be associated with writing encrypted data,  Just pass it through as it is not a real file.
        {
            ///Log(L"NormalizePath A: Blob avoidance");
            normalized_path npath;
            npath.full_path = widen(new_string);
            return npath;
        }      
        new_string = StripFileColonSlash(new_string);        // removes "file:\\" from start of path if present
        new_string = ReplaceSlashBackwardOnly(new_string);      // Ensure all slashes are backslashes
        
#ifdef MOREDEBUG
        LogString(inst,L"\t\tNormalizePath A: call impl with", new_string.c_str());
#endif
        //return NormalizePathImpl(new_string.c_str(),inst);
        normalized_path fred = NormalizePathImpl(new_string.c_str(),inst);
#ifdef MOREDEBUG
        Log(L"[%d] \t\tNormalizePath A returned",inst);
        if (fred.drive_absolute_path)
        {
            LogString(inst, L"\t\tNormalizePath A driveabs", fred.drive_absolute_path);
        }
        else
        {
            LogString(inst, L"\t\tNormalizePath A fullpath", fred.full_path.c_str());
        }
#endif
        return fred;
    }
    else
    {
#ifdef MOREDEBUG
        Log(L"[%d]\t\tNormalizePath A: null avoidance",inst);
#endif
        //return NormalizePathImpl(L".",inst);
        return NormalizePathImpl(std::filesystem::current_path().c_str(),inst);
    }
}

normalized_path NormalizePath(const wchar_t* path, DWORD inst)
{
    if (path != NULL && path[0] != 0)
    {
        std::wstring new_wstring = UrlDecode(path);   // replaces things like %3a with :
        if (IsColonColonGuid(path))
        {
            ///Log(L"NormalizePath W: Guid avoidance");
            normalized_path npath;
            npath.full_path = widen(new_wstring);
            return npath;
        }
        if (IsBlobColon(new_wstring))  // blog:hexstring has been seen, believed to be associated with writing encrypted data,  Just pass it through as it is not a real file.
        {
            ///Log(L"NormalizePath W: Blob avoidance");
            normalized_path npath;
            npath.full_path = widen(new_wstring);
            return npath;
        }        

        new_wstring = StripFileColonSlash(new_wstring);     // removes "file:\\" from start of path if present
        new_wstring = ReplaceSlashBackwardOnly(new_wstring);   // Ensure all slashes are backslashes

#ifdef MOREDEBUG
        LogString(inst,L"\t\tNormalizePath W: call impl with", new_wstring.c_str());
#endif
        normalized_path fred = NormalizePathImpl(new_wstring.c_str(),inst);
#ifdef MOREDEBUG
        Log(L"[%d] \t\tNormalizePath W returned",inst);
        if (fred.drive_absolute_path)
        {
            LogString(inst, L"\t\tNormalizePath W driveabs", fred.drive_absolute_path);
        }
        else
        {
            LogString(inst, L"\t\tNormalizePath W fullpath", fred.full_path.c_str());
        }
#endif
        return fred;
    }
    else
    {
#if _DEBUG
        Log(L"[%d]\t\tNormalizePath W: null avoidance",inst);
#endif
        //return NormalizePathImpl(L".",inst);
        return NormalizePathImpl(std::filesystem::current_path().c_str(),inst);
    }
}

#pragma endregion



#pragma region DeVirtualizePath
// If the input path is relative to the VFS folder under the package path (e.g. "${PackageRoot}\VFS\SystemX64\foo.txt"),
// then modifies that path to its virtualized equivalent (e.g. "C:\Windows\System32\foo.txt")
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
#pragma endregion

#pragma region VirtualizePath
// If the input path is a physical path outside of the package (e.g. "C:\Windows\System32\foo.txt"),
// this returns what the package VFS equivalent would be (e.g "C:\Program Files\WindowsApps\Packagename\VFS\SystemX64\foo.txt");
// NOTE: Does not check if package has this virtualized path.
normalized_path VirtualizePath(normalized_path path, [[maybe_unused]] DWORD impl)
{
    ///Log(L"[%d]\t\tVirtualizePath: Input drive_absolute_path %ls", impl, path.drive_absolute_path);
    ///Log(L"[%d]\t\tVirtualizePath: Input full_path %ls", impl, path.full_path.c_str());

    if (path.drive_absolute_path != NULL && path_relative_to(path.drive_absolute_path, g_packageRootPath))
    {
        ///Log(L"[%d]\t\tVirtualizePath: output same as input, is in package",impl);
        return path;
    }
    
    if (path.drive_absolute_path != NULL)
    {
        //I think this iteration must be forward order, otherwise C:\Windows\SysWOW64 matches to VFS\Windows\SysWOW64 instead of VFS\System32
        //for (std::vector<vfs_folder_mapping>::reverse_iterator iter = g_vfsFolderMappings.rbegin(); iter != g_vfsFolderMappings.rend(); ++iter)
        for (std::vector<vfs_folder_mapping>::iterator iter = g_vfsFolderMappings.begin(); iter != g_vfsFolderMappings.end(); ++iter)
        {
            auto& mapping = *iter;
            if (path_relative_to(path.drive_absolute_path, mapping.path))
            {
                ///LogString(impl, L"\t\t\t mapping entry match on path", mapping.path.wstring().c_str());
                ///LogString(impl, L"\t\t\t package_vfs_relative_path", mapping.package_vfs_relative_path.native().c_str());
                ///Log(L"[%d]\t\t\t rel length =%d, %d", impl, mapping.path.native().length(), mapping.package_vfs_relative_path.native().length());
                auto vfsRelativePath = path.drive_absolute_path + mapping.path.native().length();
                if (psf::is_path_separator(vfsRelativePath[0]))
                {
                    ++vfsRelativePath;
                }
                ///LogString(impl, L"\t\t\t VfsRelativePath", vfsRelativePath);
                path.full_path = (g_packageVfsRootPath / mapping.package_vfs_relative_path / vfsRelativePath).native();
                path.drive_absolute_path = path.full_path.data();
                return path;
            }
            else if (path_relative_to(path.full_path.c_str(), mapping.path))
            {
                ///LogString(impl, L"\t\t\t mapping entry match on path", mapping.path.wstring().c_str());
                ///LogString(impl, L"\t\t\t package_vfs_relative_path", mapping.package_vfs_relative_path.native().c_str());
                ///Log(L"[%d]\t\t\t rel length =%d, %d", impl, mapping.path.native().length(), mapping.package_vfs_relative_path.native().length());
                auto vfsRelativePath = path.full_path.c_str() + mapping.path.native().length();
                if (psf::is_path_separator(vfsRelativePath[0]))
                {
                    ++vfsRelativePath;
                }
                ///LogString(impl, L"\t\t\t vfsRelativePath", vfsRelativePath);
                path.full_path = (g_packageVfsRootPath / mapping.package_vfs_relative_path / vfsRelativePath).native();
                path.drive_absolute_path = path.full_path.data();
                return path;
            }
        }
    }
    else
    {
        // file is not on system drive
        ///Log(L"[%d]\t\tVirtualizePath: output same as input, not on system drive.", impl);
        return path;
    }
#if _DEBUG
    Log(L"[%d]\t\tVirtualizePath: output same as input, no match.",impl);
#endif
    return path;
}
#pragma endregion


std::wstring GenerateRedirectedPath(std::wstring_view relativePath, bool ensureDirectoryStructure, std::wstring result, [[maybe_unused]] DWORD inst)
{
    if (ensureDirectoryStructure)
    {
        for (std::size_t pos = 0; pos < relativePath.length(); )
        {
#if _DEBUG
            LogString(inst,L"\t\tGenerateRedirectedPath: Create dir", result.c_str());
#endif
            [[maybe_unused]] auto dirResult = impl::CreateDirectory(result.c_str(), nullptr);
#if _DEBUG
            auto err = ::GetLastError();
            //assert(dirResult || (err == ERROR_ALREADY_EXISTS));
            if (!(dirResult || (err == ERROR_ALREADY_EXISTS)))
            {
#if _DEBUG
                Log(L"[%d]\t\tGenerateRedirectedPath: Directory Fail=0x%x", inst, err);
#endif
            }
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


#pragma region RedirectedPath
/// <summary>
/// Figures out the absolute path to redirect to.
/// </summary>
/// <param name="deVirtualizedPath">The original path from the app</param>
/// <param name="ensureDirectoryStructure">If true, the deVirtualizedPath will be appended to the allowed write location</param>
/// <returns>The new absolute path.</returns>
std::wstring RedirectedPath(const normalized_path& deVirtualizedPath, bool ensureDirectoryStructure, std::filesystem::path destinationTargetBase, DWORD inst)
{
    std::wstring result;
    std::wstring basePath;
    std::wstring relativePath;

    bool shouldredirectToPackageRoot = false;
    auto deVirtualizedFullPath = deVirtualizedPath.full_path;
    if (deVirtualizedPath.drive_absolute_path)
    {
        deVirtualizedFullPath = deVirtualizedPath.drive_absolute_path;
    }

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

    if (deVirtualizedFullPath.find(g_packageRootPath) != std::wstring::npos)
    {
        ///Log(L"[%d]\t\t\tcase: target in package.",inst);
        ///LogString(inst,L"      destinationTargetBase:     ", destinationTargetBase.c_str());
        ///LogString(inst,L"      g_writablePackageRootPath: ", g_writablePackageRootPath.c_str());

        size_t lengthPackageRootPath = 0;
        ///auto pathType = psf::path_type(deVirtualizedFullPath.c_str());

        if (deVirtualizedPath.path_type != psf::dos_path_type::local_device &&
            deVirtualizedPath.path_type != psf::dos_path_type::root_local_device)
        {
            lengthPackageRootPath = g_packageRootPath.native().length();
            ///Log(L"[%d] dap length to remove=%d", inst, lengthPackageRootPath);
        }
        else
        {
            // dap aleady has this removed, don't need this: lengthPackageRootPath = g_finalPackageRootPath.native().length();
            lengthPackageRootPath = g_packageRootPath.native().length();
            ///Log(L"[%d] !dap length to remove=%d", inst, lengthPackageRootPath);
        }

        if (_wcsicmp(destinationTargetBase.c_str(), g_writablePackageRootPath.c_str()) == 0)
        {
            ///Log(L"[%d]\t\t\tsubcase: redirect to default.",inst);
            // PSF defaulted destination target.
            shouldredirectToPackageRoot = true;
            if (deVirtualizedPath.drive_absolute_path)
            {
                ///Log("DAP before");
                //auto stringToTurnIntoAStringView = ((std::wstring)deVirtualizedPath.drive_absolute_path).substr(lengthPackageRootPath);
                //relativePath = std::wstring_view(stringToTurnIntoAStringView);
                relativePath = deVirtualizedPath.drive_absolute_path + lengthPackageRootPath;
                ///Log("DAP after");
            }
            else
            {
                ///Log("NO DAP before");
                //auto stringToTurnIntoAStringView = deVirtualizedPath.full_path.substr(lengthPackageRootPath);
                //relativePath = std::wstring_view(stringToTurnIntoAStringView);
                relativePath = deVirtualizedPath.full_path.c_str() + lengthPackageRootPath;
                ///Log("NO DAP after");
            }
        }
        else
        {
            ///Log(L"[%d]\t\t\tsubcase: redirect specified.",inst);
            // PSF configured destination target: probably a home drive.
            relativePath = L"\\PackageCache\\" + psf::current_package_family_name() + deVirtualizedPath.full_path.substr(lengthPackageRootPath);
        }
    }
    else
    {
        ///Log(L"[%d]\t\t\tcase: target not in package.",inst);
        //if ( _wcsicmp(deVirtualizedPath.full_path.substr(0,2).c_str(), L"\\\\")==0)   this test was incorrect ue to local_device and root_local_device cases
        if (deVirtualizedPath.path_type == psf::dos_path_type::unc_absolute)
        {
            // Clearly we should never redirect files from a share
            ///Log("[%d]RedirectedPath: File share case should not be redirected ever.",inst);
            return deVirtualizedPath.full_path;
        }
        else
        {
            // input location was not in package path.
                 // TODO: Currently, this code redirects always.  We probably don't want to do that!
                 //       Ideally, we should look closer at the request; the text below is an example of what might be needed.
                 //       If the user asked for a native path and we aren't VFSing close to that path, and it's just a read, we probably shouldn't redirect.
                 //       But let's say it was a write, then probably still don't redirect and let the chips fall where they may.
                 //       But if we have a VFS folder in the package (such as VFS\AppDataCommon\Vendor) with files and the app tries to add a new file using native pathing, then we probably want to redirect.
                 //       There are probably more situations to consider.
                 // To avoid redirecting everything with the current implementation, the configuration spec should be as specific as possible so that we never get here.
            ///LogString(inst,L"      destinationTargetBase: ", destinationTargetBase.c_str());
            ///LogString(inst,L"      g_redirectRootPath:    ", g_redirectRootPath.c_str());
            if (_wcsicmp(destinationTargetBase.c_str(), g_redirectRootPath.c_str()) == 0)
            {
                ///Log(L"[%d]\t\t\tsubcase: redirect to default.",inst);
                // PSF defaulted destination target.
                relativePath = L"\\";
            }
            else
            {
                ///Log(L"[%d]\t\t\tsubcase: redirect specified.",inst);
                // PSF  configured destination target: probably a home drive.
                relativePath = L"\\PackageCache\\" + psf::current_package_family_name() + +L"\\VFS\\PackageDrive";
            }

            // NTFS doesn't allow colons in filenames, so simplest thing is to just substitute something in; use a dollar sign
            // similar to what's done for UNC paths
            ///// Note: assert here is normal it ignore for FindFileEx with file input "*" case which becomes a different type.
            assert(psf::path_type(deVirtualizedPath.drive_absolute_path) == psf::dos_path_type::drive_absolute);
            relativePath.push_back(L'\\');
            relativePath.push_back(deVirtualizedPath.drive_absolute_path[0]);
            relativePath.push_back('$');
            auto remainingLength = wcslen(deVirtualizedPath.drive_absolute_path);
            remainingLength -= 2;

            relativePath.append(deVirtualizedPath.drive_absolute_path + 2, remainingLength);
        }
    }

    ////Log(L"\tFRF devirt.full_path %ls", deVirtualizedPath.full_path.c_str());
    ////Log(L"\tFRF devirt.da_path %ls", deVirtualizedPath.drive_absolute_path);
    ///LogString(inst,L"\tFRF initial basePath", basePath.c_str());
    ///LogString(inst,L"\tFRF initial relative", relativePath.c_str());

    // Create folder structure, if needed
    if (impl::PathExists((basePath + relativePath).c_str()))
    {
        result = basePath + relativePath;
        ///Log(L"[%d]\t\tFRF Found that a copy exists in the redirected area so we skip the folder creation.",inst);
    }
    else
    {
        std::wstring_view relativePathview = std::wstring_view(relativePath);

        if (shouldredirectToPackageRoot)
        {
            result = GenerateRedirectedPath(relativePath, ensureDirectoryStructure, basePath, inst);
            ///Log(L"[%d]\t\tFRF shouldredirectToPackageRoot case returns result",inst);
            ///Log(result.c_str());
        }
        else
        {
            result = GenerateRedirectedPath(relativePath, ensureDirectoryStructure, basePath, inst);
            ///Log(L"[%d]\t\tFRF not to PackageRoot case returns result",inst);
            ///Log(result.c_str());
        }

    }


    return result;
}

//std::wstring RedirectedPath(const normalized_path& pathAsRequestedNormalized, bool ensureDirectoryStructure,DWORD inst)
//{
//    // Only until all code paths use the new version of the interface...
//    return RedirectedPath(pathAsRequestedNormalized, ensureDirectoryStructure, g_writablePackageRootPath.native(), inst);//
//}
#pragma endregion


/// <summary>
/// Given a file path that is in the WritablePackageRoot area, determine the equivalent Package Path.
/// Example "C:\Users\xxx\Appdata\Local\Packages\yyy\LocalCache\Microsoft\Local\WritablePackageRoot\VFS\zzz
/// returns "%Packageroot%\VFS\xxx" or empty string if not relevant.
std::wstring ReverseRedirectedToPackage(const std::wstring input)
{
    if (IsUnderUserPackageWritablePackageRoot(input.c_str()))
    {
        std::wstring ret = g_packageRootPath;
        //LogString(0, L"ReverseRedirection will be using g_packageRootPath", g_packageRootPath.c_str());

        constexpr wchar_t root_local_device_prefix[] = LR"(\\?\)";
        constexpr wchar_t root_local_device_prefix_dot[] = LR"(\\.\)";
        int lenwppr = lstrlenW(g_writablePackageRootPath.c_str());
        if (input.length() > (size_t(4+lenwppr)))
        {
            if (std::equal(root_local_device_prefix, root_local_device_prefix + 4, input.c_str()))
            {
                ret.append(input.substr(4 + lenwppr));
                return ret;
            }
            else if (std::equal(root_local_device_prefix_dot, root_local_device_prefix_dot + 4, input.c_str()))
            {
                ret.append(input.substr(4 + lenwppr));
                return ret;
            }
        }
        ret.append(input.substr(lenwppr));
        return ret;
    }
    return L"";
}

template <typename CharT>
static path_redirect_info ShouldRedirectImpl(const CharT* path, redirect_flags flags, DWORD inst)
{
    path_redirect_info result;

    if (!path)
    {
        return result;
    }
#if _DEBUG
    LogString(inst, L"\tFRFShouldRedirect: called for path", widen(path).c_str());
#endif
    try
    {

#if _DEBUG
    bool c_presense = flag_set(flags, redirect_flags::check_file_presence);
    bool c_copy = flag_set(flags, redirect_flags::copy_file);
    bool c_ensure = flag_set(flags, redirect_flags::ensure_directory_structure);
    Log(L"[%d]\t\tFRFShouldRedirect: flags  CheckPresense:%d  CopyFile:%d  EnsureDirectory:%d", inst, c_presense, c_copy, c_ensure);
#endif

    // normalizedPath represents the requested path, redirected to the external system if relevant, or just as requested if not.
    // vfsPath represents this as a package relative path

    auto normalizedPath = NormalizePath(path, inst);
    std::filesystem::path destinationTargetBase;

    if (normalizedPath.path_type == psf::dos_path_type::local_device)
    {
#if _DEBUG
        LogString(L"\tFRFShouldRedirect: Path is of type local device so FRF should ignore.", widen(path).c_str());
#endif
        return result;
    }

    if (!normalizedPath.drive_absolute_path)
    {
        // FUTURE: We could do better about canonicalising paths, but the cost/benefit doesn't make it worth it right now
#if _DEBUG
        Log(L"[%d] ***Normalized has no drive_absolute_path", inst);
#endif
        return result;
    }

#ifdef MOREDEBUG
    LogString(inst, L"\t\tFRFShouldRedirect: Normalized", normalizedPath.drive_absolute_path);
#endif
    // To be consistent in where we redirect files, we need to map VFS paths to their non-package-relative equivalent
    normalizedPath = DeVirtualizePath(std::move(normalizedPath));

#ifdef MOREDEBUG
    LogString(inst, L"\t\tFRFShouldRedirect: DeVirtualized", normalizedPath.drive_absolute_path);
#endif


	// If you change the below logic, or
	// you you change what goes into RedirectedPath
	// you need to mirror all changes in FindFirstFileFixup.cpp
	
	// Basically, what goes into RedirectedPath here also needs to go into 
	// FindFirstFile/NextFixup.cpp
    auto vfspath = NormalizePath(path, inst);
    vfspath = VirtualizePath(std::move(vfspath),inst);
    if (vfspath.drive_absolute_path != NULL)
    {
#ifdef MOREDEBUG
        LogString(inst, L"\t\tFRFShouldRedirect: Virtualized", vfspath.drive_absolute_path);
#endif
    }


    std::wstring reversedWritablePathWstring = ReverseRedirectedToPackage(widen(path).c_str());
    if (reversedWritablePathWstring.length() > 0)
    {
        // We were provided a path that is in the redirection area. We probably want to use this path,
        // but maybe the area it was redirected from.
#ifdef MOREDEBUG
        LogString(inst, L"\t\tFRFShouldRedirect: ReverseRedirection", reversedWritablePathWstring.c_str());
#endif

        // For now, let's return this path and let the caller deal with it.
#if _DEBUG
        LogString(inst, L"\tFRFShouldRedirect: Prevent redundant redirection.", widen(path).c_str());
#endif
        return result;
    }


    // Figure out if this is something we need to redirect
    for (auto& redirectSpec : g_redirectionSpecs)
    {
#ifdef MOREDEBUG
        //LogString(inst, L"\t\tFRFShouldRedirect: Check against: base", redirectSpec.base_path.c_str());
#endif
        if (path_relative_to(vfspath.drive_absolute_path, redirectSpec.base_path))
        {
#ifdef MOREDEBUG
            //LogString(inst, L"\t\tFRFShouldRedirect: In ball park of base", redirectSpec.base_path.c_str());
#endif
            auto relativePath = vfspath.drive_absolute_path + redirectSpec.base_path.native().length();
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
                    LogString(inst, L"\t\tFRFShouldRedirect CASE:Exclusion for path", widen(path).c_str());
#endif
                }
                else
                {
                    result.should_redirect = true;
                    result.shouldReadonly = (redirectSpec.isReadOnly == true);

                    // Check if file exists as VFS path in the package
                    std::wstring rldPath = TurnPathIntoRootLocalDevice(vfspath.drive_absolute_path);
                    if (impl::PathExists(rldPath.c_str()))
                    {
#ifdef MOREDEBUG
                        Log(L"[%d]\t\t\tFRFShouldRedirect CASE:match, existing in package.", inst);
#endif
                        destinationTargetBase = redirectSpec.redirect_targetbase;

#ifdef MOREDEBUG
                        //LogString(inst, L"\t\tFRFShouldRedirect isWide for", vfspath.drive_absolute_path);
                        //LogString(inst, L"\t\tFRFShouldRedirect isWide redir", destinationTargetBase.c_str());
#endif
                        result.redirect_path = RedirectedPath(vfspath, flag_set(flags, redirect_flags::ensure_directory_structure), destinationTargetBase, inst);

                    }
                    else
                    {
#ifdef MOREDEBUG
                        Log(L"[%d]\t\t\tFRFShouldRedirect CASE:match, not existing in package.", inst);
#endif
                        // If the folder above it exists, we might want to redirect anyway?
                        //  EX: Folder has VFS\AppData\Vendor
                        //  Request:       ...\AppData\Roaming                      Redirect yes (found above)
                        //  Request:       ...\AppData\Roaming\Vendor               Redirect yes (found above)
                        //  Request:       ...\AppData\Roaming\Vendor\foo           Redirect yes
                        //  Request:       ...\AppData\Roaming\Vendor\foo\bar       Redirect currently yes, was no
                        //  Request:       ...\AppData\Roaming\Vendor\foo\bar\now   Redirect currently yes, was no
                        std::filesystem::path abs = vfspath.drive_absolute_path;
                        std::filesystem::path abs2vfsvarfolder = trim_absvfs2varfolder(abs);
#ifdef MOREDEBUG
                        LogString(inst, L"\t\t\tFRFShouldRedirect check if VFS var-folder is in package?", abs2vfsvarfolder.c_str());
#endif
                        std::wstring rldPPath = TurnPathIntoRootLocalDevice(abs2vfsvarfolder.c_str());
                        rldPPath = rldPPath.substr(0, rldPPath.find_last_of(L"\\"));
                        if (impl::PathExists(rldPPath.c_str()))
                        {
#ifdef MOREDEBUG
                            Log(L"[%d]\t\t\tFRFShouldRedirect SUBCASE: parent-folder is in package.", inst);
#endif
                            destinationTargetBase = redirectSpec.redirect_targetbase;
                            result.redirect_path = RedirectedPath(vfspath, flag_set(flags, redirect_flags::ensure_directory_structure), destinationTargetBase, inst);
                        }
                        else
                        {

#ifdef DONTREDIRECTIFPARENTNOTINPACKAGE
                            psf::dos_path_type origType = psf::path_type(path);
#ifdef MOREDEBUG
                            Log(L"[%d]\t\t\tFRFShouldRedirect Orig Type=0%x", inst, (int)origType);
#endif
                            if (origType == psf::dos_path_type::relative)
                            {
#ifdef MOREDEBUG
                                Log(L"[%d]\t\t\tFRFShouldRedirect SUBCASE: VFS var-folder is also not in package, but req was relative path, we should redirect.", inst);
#endif
                                destinationTargetBase = redirectSpec.redirect_targetbase;
                                result.redirect_path = RedirectedPath(vfspath, flag_set(flags, redirect_flags::ensure_directory_structure), destinationTargetBase, inst);
                            }
                            else
                            {
#ifdef MOREDEBUG
                                Log(L"[%d]\t\t\tFRFShouldRedirect SUBCASE: VFS var-folder is also not in package, therefore we should NOT redirect.", inst);
#endif
                                result.should_redirect = false;
                            }
#else
#ifdef MOREDEBUG
                            Log(L"[%d]\t\t\tFRFShouldRedirect SUBCASE: VFS var-folder is also not in package, but since rule exists we should redirect.", inst);
#endif
                            destinationTargetBase = redirectSpec.redirect_targetbase;
                            result.redirect_path = RedirectedPath(vfspath, flag_set(flags, redirect_flags::ensure_directory_structure), destinationTargetBase, inst);
#endif
                            }
                        }
                    if (result.should_redirect)
                    {
#ifdef MOREDEBUG
                        LogString(inst, L"\t\tFRFShouldRedirect CASE:match on redirect_path", result.redirect_path.c_str());
#endif
                    }
                    }
                break;
                }
            else
            {
#ifdef MOREDEBUG
                //LogString(inst, L"\t\tFRFShouldRedirect: no match on parse relativePath", relativePath);
#endif
            }
            }
        else
        {
#ifdef MOREDEBUG
            //LogString(inst, L"\t\tFRFShouldRedirect: Not in ball park of base", redirectSpec.base_path.c_str());
#endif
        }
    }

#ifdef MOREDEBUG
    Log(L"[%d]\t\tFRFShouldRedirect post check 1", inst);
#endif


    if (!result.should_redirect)
    {
#if _DEBUG
        LogString(inst, L"\tFRFShouldRedirect: no redirect rule for path", widen(path).c_str());
#endif
        return result;
    }

#ifdef MOREDEBUG
    Log(L"[%d]\t\tFRFShouldRedirect post check 2", inst);
    LogString(inst, L"\t\tFRFShouldRedirect redir path currently", result.redirect_path.c_str());
#endif

    if (flag_set(flags, redirect_flags::check_file_presence))
    {
        std::wstring rldRedir = TurnPathIntoRootLocalDevice(widen(result.redirect_path).c_str());
        std::wstring rldVedir = TurnPathIntoRootLocalDevice(widen(vfspath.drive_absolute_path).c_str());
        std::wstring rldNdir = TurnPathIntoRootLocalDevice(widen(normalizedPath.drive_absolute_path).c_str());
        if (!impl::PathExists(rldRedir.c_str()) &&
            !impl::PathExists(rldVedir.c_str()) &&
            !impl::PathExists(rldNdir.c_str()))
        {
            result.should_redirect = false;
            result.redirect_path.clear();

#if _DEBUG
            LogString(inst, L"\tFRFShouldRedirect: skipped (redirected not present check failed) for path", widen(path).c_str());
#endif
            return result;
        }
    }

#ifdef MOREDEBUG
    Log(L"[%d]\t\tFRFShouldRedirect post check 3", inst);
#endif

    if (flag_set(flags, redirect_flags::copy_file))
    {
#ifdef MOREDEBUG
        Log(L"[%d]\t\tFRFShouldRedirect: copy_file flag is set", inst);
#endif
        [[maybe_unused]] BOOL copyResult = false;
        if (impl::PathExists(TurnPathIntoRootLocalDevice(widen(result.redirect_path).c_str()).c_str()))
        {
#if _DEBUG
            Log(L"[%d]\t\tFRFShouldRedirect: Found that a copy exists in the redirected area so we skip the folder creation.", inst);
#endif
        }
        else
        {
            std::filesystem::path CopySource = normalizedPath.drive_absolute_path;
            if (impl::PathExists(TurnPathIntoRootLocalDevice(vfspath.drive_absolute_path).c_str()))
            {
                CopySource = vfspath.drive_absolute_path;
            }


            auto attr = impl::GetFileAttributes(TurnPathIntoRootLocalDevice(CopySource.c_str()).c_str()); //normalizedPath.drive_absolute_path);
#ifdef MOREDEBUG
            Log(L"[%d]\t\tFRFShouldRedirect source %ls attributes=0x%x", inst, CopySource.c_str(), attr);
#endif
            if (attr != INVALID_FILE_ATTRIBUTES)
            {
                if ((attr & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
                {
#ifdef MOREDEBUG
                    Log(L"[%d]\tFRFShouldRedirect we have a file to be copied to %ls", inst, result.redirect_path.c_str());
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
                        LogString(inst, L"\t\tFRFShouldRedirect CopyFile Success From", CopySource.c_str());
                        LogString(inst, L"\t\tFRFShouldRedirect CopyFile Success To", result.redirect_path.c_str());
#endif
                    }
                    else
                    {
                        //0x72 = ERROR_INVALID_TARGET_HANDLE
                        auto err = ::GetLastError();
#if _DEBUG
                        Log("[%d]\t\tFRFShouldRedirect CopyFile Fail=0x%x", inst, err);
                        LogString(inst, L"\t\tFRFShouldRedirect CopyFile Fail From", CopySource.c_str());
                        LogString(inst, L"\t\tFRFShouldRedirect CopyFile Fail To", result.redirect_path.c_str());
#endif
                        switch (err)
                        {
                        case ERROR_FILE_EXISTS:
#if _DEBUG
                            Log(L"[%d]\t\tFRFShouldRedirect  was ERROR_FILE_EXISTS", inst);
#endif
                            break;
                        case ERROR_PATH_NOT_FOUND:
#if _DEBUG
                            Log(L"[%d]\t\tFRFShouldRedirect  was ERROR_PATH_NOT_FOUND", inst);
#endif
                            break;
                        case ERROR_FILE_NOT_FOUND:
#if _DEBUG
                            Log(L"[%d]\t\tFRFShouldRedirect  was ERROR_FILE_NOT_FOUND", inst);
#endif
                            break;
                        case ERROR_ALREADY_EXISTS:
#if _DEBUG
                            Log(L"[%d]\t\tFRFShouldRedirect  was ERROR_ALREADY_EXISTS", inst);
#endif
                            break;
                        default:
#if _DEBUG
                            Log(L"[%d]\t\tFRFShouldRedirect was 0x%x", inst, err);
#endif
                            break;
                        }
                    }
                }
                else
                {
#ifdef MOREDEBUG
                    Log(L"[%d]\tFRFShouldRedirect we have a directory to be copied to %ls.", inst, result.redirect_path.c_str());
#endif
                    copyResult = impl::CreateDirectoryEx(CopySource.c_str(), result.redirect_path.c_str(), nullptr);
                    if (copyResult)
                    {
#if _DEBUG
                        LogString(inst, L"\t\tFRFShouldRedirect CreateDir Success From", CopySource.c_str());
                        LogString(inst, L"\t\tFRFShouldRedirect CreateDir Success To", result.redirect_path.c_str());
#endif
                    }
                    else
                    {
#if _DEBUG
                        Log("[%d]\t\tFRFShouldRedirect CreateDir Fail=0x%x", inst, ::GetLastError());
                        LogString(inst, L"\t\tFRFShouldRedirect CreateDir Fail From", CopySource.c_str());
                        LogString(inst, L"\t\tFRFShouldRedirect CreateDir Fail To", result.redirect_path.c_str());
#endif
                    }
#if _DEBUG
                    auto err = ::GetLastError();
                    assert(copyResult || (err == ERROR_FILE_EXISTS) || (err == ERROR_PATH_NOT_FOUND) || (err == ERROR_FILE_NOT_FOUND) || (err == ERROR_ALREADY_EXISTS));
#endif
                }
            }
            else
            {
                //There is no source to copy, so we just want to allow it to be created in the redirected area.
#if _DEBUG
                Log(L"[%d]\t\tFRFShouldRedirect there is no package file to be copied to %ls.", inst, result.redirect_path.c_str());
#endif
            }
        }
    }
#if _DEBUG
    LogString(inst, L"\tFRFShouldRedirect: returns with result", result.redirect_path.c_str());
#endif
    }
    catch (...)
    {
        Log(L"[%d]*****FRFShouldRedirect Exeption!!", inst);
        result.should_redirect = false;  // What else to do???
    }
    return result;
}

path_redirect_info ShouldRedirect(const char* path, redirect_flags flags, DWORD inst)
{
#if _DEBUG
    Log(L"[%d]\t\tFRFShouldRedirectA",inst);
#endif
    return ShouldRedirectImpl(path, flags, inst);
}

path_redirect_info ShouldRedirect(const wchar_t* path, redirect_flags flags, DWORD inst)
{
#if _DEBUG
    Log(L"[%d]\t\tFRFShouldRedirectW",inst);
#endif
    return ShouldRedirectImpl(path, flags, inst);
}


/// <summary>
///  Given a file path, return it in root local device form, if possible, or in original form.
/// </summary>
std::string TurnPathIntoRootLocalDevice(const char* path)
{
    std::string sPath = path;
    // exclude prepending on file shares and other forms of '\\'
    if (sPath.length() > 2 &&
        path[0] != '\\' &&
        path[1] != '\\')
    {
        // exclude relative paths
        if (path[1] == ':')
        {
            sPath.insert(0, "\\\\?\\");
        }
    }
    return sPath;
}
std::wstring TurnPathIntoRootLocalDevice(const wchar_t* path)
{

    std::wstring wPath = path;
    // exclude prepending on file shares and other forms of '\\'
    if (wPath.length() > 2 &&
        path[0] != L'\\' &&
        path[1] != L'\\')
    {
        // exclude relative paths
        if (path[1] == L':')
        {
            wPath.insert(0, L"\\\\?\\");
        }
    }
    return wPath;

}


// We are seeing apps adding in an extra backslash when performing certain file operations.
// The cause seems like it must be the FRF, but we can simply fix it here like this.
std::string RemoveAnyFinalDoubleSlash(std::string input)
{
    size_t off = input.rfind("\\\\");
    if (off != std::wstring::npos &&
        off > 4)
    {
        std::string output = input.substr(0, off);
        output.append(input.substr(off + 1));
        return output;
    }
    else
        return input;
}
std::wstring RemoveAnyFinalDoubleSlash(std::wstring input)
{
    size_t off = input.rfind(L"\\\\");
    if (off != std::wstring::npos &&
        off > 4)
    {
        std::wstring output = input.substr(0, off);
        output.append(input.substr(off + 1));
        return output;
    }
    else
        return input;
}

