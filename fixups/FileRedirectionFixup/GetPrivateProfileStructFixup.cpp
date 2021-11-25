//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan, TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <errno.h>
#include "FunctionImplementations.h"
#include "PathRedirection.h"

template <typename CharT>
BOOL __stdcall GetPrivateProfileStructFixup(
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
            DWORD GetPrivateProfileStructInstance = ++g_FileIntceptInstance;
            if (fileName != NULL)
            {
                LogString(GetPrivateProfileStructInstance,L"GetPrivateProfileStructFixup for fileName", widen(fileName, CP_ACP).c_str());
                if (!IsUnderUserAppDataLocalPackages(fileName))
                {
                    auto [shouldRedirect, redirectPath, shouldReadonly] = ShouldRedirect(fileName, redirect_flags::copy_on_read);
                    if (shouldRedirect)
                    {
                        if constexpr (psf::is_ansi<CharT>)
                        {
                            return impl::GetPrivateProfileStructW(widen_argument(sectionName).c_str(), widen_argument(key).c_str(),
                                structArea, uSizeStruct, redirectPath.c_str());
                        }
                        else
                        {
                            return impl::GetPrivateProfileStructW(sectionName, key, structArea, uSizeStruct, redirectPath.c_str());
                        }
                    }
                }
                else
                {
                    Log(L"[%d]  Under LocalAppData\\Packages, don't redirect", GetPrivateProfileStructInstance);
                }
            }
            else
            {
                Log(L"[%d]  null fileName, don't redirect as may be registry based or default.", GetPrivateProfileStructInstance);
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