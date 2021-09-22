//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <filesystem>
#include <dos_paths.h>

enum class redirect_flags
{
    none = 0x0000,
    ensure_directory_structure = 0x0001,
    copy_file = 0x0002,
    check_file_presence = 0x0004,

    copy_on_read = ensure_directory_structure | copy_file,
};
DEFINE_ENUM_FLAG_OPERATORS(redirect_flags);

template <typename T>
inline constexpr bool flag_set(T value, T flag)
{
    return (value & flag) == flag;
}

struct path_redirect_info
{
    bool should_redirect = false;
    std::filesystem::path redirect_path;
    bool shouldReadonly = false;
};

path_redirect_info ShouldRedirect(const char* path, redirect_flags flags, DWORD inst = 0);
path_redirect_info ShouldRedirect(const wchar_t* path, redirect_flags flags, DWORD inst = 0);

struct normalized_path
{
    // The full_path could either be:
    //      1.  A drive-absolute path. E.g. "C:\foo\bar.txt"
    //      2.  A local device path. E.g. "\\.\C:\foo\bar.txt" or "\\.\COM1"
    //      3.  A root-local device path. E.g. "\\?\C:\foo\bar.txt" or "\\?\HarddiskVolume1\foo\bar.txt"
    //      4.  A UNC-absolute path. E.g. "\\server\share\foo\bar.txt"
    // or empty if there was a failure
    std::wstring full_path;

    psf::dos_path_type path_type;

    // A pointer inside of full_path if the path explicitly uses a drive symbolic link at the root, otherwise nullptr.
    // Note that this isn't perfect; e.g. we don't handle scenarios such as "\\localhost\C$\foo\bar.txt"
    wchar_t* drive_absolute_path = nullptr;
};

normalized_path NormalizePath(const char* path);
normalized_path NormalizePath(const wchar_t* path);


struct normalized_path2
{
    // The full_path could either be:
    //      1.  A drive-absolute path. E.g. "C:\foo\bar.txt"
    //      2.  A local device path. E.g. "\\.\C:\foo\bar.txt" or "\\.\COM1"
    //      3.  A root-local device path. E.g. "\\?\C:\foo\bar.txt" or "\\?\HarddiskVolume1\foo\bar.txt"
    //      4.  A UNC-absolute path. E.g. "\\server\share\foo\bar.txt"
    // or empty if there was a failure
    // All are stored wide
    std::wstring original_path;
    std::wstring full_path;

    psf::dos_path_type path_type;

    // A shortened full_path if the path explicitly uses a drive symbolic link at the root, otherwise the full_path or a path with working directory added; always wide.
    std::wstring drive_absolute_path;
};

normalized_path2 NormalizePath2A(const char* path);
normalized_path2 NormalizePath2W(const wchar_t* path);
void LogNormalizedPath2(normalized_path2 np2, std::wstring desc, DWORD instance);


// If the input path is relative to the VFS folder under the package path (e.g. "${PackageRoot}\VFS\SystemX64\foo.txt"),
// then modifies that path to its virtualized equivalent (e.g. "C:\Windows\System32\foo.txt")
normalized_path DeVirtualizePath(normalized_path path);

std::wstring DeVirtualizePath2(normalized_path2 path);

// If the input path is a physical path outside of the package (e.g. "C:\Windows\System32\foo.txt"),
// this returns what the package VFS equivalent would be (e.g "C:\Program Files\WindowsApps\Packagename\VFS\SystemX64\foo.txt");
// NOTE: Does not check if package has this virtualized path.
normalized_path VirtualizePath(normalized_path path, DWORD impl = 0);

std::wstring VirtualizePath2(normalized_path2 path, DWORD impl = 0);



// Short-circuit to determine what the redirected path would be. No check to see if the path should be redirected is
// performed.
std::wstring RedirectedPath(const normalized_path& deVirtualizedPath, bool ensureDirectoryStructure = false, DWORD inst = 0);
std::wstring RedirectedPath2(const normalized_path2& deVirtualizedPath, bool ensureDirectoryStructure = false, DWORD inst = 0);

// does path start with basePath
bool path_relative_to(const wchar_t* path, const std::filesystem::path& basePath);
bool path_relative_to(const char* path, const std::filesystem::path& basePath);


// Determines if the path of the filename falls under the user's appdata local or roaming folders.
bool IsUnderUserAppDataLocal(_In_ const wchar_t* fileName);
bool IsUnderUserAppDataLocalPackages(_In_ const wchar_t* fileName);
bool IsUnderUserAppDataRoaming(_In_ const wchar_t* fileName);
bool IsUnderUserAppDataLocal(_In_ const char* fileName);
bool IsUnderUserAppDataLocalPackages(_In_ const char* fileName);
bool IsUnderUserAppDataRoaming(_In_ const char* fileName);

// Return path to existing package VFS file (or NULL if not present) but only for AppData local and remote
std::filesystem::path GetPackageVFSPath(const wchar_t* fileName);
std::filesystem::path GetPackageVFSPath(const char* fileName);

//
std::filesystem::path GetPackageVFSPathAll(const wchar_t* fileName);
std::filesystem::path GetPackageVFSPathAll(const char* fileName);

std::string UrlDecode(std::string str); 
std::wstring UrlDecode(std::wstring str);

bool IsColonColonGuid(const char* path);
bool IsColonColonGuid(const wchar_t* path);

bool IsBlobColon(const std::string path);
bool IsBlobColon(const std::wstring path);

std::string StripFileColonSlash(std::string old_string);
std::wstring StripFileColonSlash(std::wstring old_string);

/// <summary>
///  Given a file path, return it in root local device form, if possible, or in original form.
/// </summary>
std::string TurnPathIntoRootLocalDevice(const char* path);
std::wstring TurnPathIntoRootLocalDevice(const wchar_t* path);

void Log(const char* fmt, ...);
void Log(const wchar_t* fmt, ...);
void LogString(const char* name, const char* value);
void LogString(const char* name, const wchar_t* value);
void LogString(const wchar_t* name, const char* value);
void LogString(const wchar_t* name, const wchar_t* value);
void LogString(DWORD inst, const char* name, const char* value);
void LogString(DWORD inst, const char* name, const wchar_t* value);
void LogString(DWORD inst, const wchar_t* name, const char* value);
void LogString(DWORD inst, const wchar_t* name, const wchar_t* value);
void Loghexdump(void* pAddressIn, long  lSize);

void  LogStringAA(DWORD inst, const char* name, const char* value);
void  LogStringAW(DWORD inst, const char* name, const wchar_t* value);
void  LogStringWA(DWORD inst, const wchar_t* name, const char* value);
void  LogStringWW(DWORD inst, const wchar_t* name, const wchar_t* value);

extern DWORD g_FileIntceptInstance;
