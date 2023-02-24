//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "pch.h"

#define PSF_DEFINE_EXPORTS
#include <psf_framework.h>
#include "psf_tracelogging.h"

TRACELOGGING_DEFINE_PROVIDER(
    g_Log_ETW_ComponentProvider,
    "Microsoft.Windows.PSFRuntime",
    (0xf7f4e8c4, 0x9981, 0x5221, 0xe6, 0xfb, 0xff, 0x9d, 0xd1, 0xcd, 0xa4, 0xe1),
    TraceLoggingOptionMicrosoftTelemetry());

bool trace_function_entry = false;
bool m_inhibitOutput = false;
bool m_shouldLog = true;

void InitializeFixups();
void InitializeConfiguration();
void Log(const char* fmt, ...);

extern "C" {


#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:PSFInitialize=_PSFInitialize@0")
#pragma comment(linker, "/EXPORT:PSFUninitialize=_PSFUninitialize@0")
#else
#pragma comment(linker, "/EXPORT:PSFInitialize=PSFInitialize")
#pragma comment(linker, "/EXPORT:PSFUninitialize=PSFUninitialize")
#endif

    BOOL APIENTRY DllMain(HMODULE, // hModule,
        DWORD  ul_reason_for_call,
        LPVOID // lpReserved
    )  noexcept try
    {

        switch (ul_reason_for_call)
        {
        case DLL_PROCESS_ATTACH:
            TraceLoggingRegister(g_Log_ETW_ComponentProvider);
            Log("Attaching RegLegacyFixups\n");

            InitializeFixups();
            InitializeConfiguration();
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            TraceLoggingUnregister(g_Log_ETW_ComponentProvider);
            break;
        }
        return TRUE;
    }
    catch (...)
    {
        psf::TraceLogExceptions("RegLegacyFixupException", "RegLegacyFixups attach ERROR");
        TraceLoggingUnregister(g_Log_ETW_ComponentProvider);
        Log("RegLegacyFixups attach ERROR\n");
        ::SetLastError(win32_from_caught_exception());
        return FALSE;
    }

}
