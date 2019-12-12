//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#define PSF_DEFINE_EXPORTS
#include <psf_framework.h>


void InitializeFixups();
void InitializeConfiguration();
void Log(const char* fmt, ...);

extern "C" {

    BOOL __stdcall DllMain(HINSTANCE, DWORD reason, LPVOID) noexcept try
    {
        if (reason == DLL_PROCESS_ATTACH)
        {
            Log("Attaching DynamicLibraryFixup");

            InitializeFixups();
            InitializeConfiguration();
        }

        return TRUE;
    }
    catch (...)
    {
        Log("RuntDynamicLibraryFixup attach ERROR");
        ::SetLastError(win32_from_caught_exception());
        return FALSE;
    }

}