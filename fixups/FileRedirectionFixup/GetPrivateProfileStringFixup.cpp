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
    try
    {
        if (guard)
        {
            LogString(L"GetPrivateProfileStringFixup for fileName", fileName);
            
            auto[shouldRedirect, redirectPath, shouldReadonly] = ShouldRedirect(fileName, redirect_flags::copy_on_read);
            if (shouldRedirect)
            {
                if constexpr (psf::is_ansi<CharT>)
                {
                    auto wideString = std::make_unique<wchar_t[]>(stringLength);
                    auto realRetValue = impl::GetPrivateProfileStringW(widen_argument(appName).c_str(), widen_argument(keyName).c_str(),
                        widen_argument(defaultString).c_str(), wideString.get(), stringLength, redirectPath.c_str());

                    if (_doserrno != ENOENT)
                    {
                        ::WideCharToMultiByte(CP_ACP, 0, wideString.get(), stringLength, string, stringLength, nullptr, nullptr);
                        return realRetValue;
                    }
                }
                else
                {
                    return impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, redirectPath.c_str());
                }
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, fileName);
}
DECLARE_STRING_FIXUP(impl::GetPrivateProfileString, GetPrivateProfileStringFixup);
