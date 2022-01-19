//-------------------------------------------------------------------------------------------------------
// Copyright (C) Rafael Rivera, Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <psf_logging.h>

template <typename CharT>
BOOL __stdcall WritePrivateProfileStringFixup(
    _In_opt_ const CharT* appName,
    _In_opt_ const CharT* keyName,
    _In_opt_ const CharT* string,
    _In_opt_ const CharT* fileName) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            DWORD WritePrivateProfileStringInstance = ++g_FileIntceptInstance;
            
            if (fileName != NULL)
            {
                LogString(WritePrivateProfileStringInstance,L"WritePrivateProfileStringFixup for fileName", fileName);
                if (!IsUnderUserAppDataLocalPackages(fileName))
                {
                    auto [shouldRedirect, redirectPath, shouldReadonly] = ShouldRedirectV2(fileName, redirect_flags::copy_on_read, WritePrivateProfileStringInstance);
                    if (shouldRedirect)
                    {
                        if constexpr (psf::is_ansi<CharT>)
                        {
                            BOOL bRet = impl::WritePrivateProfileString(appName, keyName, string, ((std::filesystem::path)redirectPath).string().c_str());
#if _DEBUG
                            Log(L"[%d] WritePrivateProfileString(A) returns %d", WritePrivateProfileStringInstance, bRet);
#endif
                            return bRet;
                        }
                        else
                        {
                            BOOL bRet = impl::WritePrivateProfileString(appName, keyName, string, redirectPath.c_str());
#if _DEBUG
                            Log(L"[%d] WritePrivateProfileString(W) returns %d", WritePrivateProfileStringInstance, bRet);
#endif                            
                            return bRet;
                        }
                    }
                }
                else
                {
                    Log(L"[%d]Under LocalAppData\\Packages, don't redirect", WritePrivateProfileStringInstance);
                }
            }
            else
            {
                Log(L"[%d]null fileName, don't redirect", WritePrivateProfileStringInstance);
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::WritePrivateProfileString(appName, keyName, string, fileName);
}
DECLARE_STRING_FIXUP(impl::WritePrivateProfileString, WritePrivateProfileStringFixup);
