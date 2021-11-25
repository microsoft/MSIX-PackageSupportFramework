//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan, TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <errno.h>
#include "FunctionImplementations.h"
#include "PathRedirection.h"

template <typename CharT>
DWORD __stdcall GetPrivateProfileSectionNamesFixup(
    _Out_writes_to_opt_(stringSize, return +1) CharT* string,
    _In_ DWORD stringLength,
    _In_opt_ const CharT* fileName) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            DWORD GetPrivateProfileSectionNamesInstance = ++g_FileIntceptInstance;
            if (fileName != NULL)
            {
                LogString(GetPrivateProfileSectionNamesInstance,L"GetPrivateProfileSectionNamesFixup for fileName", widen(fileName, CP_ACP).c_str());
                if (!IsUnderUserAppDataLocalPackages(fileName))
                {
                    auto [shouldRedirect, redirectPath, shouldReadonly] = ShouldRedirect(fileName, redirect_flags::copy_on_read);
                    if (shouldRedirect)
                    {
                        if constexpr (psf::is_ansi<CharT>)
                        {
                            auto wideString = std::make_unique<wchar_t[]>(stringLength);
                            auto realRetValue = impl::GetPrivateProfileSectionNamesW(wideString.get(), stringLength, redirectPath.c_str());

                            if (_doserrno != ENOENT)
                            {
                                ::WideCharToMultiByte(CP_ACP, 0, wideString.get(), stringLength, string, stringLength, nullptr, nullptr);
                                return realRetValue;
                            }
                        }
                        else
                        {
                            return impl::GetPrivateProfileSectionNamesW(string, stringLength, redirectPath.c_str());
                        }
                    }
                }
                else
                {
                    Log(L"[%d]  Under LocalAppData\\Packages, don't redirect", GetPrivateProfileSectionNamesInstance);
                }
            }
            else
            {
                Log(L"[%d]  null fileName, don't redirect as may be registry based or default.", GetPrivateProfileSectionNamesInstance);
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::GetPrivateProfileSectionNames(string, stringLength, fileName);
}
DECLARE_STRING_FIXUP(impl::GetPrivateProfileSectionNames, GetPrivateProfileSectionNamesFixup);
