//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <shim_framework.h>

void InitializePaths();
void InitializeConfiguration();

extern "C" {

int __stdcall ShimInitialize() noexcept try
{
    InitializeConfiguration();
    shims::attach_all();
    return ERROR_SUCCESS;
}
catch (...)
{
    return win32_from_caught_exception();
}

int __stdcall ShimUninitialize() noexcept try
{
    shims::detach_all();
    return ERROR_SUCCESS;
}
catch (...)
{
    return win32_from_caught_exception();
}

#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:ShimInitialize=_ShimInitialize@0")
#pragma comment(linker, "/EXPORT:ShimUninitialize=_ShimUninitialize@0")
#else
#pragma comment(linker, "/EXPORT:ShimInitialize=ShimInitialize")
#pragma comment(linker, "/EXPORT:ShimUninitialize=ShimUninitialize")
#endif

BOOL __stdcall DllMain(HINSTANCE, DWORD reason, LPVOID) noexcept try
{
    if (reason == DLL_PROCESS_ATTACH)
    {
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
