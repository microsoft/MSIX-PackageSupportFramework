//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan, TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <errno.h>
#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <psf_logging.h>

template <typename CharT>
UINT __stdcall GetPrivateProfileIntFixup(
    _In_opt_ const CharT* sectionName,
    _In_opt_ const CharT* key,
    _In_opt_ const INT nDefault,
    _In_opt_ const CharT* fileName) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    DWORD GetPrivateProfileIntInstance = ++g_FileIntceptInstance;
    try
    {
        if (guard)
        {
#if _DEBUG
            if constexpr (psf::is_ansi<CharT>)
            {
                if (fileName != NULL)
                {
                    LogString(GetPrivateProfileIntInstance,L"GetPrivateProfileIntFixup for fileName", widen_argument(fileName).c_str());
                }
                else
                {
                    Log(L"[%d] GetPrivateProfileIntFixup for null file.", GetPrivateProfileIntInstance);
                }
                if (sectionName != NULL)
                {
                    LogString(GetPrivateProfileIntInstance,L" Section", widen_argument(sectionName).c_str());
                }
                if (key != NULL)
                {
                    LogString(GetPrivateProfileIntInstance,L" Key", widen_argument(key).c_str());
                }
            }
            else
            {
                if (fileName != NULL)
                {
                    LogString(GetPrivateProfileIntInstance,L"GetPrivateProfileIntFixup for fileName", fileName);
                }
                else
                {
                    Log(L"[%d] GetPrivateProfileIntFixup for null file.", GetPrivateProfileIntInstance);
                }
                if (sectionName != NULL)
                {
                    LogString(GetPrivateProfileIntInstance,L" Section", sectionName);
                }
                if (key != NULL)
                {
                    LogString(GetPrivateProfileIntInstance,L" Key", key);
                }
            }
#endif
            if (fileName != NULL)
            {
                if (!IsUnderUserAppDataLocalPackages(fileName))
                {
                    auto [shouldRedirect, redirectPath, shouldReadonly] = ShouldRedirectV2(fileName, redirect_flags::copy_on_read, GetPrivateProfileIntInstance);
                    if (shouldRedirect)
                    {
                        if constexpr (psf::is_ansi<CharT>)
                        {
                            UINT retval = impl::GetPrivateProfileIntW(widen_argument(sectionName).c_str(), widen_argument(key).c_str(), nDefault, redirectPath.c_str());
#if _DEBUG
                            Log(L" [%d] Returned uint: %d ", GetPrivateProfileIntInstance, retval);
#endif
                            return retval;
                        }
                        else
                        {
                            UINT retval = impl::GetPrivateProfileIntW(sectionName, key, nDefault, redirectPath.c_str());
#if _DEBUG
                            Log(L" [%d] Returned uint: %d ", GetPrivateProfileIntInstance, retval);
#endif
                            return retval;
                        }
                    }
                }
                else
                {
#if _DEBUG
                    Log(L"[%d]  Under LocalAppData\\Packages, don't redirect", GetPrivateProfileIntInstance);
#endif
                }
            }
            else
            {
#if _DEBUG
                Log(L"[%d]  null filename, don't redirect as may be registry based or default.", GetPrivateProfileIntInstance);
#endif
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    UINT uVal = impl::GetPrivateProfileInt(sectionName, key, nDefault, fileName);
#if _DEBUG
    Log( L"[%d] Returning 0x%x", GetPrivateProfileIntInstance,uVal);
#endif
    return uVal;

}
DECLARE_STRING_FIXUP(impl::GetPrivateProfileInt, GetPrivateProfileIntFixup);
