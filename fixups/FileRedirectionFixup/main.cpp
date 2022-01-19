//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <psf_framework.h>
#include <psf_logging.h>

void InitializePaths();
void InitializeConfiguration();

extern "C" {

int __stdcall PSFInitialize() noexcept try
{
    InitializeConfiguration();
    psf::attach_all();
    return ERROR_SUCCESS;
}
catch (...)
{
    return win32_from_caught_exception();
}

int __stdcall PSFUninitialize() noexcept try
{
    psf::detach_all();
    return ERROR_SUCCESS;
}
catch (...)
{
    return win32_from_caught_exception();
}

#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:PSFInitialize=_PSFInitialize@0")
#pragma comment(linker, "/EXPORT:PSFUninitialize=_PSFUninitialize@0")
#else
#pragma comment(linker, "/EXPORT:PSFInitialize=PSFInitialize")
#pragma comment(linker, "/EXPORT:PSFUninitialize=PSFUninitialize")
#endif

BOOL __stdcall DllMain(HINSTANCE, DWORD reason, LPVOID) noexcept try
{
    if (reason == DLL_PROCESS_ATTACH)
    {
#ifdef _DEBUG
        ::OutputDebugStringA("FileRedirectionFixup attached");
#endif
        InitializePaths();
    }

    return TRUE;
}
catch (...)
{
    ::SetLastError(win32_from_caught_exception());
    return FALSE;
}

}
