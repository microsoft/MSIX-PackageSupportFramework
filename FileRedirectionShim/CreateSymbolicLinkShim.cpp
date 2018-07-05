
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
                // NOTE: If the target is a directory, then ideally we would recursively copy its contents to its
                //       redirected location (since future accesses may want to read/write to files that originated from
                //       the package). However, doing so would be quite a bit of work, so we'll defer doing so until
                //       later when we have evidence that this could be an issue.
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
