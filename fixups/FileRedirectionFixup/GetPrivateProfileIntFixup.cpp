//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan, TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <errno.h>
#include "FunctionImplementations.h"
#include "PathRedirection.h"

template <typename CharT>
UINT __stdcall GetPrivateProfileIntFixup(
    _In_opt_ const CharT* sectionName,
    _In_opt_ const CharT* key,
    _In_opt_ const INT nDefault,
    _In_opt_ const CharT* fileName) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            if constexpr (psf::is_ansi<CharT>)
            {
                LogString(L"GetPrivateProfileIntFixup for fileName", widen_argument(fileName).c_str());
                LogString(L" Section", widen_argument(sectionName).c_str());
                LogString(L" Key", widen_argument(key).c_str());
            }
            else
            {
                LogString(L"GetPrivateProfileIntFixup for fileName", fileName);
                LogString(L" Section", sectionName);
                LogString(L" Key", key);
            }
            auto [shouldRedirect, redirectPath, shouldReadonly] = ShouldRedirect(fileName, redirect_flags::copy_on_read);
            if (shouldRedirect)
            {
                if constexpr (psf::is_ansi<CharT>)
                {
                    UINT retval =  impl::GetPrivateProfileIntW(widen_argument(sectionName).c_str(), widen_argument(key).c_str(), nDefault, redirectPath.c_str());
                    Log(L" Returned uint: %d ", retval);
                    return retval;
                }
                else
                {
                    UINT retval = impl::GetPrivateProfileIntW(sectionName, key, nDefault, redirectPath.c_str());
                    Log(L" Returned uint: %d ", retval);
                    return retval;
                }
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::GetPrivateProfileInt(sectionName, key, nDefault, fileName);
}
DECLARE_STRING_FIXUP(impl::GetPrivateProfileInt, GetPrivateProfileIntFixup);
