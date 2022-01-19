//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <psf_logging.h>

/// Consider adding AddDllDirectory hooking to also add virtual equivalents
/// There seems to be some dll loading that occurs outside of loadlibrary???
/// It would be done in FRF as only FRF knows the alternate paths.

template <typename CharT>
BOOL __stdcall SetCurrentDirectoryFixup(_In_ const CharT* filePath) noexcept
{
    auto guard = g_reentrancyGuard.enter();
   
    try
    {
        if (guard)
        {
            DWORD SetWorkingDirectoryInstance = ++g_FileIntceptInstance;
            if constexpr (psf::is_ansi<CharT>)
            {
                LogStringWA(SetWorkingDirectoryInstance,L"SetWorkingDirectoryInstance A input is", (const char *)filePath);
                ///Loghexdump((void*)filePath, (long)strnlen_s(filePath, 512)+1);
                ///Log(L"[%d]SetWorkingDirectoryInstance 0x%x 0x%x 0x%x 0x%x", SetWorkingDirectoryInstance, (BYTE)filePath[0], (BYTE)filePath[1], (BYTE)filePath[2], (BYTE)filePath[3]);
                ////for (int inx = 0; inx <= (int)strnlen_s(filePath, 512); inx++)  // yeah, show the null too
                ////{
                ////    Log(L"\t\t0x%x", (BYTE)filePath[inx]);
                ////}
            }
            else
            {
#if _DEBUG
                LogStringWW(SetWorkingDirectoryInstance,L"SetCurrentDirectoryFixup W input is", filePath);
#endif
            }
            std::wstring wFilePath = widen(filePath);
#if _DEBUG
            LogString(SetWorkingDirectoryInstance, L"SetCurrentDirectoryFixup ", wFilePath.c_str());
#endif
            if (!path_relative_to(wFilePath.c_str(), psf::current_package_path()))
            {
                normalized_path normalized = NormalizePath(wFilePath.c_str(), SetWorkingDirectoryInstance);
                normalized_path virtualized = VirtualizePath(normalized, SetWorkingDirectoryInstance);
                if (impl::PathExists(virtualized.full_path.c_str()))
                {
#if _DEBUG
                    LogString(SetWorkingDirectoryInstance, L"SetCurrentDirectoryFixup Use Folder", virtualized.full_path.c_str());
#endif
                    return impl::SetCurrentDirectoryW(virtualized.full_path.c_str());
                }
                else
                {
                    // Fall through to original call
#if _DEBUG
                    LogString(SetWorkingDirectoryInstance, L"SetCurrentDirectoryFixup ", L"Virtualized folder not in package, use requested folder."); 
#endif
                }
            }
            else
            {
                // Fall through to original call
#if _DEBUG
                LogString(SetWorkingDirectoryInstance, L"SetCurrentDirectoryFixup ", L"Requested folder is part of package, use requested folder.");
#endif
            }
            return ::SetCurrentDirectoryW(wFilePath.c_str());
        }

    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
        LogString(L"SetCurrentDirectoryFixup ", L"***Exception; use requested folder.***");
    }

    return impl::SetCurrentDirectory(filePath);
}
DECLARE_STRING_FIXUP(impl::SetCurrentDirectory, SetCurrentDirectoryFixup);


template <typename CharT>
DWORD __stdcall GetCurrentDirectoryFixup(_In_ DWORD nBufferLength, _Out_ CharT* filePath) noexcept 
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (!guard)
        {
            return impl::GetCurrentDirectory(nBufferLength, filePath);
        }
        else
        {

            // This exists for debugging only.
#if _DEBUG
            DWORD GetWorkingDirectoryInstance = ++g_FileIntceptInstance;
#endif
            DWORD dRet = impl::GetCurrentDirectory(nBufferLength, filePath);

#if _DEBUG
            Log(L"[%d]GetCurrentDirectory: returns 0x%x", GetWorkingDirectoryInstance, dRet);
#endif
            if (dRet != 0)
            {
                if (nBufferLength >= dRet)
                {
                    try
                    {
#if _DEBUG
                        LogString(GetWorkingDirectoryInstance, L"GetCurrentDirectory path", filePath);
#endif
                    }
                    catch (...)
                    {
                        Log(L"[%d] Exception printing GetCurrentDirectory.");
                    }
                }
                else
                {
#if _DEBUG
                    Log(L"[%d]GetCurrentDirectory but buffer was only 0x%x", GetWorkingDirectoryInstance, nBufferLength);
#endif
                }
            }
            else
            {
#if _DEBUG
                Log(L"[%d]GetCurrentDirectory Error = 0x%x", GetWorkingDirectoryInstance, GetLastError());
#endif
            }

            return dRet;
        }
    }
    catch (...)
    {
        int err = win32_from_caught_exception();
        Log(L"***GetCurrentDirectory Exception 0x%x***", err);
        ::SetLastError(err);
        return err;
    }
}

DECLARE_STRING_FIXUP(impl::GetCurrentDirectory, GetCurrentDirectoryFixup);

