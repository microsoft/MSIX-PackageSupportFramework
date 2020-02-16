//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"

template <typename CharT>
BOOL __stdcall CreateDirectoryFixup(_In_ const CharT* pathName, _In_opt_ LPSECURITY_ATTRIBUTES securityAttributes) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            DWORD CreateDirectoryInstance = ++g_FileIntceptInstance;
            LogString(CreateDirectoryInstance,L"CreateDirectoryFixup for path", pathName);
            
            if (!IsUnderUserAppDataLocalPackages(pathName))
            {
                auto [shouldRedirect, redirectPath, shouldReadonlySource] = ShouldRedirect(pathName, redirect_flags::ensure_directory_structure, CreateDirectoryInstance);
                if (shouldRedirect)
                {
                    return impl::CreateDirectory(redirectPath.c_str(), securityAttributes);
                }
            }
            else
            {
                Log(L"[%d]Under LocalAppData\\Packages, don't redirect", CreateDirectoryInstance);
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::CreateDirectory(pathName, securityAttributes);
}
DECLARE_STRING_FIXUP(impl::CreateDirectory, CreateDirectoryFixup);

template <typename CharT>
BOOL __stdcall CreateDirectoryExFixup(
    _In_ const CharT* templateDirectory,
    _In_ const CharT* newDirectory,
    _In_opt_ LPSECURITY_ATTRIBUTES securityAttributes) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            DWORD CreateDirectoryExInstance = ++g_FileIntceptInstance;

            LogString(CreateDirectoryExInstance,L"CreateDirectoryExFixup for", templateDirectory);
            LogString(CreateDirectoryExInstance,L"CreateDirectoryExFixup to",  newDirectory);
            
            auto [redirectTemplate, redirectTemplatePath,shouldReadonlySource] = ShouldRedirect(templateDirectory, redirect_flags::check_file_presence, CreateDirectoryExInstance);
            auto [redirectDest, redirectDestPath,shouldReadonlyDest] = ShouldRedirect(newDirectory, redirect_flags::ensure_directory_structure, CreateDirectoryExInstance);
            if (redirectTemplate || redirectDest)
            {
                return impl::CreateDirectoryEx(
                    redirectTemplate ? redirectTemplatePath.c_str() : widen_argument(templateDirectory).c_str(),
                    redirectDest ? redirectDestPath.c_str() : widen_argument(newDirectory).c_str(),
                    securityAttributes);
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::CreateDirectoryEx(templateDirectory, newDirectory, securityAttributes);
}
DECLARE_STRING_FIXUP(impl::CreateDirectoryEx, CreateDirectoryExFixup);
