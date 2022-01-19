//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "pch.h"

#define PSF_DEFINE_EXPORTS
#include <psf_framework.h>
#include <psf_logging.h>

bool trace_function_entry = false;
bool m_inhibitOutput = false;
bool m_shouldLog = true;

void InitializeFixups();
void InitializeConfiguration();

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
#if _DEBUG
            Log(L"Attaching RegLegacyFixups\n");
#endif
            InitializeFixups();
            InitializeConfiguration();
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
        }
        return TRUE;
    }
    catch (...)
    {
        Log(L"RegLegacyFixups attach ERROR\n");
        ::SetLastError(win32_from_caught_exception());
        return FALSE;
    }

}
