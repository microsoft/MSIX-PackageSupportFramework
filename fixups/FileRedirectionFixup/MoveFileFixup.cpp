//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"

template <typename CharT>
BOOL __stdcall MoveFileFixup(_In_ const CharT* existingFileName, _In_ const CharT* newFileName) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            if constexpr (psf::is_ansi<CharT>)
            {
                Log("MoveFileFixup for %s  %s", existingFileName, newFileName);
            }
            else
            {
                Log(L"MoveFileFixup for %ls  %ls", existingFileName, newFileName);
            }

            // NOTE: MoveFile needs delete access to the existing file, but since we won't have delete access to the
            //       file if it is in the package, we copy-on-read it here. This is slightly wasteful since we're
            //       copying it only for it to immediately get deleted, but that's simpler than trying to roll our own
            //       implementation of MoveFile. And of course, the same limitation for deleting files applies here as
            //       well. Additionally, we don't copy-on-read the destination file for the same reason we don't do the
            //       same for CopyFile: we give the application the benefit of the doubt that they previously tried to
            //       delete the file if it exists in the package path.
            auto [redirectExisting, existingRedirectPath, shouldReadonlSource] = ShouldRedirect(existingFileName, redirect_flags::copy_on_read);
            auto [redirectDest, destRedirectPath, shouldReadonlyDest] = ShouldRedirect(newFileName, redirect_flags::ensure_directory_structure);
            if (redirectExisting || redirectDest)
            {
                return impl::MoveFile(
                    redirectExisting ? existingRedirectPath.c_str() : widen_argument(existingFileName).c_str(),
                    redirectDest ? destRedirectPath.c_str() : widen_argument(newFileName).c_str());
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::MoveFile(existingFileName, newFileName);
}
DECLARE_STRING_FIXUP(impl::MoveFile, MoveFileFixup);

template <typename CharT>
BOOL __stdcall MoveFileExFixup(
    _In_ const CharT* existingFileName,
    _In_opt_ const CharT* newFileName,
    _In_ DWORD flags) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            if constexpr (psf::is_ansi<CharT>)
            {
                Log("MoveFileExFixup for %s  %s", existingFileName, newFileName);
            }
            else
            {
                Log(L"MoveFileExFixup for %ls  %ls", existingFileName, newFileName);
            }

            // See note in MoveFile for commentary on copy-on-read functionality (though we could do better by checking
            // flags for MOVEFILE_REPLACE_EXISTING)
            auto [redirectExisting, existingRedirectPath, shouldReadonlySource] = ShouldRedirect(existingFileName, redirect_flags::copy_on_read);
            auto [redirectDest, destRedirectPath, shouldReadonlyDest] = ShouldRedirect(newFileName, redirect_flags::ensure_directory_structure);
            if (redirectExisting || redirectDest)
            {
                return impl::MoveFileEx(
                    redirectExisting ? existingRedirectPath.c_str() : widen_argument(existingFileName).c_str(),
                    redirectDest ? destRedirectPath.c_str() : widen_argument(newFileName).c_str(),
                    flags);
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::MoveFileEx(existingFileName, newFileName, flags);
}
DECLARE_STRING_FIXUP(impl::MoveFileEx, MoveFileExFixup);
