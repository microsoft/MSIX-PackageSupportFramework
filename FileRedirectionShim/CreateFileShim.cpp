
#include "FunctionImplementations.h"
#include "PathRedirection.h"

template <typename CharT>
HANDLE __stdcall CreateFileShim(
    _In_ const CharT* fileName,
    _In_ DWORD desiredAccess,
    _In_ DWORD shareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES securityAttributes,
    _In_ DWORD creationDisposition,
    _In_ DWORD flagsAndAttributes,
    _In_opt_ HANDLE templateFile) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            // TODO: Smarter redirect flags using creationDisposition?
            auto [shouldRedirect, redirectPath] = ShouldRedirect(fileName, redirect_flags::copy_on_read);
            if (shouldRedirect)
            {
                return impl::CreateFile(redirectPath.c_str(), desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile);
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::CreateFile(fileName, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile);
}
DECLARE_STRING_SHIM(impl::CreateFile, CreateFileShim);

HANDLE __stdcall CreateFile2Shim(
    _In_ LPCWSTR fileName,
    _In_ DWORD desiredAccess,
    _In_ DWORD shareMode,
    _In_ DWORD creationDisposition,
    _In_opt_ LPCREATEFILE2_EXTENDED_PARAMETERS createExParams) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            // TODO: Smarter redirect flags using creationDisposition?
            auto [shouldRedirect, redirectPath] = ShouldRedirect(fileName, redirect_flags::copy_on_read);
            if (shouldRedirect)
            {
                return impl::CreateFile2(redirectPath.c_str(), desiredAccess, shareMode, creationDisposition, createExParams);
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
    }

    return impl::CreateFile2(fileName, desiredAccess, shareMode, creationDisposition, createExParams);
}
DECLARE_SHIM(impl::CreateFile2, CreateFile2Shim);
