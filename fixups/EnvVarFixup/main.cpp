//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#define PSF_DEFINE_EXPORTS
#include <psf_framework.h>
#include <psf_logging.h>


void InitializeFixups();
void InitializeConfiguration();

extern "C" {

    BOOL __stdcall DllMain(HINSTANCE, DWORD reason, LPVOID) noexcept try
    {
        if (reason == DLL_PROCESS_ATTACH)
        {
#if _DEBUG
            Log(L"Attaching EnvVarFixup");
#endif

            InitializeFixups();
            InitializeConfiguration();
        }

        return TRUE;
    }
    catch (...)
    {
        Log(L"EnvVarFixup attach ERROR");
        ::SetLastError(win32_from_caught_exception());
        return FALSE;
    }

}