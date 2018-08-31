#pragma once

#include "shim_framework.h"
#include "reentrancy_guard.h"

inline thread_local shims::reentrancy_guard g_reentrancyGuard;

namespace impl 
{
    // Directory Manipulation
    inline auto CreateDirectory     = shims::detoured_string_function(&::CreateDirectoryA    , &::CreateDirectoryW);
    inline auto RemoveDirectory     = shims::detoured_string_function(&::RemoveDirectoryA    , &::RemoveDirectoryW);

    // Attribute Manipulation
    inline auto GetFileAttributes   = shims::detoured_string_function(&::GetFileAttributesA  , &::GetFileAttributesW  );
    inline auto GetFileAttributesEx = shims::detoured_string_function(&::GetFileAttributesExA, &::GetFileAttributesExW);
    inline auto SetFileAttributes   = shims::detoured_string_function(&::SetFileAttributesA  , &::SetFileAttributesW  );

    // File Manipulation
    inline auto FindFirstFileEx     = shims::detoured_string_function(&::FindFirstFileExA    , &::FindFirstFileExW);
    inline auto MoveFile            = shims::detoured_string_function(&::MoveFileA           , &::MoveFileW       );
    inline auto MoveFileEx          = shims::detoured_string_function(&::MoveFileExA         , &::MoveFileExW     );
    inline auto CopyFile            = shims::detoured_string_function(&::CopyFileA           , &::CopyFileW       );
    inline auto ReplaceFile         = shims::detoured_string_function(&::ReplaceFileA        , &::ReplaceFileW    );
    inline auto DeleteFile          = shims::detoured_string_function(&::DeleteFileA         , &::DeleteFileW     );
    inline auto CreateFile          = shims::detoured_string_function(&::CreateFileA         , &::CreateFileW     );

    // Pipe Manipulation
    inline auto CreateNamedPipe     = shims::detoured_string_function(&::CreateNamedPipeA    , &::CreateNamedPipeW);
    inline auto CallNamedPipe       = shims::detoured_string_function(&::CallNamedPipeA      , &::CallNamedPipeW  );
    inline auto WaitNamedPipe       = shims::detoured_string_function(&::WaitNamedPipeA      , &::WaitNamedPipeW  );
} // namespace impl
