//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <psf_logging.h>

template <typename CharT>
BOOL __stdcall MoveFileFixup(_In_ const CharT* existingFileName, _In_ const CharT* newFileName) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            DWORD MoveFileInstance = ++g_FileIntceptInstance;
#if _DEBUG
            LogString(MoveFileInstance,L"MoveFileFixup From", existingFileName);
            LogString(MoveFileInstance,L"MoveFileFixup To",   newFileName);
#endif

            // NOTE: MoveFile needs delete access to the existing file, but since we won't have delete access to the
            //       file if it is in the package, we copy-on-read it here. This is slightly wasteful since we're
            //       copying it only for it to immediately get deleted, but that's simpler than trying to roll our own
            //       implementation of MoveFile. And of course, the same limitation for deleting files applies here as
            //       well. Additionally, we don't copy-on-read the destination file for the same reason we don't do the
            //       same for CopyFile: we give the application the benefit of the doubt that they previously tried to
            //       delete the file if it exists in the package path.
            auto [redirectExisting, existingRedirectPath, shouldReadonlSource] = ShouldRedirectV2(existingFileName, redirect_flags::copy_on_read, MoveFileInstance);
            auto [redirectDest, destRedirectPath, shouldReadonlyDest] = ShouldRedirectV2(newFileName, redirect_flags::ensure_directory_structure, MoveFileInstance);
            if (redirectExisting || redirectDest)
            {
                BOOL bRet = impl::MoveFile(
                    redirectExisting ? existingRedirectPath.c_str() : widen_argument(existingFileName).c_str(),
                    redirectDest ? destRedirectPath.c_str() : widen_argument(newFileName).c_str());
#if _DEBUG
                if (bRet)
                    Log(L"[%d]MoveFile returns true.", MoveFileInstance);
                else
                    Log(L"[%d]MoveFile returns false. err=0x%x", MoveFileInstance,GetLastError());
#endif
                return bRet;
            }
        }
        else
        {
#if _DEBUG
            LogString(0, L"MoveFileFixup Unguarded From", existingFileName);
            LogString(0, L"MoveFileFixup Unguarded To", newFileName);
#endif
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
        Log(L"MoveFileFixup ***Exception detected; fallback.***");
    }

    return impl::MoveFile(existingFileName, newFileName);
}
DECLARE_STRING_FIXUP(impl::MoveFile, MoveFileFixup);

template <typename CharT>
BOOL __stdcall MoveFileExFixup(
    _In_ const CharT* existingFileName,
    _In_opt_ const CharT* newFileName,
    _In_ DWORD flags) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            DWORD MoveFileExInstance = ++g_FileIntceptInstance;
#if _DEBUG
            LogString(MoveFileExInstance,L"MoveFileExFixup From", existingFileName);
            LogString(MoveFileExInstance,L"MoveFileExFixup To",   newFileName);
#endif
           

            // See note in MoveFile for commentary on copy-on-read functionality (though we could do better by checking
            // flags for MOVEFILE_REPLACE_EXISTING)
            auto [redirectExisting, existingRedirectPath, shouldReadonlySource] = ShouldRedirectV2(existingFileName, redirect_flags::copy_on_read, MoveFileExInstance);
            auto [redirectDest, destRedirectPath, shouldReadonlyDest] = ShouldRedirectV2(newFileName, redirect_flags::ensure_directory_structure, MoveFileExInstance);
            if (redirectExisting || redirectDest)
            {
                std::wstring rldExistingFileName = TurnPathIntoRootLocalDevice(redirectExisting ? existingRedirectPath.c_str() : widen_argument(existingFileName).c_str());
                std::wstring rldNewDirectory = TurnPathIntoRootLocalDevice(redirectDest ? destRedirectPath.c_str() : widen_argument(newFileName).c_str());
                BOOL bRet= impl::MoveFileEx(rldExistingFileName.c_str(), rldNewDirectory.c_str(), flags);
#if _DEBUG
                if (bRet)
                    Log(L"[%d]MoveFileEx returns true.", MoveFileExInstance);
                else
                    Log(L"[%d]MoveFileEx returns false with err 0x%x", MoveFileExInstance,GetLastError());
#endif
                return bRet;
            }
        }
        else
        {
#if _DEBUG
            LogString(0, L"MoveFileExFixup Unguarded From", existingFileName);
            LogString(0, L"MoveFileExFixup Unguarded To", newFileName);
#endif
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
        Log(L"MoveFileExFixup ***Exception detected; fallback.***");
    }

    if constexpr (psf::is_ansi<CharT>)
    {
        std::string rldExistingFileName = TurnPathIntoRootLocalDevice(existingFileName);
        std::string rldNewFileName = TurnPathIntoRootLocalDevice(newFileName);
        return impl::MoveFileEx(rldExistingFileName.c_str(), rldNewFileName.c_str(), flags);
    }
    else
    {
        std::wstring rldExistingFileName = TurnPathIntoRootLocalDevice(existingFileName);
        std::wstring rldNewDirectory = TurnPathIntoRootLocalDevice(newFileName);
        return impl::MoveFileEx(rldExistingFileName.c_str(), rldNewDirectory.c_str(), flags);
    }
    ///return impl::MoveFileEx(existingFileName, newFileName, flags);
}
DECLARE_STRING_FIXUP(impl::MoveFileEx, MoveFileExFixup);
