
#include "FunctionImplementations.h"
#include "PathRedirection.h"

template <typename CharT>
BOOLEAN __stdcall CreateSymbolicLinkShim(
    _In_ const CharT* symlinkFileName,
    _In_ const CharT* targetFileName,
    _In_ DWORD flags) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            auto [redirectLink, redirectPath] = ShouldRedirect(symlinkFileName, redirect_flags::ensure_directory_structure);
            auto [redirectTarget, redirectTargetPath] = ShouldRedirect(targetFileName, redirect_flags::copy_on_read);
            if (redirectLink || redirectTarget)
            {
                return impl::CreateSymbolicLink(
                    redirectLink ? redirectPath.c_str() : widen_argument(symlinkFileName).c_str(),
                    redirectTarget ? redirectTargetPath.c_str() : widen_argument(targetFileName).c_str(),
                    flags);
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::CreateSymbolicLink(symlinkFileName, targetFileName, flags);
}
DECLARE_STRING_SHIM(impl::CreateSymbolicLink, CreateSymbolicLinkShim);
