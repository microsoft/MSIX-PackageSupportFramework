
#include "FunctionImplementations.h"
#include "PathRedirection.h"

template <typename CharT>
BOOL __stdcall CreateHardLinkShim(
    _In_ const CharT* fileName,
    _In_ const CharT* existingFileName,
    _Reserved_ LPSECURITY_ATTRIBUTES securityAttributes) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            // TODO: If the link already exists in a non-redirected location, should we try and copy it? For now just
            //       assume that this is not a scenario that we'll ever see and that if we do ever run into it, not
            //       copying the file is likely the correct course of action. Alternatively, we could check to see if
            //       the file exists and then just fail if so without doing the copy.
            auto [redirectLink, redirectPath] = ShouldRedirect(fileName, redirect_flags::ensure_directory_structure);
            auto [redirectTarget, redirectTargetPath] = ShouldRedirect(existingFileName, redirect_flags::copy_on_read);
            if (redirectLink || redirectTarget)
            {
                return impl::CreateHardLink(
                    redirectLink ? redirectPath.c_str() : widen_argument(fileName).c_str(),
                    redirectTarget ? redirectTargetPath.c_str() : widen_argument(existingFileName).c_str(),
                    securityAttributes);
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::CreateHardLink(fileName, existingFileName, securityAttributes);
}
DECLARE_STRING_SHIM(impl::CreateHardLink, CreateHardLinkShim);
