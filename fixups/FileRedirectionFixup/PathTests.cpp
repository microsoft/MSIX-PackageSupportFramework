
//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies. All rights reserved
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


/// <summary>
/// Utility functions to determine if a given file path string is relative to another, as in the path starts the same.
/// Comparison is perfomed case insensitive.
/// </summary>
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

/// <summary>
/// Utility functions to determine if a given file path string is the same as another, including the same length.
/// Comparison is perfomed case insensitive.
/// </summary>
template <typename CharT>
bool path_same_asImpl(const CharT* path, const std::filesystem::path& basePath)
{
    if (basePath.wstring().size() == (widen(path)).size())
    {
        return std::equal(basePath.native().begin(), basePath.native().end(), path, psf::path_compare{});
    }
    return false;
}
bool path_same_as(const wchar_t* path, const std::filesystem::path& basePath)
{
    return path_same_asImpl(path, basePath);
}
bool path_same_as(const char* path, const std::filesystem::path& basePath)
{
    return path_same_asImpl(path, basePath);
}


/// <summary>
/// Utility functions to determine if a given file path string is relative to another, as in the path starts the same.
/// Comparison is perfomed case insensitive. Handles null paths.
/// </summary>
template <typename CharT>
bool _stdcall IsUnderFolderImpl(_In_ const CharT* fileName, _In_ const std::filesystem::path folder)
{
    if (fileName == NULL || folder.empty())
    {
        return false;
    }
    constexpr wchar_t root_local_device_prefix[] = LR"(\\?\)";
    constexpr wchar_t root_local_device_prefix_dot[] = LR"(\\.\)";

    if constexpr (psf::is_ansi<CharT>)
    {
        if (strlen(fileName) > 4)
        {
            if (std::equal(root_local_device_prefix, root_local_device_prefix + 4, fileName))
            {
                return path_relative_to(fileName + 4, folder);
            }
            else if (std::equal(root_local_device_prefix_dot, root_local_device_prefix_dot + 4, fileName))
            {
                return path_relative_to(fileName + 4, folder);
            }
        }
    }
    else
    {
        if (wcslen(fileName) > 4)
        {
            if (std::equal(root_local_device_prefix, root_local_device_prefix + 4, fileName))
            {
                return path_relative_to(fileName + 4, folder);
            }
            else if (std::equal(root_local_device_prefix_dot, root_local_device_prefix_dot + 4, fileName))
            {
                return path_relative_to(fileName + 4, folder);
            }
        }
    }
    return path_relative_to(fileName, folder);
}

/// <summary>
/// Utility functions to determine if a given file path string is relative to another, including the same length.
/// Comparison is perfomed case insensitive. Handles null paths.
/// </summary>
template <typename CharT>
bool IsThisFolderImpl(_In_ const CharT* fileName, _In_ const std::filesystem::path folder)
{
    if (fileName == NULL || folder.empty())
    {
        return false;
    }
    constexpr wchar_t root_local_device_prefix[] = LR"(\\?\)";
    constexpr wchar_t root_local_device_prefix_dot[] = LR"(\\.\)";
    if constexpr (psf::is_ansi<CharT>)
    {
        if (strlen(fileName) > 4)
        {
            if (std::equal(root_local_device_prefix, root_local_device_prefix + 4, fileName))
            {
                return path_same_as(fileName + 4, folder);
            }
            else if (std::equal(root_local_device_prefix_dot, root_local_device_prefix_dot + 4, fileName))
            {
                return path_same_as(fileName + 4, folder);
            }
        }
    }
    else
    {
        if (wcslen(fileName) > 4)
        {
            if (std::equal(root_local_device_prefix, root_local_device_prefix + 4, fileName))
            {
                return path_same_as(fileName + 4, folder);
            }
            else if (std::equal(root_local_device_prefix_dot, root_local_device_prefix_dot + 4, fileName))
            {
                return path_same_as(fileName + 4, folder);
            }
        }
    }
    return path_same_as(fileName, folder);
}


#pragma region IsUnderUserAppDataLocal
template <typename CharT>
bool _stdcall IsUnderUserAppDataLocalImpl(_In_ const CharT* fileName)
{
    return IsUnderFolderImpl(fileName, psf::known_folder(FOLDERID_LocalAppData));
}
bool IsUnderUserAppDataLocal(_In_ const wchar_t* fileName)
{
    return IsUnderUserAppDataLocalImpl(fileName);
}
bool IsUnderUserAppDataLocal(_In_ const char* fileName)
{
    return IsUnderUserAppDataLocalImpl(fileName);
}
#pragma endregion

