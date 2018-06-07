#pragma once

namespace shims
{
    // Detours will auto-rename from *32.dll to *64.dll and vice-versa when doing cross-architectures launches. It will
    // _not_, however auto-rename when the architectures match, so we need to make sure that we reference the binary
    // name that's consistent with the current architecture.
#ifdef _M_IX86
    // 32-bit
    constexpr wchar_t runtime_dll_name[] = L"ShimRuntime32.dll";

    // 32-bit binaries should invoke the 64-bit version of ShimRunDll
    constexpr char run_dll_name[] = "ShimRunDll64.exe";
    constexpr wchar_t wrun_dll_name[] = L"ShimRunDll64.exe";

    constexpr char arch_string[] = "32";
    constexpr wchar_t warch_string[] = L"32";
#else
    // 64 bit
    constexpr wchar_t runtime_dll_name[] = L"ShimRuntime64.dll";

    // 64-bit binaries should invoke the 32-bit version of ShimRunDll
    constexpr char run_dll_name[] = "ShimRunDll32.exe";
    constexpr wchar_t wrun_dll_name[] = L"ShimRunDll32.exe";

    constexpr char arch_string[] = "64";
    constexpr wchar_t warch_string[] = L"64";
#endif
}
