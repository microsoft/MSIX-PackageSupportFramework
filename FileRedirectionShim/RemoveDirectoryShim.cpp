
#include "FunctionImplementations.h"
#include "PathRedirection.h"

template <typename CharT>
BOOL __stdcall RemoveDirectoryShim(_In_ const CharT* pathName) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            // NOTE: See commentary in DeleteFileShim for limitations on deleting files/directories
            auto [shouldRedirect, redirectPath] = ShouldRedirect(pathName, redirect_flags::none);
            if (shouldRedirect)
            {
                return impl::RemoveDirectory(redirectPath.c_str());
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::RemoveDirectory(pathName);
}
DECLARE_STRING_SHIM(impl::RemoveDirectory, RemoveDirectoryShim);
