//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"

template <typename CharT>
BOOL __stdcall CopyFileFixup(_In_ const CharT* existingFileName, _In_ const CharT* newFileName, _In_ BOOL failIfExists) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            // NOTE: We don't want to copy either file in the event one/both exist. Copying the source file would be
            //       wasteful since it's not the file that we care about (nor do we need write permissions to it); we
            //       just need to know its redirect path so that we can copy the most up to date one. It wouldn't
            //       necessarily be a complete waste of time to copy the destination file, particularly if
            //       'failIfExists' is true, but we'll go ahead and make the assumption that no application will try and
            //       copy to a file it wrote on install with 'failIfExists' as true. One possible option would be to
            //       manually fail out ourselves if 'failIfExists' is true and the file exists in the package, but
            //       that's arguably worse since we currently aren't handling the case where an application tries to
            //       delete a file in its package path.
            auto [redirectSource, sourceRedirectPath] = ShouldRedirect(existingFileName, redirect_flags::check_file_presence);
            auto [redirectDest, destRedirectPath] = ShouldRedirect(newFileName, redirect_flags::ensure_directory_structure);
            if (redirectSource || redirectDest)
            {
                return impl::CopyFile(
                    redirectSource ? sourceRedirectPath.c_str() : widen_argument(existingFileName).c_str(),
                    redirectDest ? destRedirectPath.c_str() : widen_argument(newFileName).c_str(),
                    failIfExists);
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::CopyFile(existingFileName, newFileName, failIfExists);
}
DECLARE_STRING_FIXUP(impl::CopyFile, CopyFileFixup);

template <typename CharT>
BOOL __stdcall CopyFileExFixup(
    _In_ const CharT* existingFileName,
    _In_ const CharT* newFileName,
    _In_opt_ LPPROGRESS_ROUTINE progressRoutine,
    _In_opt_ LPVOID data,
    _When_(cancel != NULL, _Pre_satisfies_(*cancel == FALSE)) _Inout_opt_ LPBOOL cancel,
    _In_ DWORD copyFlags) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            // See note in CopyFileFixup for commentary on copy-on-read policy
            auto [redirectSource, sourceRedirectPath] = ShouldRedirect(existingFileName, redirect_flags::check_file_presence);
            auto [redirectDest, destRedirectPath] = ShouldRedirect(newFileName, redirect_flags::ensure_directory_structure);
            if (redirectSource || redirectDest)
            {
                return impl::CopyFileEx(
                    redirectSource ? sourceRedirectPath.c_str() : widen_argument(existingFileName).c_str(),
                    redirectDest ? destRedirectPath.c_str() : widen_argument(newFileName).c_str(),
                    progressRoutine,
                    data,
                    cancel,
                    copyFlags);
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::CopyFileEx(existingFileName, newFileName, progressRoutine, data, cancel, copyFlags);
}
DECLARE_STRING_FIXUP(impl::CopyFileEx, CopyFileExFixup);

HRESULT __stdcall CopyFile2Fixup(
    _In_ PCWSTR existingFileName,
    _In_ PCWSTR newFileName,
    _In_opt_ COPYFILE2_EXTENDED_PARAMETERS* extendedParameters) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            // See note in CopyFileFixup for commentary on copy-on-read policy
            auto [redirectSource, sourceRedirectPath] = ShouldRedirect(existingFileName, redirect_flags::check_file_presence);
            auto [redirectDest, destRedirectPath] = ShouldRedirect(newFileName, redirect_flags::ensure_directory_structure);
            if (redirectSource || redirectDest)
            {
                return impl::CopyFile2(
                    redirectSource ? sourceRedirectPath.c_str() : existingFileName,
                    redirectDest ? destRedirectPath.c_str() : newFileName,
                    extendedParameters);
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::CopyFile2(existingFileName, newFileName, extendedParameters);
}
DECLARE_FIXUP(impl::CopyFile2, CopyFile2Fixup);
