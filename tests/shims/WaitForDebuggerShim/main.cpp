//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <debug.h>
#include <shim_framework.h>

extern "C" {

// This is a useful "shim" for debugging other shims. When loaded, it will hold the process until a debugger is
// attached. This is typically most useful when scoped to ShimLauncher.* in config.json. This will hold the process on
// initial launch, at which point the debugger can be attached and set up to debug child processes. E.g. your config
// file might look something like:
//
//      {
//          "applications": [ ... ],
//          "processes": [
//              {
//                  "executable": "ShimLauncher.*",
//                  "shims": [
//                      {
//                          "dll": "WaitForDebuggerShim.dll"
//                      }
//                  ]
//              }, ...
//          ]
//      }
int __stdcall ShimInitialize() noexcept
{
    bool waitForDebugger = true;
    if (auto config = ::ShimQueryCurrentDllConfig())
    {
        auto& configObject = config->as_object();
        if (auto enabledValue = configObject.try_get("enabled"))
        {
            waitForDebugger = enabledValue->as_boolean().get();
        }
    }

    if (waitForDebugger)
    {
        shims::wait_for_debugger();
    }

    return ERROR_SUCCESS;
}

int __stdcall ShimUninitialize() noexcept
{
    return ERROR_SUCCESS;
}

#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:ShimInitialize=_ShimInitialize@0")
#pragma comment(linker, "/EXPORT:ShimUninitialize=_ShimUninitialize@0")
#else
#pragma comment(linker, "/EXPORT:ShimInitialize=ShimInitialize")
#pragma comment(linker, "/EXPORT:ShimUninitialize=ShimUninitialize")
#endif

}
