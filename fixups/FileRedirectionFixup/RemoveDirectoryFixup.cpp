//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"

template <typename CharT>
BOOL __stdcall RemoveDirectoryFixup(_In_ const CharT* pathName) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            if constexpr (psf::is_ansi<CharT>)
            {
                Log("RemoveDirectoryFixup for %s", pathName);
            }
            else
            {
                Log(L"RemoveDirectoryFixup for %ls", pathName);
            }

            // NOTE: See commentary in DeleteFileFixup for limitations on deleting files/directories
            auto [shouldRedirect, redirectPath, shouldReadonly] = ShouldRedirect(pathName, redirect_flags::none);
            if (shouldRedirect)
            {
                if (!impl::PathExists(redirectPath.c_str()) && impl::PathExists(pathName))
                {
                    // If the directory does not exist in the redirected location, but does in the non-redirected
                    // location, then we want to give the "illusion" that the delete succeeded
                    return TRUE;
                }
                else
                {
                    return impl::RemoveDirectory(redirectPath.c_str());
                }
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::RemoveDirectory(pathName);
}
DECLARE_STRING_FIXUP(impl::RemoveDirectory, RemoveDirectoryFixup);