#pragma region IsUnderUserAppDataLocalPackages
template <typename CharT>
bool _stdcall IsUnderUserAppDataLocalPackagesImpl(_In_ const CharT* fileName)
{
    return IsUnderFolderImpl(fileName, psf::known_folder(FOLDERID_LocalAppData) / L"Packages");
}
bool IsUnderUserAppDataLocalPackages(_In_ const wchar_t* fileName)
{
    return IsUnderUserAppDataLocalPackagesImpl(fileName);
}
bool IsUnderUserAppDataLocalPackages(_In_ const char* fileName)
{
    return IsUnderUserAppDataLocalPackagesImpl(fileName);
}
#pragma endregion

#pragma region IsUnderUserAppDataRoaming
template <typename CharT>
bool _stdcall IsUnderUserAppDataRoamingImpl(_In_ const CharT* fileName)
{
    return IsUnderFolderImpl(fileName, psf::known_folder(FOLDERID_RoamingAppData));
}
bool IsUnderUserAppDataRoaming(_In_ const wchar_t* fileName)
{
    return IsUnderUserAppDataRoamingImpl(fileName);
}
bool IsUnderUserAppDataRoaming(_In_ const char* fileName)
{
    return IsUnderUserAppDataRoamingImpl(fileName);
}
#pragma endregion

#pragma region IsUnderUserPackageWritablePackageRoot
template <typename CharT>
bool _stdcall IsUnderUserPackageWritablePackageRootImpl(_In_ const CharT* fileName)
{
    return IsUnderFolderImpl(fileName, g_writablePackageRootPath);
}
bool IsUnderUserPackageWritablePackageRoot(_In_ const wchar_t* fileName)
{
    return IsUnderUserPackageWritablePackageRootImpl(fileName);
}
bool IsUnderUserPackageWritablePackageRoot(_In_ const char* fileName)
{
    return IsUnderUserPackageWritablePackageRootImpl(fileName);
}
#pragma endregion

#pragma region IsUnderPackageRoot
template <typename CharT>
bool _stdcall IsUnderPackageRootImpl(_In_ const CharT* fileName)
{
    return IsUnderFolderImpl(fileName, g_packageRootPath);
}
bool IsUnderPackageRoot(_In_ const wchar_t* fileName)
{
    return IsUnderPackageRootImpl(fileName);
}
bool IsUnderPackageRoot(_In_ const char* fileName)
{
    return IsUnderPackageRootImpl(fileName);
}
#pragma endregion

#pragma region IsPackageRoot
template <typename CharT>
bool IsPackageRootImpl(_In_ const CharT* fileName)
{
    return IsThisFolderImpl(fileName, g_packageRootPath);
}
bool IsPackageRoot(_In_ const wchar_t* fileName)
{
    return IsPackageRootImpl(fileName);
}

bool IsPackageRoot(_In_ const char* fileName)
{
    return IsPackageRootImpl(fileName);
}
#pragma endregion


#pragma region SpecialtyPaths
bool IsColonColonGuid(const char* path)
{
    if (path != nullptr && strlen(path) > 39)
    {
        if (path[0] == ':' &&
            path[1] == ':' &&
            path[2] == '{')
        {
            return true;
        }
    }
    return false;
}

bool IsColonColonGuid(const wchar_t* path)
{
    if (path != nullptr && wcslen(path) > 39)
    {
        if (path[0] == L':' &&
            path[1] == L':' &&
            path[2] == L'{')
        {
            return true;
        }
    }
    return false;
}

bool IsBlobColon(const std::string path)
{
    if (!path.empty())
    {
        size_t found = path.find("blob:", 0);
        if (found == 0)
        {
            return true;
        }
        found = path.find("BLOB:", 0);
        if (found == 0)
        {
            return true;
        }
    }
    return false;
}

bool IsBlobColon(const std::wstring path)
{
    if (!path.empty())
    {
        size_t found = path.find(L"blob:", 0);
        if (found == 0)
        {
            return true;
        }
        found = path.find(L"BLOB:", 0);
        if (found == 0)
        {
            return true;
        }
    }
    return false;
}
#pragma endregion

