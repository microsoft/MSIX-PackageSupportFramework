//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <psf_framework.h>
#include "psf_tracelogging.h"

TRACELOGGING_DEFINE_PROVIDER(
    g_Log_ETW_ComponentProvider,
    "Microsoft.Windows.PSFRuntime",
    (0xf7f4e8c4, 0x9981, 0x5221, 0xe6, 0xfb, 0xff, 0x9d, 0xd1, 0xcd, 0xa4, 0xe1),
    TraceLoggingOptionMicrosoftTelemetry());

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
    psf::TraceLogExceptions("FileRedirectionFixupException", "FileRedirectionFixup configuration intialization exception");
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
        TraceLoggingRegister(g_Log_ETW_ComponentProvider);
        ::OutputDebugStringA("FileRedirectionFixup attached");
        InitializePaths();
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        TraceLoggingUnregister(g_Log_ETW_ComponentProvider);
    }
    return TRUE;
}
catch (...)
{
    ::SetLastError(win32_from_caught_exception());
    return FALSE;
}

}
