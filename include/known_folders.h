//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <filesystem>

#include <windows.h>
#include <combaseapi.h>
#include <Shlobj.h>
#include <KnownFolders.h>

#include "dos_paths.h"

namespace shims
{
    struct cotaskmemfree_deleter
    {
        void operator()(wchar_t* ptr)
        {
            if (ptr)
            {
                ::CoTaskMemFree(ptr);
            }
        }
    };

    using unique_cotaskmem_string = std::unique_ptr<wchar_t, cotaskmemfree_deleter>;

    inline std::filesystem::path remove_trailing_path_separators(std::filesystem::path path)
    {
        // To make string comparisons easier, don't terminate directory paths with trailing separators
        return path.has_filename() ? path : path.parent_path();
    }

    inline std::filesystem::path known_folder(const GUID& id, DWORD flags = KF_FLAG_DEFAULT)
    {
        PWSTR path;
        if (FAILED(::SHGetKnownFolderPath(id, flags, nullptr, &path)))
        {
            throw std::runtime_error("Failed to get known folder path");
        }
        unique_cotaskmem_string uniquePath(path);

        // For consistency, and therefore simplicity, ensure that all paths are drive-absolute
        if (auto pathType = path_type(path);
            (pathType == dos_path_type::root_local_device) || (pathType == dos_path_type::local_device))
        {
            path += 4;
        }
        assert(path_type(path) == dos_path_type::drive_absolute);

        return remove_trailing_path_separators(path);
    }
}
