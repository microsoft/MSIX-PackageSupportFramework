//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <algorithm>
#include <filesystem>
#include <known_folders.h>

#include <windows.h>

#include <file_paths.h>

struct vfs_mapping
{
    std::filesystem::path path;
    std::filesystem::path package_path;
};

// All files placed in the package path have the same initial contents
constexpr const char g_packageFileContents[] = "You are reading from the package path";

constexpr const wchar_t g_packageFileName[] = L"ÞáçƙáϱèFïℓè.txt";

inline std::filesystem::path g_packageRootPath;

inline vfs_mapping g_systemX86Mapping;
inline vfs_mapping g_programFilesX86Mapping;
inline vfs_mapping g_programFilesCommonX86Mapping;
#if !_M_IX86
inline vfs_mapping g_systemX64Mapping;
inline vfs_mapping g_programFilesX64Mapping;
inline vfs_mapping g_programFilesCommonX64Mapping;
#endif
inline vfs_mapping g_windowsMapping;
inline vfs_mapping g_commonAppDataMapping;
inline vfs_mapping g_system32CatrootMapping;
inline vfs_mapping g_system32Catroot2Mapping;
inline vfs_mapping g_system32DriversEtcMapping;
inline vfs_mapping g_system32DriverStoreMapping;
inline vfs_mapping g_system32LogFilesMapping;
inline vfs_mapping g_system32SpoolMapping;

inline const std::reference_wrapper<const vfs_mapping> g_vfsMappings[] =
{
    g_systemX86Mapping,
    g_programFilesX86Mapping,
    g_programFilesCommonX86Mapping,
#if !_M_IX86
    g_systemX64Mapping,
    g_programFilesX64Mapping,
    g_programFilesCommonX64Mapping,
#endif
    g_windowsMapping,
    g_commonAppDataMapping,
    g_system32CatrootMapping,
    g_system32Catroot2Mapping,
    g_system32DriversEtcMapping,
    g_system32DriverStoreMapping,
    g_system32LogFilesMapping,
    g_system32SpoolMapping,
};

inline std::wstring test_filename(const vfs_mapping& mapping)
{
    auto result = mapping.package_path.filename().native() + L"Fïℓè.txt";

    // Remove any spaces
    result.erase(std::remove(result.begin(), result.end(), ' '), result.end());

    // Remove 'AppVSystem32' prefix, if present
    constexpr std::wstring_view appvSystem32Prefix = L"AppVSystem32";
    if (std::wstring_view(result).substr(0, appvSystem32Prefix.length()) == appvSystem32Prefix)
    {
        result.erase(0, appvSystem32Prefix.length());
    }

    return result;
}

inline std::filesystem::path actual_path(HANDLE file)
{
    auto len = ::GetFinalPathNameByHandleW(file, nullptr, 0, 0);
    if (!len)
    {
        return {};
    }

    std::wstring result(len - 1, '\0');
    len = ::GetFinalPathNameByHandleW(file, result.data(), len, 0);
    if (!len)
    {
        return {};
    }

    assert(len <= result.length());
    result.resize(len);
    return result;
}

inline std::filesystem::path actual_path(const wchar_t* path)
{
    auto handle = ::CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE)
    {
        return {};
    }

    try
    {
        auto result = actual_path(handle);
        ::CloseHandle(handle);
        return result;
    }
    catch (...)
    {
        ::CloseHandle(handle);
        throw;
    }
}
