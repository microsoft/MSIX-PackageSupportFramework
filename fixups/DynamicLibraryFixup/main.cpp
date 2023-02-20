//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#define PSF_DEFINE_EXPORTS
#include <psf_framework.h>
#include "psf_tracelogging.h"

TRACELOGGING_DEFINE_PROVIDER(
    g_Log_ETW_ComponentProvider,
    "Microsoft.Windows.PSFRuntime",
    (0xf7f4e8c4, 0x9981, 0x5221, 0xe6, 0xfb, 0xff, 0x9d, 0xd1, 0xcd, 0xa4, 0xe1),
    TraceLoggingOptionMicrosoftTelemetry());

void InitializeFixups();
void InitializeConfiguration();
void Log(const char* fmt, ...);

extern "C" {

    BOOL __stdcall DllMain(HINSTANCE, DWORD reason, LPVOID) noexcept try
    {
        if (reason == DLL_PROCESS_ATTACH)
        {
            TraceLoggingRegister(g_Log_ETW_ComponentProvider);
            Log("Attaching DynamicLibraryFixup");

            InitializeFixups();
            InitializeConfiguration();
        }
        else if (reason == DLL_PROCESS_DETACH)
        {
            TraceLoggingUnregister(g_Log_ETW_ComponentProvider);
        }
        return TRUE;
    }
    catch (...)
    {
        psf::TraceLogExceptions("DynamicLibraryFixupException", L"DynamicLibraryFixup attach ERROR");
        TraceLoggingUnregister(g_Log_ETW_ComponentProvider);
        Log("RuntDynamicLibraryFixup attach ERROR");
        ::SetLastError(win32_from_caught_exception());
        return FALSE;
    }

}