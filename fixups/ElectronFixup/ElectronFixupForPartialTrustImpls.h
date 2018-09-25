#pragma once

#include "psf_framework.h"
#include "reentrancy_guard.h"

inline thread_local psf::reentrancy_guard g_reentrancyGuard;

namespace impl 
{
    // Directory Manipulation
    inline auto CreateDirectory     = psf::detoured_string_function(&::CreateDirectoryA    , &::CreateDirectoryW);
    inline auto RemoveDirectory     = psf::detoured_string_function(&::RemoveDirectoryA    , &::RemoveDirectoryW);

    // Attribute Manipulation
    inline auto GetFileAttributes   = psf::detoured_string_function(&::GetFileAttributesA  , &::GetFileAttributesW  );
    inline auto GetFileAttributesEx = psf::detoured_string_function(&::GetFileAttributesExA, &::GetFileAttributesExW);
    inline auto SetFileAttributes   = psf::detoured_string_function(&::SetFileAttributesA  , &::SetFileAttributesW  );

    // File Manipulation
    inline auto FindFirstFileEx     = psf::detoured_string_function(&::FindFirstFileExA    , &::FindFirstFileExW);
    inline auto MoveFile            = psf::detoured_string_function(&::MoveFileA           , &::MoveFileW       );
    inline auto MoveFileEx          = psf::detoured_string_function(&::MoveFileExA         , &::MoveFileExW     );
    inline auto CopyFile            = psf::detoured_string_function(&::CopyFileA           , &::CopyFileW       );
    inline auto ReplaceFile         = psf::detoured_string_function(&::ReplaceFileA        , &::ReplaceFileW    );
    inline auto DeleteFile          = psf::detoured_string_function(&::DeleteFileA         , &::DeleteFileW     );
    inline auto CreateFile          = psf::detoured_string_function(&::CreateFileA         , &::CreateFileW     );

    // Pipe Manipulation
    inline auto CreateNamedPipe     = psf::detoured_string_function(&::CreateNamedPipeA    , &::CreateNamedPipeW);
    inline auto CallNamedPipe       = psf::detoured_string_function(&::CallNamedPipeA      , &::CallNamedPipeW  );
    inline auto WaitNamedPipe       = psf::detoured_string_function(&::WaitNamedPipeA      , &::WaitNamedPipeW  );
} // namespace impl
