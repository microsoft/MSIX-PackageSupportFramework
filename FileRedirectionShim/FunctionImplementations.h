//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// Collection of function pointers that will always point to (what we think are) the actual function implementations.
// That is, impl::CreateFile will (presumably) call kernelbase!CreateFileA/W, even though we detour that call. Useful to
// reduce the risk of us shimming ourselves, which can easily lead to infinite recursion. Note that this isn't perfect.
// For example, CreateFileShim could call kernelbase!CopyFileW, which could in turn call (the shimmed) CreateFile again
#pragma once

#include <reentrancy_guard.h>
#include <shim_framework.h>

// A much bigger hammer to avoid reentrancy. Still, the impl::* functions are good to have around to prevent the
// unnecessary invocation of the shim
inline thread_local shims::reentrancy_guard g_reentrancyGuard;

namespace impl
{
    inline auto CopyFile = shims::detoured_string_function(&::CopyFileA, &::CopyFileW);
    inline auto CopyFileEx = shims::detoured_string_function(&::CopyFileExA, &::CopyFileExW);
    inline auto CopyFile2 = &::CopyFile2;

    inline auto CreateDirectory = shims::detoured_string_function(&::CreateDirectoryA, &::CreateDirectoryW);
    inline auto CreateDirectoryEx = shims::detoured_string_function(&::CreateDirectoryExA, &::CreateDirectoryExW);

    inline auto CreateFile = shims::detoured_string_function(&::CreateFileA, &::CreateFileW);
    inline auto CreateFile2 = &::CreateFile2;

    inline auto CreateHardLink = shims::detoured_string_function(&::CreateHardLinkA, &::CreateHardLinkW);

    inline auto CreateSymbolicLink = shims::detoured_string_function(&::CreateSymbolicLinkA, &::CreateSymbolicLinkW);

    inline auto DeleteFile = shims::detoured_string_function(&::DeleteFileA, &::DeleteFileW);

    inline auto FindClose = &::FindClose;
    inline auto FindFirstFile = shims::detoured_string_function(&::FindFirstFileA, &::FindFirstFileW);
    inline auto FindFirstFileEx = shims::detoured_string_function(&::FindFirstFileExA, &::FindFirstFileExW);
    inline auto FindNextFile = shims::detoured_string_function(&::FindNextFileA, &::FindNextFileW);

    inline auto GetFileAttributes = shims::detoured_string_function(&::GetFileAttributesA, &::GetFileAttributesW);
    inline auto GetFileAttributesEx = shims::detoured_string_function(&::GetFileAttributesExA, &::GetFileAttributesExW);

    inline auto MoveFile = shims::detoured_string_function(&::MoveFileA, &::MoveFileW);
    inline auto MoveFileEx = shims::detoured_string_function(&::MoveFileExA, &::MoveFileExW);

    inline auto RemoveDirectory = shims::detoured_string_function(&::RemoveDirectoryA, &::RemoveDirectoryW);

    inline auto ReplaceFile = shims::detoured_string_function(&::ReplaceFileA, &::ReplaceFileW);

    inline auto SetFileAttributes = shims::detoured_string_function(&::SetFileAttributesA, &::SetFileAttributesW);

    // Most internal use of GetFileAttributes is to check to see if a file/directory exists, so provide a helper
    template <typename CharT>
    inline bool PathExists(CharT* path) noexcept
    {
        return GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES;
    }
}
