//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <cassert>
#include <cwctype>
#include <string>

#include <windows.h>

namespace psf
{
    template <typename CharT>
    inline constexpr bool is_path_separator(CharT ch)
    {
        return (ch == '\\') || (ch == '/');
    }

    struct path_compare
    {
        bool operator()(wchar_t lhs, wchar_t rhs)
        {
            if (std::towlower(lhs) == std::towlower(rhs))
            {
                return true;
            }

            // Otherwise, both must be separators
            return is_path_separator(lhs) && is_path_separator(rhs);
        }
    };

    enum class dos_path_type
    {
        unknown,
        unc_absolute,       // E.g. "\\servername\share\path\to\file"
        drive_absolute,     // E.g. "C:\path\to\file"
        drive_relative,     // E.g. "C:path\to\file"
        rooted,             // E.g. "\path\to\file"
        relative,           // E.g. "path\to\file"
        local_device,       // E.g. "\\.\C:\path\to\file"
        root_local_device,  // E.g. "\\?\C:\path\to\file"
    };

    template <typename CharT>
    inline dos_path_type path_type(const CharT* path) noexcept
    {
        // NOTE: Root-local device paths don't get normalized and therefore do not allow forward slashes
        constexpr wchar_t root_local_device_prefix[] = LR"(\\?\)";
        if (std::equal(root_local_device_prefix, root_local_device_prefix + 4, path))
        {
            return dos_path_type::root_local_device;
        }

        constexpr wchar_t root_local_device_prefix_dot[] = LR"(\\.\)";
        if (std::equal(root_local_device_prefix_dot, root_local_device_prefix_dot + 4, path))
        {
            return dos_path_type::local_device;
        }

        constexpr wchar_t unc_prefix[] = LR"(\\)";
        if (std::equal(unc_prefix, unc_prefix + 2, path))
        {
            // Otherwise assume any other character is the start of a server name
            return dos_path_type::unc_absolute;
        }

        constexpr wchar_t rooted_prefix[] = LR"(\)";
        if (std::equal(rooted_prefix, rooted_prefix + 1, path))
        {
            return dos_path_type::rooted;
        }

        if (std::iswalpha(path[0]) && (path[1] == ':'))
        {
            return is_path_separator(path[2]) ? dos_path_type::drive_absolute : dos_path_type::drive_relative;
        }

        // Otherwise assume that it's a relative path
        return dos_path_type::relative;
    }

    inline DWORD get_full_path_name(const char* path, DWORD length, char* buffer, char** filePart = nullptr)
    {
        return ::GetFullPathNameA(path, length, buffer, filePart);
    }

    inline DWORD get_full_path_name(const wchar_t* path, DWORD length, wchar_t* buffer, wchar_t** filePart = nullptr)
    {
        return ::GetFullPathNameW(path, length, buffer, filePart);
    }

    // Wrapper around GetFullPathName
    // NOTE: We prefer this over std::filesystem::absolute since it saves one allocation/copy in the best case and is
    //       equivalent in the worst case since there's no generic null-terminated-string overload. It also gives us
    //       future flexibility to fixup GetFullPathName, which is something that we can't control in the implementation
    //       of the <filesystem> header
    template <typename CharT>
    inline std::basic_string<CharT> full_path(const CharT* path)
    {
        // Root-local device paths are forwarded to the object manager with minimal modification, so we shouldn't be
        // trying to expand them here
        assert(path_type(path) != dos_path_type::root_local_device);

        auto len = get_full_path_name(path, 0, nullptr);
        if (!len)
        {
            // Error occurred. We don't expect to ever see this, but in the event that we do, give back an empty path
            // and let the caller decide how to handle it
            assert(false);
            return {};
        }

        std::basic_string<CharT> buffer(len - 1, '\0');
        len = get_full_path_name(path, len, buffer.data());
        assert(len <= buffer.length());
        if (!len || (len > buffer.length()))
        {
            assert(false);
            return {};
        }

        buffer.resize(len);
        return buffer;
    }
}
