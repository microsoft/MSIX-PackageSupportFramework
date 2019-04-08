//-------------------------------------------------------------------------------------------------------    
// Copyright (C) Microsoft Corporation. All rights reserved.    
// Licensed under the MIT license. See LICENSE file in the project root for full license information.    
//-------------------------------------------------------------------------------------------------------    
    
#include <debug.h>    
#include <psf_framework.h>    
    
extern "C" {    
    
// This is a useful "fixup" for debugging other fixups. When loaded, it will hold the process until a debugger is    
// attached. This is typically most useful when scoped to PsfLauncher.* in config.json. This will hold the process on    
// initial launch, at which point the debugger can be attached and set up to debug child processes. E.g. your config    
// file might look something like:    
//    
//      {    
//          "applications": [ ... ],    
//          "processes": [    
//              {    
//                  "executable": "PsfLauncher.*",    
//                  "fixups": [    
//                      {    
//                          "dll": "WaitForDebuggerFixup.dll"    
//                      }    
//                  ]    
//              }, ...    
//          ]    
//      }    
int __stdcall PSFInitialize() noexcept    
{    
    bool waitForDebugger = true;    
    if (auto config = ::PSFQueryCurrentDllConfig())    
    {    
        auto& configObject = config->as_object();    
        if (auto enabledValue = configObject.try_get("enabled"))    
        {    
            waitForDebugger = enabledValue->as_boolean().get();    
        }    
    }    
    
    if (waitForDebugger)    
    {    
        psf::wait_for_debugger();    
    }    
    
    return ERROR_SUCCESS;    
}    
    
int __stdcall PSFUninitialize() noexcept    
{    
    return ERROR_SUCCESS;    
}    
    
#ifdef _M_IX86    
#pragma comment(linker, "/EXPORT:PSFInitialize=_PSFInitialize@0")    
#pragma comment(linker, "/EXPORT:PSFUninitialize=_PSFUninitialize@0")    
#else    
#pragma comment(linker, "/EXPORT:PSFInitialize=PSFInitialize")    
#pragma comment(linker, "/EXPORT:PSFUninitialize=PSFUninitialize")    
#endif    
    
}
