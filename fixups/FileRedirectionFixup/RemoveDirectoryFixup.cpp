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
            DWORD RemoveDirectoryInstance = ++g_FileIntceptInstance;
            std::wstring wPathName = widen(pathName);
            LogString(RemoveDirectoryInstance,L"RemoveDirectoryFixup for pathName", wPathName.c_str());
            
            if (!IsUnderUserAppDataLocalPackages(wPathName.c_str()))
            {
                // NOTE: See commentary in DeleteFileFixup for limitations on deleting files/directories
                auto [shouldRedirect, redirectPath, shouldReadonly] = ShouldRedirect(wPathName.c_str(), redirect_flags::none);
                if (shouldRedirect)
                {
                    std::wstring rldPathName = TurnPathIntoRootLocalDevice(wPathName.c_str());
                    std::wstring rldRedirectPath = TurnPathIntoRootLocalDevice(widen_argument(redirectPath.c_str()).c_str());
                    if (!impl::PathExists(rldRedirectPath.c_str()) && impl::PathExists(rldPathName.c_str()))
                    {
                        // If the directory does not exist in the redirected location, but does in the non-redirected
                        // location, then we want to give the "illusion" that the delete succeeded
                        LogString(RemoveDirectoryInstance, L"RemoveDirectoryFixup In package but not redirected area.", L"Fake return true.");
                        return TRUE;
                    }
                    else
                    {
                        LogString(RemoveDirectoryInstance, L"RemoveDirectoryFixup Use Folder", redirectPath.c_str());
                        BOOL bRet = impl::RemoveDirectory(rldRedirectPath.c_str());
                        Log(L"[%d]RemoveDirectoryFixup deletes redirected with result: %d", RemoveDirectoryInstance, bRet);
                        return bRet;
                    }
                }
            }
            else
            {
                Log(L"[%d]RemoveDirectoryFixup Under LocalAppData\\Packages, don't redirect", RemoveDirectoryInstance);
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
        LogString(L"RemoveDirectoryFixup ", L"***Exception; use requested folder.***");
    }

    std::wstring rldPathName = TurnPathIntoRootLocalDevice(widen_argument(pathName).c_str());
    return impl::RemoveDirectory(rldPathName.c_str());
}
DECLARE_STRING_FIXUP(impl::RemoveDirectory, RemoveDirectoryFixup);