#pragma region URLEncodeDecode
// Method to decode a string that includes %xx replacement characters commonly found in URLs with the
// native equivalent (std::string forrm).
// Example input: C:\Program%20Files\Vendor%20Name
std::string UrlDecode(std::string str) {
    std::string ret;
    char ch;
    size_t i, len = str.length();
    unsigned int ii;

    for (i = 0; i < len; i++)
    {
        if (str[i] != '%')
        {
            ret += str[i];
        }
        else
        {
            if (sscanf_s(str.substr(i + 1, 4).c_str(), "%x", &ii) == 1)
            {
                ch = static_cast<char>(ii);
                ret += ch;
                if (ii <= 255)
                {
                    i = i + 2;
                }
                else
                {
                    i = i + 4;
                }
            }
            else
            {
                ret += str[i];
            }
        }
    }
    return ret;
}

// Method to decode a string that includes %xx replacement characters commonly found in URLs with the
// native equivalent (std::wstring forrm).
// Example input: C:\Program%20Files\Vendor%20Name
std::wstring UrlDecode(std::wstring str)
{
    std::wstring ret;
    char ch;
    size_t i, len = str.length();
    unsigned int ii;

    for (i = 0; i < len; i++)
    {
        if (str[i] != L'%')
        {
            ret += str[i];
        }
        else
        {
            if (swscanf_s(str.substr(i + 1, 4).c_str(), L"%x", &ii) == 1)
            {
                ch = static_cast<char>(ii);
                ret += ch;
                if (ii <= 255)
                {
                    i = i + 2;
                }
                else
                {
                    i = i + 4;
                }
            }
            else
            {
                ret += str[i];
            }
        }
    }
    return ret;
}
#pragma endregion


#pragma region ReplaceChars

std::string ReplaceSlashBackwardOnly(std::string old_string)
{
    std::string dap = old_string;
    std::replace(dap.begin(), dap.end(), '/', '\\');
    return dap;
}

std::wstring ReplaceSlashBackwardOnly(std::wstring old_string)
{
    std::wstring dap = old_string;
    std::replace(dap.begin(), dap.end(), L'/', L'\\');
    return dap;
}
#pragma endregion


#pragma region StripAtStarts
// Method to remove certain URI prefixes (std::string form)
std::string StripAtStart(std::string old_string, const char* toStrip)
{
    size_t found = old_string.find(toStrip, 0);
    if (found == 0)
    {
        return old_string.substr(strlen(toStrip));
    }
    return old_string;
}

// Method to remove certain URI prefixes (std::wstring form)
std::wstring StripAtStartW(std::wstring old_string, const wchar_t* toStrip)
{
    size_t found = old_string.find(toStrip, 0);
    if (found == 0)
    {
        return old_string.substr(wcslen(toStrip));
    }
    return old_string;
}


std::string StripFileColonSlash(std::string old_string)
{
    std::string sRet = old_string;
    sRet = StripAtStart(sRet, "file:\\\\");
    sRet = StripAtStart(sRet, "file://");
    sRet = StripAtStart(sRet, "FILE:\\\\");
    sRet = StripAtStart(sRet, "FILE://");
    sRet = StripAtStart(sRet, "\\\\file\\");
    sRet = StripAtStart(sRet, "\\\\FILE\\");

    return sRet;
}
std::wstring StripFileColonSlash(std::wstring old_string)
{
    std::wstring sRet = old_string;
    sRet = StripAtStartW(sRet, L"file:\\\\");
    sRet = StripAtStartW(sRet, L"file://");
    sRet = StripAtStartW(sRet, L"FILE:\\\\");
    sRet = StripAtStartW(sRet, L"FILE://");
    sRet = StripAtStartW(sRet, L"\\\\file\\");
    sRet = StripAtStartW(sRet, L"\\\\FILE\\");

    return sRet;
}

#pragma endregion


///  Given an absolute path, trim it to the folder after the VFS folder
//   For example:  C:\Program Files\WindowsApps\pfn\VFS\AppData\Vendor\foo\bar\now returns C:\Program Files\WindowsApps\pfn\VFS\AppData
std::filesystem::path trim_absvfs2varfolder(std::filesystem::path abs)
{
    std::filesystem::path trimmed = abs;
    if (trimmed.wstring().find(L"VFS") != std::wstring::npos)
    {
        while (trimmed.has_parent_path())
        {
            if (trimmed.parent_path().filename().wstring().compare(L"VFS") == 0)
                return trimmed;
            trimmed = trimmed.parent_path();
        }
    }
    return abs;
}

