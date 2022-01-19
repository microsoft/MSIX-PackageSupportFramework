//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <psf_logging.h>

template <typename CharT>
BOOL __stdcall CreateDirectoryFixup(_In_ const CharT* pathName, _In_opt_ LPSECURITY_ATTRIBUTES securityAttributes) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    
    try
    {
        if (guard)
        {
            DWORD CreateDirectoryInstance = ++g_FileIntceptInstance;
            std::wstring wPathName = widen(pathName);
#if _DEBUG
            LogString(CreateDirectoryInstance,L"CreateDirectoryFixup for path", pathName);
#endif
            std::replace(wPathName.begin(), wPathName.end(), L'/', L'\\');

            if (IsUnderUserPackageWritablePackageRoot(wPathName.c_str()))
            {
                wPathName = ReverseRedirectedToPackage(wPathName.c_str());
#if _DEBUG
                LogString(CreateDirectoryInstance, L"Use ReverseRedirected fileName", wPathName.c_str());
#endif
            }

            if (!IsUnderUserAppDataLocalPackages(wPathName.c_str()))
            {
                auto [shouldRedirect, redirectPath, shouldReadonlySource] = ShouldRedirectV2(pathName, redirect_flags::ensure_directory_structure, CreateDirectoryInstance);
                if (shouldRedirect)
                {
#if _DEBUG
                    LogString(CreateDirectoryInstance, L"CreateDirectoryFixup Use Folder", redirectPath.c_str());
#endif
                    return impl::CreateDirectory(redirectPath.c_str(), securityAttributes);
                }
            }
            else
            {
#if _DEBUG
                Log(L"[%d]Under LocalAppData\\Packages, don't redirect", CreateDirectoryInstance);
#endif
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
        LogString(L"CreateDirectoryFixup ", L"*** Exception; use requested folder.***");
    }

    // In the spirit of app compatability, make the path long formed just in case.
    if constexpr (psf::is_ansi<CharT>)
    {
        std::string sPathName = TurnPathIntoRootLocalDevice(pathName);
        return impl::CreateDirectory(sPathName.c_str(), securityAttributes);
    }
    else
    {
        std::wstring wPathName = TurnPathIntoRootLocalDevice(pathName);
        return impl::CreateDirectory(wPathName.c_str(), securityAttributes);
    }

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
#if _DEBUG
            LogString(CreateDirectoryExInstance,L"CreateDirectoryExFixup for", templateDirectory);
            LogString(CreateDirectoryExInstance,L"CreateDirectoryExFixup to",  newDirectory);
#endif
            std::wstring WtemplateDirectory = widen(templateDirectory);
            std::wstring WnewDirectory = widen(newDirectory);
            std::replace(WtemplateDirectory.begin(), WtemplateDirectory.end(), L'/', L'\\');
            std::replace(WnewDirectory.begin(), WnewDirectory.end(), L'/', L'\\');

            if (IsUnderUserPackageWritablePackageRoot(WtemplateDirectory.c_str()))
            {
                WtemplateDirectory = ReverseRedirectedToPackage(WtemplateDirectory.c_str());
#if _DEBUG
                LogString(CreateDirectoryExInstance, L"Use ReverseRedirected templateDirectory", WtemplateDirectory.c_str());
#endif
            }
            if (IsUnderUserPackageWritablePackageRoot(WnewDirectory.c_str()))
            {
                WnewDirectory = ReverseRedirectedToPackage(WtemplateDirectory.c_str());
#if _DEBUG
                LogString(CreateDirectoryExInstance, L"Use ReverseRedirected newDirectory", WnewDirectory.c_str());
#endif
            }

            
            auto [redirectTemplate, redirectTemplatePath,shouldReadonlySource] = ShouldRedirectV2(WtemplateDirectory.c_str(), redirect_flags::check_file_presence, CreateDirectoryExInstance);
            auto [redirectDest, redirectDestPath,shouldReadonlyDest] = ShouldRedirectV2(WnewDirectory.c_str(), redirect_flags::ensure_directory_structure, CreateDirectoryExInstance);
            if (redirectTemplate || redirectDest)
            {
                std::wstring rldRedirectTemplate = TurnPathIntoRootLocalDevice(redirectTemplate ? redirectTemplatePath.c_str() : WtemplateDirectory.c_str());
                std::wstring rldDest = TurnPathIntoRootLocalDevice(redirectDest ? redirectDestPath.c_str() : WnewDirectory.c_str());
                return impl::CreateDirectoryEx(rldRedirectTemplate.c_str(), rldDest.c_str(), securityAttributes);
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming less redirection is necessary
        LogString(L"CreateDirectoryExFixup ", L"*** Exception; use requested folder.***");
    }

    // In the spirit of app compatability, make the path long formed just in case.
    try
    {
        std::wstring WtemplateDirectory = widen(templateDirectory);
        std::wstring WnewDirectory = widen(newDirectory);
        std::wstring rldTemplateDirectory = TurnPathIntoRootLocalDevice(WtemplateDirectory.c_str());
        std::wstring rldNewDirectory = TurnPathIntoRootLocalDevice(WnewDirectory.c_str());
        return impl::CreateDirectoryEx(rldTemplateDirectory.c_str(), rldNewDirectory.c_str(), securityAttributes);
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
        LogString(L"CreateDirectoryExFixup ", L"*** Exception; use requested folder.***");
    }
    return impl::CreateDirectoryEx(templateDirectory, newDirectory, securityAttributes);
}
DECLARE_STRING_FIXUP(impl::CreateDirectoryEx, CreateDirectoryExFixup);
