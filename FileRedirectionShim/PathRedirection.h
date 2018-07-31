//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <filesystem>

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
};

path_redirect_info ShouldRedirect(const char* path, redirect_flags flags);
path_redirect_info ShouldRedirect(const wchar_t* path, redirect_flags flags);

struct normalized_path
{
    // The full_path could either be:
    //      1.  A drive-absolute path. E.g. "C:\foo\bar.txt"
    //      2.  A local device path. E.g. "\\.\C:\foo\bar.txt" or "\\.\COM1"
    //      3.  A root-local device path. E.g. "\\?\C:\foo\bar.txt" or "\\?\HarddiskVolume1\foo\bar.txt"
    //      4.  A UNC-absolute path. E.g. "\\server\share\foo\bar.txt"
    // or empty if there was a failure
    std::wstring full_path;

    // A pointer inside of full_path if the path explicitly uses a drive symbolic link at the root, otherwise nullptr.
    // Note that this isn't perfect; e.g. we don't handle scenarios such as "\\localhost\C$\foo\bar.txt"
    wchar_t* drive_absolute_path = nullptr;
};

normalized_path NormalizePath(const char* path);
normalized_path NormalizePath(const wchar_t* path);

// If the input path is relative to the VFS folder under the package path (e.g. "${PackageRoot}\VFS\SystemX64\foo.txt"),
// then modifies that path to its virtualized equivalent (e.g. "C:\Windows\System32\foo.txt")
normalized_path DeVirtualizePath(normalized_path path);

// Short-circuit to determine what the redirected path would be. No check to see if the path should be redirected is
// performed.
std::wstring RedirectedPath(const normalized_path& deVirtualizedPath, bool ensureDirectoryStructure = false);

// A convenience type to avoid a copy for already-wide function argument strings
struct wide_argument_string
{
    const wchar_t* value = nullptr;
    const wchar_t* c_str() const noexcept
    {
        return value;
    }
};

struct wide_argument_string_with_buffer : wide_argument_string
{
    std::wstring buffer;

    wide_argument_string_with_buffer() = default;
    wide_argument_string_with_buffer(std::wstring str) :
        buffer(std::move(str))
    {
        value = buffer.c_str();
    }
};

inline wide_argument_string_with_buffer widen_argument(const char* str)
{
    if (str)
    {
        return wide_argument_string_with_buffer{ widen(str) };
    }
    else
    {
        // Calling widen with a null pointer will crash. Default constructed wide_argument_string_with_buffer will
        // preserve the null argument
        return wide_argument_string_with_buffer{};
    }
}

inline wide_argument_string widen_argument(const wchar_t* str) noexcept
{
    return wide_argument_string{ str };
}
