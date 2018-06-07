
#include "FunctionImplementations.h"
#include "PathRedirection.h"

template <typename CharT>
BOOL __stdcall MoveFileShim(_In_ const CharT* existingFileName, _In_ const CharT* newFileName) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            // TODO: Should we copy-on-read the destination file so that the function fails if the file already exists
            //       (or manually fail out ourselves)? Note that without good DeleteFile support, this does not have a
            //       simple answer
            auto [redirectExisting, existingRedirectPath] = ShouldRedirect(existingFileName, redirect_flags::check_file_presence);
            auto [redirectDest, destRedirectPath] = ShouldRedirect(newFileName, redirect_flags::ensure_directory_structure);
            if (redirectExisting || redirectDest)
            {
                return impl::MoveFile(
                    redirectExisting ? existingRedirectPath.c_str() : widen_argument(existingFileName).c_str(),
                    redirectDest ? destRedirectPath.c_str() : widen_argument(newFileName).c_str());
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::MoveFile(existingFileName, newFileName);
}
DECLARE_STRING_SHIM(impl::MoveFile, MoveFileShim);

template <typename CharT>
BOOL __stdcall MoveFileExShim(
    _In_ const CharT* existingFileName,
    _In_opt_ const CharT* newFileName,
    _In_ DWORD flags) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            // See note in MoveFile for commentary on copy-on-read functionality (though we could do better by checking
            // flags for MOVEFILE_REPLACE_EXISTING)
            auto [redirectExisting, existingRedirectPath] = ShouldRedirect(existingFileName, redirect_flags::check_file_presence);
            auto [redirectDest, destRedirectPath] = ShouldRedirect(newFileName, redirect_flags::ensure_directory_structure);
            if (redirectExisting || redirectDest)
            {
                return impl::MoveFileEx(
                    redirectExisting ? existingRedirectPath.c_str() : widen_argument(existingFileName).c_str(),
                    redirectDest ? destRedirectPath.c_str() : widen_argument(newFileName).c_str(),
                    flags);
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::MoveFileEx(existingFileName, newFileName, flags);
}
DECLARE_STRING_SHIM(impl::MoveFileEx, MoveFileExShim);
