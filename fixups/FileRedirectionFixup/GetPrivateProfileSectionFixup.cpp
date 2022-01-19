//-------------------------------------------------------------------------------------------------------
// Copyright (C) Rafael Rivera, Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <errno.h>
#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <psf_logging.h>

template <typename CharT>
DWORD __stdcall GetPrivateProfileSectionFixup(
    _In_opt_ const CharT* appName,
    _Out_writes_to_opt_(stringSize, return +1) CharT* string,
    _In_ DWORD stringLength,
    _In_opt_ const CharT* fileName) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            DWORD GetPrivateProfileSectionInstance = ++g_FileIntceptInstance;
            if (fileName != NULL)
            {
#if _DEBUG
                LogString(GetPrivateProfileSectionInstance,L"GetPrivateProfileSectionFixup for fileName", widen(fileName, CP_ACP).c_str());
#endif
                if (!IsUnderUserAppDataLocalPackages(fileName))
                {
                    auto [shouldRedirect, redirectPath, shouldReadonly] = ShouldRedirectV2(fileName, redirect_flags::copy_on_read, GetPrivateProfileSectionInstance);
                    if (shouldRedirect)
                    {
                        if constexpr (psf::is_ansi<CharT>)
                        {
                            auto wideString = std::make_unique<wchar_t[]>(stringLength);
                            auto realRetValue = impl::GetPrivateProfileSectionW(widen_argument(appName).c_str(), wideString.get(),
                                stringLength, redirectPath.c_str());

                            if (_doserrno != ENOENT)
                            {
                                ::WideCharToMultiByte(CP_ACP, 0, wideString.get(), stringLength, string, stringLength, nullptr, nullptr);
                                return realRetValue;
                            }
                        }
                        else
                        {
                            return impl::GetPrivateProfileSectionW(appName, string, stringLength, redirectPath.c_str());
                        }
                    }
                }
                else
                {
#if _DEBUG
                    Log(L"[%d]  Under LocalAppData\\Packages, don't redirect", GetPrivateProfileSectionInstance);
#endif
                }
            }
            else
            {
#if _DEBUG
                Log(L"[%d]  null fileName, don't redirect as may be registry based or default.", GetPrivateProfileSectionInstance);
#endif
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::GetPrivateProfileSection(appName, string, stringLength, fileName);
}
DECLARE_STRING_FIXUP(impl::GetPrivateProfileSection, GetPrivateProfileSectionFixup);
