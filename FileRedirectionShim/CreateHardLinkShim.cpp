//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"

template <typename CharT>
BOOL __stdcall CreateHardLinkShim(
    _In_ const CharT* fileName,
    _In_ const CharT* existingFileName,
    _Reserved_ LPSECURITY_ATTRIBUTES securityAttributes) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            // NOTE: We need to copy-on-read the existing file since the application may want to open the hard-link file
            //       for write in the future. As for the link file, we currently _don't_ copy-on-read it due to the fact
            //       that we don't handle file deletions as robustly as we could and CreateHardLink will fail if the
            //       link file already exists. I.e. we're giving the application the benefit of the doubt that, if they
            //       are trying to create a hard-link with the same path as a file inside the package, they had
            //       previously attempted to delete that file.
            auto [redirectLink, redirectPath] = ShouldRedirect(fileName, redirect_flags::ensure_directory_structure);
            auto [redirectTarget, redirectTargetPath] = ShouldRedirect(existingFileName, redirect_flags::copy_on_read);
            if (redirectLink || redirectTarget)
            {
                return impl::CreateHardLink(
                    redirectLink ? redirectPath.c_str() : widen_argument(fileName).c_str(),
                    redirectTarget ? redirectTargetPath.c_str() : widen_argument(existingFileName).c_str(),
                    securityAttributes);
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::CreateHardLink(fileName, existingFileName, securityAttributes);
}
DECLARE_STRING_SHIM(impl::CreateHardLink, CreateHardLinkShim);
