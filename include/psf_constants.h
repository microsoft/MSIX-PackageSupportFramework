//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace psf
{
    // Detours will auto-rename from *32.dll to *64.dll and vice-versa when doing cross-architectures launches. It will
    // _not_, however auto-rename when the architectures match, so we need to make sure that we reference the binary
    // name that's consistent with the current architecture.
#ifdef _M_IX86
    // 32-bit
    constexpr wchar_t runtime_dll_name[] = L"PsfRuntime32.dll";

    // 32-bit binaries should invoke the 64-bit version of PsfRunDll
    constexpr char run_dll_name[] = "PsfRunDll64.exe";
    constexpr wchar_t wrun_dll_name[] = L"PsfRunDll64.exe";

    constexpr char arch_string[] = "32";
    constexpr wchar_t warch_string[] = L"32";

    constexpr wchar_t current_version[] = L"0.0.000000.0";
#else
    // 64 bit
    constexpr wchar_t runtime_dll_name[] = L"PsfRuntime64.dll";

    // 64-bit binaries should invoke the 32-bit version of PsfRunDll
    constexpr char run_dll_name[] = "PsfRunDll32.exe";
    constexpr wchar_t wrun_dll_name[] = L"PsfRunDll32.exe";

    constexpr char arch_string[] = "64";
    constexpr wchar_t warch_string[] = L"64";

    constexpr wchar_t current_version[] = L"0.0.000000.0";
#endif
}
