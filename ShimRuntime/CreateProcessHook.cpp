//
// The ShimRuntime intercepts all CreateProcess[AW] calls so that shims can be propagated to child processes. The
// general call graph looks something like the following:
//      * CreateProcessShim gets called by application code; forwards arguments on to:
//      * DetourCreateProcessWithDlls[AW], which calls:
//      * CreateProcessInterceptRunDll32, which identifies attempts to launch rundll32 from System32/SysWOW64,
//        redirecting those attempts to either ShimRunDll32.exe or ShimRunDll64.exe, and then calls:
//      * The actual implementation of CreateProcess[AW]
//
// NOTE: Other CreateProcess variants (e.g. CreateProcessAsUser[AW]) aren't currently detoured as the Detours framework
//       does not make it easy to accomplish that at this time.
//

#include <string_view>
#include <vector>

#include <windows.h>
#include <detours.h>
#include <shim_constants.h>
#include <shim_framework.h>

#include "Config.h"

using namespace std::literals;

auto CreateProcessImpl = shims::detoured_string_function(&::CreateProcessA, &::CreateProcessW);
// TODO: CreateProcessAsUser, CreateProcessWithLogon, CreateProcessWithToken?

template <typename CharT>
using create_process_t = std::conditional_t<std::is_same_v<CharT, char>, decltype(&::CreateProcessA), decltype(&::CreateProcessW)>;

template <typename CharT>
using startup_info_t = std::conditional_t<std::is_same_v<CharT, char>, STARTUPINFOA, STARTUPINFOW>;

// Detours attempts to use rundll32.exe when architectures don't match. This is a problem since rundll32 exists under
// System32/SysWOW64 and will cause a "break-away" from the Centennial container. Intercept these attempts and replace
// with our own custom rundll32-like executable
template <typename CharT>
BOOL WINAPI CreateProcessInterceptRunDll32(
    _In_opt_ const CharT* applicationName,
    _Inout_opt_ CharT* commandLine,
    _In_opt_ LPSECURITY_ATTRIBUTES processAttributes,
    _In_opt_ LPSECURITY_ATTRIBUTES threadAttributes,
    _In_ BOOL inheritHandles,
    _In_ DWORD creationFlags,
    _In_opt_ LPVOID environment,
    _In_opt_ const CharT* currentDirectory,
    _In_ startup_info_t<CharT>* startupInfo,
    _Out_ LPPROCESS_INFORMATION processInformation)
{
    // Used to hold the values of applicationName/commandLine _only_ if we're modifying them
    std::basic_string<CharT> appName;
    std::basic_string<CharT> cmdLine;

    if constexpr (std::is_same_v<CharT, char>)
    {
        if (auto exeStart = std::strstr(commandLine, "rundll32.exe"))
        {
            cmdLine = shims::run_dll_name;
            cmdLine += (exeStart + 12); // +12 to get to the first space

            appName = (PackageRootPath() / shims::wrun_dll_name).string();
        }
    }
    else
    {
        if (auto exeStart = std::wcsstr(commandLine, L"rundll32.exe"))
        {
            cmdLine = shims::wrun_dll_name;
            cmdLine += (exeStart + 12); // +12 to get to the first space

            appName = (PackageRootPath() / shims::wrun_dll_name).wstring();
        }
    }

    if (!appName.empty())
    {
        applicationName = appName.c_str();
        commandLine = cmdLine.data();
    }

    return CreateProcessImpl(
        applicationName,
        commandLine,
        processAttributes,
        threadAttributes,
        inheritHandles,
        creationFlags,
        environment,
        currentDirectory,
        startupInfo,
        processInformation);
}

template <typename CharT>
struct detour_create_process;

template <typename CharT>
constexpr auto detour_create_process_v = detour_create_process<CharT>::value;

template <>
struct detour_create_process<char>
{
    static constexpr auto value = &::DetourCreateProcessWithDllsA;
};

template <>
struct detour_create_process<wchar_t>
{
    static constexpr auto value = &::DetourCreateProcessWithDllsW;
};

template <typename CharT>
BOOL WINAPI CreateProcessShim(
    _In_opt_ const CharT* applicationName,
    _Inout_opt_ CharT* commandLine,
    _In_opt_ LPSECURITY_ATTRIBUTES processAttributes,
    _In_opt_ LPSECURITY_ATTRIBUTES threadAttributes,
    _In_ BOOL inheritHandles,
    _In_ DWORD creationFlags,
    _In_opt_ LPVOID environment,
    _In_opt_ const CharT* currentDirectory,
    _In_ startup_info_t<CharT>* startupInfo,
    _Out_ LPPROCESS_INFORMATION processInformation)
{
    // TODO: Load from config? (NOTE: Unnecessary if we go with the ShimRuntime-loads-shims approach)
    std::vector<std::string> shimDllPaths = { (PackageRootPath() / shims::runtime_dll_name).string() };
    std::vector<const char*> shimDlls = { shimDllPaths[0].c_str() };

    return detour_create_process_v<CharT>(
        applicationName,
        commandLine,
        processAttributes,
        threadAttributes,
        inheritHandles,
        creationFlags,
        environment,
        currentDirectory,
        startupInfo,
        processInformation,
        static_cast<DWORD>(shimDlls.size()),
        shimDlls.data(),
        CreateProcessInterceptRunDll32<CharT>);
}

DECLARE_STRING_SHIM(CreateProcessImpl, CreateProcessShim);
