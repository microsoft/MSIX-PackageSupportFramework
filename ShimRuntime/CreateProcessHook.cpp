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

BOOL WINAPI CreateProcessWithShimRunDll(
    [[maybe_unused]] _In_opt_ LPCWSTR applicationName,
    _Inout_opt_ LPWSTR commandLine,
    _In_opt_ LPSECURITY_ATTRIBUTES processAttributes,
    _In_opt_ LPSECURITY_ATTRIBUTES threadAttributes,
    _In_ BOOL inheritHandles,
    _In_ DWORD creationFlags,
    _In_opt_ LPVOID environment,
    _In_opt_ LPCWSTR currentDirectory,
    _In_ LPSTARTUPINFOW startupInfo,
    _Out_ LPPROCESS_INFORMATION processInformation) noexcept try
{
    // Detours should only be calling this function with the intent to execute "rundll32.exe"
    assert(std::wcsstr(applicationName, L"rundll32.exe"));
    assert(std::wcsstr(commandLine, L"rundll32.exe"));

    std::wstring cmdLine = shims::wrun_dll_name;
    cmdLine += (commandLine + 12); // +12 to get to the first space after "rundll32.exe"

    return CreateProcessImpl(
        (PackageRootPath() / shims::wrun_dll_name).c_str(),
        cmdLine.data(),
        processAttributes,
        threadAttributes,
        inheritHandles,
        creationFlags,
        environment,
        currentDirectory,
        startupInfo,
        processInformation);
}
catch (...)
{
    ::SetLastError(win32_from_caught_exception());
    return FALSE;
}

template <typename CharT>
using startup_info_t = std::conditional_t<std::is_same_v<CharT, char>, STARTUPINFOA, STARTUPINFOW>;

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
    _Out_ LPPROCESS_INFORMATION processInformation) noexcept try
{
    // We can't detour child processes whose executables are located outside of the package as they won't have execute
    // access to the shim dlls. Instead of trying to replicate the executable search logic when determining the location
    // of the target executable, create the process as suspended and let the system tell us where the executable is
    PROCESS_INFORMATION pi;
    if (!processInformation)
    {
        processInformation = &pi;
    }

    if (!CreateProcessImpl(
        applicationName,
        commandLine,
        processAttributes,
        threadAttributes,
        inheritHandles,
        creationFlags | CREATE_SUSPENDED,
        environment,
        currentDirectory,
        startupInfo,
        processInformation))
    {
        return FALSE;
    }

    iwstring path;
    DWORD size = MAX_PATH;
    path.resize(size - 1);
    while (true)
    {
        if (::QueryFullProcessImageNameW(processInformation->hProcess, 0, path.data(), &size))
        {
            path.resize(size);
            break;
        }
        else if (auto err = ::GetLastError(); err == ERROR_INSUFFICIENT_BUFFER)
        {
            size *= 2;
            path.resize(size - 1);
        }
        else
        {
            // Unexpected error
            ::TerminateProcess(processInformation->hProcess, ~0u);
            ::CloseHandle(processInformation->hProcess);
            ::CloseHandle(processInformation->hThread);

            ::SetLastError(err);
            return FALSE;
        }
    }

    // std::filesystem::path comparison doesn't seem to handle case-insensitivity or root-local device paths...
    iwstring_view packagePath(PackageRootPath().native().c_str(), PackageRootPath().native().length());
    iwstring_view exePath = path;
    auto fixupPath = [](iwstring_view& p)
    {
        if ((p.length() >= 4) && (p.substr(0, 4) == LR"(\\?\)"_isv))
        {
            p = p.substr(4);
        }
    };
    fixupPath(packagePath);
    fixupPath(exePath);

    if ((exePath.length() >= packagePath.length()) && (exePath.substr(0, packagePath.length()) == packagePath))
    {
        // The target executable is in the package, so we _do_ want to shim it
        static const auto pathToShimRuntime = (PackageRootPath() / shims::runtime_dll_name).string();
        PCSTR targetDll = pathToShimRuntime.c_str();
        if (!::DetourUpdateProcessWithDll(processInformation->hProcess, &targetDll, 1))
        {
            // We failed to detour the created process. Assume that it the failure was due to an architecture mis-match
            // and try the launch using ShimRunDll
            if (!::DetourProcessViaHelperDllsW(processInformation->dwProcessId, 1, &targetDll, CreateProcessWithShimRunDll))
            {
                // Could not detour the target process, so return failure
                auto err = ::GetLastError();

                ::TerminateProcess(processInformation->hProcess, ~0u);
                ::CloseHandle(processInformation->hProcess);
                ::CloseHandle(processInformation->hThread);

                ::SetLastError(err);
                return FALSE;
            }
        }
    }

    if ((creationFlags & CREATE_SUSPENDED) != CREATE_SUSPENDED)
    {
        // Caller did not want the process to start suspended
        ::ResumeThread(processInformation->hThread);
    }

    if (processInformation == &pi)
    {
        ::CloseHandle(processInformation->hProcess);
        ::CloseHandle(processInformation->hThread);
    }

    return TRUE;
}
catch (...)
{
    ::SetLastError(win32_from_caught_exception());
    return FALSE;
}

DECLARE_STRING_SHIM(CreateProcessImpl, CreateProcessShim);
