//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <psf_logging.h>

template <typename CharT>
BOOL __stdcall ReplaceFileFixup(
    _In_ const CharT* replacedFileName,
    _In_ const CharT* replacementFileName,
    _In_opt_ const CharT* backupFileName,
    _In_ DWORD replaceFlags,
    _Reserved_ LPVOID exclude,
    _Reserved_ LPVOID reserved) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            DWORD ReplaceFileInstance = ++g_FileIntceptInstance;
#if _DEBUG
            LogString(ReplaceFileInstance,L"ReplaceFileFixup From", replacedFileName);
            LogString(ReplaceFileInstance,L"ReplaceFileFixup To",   replacementFileName);
            if (backupFileName != nullptr)
            {
                LogString(ReplaceFileInstance, L"ReplaceFileFixup with backup", backupFileName);
            }
#endif

            // NOTE: ReplaceFile will delete the "replacement file" (the file we're copying from), so therefore we need
            //       delete access to it, thus we copy-on-read it here. I.e. we're copying the file only for it to
            //       immediately get deleted. We could improve this in the future if we wanted, but that would
            //       effectively require that we re-write ReplaceFile, which we opt not to do right now. Also note that
            //       this implies that we have the same file deletion limitation that we have for DeleteFile, etc.
            auto [redirectTarget, targetRedirectPath, shouldReadonlyTarget] = ShouldRedirectV2(replacedFileName, redirect_flags::copy_on_read, ReplaceFileInstance);
            auto [redirectSource, sourceRedirectPath, shouldReadonlySource] = ShouldRedirectV2(replacementFileName, redirect_flags::ensure_directory_structure, ReplaceFileInstance);
            auto [redirectBackup, backupRedirectPath, shouldReadonlyDest] = ShouldRedirectV2(backupFileName, redirect_flags::ensure_directory_structure, ReplaceFileInstance);
            if (redirectTarget || redirectSource || redirectBackup)
            {
                std::wstring rldReplacedFileName = TurnPathIntoRootLocalDevice(redirectTarget ? targetRedirectPath.c_str() : widen_argument(replacedFileName).c_str());
                std::wstring rldReplacementFileName = TurnPathIntoRootLocalDevice(redirectSource ? sourceRedirectPath.c_str() : widen_argument(replacementFileName).c_str());
                if (backupFileName != nullptr)
                {
                    std::wstring rldBackupFileName = TurnPathIntoRootLocalDevice(redirectBackup ? backupRedirectPath.c_str() : widen_argument(backupFileName).c_str());
                    return impl::ReplaceFile(rldReplacedFileName.c_str(), rldReplacementFileName.c_str(), rldBackupFileName.c_str(), replaceFlags, exclude, reserved);
                }
                else
                {
                    return impl::ReplaceFile(rldReplacedFileName.c_str(), rldReplacementFileName.c_str(), nullptr, replaceFlags, exclude, reserved);
                }
            }
        }
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
        LogString(L"ReplaceFileFixup ", L"***Exception; use requested files.***");
    }

    if constexpr (psf::is_ansi<CharT>)
    {
        std::string rldReplacedFileName = TurnPathIntoRootLocalDevice(replacedFileName);
        std::string rldReplacementFileName = TurnPathIntoRootLocalDevice(replacementFileName);
        if (backupFileName != nullptr)
        {
            std::string rldBackupFileName = TurnPathIntoRootLocalDevice(backupFileName);
            return impl::ReplaceFile(rldReplacedFileName.c_str(), rldReplacementFileName.c_str(), rldBackupFileName.c_str(), replaceFlags, exclude, reserved);
        }
        else
        {
            return impl::ReplaceFile(rldReplacedFileName.c_str(), rldReplacementFileName.c_str(), nullptr, replaceFlags, exclude, reserved);
        }
    }
    else
    {
        std::wstring rldReplacedFileName = TurnPathIntoRootLocalDevice(replacedFileName);
        std::wstring rldReplacementFileName = TurnPathIntoRootLocalDevice(replacementFileName);
        if (backupFileName != nullptr)
        {
            std::wstring rldBackupFileName = TurnPathIntoRootLocalDevice(backupFileName);
            return impl::ReplaceFile(rldReplacedFileName.c_str(), rldReplacementFileName.c_str(), rldBackupFileName.c_str(), replaceFlags, exclude, reserved);
        }
        else
        {
            return impl::ReplaceFile(rldReplacedFileName.c_str(), rldReplacementFileName.c_str(), nullptr, replaceFlags, exclude, reserved);
        }
    }
    ///return impl::ReplaceFile(replacedFileName, replacementFileName, backupFileName, replaceFlags, exclude, reserved);
}
DECLARE_STRING_FIXUP(impl::ReplaceFile, ReplaceFileFixup);
