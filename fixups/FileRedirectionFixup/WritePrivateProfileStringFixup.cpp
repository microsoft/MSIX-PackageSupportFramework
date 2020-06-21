//-------------------------------------------------------------------------------------------------------
// Copyright (C) Rafael Rivera, Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"

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
                LogString(WritePrivateProfileStringInstance, L"WritePrivateProfileStringFixup for fileName", fileName);
                if (!IsUnderUserAppDataLocalPackages(fileName))
                {
                    auto [shouldRedirect, redirectPath, shouldReadonly] = ShouldRedirect(fileName, redirect_flags::copy_on_read);
                    if (shouldRedirect)
                    {
                        if constexpr (psf::is_ansi<CharT>)
                        {
                            return impl::WritePrivateProfileStringW(widen_argument(appName).c_str(), widen_argument(keyName).c_str(),
                                widen_argument(string).c_str(), redirectPath.c_str());
                        }
                        else
                        {
                            return impl::WritePrivateProfileString(appName, keyName, string, redirectPath.c_str());
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
