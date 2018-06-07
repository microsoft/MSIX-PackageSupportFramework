
#include "FunctionImplementations.h"
#include "PathRedirection.h"

template <typename CharT>
BOOL __stdcall CreateDirectoryShim(_In_ const CharT* pathName, _In_opt_ LPSECURITY_ATTRIBUTES securityAttributes) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            auto [shouldRedirect, redirectPath] = ShouldRedirect(pathName, redirect_flags::ensure_directory_structure);
            if (shouldRedirect)
            {
                return impl::CreateDirectory(redirectPath.c_str(), securityAttributes);
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::CreateDirectory(pathName, securityAttributes);
}
DECLARE_STRING_SHIM(impl::CreateDirectory, CreateDirectoryShim);

template <typename CharT>
BOOL __stdcall CreateDirectoryExShim(
    _In_ const CharT* templateDirectory,
    _In_ const CharT* newDirectory,
    _In_opt_ LPSECURITY_ATTRIBUTES securityAttributes) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            auto [redirectTemplate, redirectTemplatePath] = ShouldRedirect(templateDirectory, redirect_flags::check_file_presence);
            auto [redirectDest, redirectDestPath] = ShouldRedirect(newDirectory, redirect_flags::ensure_directory_structure);
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
DECLARE_STRING_SHIM(impl::CreateDirectoryEx, CreateDirectoryExShim);
