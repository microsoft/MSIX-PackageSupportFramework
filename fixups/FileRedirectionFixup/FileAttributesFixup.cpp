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
            DWORD GetFileAttributesInstance = ++g_FileIntceptInstance;
            LogString(GetFileAttributesInstance,L"GetFileAttributesFixup for fileName", fileName);

            auto[shouldRedirect, redirectPath, shouldReadonly] = ShouldRedirect(fileName, redirect_flags::check_file_presence, GetFileAttributesInstance);
            if (shouldRedirect)
            {
                DWORD attributes = impl::GetFileAttributes(redirectPath.c_str());
                if (attributes == INVALID_FILE_ATTRIBUTES)
                {
                    // Might be file/dir has not been copied yet, but might also be funky ADL/ADR.
                    if (IsUnderUserAppDataLocal(fileName) ||
                        IsUnderUserAppDataRoaming(fileName))
                    {
                        // special case.  Need to do the copy ourselves if present in the package as MSIX Runtime doesn't take care of these cases.
                        std::filesystem::path PackageVersion = GetPackageVFSPath(fileName);
                        if (wcslen(PackageVersion.c_str()) >= 0)
                        {
                            Log(L"[%d]GetFileAttributes: uncopied ADL/ADR case", GetFileAttributesInstance);
                            attributes = impl::GetFileAttributes(PackageVersion.c_str());
                            if (attributes != INVALID_FILE_ATTRIBUTES)
                            {
                                Log(L"[%d]GetFileAttributes: fall back to original request location.", GetFileAttributesInstance);
                                attributes = impl::GetFileAttributes(fileName);
                            }
                        }
                    }
                    else
                    {
                        Log(L"[%d]GetFileAttributes: other not yet redirected case", GetFileAttributesInstance);
                        attributes = impl::GetFileAttributes(fileName);
                    }
                }
                if (attributes != INVALID_FILE_ATTRIBUTES)
                {
                    if (shouldReadonly)
                    {
                        if ((attributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                            attributes |= FILE_ATTRIBUTE_READONLY;
                    }
                    else
                    {
                        attributes &= ~FILE_ATTRIBUTE_READONLY;
                    }
                }
                Log(L"[%d]GetFileAttributes: ShouldRedirect att=%d", GetFileAttributesInstance,attributes);
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
            DWORD GetFileAttributesExInstance = ++g_FileIntceptInstance;
            LogString(GetFileAttributesExInstance,L"GetFileAttributesExFixup for fileName", fileName);

            auto [shouldRedirect, redirectPath, shouldReadonly] = ShouldRedirect(fileName, redirect_flags::check_file_presence, GetFileAttributesExInstance);
            if (shouldRedirect)
            {
                BOOL retval = impl::GetFileAttributesEx(redirectPath.c_str(), infoLevelId, fileInformation);
                if (retval == 0)
                {
                    // We know it exists, so must be file/dir has not been copied yet.
                    if (IsUnderUserAppDataLocal(fileName) ||
                        IsUnderUserAppDataRoaming(fileName))
                    {
                        // special case.  Need to do the copy ourselves if present in the package as MSIX Runtime doesn't take care of these cases.
                        std::filesystem::path PackageVersion = GetPackageVFSPath(fileName);
                        if (wcslen(PackageVersion.c_str()) >= 0)
                        {
                            Log(L"[%d]GetFileAttributesEx: uncopied ADL/ADR case", GetFileAttributesExInstance);
                            retval = impl::GetFileAttributesEx(PackageVersion.c_str(), infoLevelId, fileInformation);
                            if (retval == 0)
                            {
                                Log(L"[%d]GetFileAttributesEx: fall back to original location.", GetFileAttributesExInstance);
                                retval = impl::GetFileAttributesEx(fileName, infoLevelId, fileInformation);
                            }
                        }
                    }
                    else
                    {
                        Log(L"[%d]GetFileAttributesEx: uncopied other case", GetFileAttributesExInstance);
                        retval = impl::GetFileAttributesEx(fileName, infoLevelId, fileInformation);
                    }
                }
                if (retval != 0)
                {
                    if (shouldReadonly)
                    {
                        if (infoLevelId == GetFileExInfoStandard)
                        {
                            if ((((WIN32_FILE_ATTRIBUTE_DATA*)fileInformation)->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                                ((WIN32_FILE_ATTRIBUTE_DATA*)fileInformation)->dwFileAttributes |= FILE_ATTRIBUTE_READONLY;
                        }
                    }
                    else
                    {
                        if (infoLevelId == GetFileExInfoStandard)
                        {
                            ((WIN32_FILE_ATTRIBUTE_DATA*)fileInformation)->dwFileAttributes &= ~FILE_ATTRIBUTE_READONLY;
                        }
                    }
                }
                if (retval != 0)
                {
                    Log(L"[%d]GetFileAttributesEx: ShouldRedirect retval=%d att=%d", GetFileAttributesExInstance,retval, ((WIN32_FILE_ATTRIBUTE_DATA*)fileInformation)->dwFileAttributes);
                }
                else
                {
                    Log(L"[%d]GetFileAttributesEx: ShouldRedirect retval=%d", GetFileAttributesExInstance,retval);
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
            DWORD SetFileAttributesInstance = ++g_FileIntceptInstance;
            LogString(SetFileAttributesInstance,L"SetFileAttributesFixup for fileName", fileName);

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
