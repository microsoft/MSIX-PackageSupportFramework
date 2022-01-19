//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan, TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <psf_logging.h>

template <typename CharT>
BOOL __stdcall WritePrivateProfileSectionFixup(
    _In_opt_ const CharT* appName,
    _In_opt_ const CharT* string,
    _In_opt_ const CharT* fileName) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            DWORD WritePrivateProfileSectionInstance = ++g_FileIntceptInstance;

            if (fileName != NULL)
            {
                LogString(WritePrivateProfileSectionInstance,L"WritePrivateProfileSectionFixup for fileName", fileName);
                if (!IsUnderUserAppDataLocalPackages(fileName))
                {
                    auto [shouldRedirect, redirectPath, shouldReadonly] = ShouldRedirectV2(fileName, redirect_flags::copy_on_read, WritePrivateProfileSectionInstance);
                    if (shouldRedirect)
                    {
                        if constexpr (psf::is_ansi<CharT>)
                        {
                            return impl::WritePrivateProfileSectionW(widen_argument(appName).c_str(),
                                widen_argument(string).c_str(), redirectPath.c_str());
                        }
                        else
                        {
                            return impl::WritePrivateProfileSection(appName, string, redirectPath.c_str());
                        }
                    }
                }
                else
                {
                    Log(L"[%d]Under LocalAppData\\Packages, don't redirect", WritePrivateProfileSectionInstance);
                }
            }
            else
            {
                Log(L"[%d]null fileName, don't redirect", WritePrivateProfileSectionInstance);
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::WritePrivateProfileSection(appName, string, fileName);
}
DECLARE_STRING_FIXUP(impl::WritePrivateProfileSection, WritePrivateProfileSectionFixup);
