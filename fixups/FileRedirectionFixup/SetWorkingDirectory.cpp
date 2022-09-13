//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"


template <typename CharT>
BOOL __stdcall SetCurrentDirectoryFixup(_In_ const CharT* filePath) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            DWORD SetWorkingDirectoryInstance = ++g_FileIntceptInstance;
            LogString(SetWorkingDirectoryInstance, L"SetCurrentDirectoryFixup ", filePath);
            if (!path_relative_to(filePath, psf::current_package_path()))
            {
                normalized_path normalized = NormalizePath(filePath);
                normalized_path virtualized = VirtualizePath(normalized, SetWorkingDirectoryInstance);
                if (impl::PathExists(virtualized.full_path.c_str()))
                {
                    return impl::SetCurrentDirectory(virtualized.full_path.c_str());
                }
                else
                {
                    // Fall through to original call
                }
            }
            else
            {
                // Fall through to original call
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::SetCurrentDirectory(filePath);
}
DECLARE_STRING_FIXUP(impl::SetCurrentDirectory, SetCurrentDirectoryFixup);
