//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"

template <typename CharT>
DWORD __stdcall GetFileAttributesFixup(_In_ const CharT* fileName) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            auto [shouldRedirect, redirectPath, shouldReadonly] = ShouldRedirect(fileName, redirect_flags::check_file_presence);
            if (shouldRedirect)
            {
                DWORD attributes =  impl::GetFileAttributes(redirectPath.c_str());
                if (shouldReadonly)
                {
                    if ((attributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                    attributes |= FILE_ATTRIBUTE_READONLY;
                }
                return attributes;
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::GetFileAttributes(fileName);
}
DECLARE_STRING_FIXUP(impl::GetFileAttributes, GetFileAttributesFixup);

template <typename CharT>
BOOL __stdcall GetFileAttributesExFixup(
    _In_ const CharT* fileName,
    _In_ GET_FILEEX_INFO_LEVELS infoLevelId,
    _Out_writes_bytes_(sizeof(WIN32_FILE_ATTRIBUTE_DATA)) LPVOID fileInformation) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            auto [shouldRedirect, redirectPath, shouldReadonly] = ShouldRedirect(fileName, redirect_flags::check_file_presence);
            if (shouldRedirect)
            {
                BOOL retval = impl::GetFileAttributesEx(redirectPath.c_str(), infoLevelId, fileInformation);
                if (retval != 0 && shouldReadonly)
                {
                    if (infoLevelId == GetFileExInfoStandard  )
                    {
                        if ((((WIN32_FILE_ATTRIBUTE_DATA*)fileInformation)->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                            ((WIN32_FILE_ATTRIBUTE_DATA*)fileInformation)->dwFileAttributes |= FILE_ATTRIBUTE_READONLY;
                    }
                }
                return retval;
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::GetFileAttributesEx(fileName, infoLevelId, fileInformation);
}
DECLARE_STRING_FIXUP(impl::GetFileAttributesEx, GetFileAttributesExFixup);

template <typename CharT>
BOOL __stdcall SetFileAttributesFixup(_In_ const CharT* fileName, _In_ DWORD fileAttributes) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            auto [shouldRedirect, redirectPath, shouldReadonly] = ShouldRedirect(fileName, redirect_flags::copy_on_read);
            if (shouldRedirect)
            {
                DWORD redirectedAttributes = fileAttributes;
                if (shouldReadonly)
                {
                    if ((fileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                        redirectedAttributes |= FILE_ATTRIBUTE_READONLY;
                }
                return impl::SetFileAttributes(redirectPath.c_str(), redirectedAttributes);
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::SetFileAttributes(fileName, fileAttributes);
}
DECLARE_STRING_FIXUP(impl::SetFileAttributes, SetFileAttributesFixup);
