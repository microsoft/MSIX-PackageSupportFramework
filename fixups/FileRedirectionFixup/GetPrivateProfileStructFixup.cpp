//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan, TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <errno.h>
#include "FunctionImplementations.h"
#include "PathRedirection.h"

template <typename CharT>
DWORD __stdcall GetPrivateProfileStructFixup(
    _In_opt_ const CharT* sectionName,
    _In_opt_ const CharT* key,
    _Out_writes_to_opt_(uSizeStruct, return) LPVOID structArea,
    _In_ UINT uSizeStruct,
    _In_opt_ const CharT* fileName) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            LogString(L"GetPrivateProfileStructFixup for fileName", fileName);
            auto [shouldRedirect, redirectPath, shouldReadonly] = ShouldRedirect(fileName, redirect_flags::copy_on_read);
            if (shouldRedirect)
            {
                if constexpr (psf::is_ansi<CharT>)
                {
                    auto wideString = std::make_unique<wchar_t[]>(stringLength);
                    return impl::GetPrivateProfileSectionW(widen_argument(sectionName).c_str(), widen_argument(key).c_str(), 
                                                                        structArea, uSizeStruct, redirectPath.c_str());
                }
                else
                {
                    return impl::GetPrivateProfileSectionW(sectionName, key, structArea, uSizeStruct, redirectPath.c_str());
                }
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::GetPrivateProfileStruct(sectionName, key, structArea, uSizeStruct, fileName);
}
DECLARE_STRING_FIXUP(impl::GetPrivateProfileStruct, GetPrivateProfileStructFixup);