//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <psf_logging.h>

template <typename CharT>
BOOL __stdcall DeleteFileFixup(_In_ const CharT* fileName) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            DWORD DeleteFileInstance = ++g_FileIntceptInstance;
#if _DEBUG
            LogString(DeleteFileInstance,L"DeleteFileFixup for fileName", fileName);
#endif
            
            if (!IsUnderUserAppDataLocalPackages(fileName))
            {

                // NOTE: This will only delete the redirected file. If the file previously existed in the package path, then
                //       it will remain there and a later attempt to open, etc. the file will succeed. In the future, if
                //       this proves to be an issue, we could maintain a collection of package files that have been
                //       "deleted" and then just pretend like they've been deleted. Such a change would be rather large and
                //       disruptful and probably fairly inefficient as it would impact virtually every code path, so we'll
                //       put it off for now.
                auto [shouldRedirect, redirectPath, shoudReadonly] = ShouldRedirectV2(fileName, redirect_flags::none, DeleteFileInstance);
                if (shouldRedirect)
                {
                    std::wstring rldFileName = TurnPathIntoRootLocalDevice(widen_argument(fileName).c_str());
                    std::wstring rldRedirPath = TurnPathIntoRootLocalDevice(widen_argument(redirectPath.c_str()).c_str());
                    if (!impl::PathExists(rldRedirPath.c_str()) && impl::PathExists(rldFileName.c_str()))
                    {
                        // If the file does not exist in the redirected location, but does in the non-redirected location,
                        // then we want to give the "illusion" that the delete succeeded
#if _DEBUG
                        Log(L"[%d]DeleteFileFixup Exists in package but not redir, so fake success.", DeleteFileInstance);
#endif
                        return TRUE;
                    }
                    else
                    {
                        BOOL bRet = impl::DeleteFile(rldRedirPath.c_str());
#if _DEBUG
                        Log(L"[%d]DeleteFileFixup deletes from redir with result: %d %ls", DeleteFileInstance,bRet, rldRedirPath.c_str());
#endif
                        return bRet;
                    }
                }
            }
            else
            {
                std::wstring rldFileName = TurnPathIntoRootLocalDevice(widen_argument(fileName).c_str());
                BOOL bRet = impl::DeleteFile(rldFileName.c_str());
#if _DEBUG
                Log(L"[%d]DeleteFileFixup Under LocalAppData\\Packages, don't redirect. deletes with result: %d", DeleteFileInstance, bRet);
#endif
                return bRet;
            }
        }
        else
        {
#if _DEBUG
            LogString(0, L"DeleteFileFixup Unguarded for fileName", fileName);
#endif
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
        Log(L"DeleteFileFixup *** Exception *** ");
    }

    std::wstring rldFileName = TurnPathIntoRootLocalDevice(widen_argument(fileName).c_str());
    return impl::DeleteFile(rldFileName.c_str());
}
DECLARE_STRING_FIXUP(impl::DeleteFile, DeleteFileFixup);