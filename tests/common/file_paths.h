//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <string>

#include <known_folders.h>

#include "test_config.h"

inline void clean_redirection_path()
{
    // NOTE: This makes an assumption about the location of redirected files! If this path ever changes, then the logic
    //       here will need to get updated. At that time, it may just be better to add an export to PsfRuntime for
    //       identifying what the location is.
    // NOTE: In the future, if we ever do handle deletes of files in the package path, we should also handle that here,
    //       e.g. by deleting the file that's tracking the deletions, or by calling into an export in PsfRuntime to do
    //       so for us.
    static const auto redirectRoot = std::filesystem::path(LR"(\\?\)" + psf::known_folder(FOLDERID_LocalAppData).native()) / L"VFS";

    std::error_code ec;
    std::filesystem::remove_all(redirectRoot, ec);
    if (ec)
    {
        trace_message(L"WARNING: Failed to clean the redirected path. Future tests may be impacted by this...\n", warning_color);
        trace_messages(warning_color, L"WARNING: ", ec.message(), new_line);
    }
    else
    {
        // The file redirection fixup makes an assumption about the existence of the VFS directory, but we just deleted
        // it above, so re-create it to avoid future issues
        if (!::CreateDirectoryW(redirectRoot.c_str(), nullptr))
        {
            trace_message(L"WARNING: Failed to re-create the VFS directory in the redirected path. Future tests may be impacted by this...\n", warning_color);
            trace_messages(warning_color, L"WARNING: ", error_message(::GetLastError()), new_line);
        }
    }
}

inline std::string read_entire_file(const wchar_t* path)
{
    std::string result;
    auto file = ::CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file != INVALID_HANDLE_VALUE)
    {
        char buffer[256];
        DWORD bytesRead;
        do
        {
            if (::ReadFile(file, buffer, sizeof(buffer), &bytesRead, nullptr))
            {
                result.append(buffer, bytesRead);
            }
            else
            {
                trace_messages(warning_color, L"WARNING: Failed to read from file: ", warning_info_color, path, new_line);
                trace_messages(warning_color, L"WARNING: Failure was: ", warning_info_color, error_message(::GetLastError()), new_line);
                break;
            }
        } while (bytesRead > 0);

        ::CloseHandle(file);
    }
    else
    {
        trace_messages(warning_color, L"WARNING: Failed to open file: ", warning_info_color, path, new_line);
        trace_messages(warning_color, L"WARNING: Failure was: ", warning_info_color, error_message(::GetLastError()), new_line);
    }

    return result;
}

inline bool write_entire_file(const wchar_t* path, const char* contents)
{
    auto file = ::CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        trace_messages(warning_color, L"WARNING: Failed to create file: ", warning_info_color, path, new_line);
        trace_messages(warning_color, L"WARNING: Failure was: ", warning_info_color, error_message(::GetLastError()), new_line);
        return false;
    }

    auto len = static_cast<DWORD>(std::strlen(contents));
    DWORD bytesWritten;
    if (!::WriteFile(file, contents, len, &bytesWritten, nullptr))
    {
        ::CloseHandle(file);
        trace_messages(warning_color, L"WARNING: Failed to write to file: ", warning_info_color, path, new_line);
        trace_messages(warning_color, L"WARNING: Failure was: ", warning_info_color, error_message(::GetLastError()), new_line);
        return false;
    }

    assert(len == bytesWritten);
    ::CloseHandle(file);
    return true;
}
