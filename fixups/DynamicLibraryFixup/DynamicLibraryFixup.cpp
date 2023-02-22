//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <psf_framework.h>
#include "FunctionImplementations.h"
#include "dll_location_spec.h"
#include "psf_tracelogging.h"

extern bool                  g_dynf_forcepackagedlluse;
extern std::vector<dll_location_spec> g_dynf_dllSpecs;


auto LoadLibraryImpl = psf::detoured_string_function(&::LoadLibraryA, &::LoadLibraryW);
template <typename CharT>
HMODULE __stdcall LoadLibraryFixup(_In_ const CharT* libFileName)
{
#if _DEBUG
    LogString("LoadLibraryFixup called for", libFileName);
#endif
    auto guard = g_reentrancyGuard.enter();
    HMODULE result;

    if (guard)
    {
#if _DEBUG
        Log("LoadLibraryFixup unguarded.");
#endif
        // Check against known dlls in package.
        std::wstring libFileNameW = GetFilenameOnly(InterpretStringW(libFileName));

        if (g_dynf_forcepackagedlluse)
        {
#if _DEBUG
            Log("LoadLibraryFixup forcepackagedlluse.");
#endif
            for (dll_location_spec spec : g_dynf_dllSpecs)
            {
#if _DEBUG
                Log("LoadLibraryFixup test");
#endif
                try
                {
#if _DEBUG
                    LogString("LoadLibraryFixup testing against", spec.filename.data());
#endif
                    if (spec.filename.compare(libFileNameW + L".dll") == 0 ||
                        spec.filename.compare(libFileNameW) == 0)
                    {
                        LogString("LoadLibraryFixup using", spec.full_filepath.c_str());
                        result = LoadLibraryImpl(spec.full_filepath.c_str());
                        return result;
                    }
                }
                catch (...)
                {
                    psf::TraceLogExceptions("DynamicLibraryFixupException", L"LoadLibraryFixup: LoadLibraryFixup ERROR");
                    Log("LoadLibraryFixup ERROR");
                }
            }
        }
    }
    result = LoadLibraryImpl(libFileName);
    ///QueryPerformanceCounter(&TickEnd);
    return result;
}
DECLARE_STRING_FIXUP(LoadLibraryImpl, LoadLibraryFixup);

auto LoadLibraryExImpl = psf::detoured_string_function(&::LoadLibraryExA, &::LoadLibraryExW);
template <typename CharT>
HMODULE __stdcall LoadLibraryExFixup(_In_ const CharT* libFileName, _Reserved_ HANDLE file, _In_ DWORD flags)
{
#if _DEBUG
    LogString("LoadLibraryExFixup called on",libFileName);
#endif
    auto guard = g_reentrancyGuard.enter();
    HMODULE result;

    if (guard)
    {
#if _DEBUG
        Log("LoadLibraryExFixup unguarded.");
#endif
        // Check against known dlls in package.
        std::wstring libFileNameW = InterpretStringW(libFileName);
        
        if (g_dynf_forcepackagedlluse)
        {
            for (dll_location_spec spec : g_dynf_dllSpecs)
            {
                try
                {
#if _DEBUG
                    //Log("LoadLibraryExFixup testing %ls against %ls", libFileNameW.c_str(), spec.full_filepath.native().c_str());
                    LogString("LoadLibraryExFixup testing against", spec.filename.data());
#endif
                    if (spec.filename.compare(libFileNameW + L".dll") == 0 ||
                        spec.filename.compare(libFileNameW) == 0)
                    {
                        LogString("LoadLibraryExFixup using", spec.full_filepath.c_str());
                        result = LoadLibraryExImpl(spec.full_filepath.c_str(), file, flags);
                        return result;
                    }
                }
                catch (...)
                {
                    psf::TraceLogExceptions("DynamicLibraryFixupException", L"LoadLibraryExFixup: LoadLibraryExFixup ERROR");
                    Log("LoadLibraryExFixup Error");
                }
            }
        }
    }
    result = LoadLibraryExImpl(libFileName, file, flags);
    ///QueryPerformanceCounter(&TickEnd);
    return result;
}
DECLARE_STRING_FIXUP(LoadLibraryExImpl, LoadLibraryExFixup);


// NOTE: The following is a list of functions taken from https://msdn.microsoft.com/en-us/library/windows/desktop/ms682599(v=vs.85).aspx
//       that are _not_ present above. This is just a convenient collection of what's missing; it is not a collection of
//       future work.
//
// AddDllDirectory
// LoadModule
// LoadPackagedLibrary
// RemoveDllDirectory
// SetDefaultDllDirectories
// SetDllDirectory
// 
// DisableThreadLibraryCalls
// DllMain
// FreeLibrary
// FreeLibraryAndExitThread
// GetDllDirectory
// GetModuleFileName
// GetModuleFileNameEx
// GetModuleHandle
// GetModuleHandleEx
// GetProcAddress
// QueryOptionalDelayLoadedAPI
