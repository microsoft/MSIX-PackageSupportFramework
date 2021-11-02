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

// We are seeing apps adding in an extra backslash when performing certain file operations.
// The cause seems like it must be the FRF, but we can simply fix it here like this.
std::string RemoveAnyFinalDoubleSlash(std::string input)
{
    size_t off = input.rfind("\\\\");
    if (off != std::wstring::npos &&
        off > 4)
    {
        std::string output = input.substr(0, off);
        output.append(input.substr(off + 1));
        return output;
    }
    else
        return input;
}
std::wstring RemoveAnyFinalDoubleSlash(std::wstring input)
{
    size_t off = input.rfind(L"\\\\");
    if (off != std::wstring::npos &&
        off > 4)
    {
        std::wstring output = input.substr(0, off);
        output.append(input.substr(off + 1));
        return output;
    }
    else
            return input;
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
    DWORD CreateFileInstance = ++g_FileIntceptInstance;
    
    std::string FileNameString;
    std::wstring WFileNameString;
    const CharT* FixedFileName = fileName;
    try
    {
        if (guard)
        {
            if constexpr (psf::is_ansi<CharT>)
            {
                FileNameString = fileName;
                FileNameString = RemoveAnyFinalDoubleSlash(FileNameString);
                FixedFileName = FileNameString.c_str();
                LogString(CreateFileInstance, L"CreateFileFixup A for fileName", widen(fileName).c_str()); 
                //LogString(CreateFileInstance, L"DEBUG: CreateFileFixup FixedfileName", FixedFileName);
            }
            else
            {
                WFileNameString = fileName;
                WFileNameString = RemoveAnyFinalDoubleSlash(WFileNameString);
                FixedFileName = WFileNameString.c_str();
                LogString(CreateFileInstance, L"CreateFileFixup W for fileName", fileName);
                //LogString(CreateFileInstance, L"DEBUG: CreateFileFixup FixedfileName", FixedFileName);
            }
            
            if (!IsUnderUserAppDataLocalPackages(FixedFileName))
            {
                // FUTURE: If 'creationDisposition' is something like 'CREATE_ALWAYS', we could get away with something
                //         cheaper than copy-on-read, but we'd also need to be mindful of ensuring the correct error if so
                auto [shouldRedirect, redirectPath, shouldReadonly] = ShouldRedirect(FixedFileName, redirect_flags::copy_on_read, CreateFileInstance);
                if (shouldRedirect)
                {
                    if (IsUnderUserAppDataLocal(FixedFileName))
                    {
                        Log(L"[%d]Under LocalAppData", CreateFileInstance);
                        // special case.  Need to do the copy ourselves if present in the package as MSIX Runtime doesn't take care of these cases.
                        std::filesystem::path PackageVersion = GetPackageVFSPath(FixedFileName);
                        if (wcslen(PackageVersion.c_str()) >= 0)
                        {
                            if (impl::PathExists(PackageVersion.c_str()))
                            {
                                if (!impl::PathExists(redirectPath.c_str()))
                                {
                                    // Need to copy now
                                    LogString(CreateFileInstance, L"\tFRF CreateFile COA from ADL to", redirectPath.c_str());
                                    impl::CopyFileW(PackageVersion.c_str(), redirectPath.c_str(), true);
                                }
                            }
                        }
                    }
                    else if (IsUnderUserAppDataRoaming(FixedFileName))
                    {
                        Log(L"[%d]Under AppData(roaming)", CreateFileInstance);
                        // special case.  Need to do the copy ourselves if present in the package as MSIX Runtime doesn't take care of these cases.
                        std::filesystem::path PackageVersion = GetPackageVFSPath(FixedFileName);
                        if (wcslen(PackageVersion.c_str()) >= 0)
                        {
                            if (impl::PathExists(PackageVersion.c_str()))
                            {
                                if (!impl::PathExists(redirectPath.c_str()))
                                {
                                    // Need to copy now
                                    LogString(CreateFileInstance, L"\tFRF CreateFile COA from ADR to", redirectPath.c_str());
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
                    HANDLE hRet = impl::CreateFile(redirectPath.c_str(), redirectedAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile);
                    if (hRet == INVALID_HANDLE_VALUE)
                    {
                        Log(L"[%d]CreateFile redirected uses %ls. FAILURE.", CreateFileInstance, redirectPath.c_str());
                        // Fall back to original request
                    }
                    else
                    {
                        Log(L"[%d]CreateFile redirected uses %ls. SUCCESS.", CreateFileInstance, redirectPath.c_str());
                        return hRet;
                    }
                }
            }
            else
            {
                Log(L"[%d]Under LocalAppData\\Packages, don't redirect", CreateFileInstance);
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    // In the spirit of app compatability, make the path long formed just in case.
    if constexpr (psf::is_ansi<CharT>)
    {
        std::string sFileName = TurnPathIntoRootLocalDevice(FixedFileName);
        HANDLE hRes = impl::CreateFile(sFileName.c_str(), desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile);
        if (hRes == INVALID_HANDLE_VALUE)
            LogString(CreateFileInstance, L"CreateFileFixup fall through call returns FAILURE.", sFileName.c_str());
        else
            LogString(CreateFileInstance, L"CreateFileFixup fall through call returns SUCCESS.", sFileName.c_str());
        return hRes;
    }
    else
    {
        std::wstring wFileName = TurnPathIntoRootLocalDevice(FixedFileName);
        HANDLE hRes = impl::CreateFile(wFileName.c_str(), desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile);
        if (hRes == INVALID_HANDLE_VALUE)
            LogString(CreateFileInstance,L"CreateFileFixup fall through call returns FAILURE.", wFileName.c_str());
        else
            LogString(CreateFileInstance,L"CreateFileFixup fall through call returns SUCCESS.", wFileName.c_str());
        return hRes;
    }
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
    DWORD CreateFile2Instance = ++g_FileIntceptInstance;
    std::wstring WFileNameString = fileName;
    try
    {
        if (guard)
        {
            WFileNameString = RemoveAnyFinalDoubleSlash(WFileNameString);

            ///Log(L"[%d]CreateFile2Fixup for %ls", CreateFile2Instance, widen(fileName, CP_ACP).c_str());
            //Log(L"[%d]CreateFile2Fixup for %ls", CreateFile2Instance, fileName);
            LogString(CreateFile2Instance,L"CreateFile2Fixup for ", fileName);

            if (!IsUnderUserAppDataLocalPackages(WFileNameString.c_str()))
            {
                // FUTURE: See comment in CreateFileFixup about using 'creationDisposition' to choose a potentially better
            //         redirect flags value
                auto [shouldRedirect, redirectPath, shouldReadonly] = ShouldRedirect(WFileNameString.c_str(), redirect_flags::copy_on_read, CreateFile2Instance);
                if (shouldRedirect)
                {
                    if (IsUnderUserAppDataLocal(WFileNameString.c_str()))
                    {
                        Log(L"[%d]Under LocalAppData", CreateFile2Instance);
                        // special case.  Need to do the copy ourselves if present in the package as MSIX Runtime doesn't take care of these cases.
                        std::filesystem::path PackageVersion = GetPackageVFSPath(fileName);
                        if (wcslen(PackageVersion.c_str()) >= 0)
                        {
                            if (impl::PathExists(PackageVersion.c_str()))
                            {
                                if (!impl::PathExists(redirectPath.c_str()))
                                {
                                    // Need to copy now
                                    LogString(CreateFile2Instance, L"\tFRF CreateFile2 COA from ADL to", redirectPath.c_str());
                                    impl::CopyFileW(PackageVersion.c_str(), redirectPath.c_str(), true);
                                }
                            }
                        }
                    }
                    else if (IsUnderUserAppDataRoaming(WFileNameString.c_str()))
                    {
                        Log(L"[%d]Under AppData(roaming)", CreateFile2Instance);
                        // special case.  Need to do the copy ourselves if present in the package as MSIX Runtime doesn't take care of these cases.
                        std::filesystem::path PackageVersion = GetPackageVFSPath(fileName);
                        if (wcslen(PackageVersion.c_str()) >= 0)
                        {
                            if (impl::PathExists(PackageVersion.c_str()))
                            {
                                if (!impl::PathExists(redirectPath.c_str()))
                                {
                                    // Need to copy now
                                    LogString(CreateFile2Instance, L"\tFRF CreateFile2 COA from ADR to", redirectPath.c_str());
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

                    HANDLE hRet = impl::CreateFile2(redirectPath.c_str(), desiredAccess, shareMode, creationDisposition, createExParams);
                    if (hRet == INVALID_HANDLE_VALUE)
                    {
                        Log(L"[%d]CreateFile2 redirected uses %ls. FAILURE.", CreateFile2Instance, redirectPath.c_str());
                        // Fall back to original request
                    }
                    else
                    {
                        Log(L"[%d]CreateFile2 redirected uses %ls. SUCCESS.", CreateFile2Instance, redirectPath.c_str());
                        return hRet;
                    }
                }
            }
            else
            {
                Log(L"[%d]Under LocalAppData\\Packages, don't redirect", CreateFile2Instance);
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    // In the spirit of app compatability, make the path long formed just in case.
    std::wstring wFileName = TurnPathIntoRootLocalDevice(WFileNameString.c_str());
    /////return impl::CreateFile2(fileName, desiredAccess, shareMode, creationDisposition, createExParams);
    HANDLE hRet2= impl::CreateFile2(wFileName.c_str(), desiredAccess, shareMode, creationDisposition, createExParams);
    if (hRet2 == INVALID_HANDLE_VALUE)
    {
        LogString(CreateFile2Instance, L"CreateFile2 fallthrough FAILURE.",  wFileName.c_str());
        // Fall back to original request
    }
    else
    {
        LogString(CreateFile2Instance, L"CreateFile2 fallthrough SUCCESS.",  wFileName.c_str());
    }
    return hRet2;
}
DECLARE_FIXUP(impl::CreateFile2, CreateFile2Fixup);
