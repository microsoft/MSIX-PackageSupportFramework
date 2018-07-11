//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"

template <typename CharT>
BOOL __stdcall DeleteFileShim(_In_ const CharT* fileName) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            // NOTE: This will only delete the redirected file. If the file previously existed in the package path, then
            //       it will remain there and a later attempt to open, etc. the file will succeed. In the future, if
            //       this proves to be an issue, we could maintain a collection of package files that have been
            //       "deleted" and then just pretend like they've been deleted. Such a change would be rather large and
            //       disruptful and probably fairly inefficient as it would impact virtually every code path, so we'll
            //       put it off for now.
            auto [shouldRedirect, redirectPath] = ShouldRedirect(fileName, redirect_flags::none);
            if (shouldRedirect)
            {
                if (!impl::PathExists(redirectPath.c_str()) && impl::PathExists(fileName))
                {
                    // If the file does not exist in the redirected location, but does in the non-redirected location,
                    // then we want to give the "illusion" that the delete succeeded
                    return TRUE;
                }
                else
                {
                    return impl::DeleteFile(redirectPath.c_str());
                }
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::DeleteFile(fileName);
}
DECLARE_STRING_SHIM(impl::DeleteFile, DeleteFileShim);
