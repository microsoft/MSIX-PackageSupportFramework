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
            std::wstring wfileName = widen(fileName);
            LogString(GetFileAttributesInstance,L"GetFileAttributesFixup for fileName", wfileName.c_str());

            if (!IsUnderUserAppDataLocalPackages(wfileName.c_str()))
            {
                auto [shouldRedirect, redirectPath, shouldReadonly] = ShouldRedirect(wfileName.c_str(), redirect_flags::check_file_presence, GetFileAttributesInstance);
                if (shouldRedirect)
                {
                    Log(L"[%d]GetFileAttributes: Should Redirect says yes.", GetFileAttributesInstance);
                    DWORD attributes = impl::GetFileAttributes(redirectPath.c_str());
                    if (attributes == INVALID_FILE_ATTRIBUTES)
                    {
                        // Might be file/dir has not been copied yet, but might also be funky ADL/ADR.
                        if (IsUnderUserAppDataLocal(wfileName.c_str()) ||
                            IsUnderUserAppDataRoaming(wfileName.c_str()))
                        {
                            // special case.  Need to do the copy ourselves if present in the package as MSIX Runtime doesn't take care of these cases.
                            std::filesystem::path PackageVersion = GetPackageVFSPath(wfileName.c_str());
                            if (wcslen(PackageVersion.c_str()) > 0)
                            {
                                Log(L"[%d]GetFileAttributes: uncopied ADL/ADR case %ls", GetFileAttributesInstance,PackageVersion.c_str());
                                attributes = impl::GetFileAttributes(PackageVersion.c_str());
                                if (attributes == INVALID_FILE_ATTRIBUTES)
                                {
                                    Log(L"[%d]GetFileAttributes: fall back to original request location.", GetFileAttributesInstance);
                                    attributes = impl::GetFileAttributesW(wfileName.c_str());
                                }
                            }
                        }
                        else
                        {
                            Log(L"[%d]GetFileAttributes: other not yet redirected case", GetFileAttributesInstance);
                            attributes = impl::GetFileAttributesW(wfileName.c_str());
                        }
                    }
                    else if (attributes != INVALID_FILE_ATTRIBUTES)
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
                    Log(L"[%d]GetFileAttributes: ShouldRedirect att=0x%x", GetFileAttributesInstance, attributes);
                    return attributes;
                }
                else
                {
                    Log(L"[%d]GetFileAttributes: No Redirect, make original call ", GetFileAttributesInstance);
                }
            }
            else
            {
                Log(L"[%d]GetFileAttributes: Under LocalAppData\\Packages, don't redirect, make original call", GetFileAttributesInstance);
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
        Log(L"GetFileAttributes: *** Exception ***");
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
            std::wstring wfileName = widen(fileName);
            LogString(GetFileAttributesExInstance,L"GetFileAttributesExFixup for fileName", wfileName.c_str());

            if (!IsUnderUserAppDataLocalPackages(fileName))
            {
                auto [shouldRedirect, redirectPath, shouldReadonly] = ShouldRedirect(wfileName.c_str(), redirect_flags::check_file_presence, GetFileAttributesExInstance);
                if (shouldRedirect)
                {
                    BOOL retval = impl::GetFileAttributesExW(redirectPath.c_str(), infoLevelId, fileInformation);
                    if (retval == 0)
                    {
                        // We know it exists, so must be file/dir has not been copied yet.
                        if (IsUnderUserAppDataLocal(wfileName.c_str()) ||
                            IsUnderUserAppDataRoaming(wfileName.c_str()))
                        {
                            // special case.  Need to do the copy ourselves if present in the package as MSIX Runtime doesn't take care of these cases.
                            std::filesystem::path PackageVersion = GetPackageVFSPath(wfileName.c_str());
                            if (wcslen(PackageVersion.c_str()) > 0)
                            {
                                Log(L"[%d]GetFileAttributesEx: uncopied ADL/ADR case %ls", GetFileAttributesExInstance,PackageVersion.c_str());
                                retval = impl::GetFileAttributesExW(PackageVersion.c_str(), infoLevelId, fileInformation);
                                if (retval == 0)
                                {
                                    Log(L"[%d]GetFileAttributesEx: fall back to original location.", GetFileAttributesExInstance);
                                    retval = impl::GetFileAttributesExW(wfileName.c_str(), infoLevelId, fileInformation);
                                }
                            }
                        }
                        else
                        {
                            Log(L"[%d]GetFileAttributesEx: uncopied other case", GetFileAttributesExInstance);
                            retval = impl::GetFileAttributesExW(wfileName.c_str(), infoLevelId, fileInformation);
                        }
                    }
                    else if (retval != 0)
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
                        Log(L"[%d]GetFileAttributesEx: ShouldRedirect retval=%d att=%d", GetFileAttributesExInstance, retval, ((WIN32_FILE_ATTRIBUTE_DATA*)fileInformation)->dwFileAttributes);
                    }
                    else
                    {
                        Log(L"[%d]GetFileAttributesEx: ShouldRedirect retval=%d", GetFileAttributesExInstance, retval);
                    }
                    return retval;
                }
            }
            else
            {
                Log(L"[%d]Under LocalAppData\\Packages, don't redirect", GetFileAttributesExInstance);
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
            std::wstring wfileName = widen(fileName);
            LogString(SetFileAttributesInstance,L"SetFileAttributesFixup for fileName", wfileName.c_str());

            if (!IsUnderUserAppDataLocalPackages(fileName))
            {
                auto [shouldRedirect, redirectPath, shouldReadonly] = ShouldRedirect(wfileName.c_str(), redirect_flags::copy_on_read, SetFileAttributesInstance);
                if (shouldRedirect)
                {
                    DWORD redirectedAttributes = fileAttributes;
                    if (shouldReadonly)
                    {
                        if ((fileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                            redirectedAttributes |= FILE_ATTRIBUTE_READONLY;
                    }
                    Log(L"[%d] Setting on redirected Equivalent with 0x%x", SetFileAttributesInstance, redirectedAttributes);
                    std::wstring rldRedirectPath = TurnPathIntoRootLocalDevice(widen_argument(redirectPath.c_str()).c_str());
                    return impl::SetFileAttributesW(rldRedirectPath.c_str(), redirectedAttributes);
                }
            }
            else
            {
                Log(L"[%d]Under LocalAppData\\Packages, don't redirect", SetFileAttributesInstance);
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    std::wstring rldFileName = TurnPathIntoRootLocalDevice(widen_argument(fileName).c_str());
    return impl::SetFileAttributes(rldFileName.c_str(), fileAttributes);
}
DECLARE_STRING_FIXUP(impl::SetFileAttributes, SetFileAttributesFixup);
