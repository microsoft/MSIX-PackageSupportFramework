//-------------------------------------------------------------------------------------------------------
// Copyright (C) Rafael Rivera, Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"

template <typename CharT>
DWORD __stdcall GetPrivateProfileStringFixup(
    _In_opt_ const CharT* appName,
    _In_opt_ const CharT* keyName,
    _In_opt_ const CharT* defaultString,
    _Out_writes_to_opt_(returnStringSizeInChars, return +1) CharT* string,
    _In_ DWORD stringLength,
    _In_opt_ const CharT* fileName) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    DWORD GetPrivateProfileStringInstance = ++g_FileIntceptInstance;
    try
    {
        if (guard)
        {
            if constexpr (psf::is_ansi<CharT>)
            {
                if (fileName != NULL)
                {
                    LogString(GetPrivateProfileStringInstance,L"GetPrivateProfileStringFixup for fileName", widen(fileName, CP_ACP).c_str());
                }
                else
                {
                    Log(L"[%d] GetPrivateProfileStringFixup for null file.", GetPrivateProfileStringInstance);
                }
                if (appName != NULL)
                {

                    LogString(GetPrivateProfileStringInstance,L" Section", widen_argument(appName).c_str());
                }
                if (keyName != NULL)
                {
                        LogString(GetPrivateProfileStringInstance,L" Key", widen_argument(keyName).c_str());
                }
            }
            else
            {
                if (fileName != NULL)
                {
                    LogString(GetPrivateProfileStringInstance,L"GetPrivateProfileStringFixup for fileName", widen(fileName, CP_ACP).c_str());
                }
                else
                {
                    Log(L"[%d] GetPrivateProfileStringFixup for null file.", GetPrivateProfileStringInstance);
                }
                if (appName != NULL)
                {

                    LogString(GetPrivateProfileStringInstance,L" Section", appName);
                }
                if (keyName != NULL)
                {
                        LogString(GetPrivateProfileStringInstance,L" Key", keyName);
                }
            }
            if (fileName != NULL)
            {
                if (!IsUnderUserAppDataLocalPackages(fileName))
                {
                    auto [shouldRedirect, redirectPath, shouldReadonly] = ShouldRedirect(fileName, redirect_flags::copy_on_read);
                    if (shouldRedirect)
                    {
                        if constexpr (psf::is_ansi<CharT>)
                        {
                            
                            auto realRetValue = impl::GetPrivateProfileString(appName, keyName,
                                                                               defaultString, string, stringLength, 
                                                                               narrow(redirectPath.c_str()).c_str() );
                            
                            Log(L"[%d] Ansi Returned length=0x%x", GetPrivateProfileStringInstance, realRetValue);
                            LogString(GetPrivateProfileStringInstance, " Ansi Returned string", string);
                            return realRetValue;
                        }
                        else
                        {
                            auto realRetValue = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, redirectPath.c_str());
                            LogString(GetPrivateProfileStringInstance, L" Returned string", string);
                            return realRetValue;
                        }
                    }
                }
                else
                {
                    Log(L"[%d]  Under LocalAppData\\Packages, don't redirect", GetPrivateProfileStringInstance);
                }
            }
            else
            {
                Log(L"[%d]  null fileName, don't redirect as may be registry based or default.", GetPrivateProfileStringInstance);
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    DWORD dRet =  impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, fileName);
    LogString(GetPrivateProfileStringInstance, L"Returning ", string);
    return dRet;
}
DECLARE_STRING_FIXUP(impl::GetPrivateProfileString, GetPrivateProfileStringFixup);
