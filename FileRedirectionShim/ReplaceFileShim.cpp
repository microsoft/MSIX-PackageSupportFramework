//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"

template <typename CharT>
BOOL __stdcall ReplaceFileShim(
    _In_ const CharT* replacedFileName,
    _In_ const CharT* replacementFileName,
    _In_opt_ const CharT* backupFileName,
    _In_ DWORD replaceFlags,
    _Reserved_ LPVOID exclude,
    _Reserved_ LPVOID reserved) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            // NOTE: ReplaceFile will delete the "replacement file" (the file we're copying from), so therefore we need
            //       delete access to it, thus we copy-on-read it here. I.e. we're copying the file only for it to
            //       immediately get deleted. We could improve this in the future if we wanted, but that would
            //       effectively require that we re-write ReplaceFile, which we opt not to do right now. Also note that
            //       this implies that we have the same file deletion limitation that we have for DeleteFile, etc.
            auto [redirectTarget, targetRedirectPath] = ShouldRedirect(replacedFileName, redirect_flags::copy_on_read);
            auto [redirectSource, sourceRedirectPath] = ShouldRedirect(replacementFileName, redirect_flags::copy_on_read);
            auto [redirectBackup, backupRedirectPath] = ShouldRedirect(backupFileName, redirect_flags::ensure_directory_structure);
            if (redirectTarget || redirectSource || redirectBackup)
            {
                return impl::ReplaceFile(
                    redirectTarget ? targetRedirectPath.c_str() : widen_argument(replacedFileName).c_str(),
                    redirectSource ? sourceRedirectPath.c_str() : widen_argument(replacementFileName).c_str(),
                    redirectBackup ? backupRedirectPath.c_str() : widen_argument(backupFileName).c_str(),
                    replaceFlags,
                    exclude,
                    reserved);
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::ReplaceFile(replacedFileName, replacementFileName, backupFileName, replaceFlags, exclude, reserved);
}
DECLARE_STRING_SHIM(impl::ReplaceFile, ReplaceFileShim);
