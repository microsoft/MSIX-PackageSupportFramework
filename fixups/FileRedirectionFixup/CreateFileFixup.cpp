//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"

/// ConvertToReadOnlyAccess: Modify a file operation call if it requests write access to one without write access.
DWORD inline ConvertToReadOnlyAccess(DWORD desiredAccess)
{
    DWORD redirectedAccess = desiredAccess;
    if ((desiredAccess & GENERIC_ALL) != 0)
    {
        redirectedAccess &= ~GENERIC_ALL;
    }
    if ((desiredAccess & GENERIC_WRITE) != 0)
    {
        redirectedAccess &= ~GENERIC_WRITE;
    }
    if ((desiredAccess & (GENERIC_ALL | GENERIC_WRITE)) != 0 &&
        (desiredAccess & ~(GENERIC_ALL | GENERIC_WRITE)) == 0)
    {
        redirectedAccess |= GENERIC_READ;
    }
    if ((desiredAccess & FILE_GENERIC_WRITE) != 0)
    {
        redirectedAccess &= ~FILE_GENERIC_WRITE;
        redirectedAccess |= FILE_GENERIC_READ;
    }
    if ((desiredAccess & FILE_WRITE_DATA) != 0)
    {
        redirectedAccess &= ~FILE_WRITE_DATA;
    }
    if ((desiredAccess & FILE_WRITE_ATTRIBUTES) != 0)
    {
        redirectedAccess &= FILE_WRITE_ATTRIBUTES;
    }
    if ((desiredAccess & FILE_WRITE_EA) != 0)
    {
        redirectedAccess &= FILE_WRITE_EA;
    }
    return redirectedAccess;
}


template <typename CharT>
HANDLE __stdcall CreateFileFixup(
    _In_ const CharT* fileName,
    _In_ DWORD desiredAccess,
    _In_ DWORD shareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES securityAttributes,
    _In_ DWORD creationDisposition,
    _In_ DWORD flagsAndAttributes,
    _In_opt_ HANDLE templateFile) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            Log(L"CreateFileFixup for %ls", fileName);
            // FUTURE: If 'creationDisposition' is something like 'CREATE_ALWAYS', we could get away with something
            //         cheaper than copy-on-read, but we'd also need to be mindful of ensuring the correct error if so
            auto[shouldRedirect, redirectPath, shouldReadonly] = ShouldRedirect(fileName, redirect_flags::copy_on_read);
            if (shouldRedirect)
            {
                if (IsUnderUserAppDataLocal(fileName))
                {
                    // special case.  Need to do the copy ourselves if present in the package as MSIX Runtime doesn't take care of these cases.
                    std::filesystem::path PackageVersion = GetPackageVFSPath(fileName);
                    if (wcslen(PackageVersion.c_str()) >= 0)
                    {
                        if (impl::PathExists(PackageVersion.c_str()))
                        {
                            if (!impl::PathExists(redirectPath.c_str()))
                            {
                                // Need to copy now
                                Log(L"\tFRF CreateFile COA from ADL to %ls", redirectPath.c_str());
                                impl::CopyFileW(PackageVersion.c_str(), redirectPath.c_str(), true);
                            }
                        }
                    }
                }
                else if (IsUnderUserAppDataRoaming(fileName))
                {
                    // special case.  Need to do the copy ourselves if present in the package as MSIX Runtime doesn't take care of these cases.
                    std::filesystem::path PackageVersion = GetPackageVFSPath(fileName);
                    if (wcslen(PackageVersion.c_str()) >= 0)
                    {
                        if (impl::PathExists(PackageVersion.c_str()))
                        {
                            if (!impl::PathExists(redirectPath.c_str()))
                            {
                                // Need to copy now
                                Log(L"\tFRF CreateFile COA from ADR to %ls", redirectPath.c_str());
                                impl::CopyFileW(PackageVersion.c_str(), redirectPath.c_str(), true);
                            }
                        }
                    }
                }
            
                DWORD redirectedAccess = desiredAccess;
                if (shouldReadonly)
                {
                    redirectedAccess = ConvertToReadOnlyAccess(desiredAccess);
                }

                return impl::CreateFile(redirectPath.c_str(), desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile);
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::CreateFile(fileName, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile);
}
DECLARE_STRING_FIXUP(impl::CreateFile, CreateFileFixup);

HANDLE __stdcall CreateFile2Fixup(
    _In_ LPCWSTR fileName,
    _In_ DWORD desiredAccess,
    _In_ DWORD shareMode,
    _In_ DWORD creationDisposition,
    _In_opt_ LPCREATEFILE2_EXTENDED_PARAMETERS createExParams) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            Log(L"CreateFile2Fixup for %ls", fileName);
            // FUTURE: See comment in CreateFileFixup about using 'creationDisposition' to choose a potentially better
            //         redirect flags value
            auto[shouldRedirect, redirectPath, shouldReadonly] = ShouldRedirect(fileName, redirect_flags::copy_on_read);
            if (shouldRedirect)
            {
                if (IsUnderUserAppDataLocal(fileName))
                {
                    // special case.  Need to do the copy ourselves if present in the package as MSIX Runtime doesn't take care of these cases.
                    std::filesystem::path PackageVersion = GetPackageVFSPath(fileName);
                    if (wcslen(PackageVersion.c_str()) >= 0)
                    {
                        if (impl::PathExists(PackageVersion.c_str()))
                        {
                            if (!impl::PathExists(redirectPath.c_str()))
                            {
                                // Need to copy now
                                Log(L"\tFRF CreateFile2 COA from ADL to %ls", redirectPath.c_str());
                                impl::CopyFileW(PackageVersion.c_str(), redirectPath.c_str(), true);
                            }
                        }
                    }
                }
                else if (IsUnderUserAppDataRoaming(fileName))
                {
                    // special case.  Need to do the copy ourselves if present in the package as MSIX Runtime doesn't take care of these cases.
                    std::filesystem::path PackageVersion = GetPackageVFSPath(fileName);
                    if (wcslen(PackageVersion.c_str()) >= 0)
                    {
                        if (impl::PathExists(PackageVersion.c_str()))
                        {
                            if (!impl::PathExists(redirectPath.c_str()))
                            {
                                // Need to copy now
                                Log(L"\tFRF CreateFile2 COA from ADR to %ls", redirectPath.c_str());
                                impl::CopyFileW(PackageVersion.c_str(), redirectPath.c_str(), true);
                            }
                        }
                    }
                }
                DWORD redirectedAccess = desiredAccess;
                if (shouldReadonly)
                {
                    redirectedAccess = ConvertToReadOnlyAccess(desiredAccess);
                }
                
                return impl::CreateFile2(redirectPath.c_str(), desiredAccess, shareMode, creationDisposition, createExParams);
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::CreateFile2(fileName, desiredAccess, shareMode, creationDisposition, createExParams);
}
DECLARE_FIXUP(impl::CreateFile2, CreateFile2Fixup);
