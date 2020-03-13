//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// The PsfRuntime intercepts all CreateProcess[AW] calls so that fixups can be propagated to child processes. The
// general call graph looks something like the following:
//      * CreateProcessFixup gets called by application code; forwards arguments on to:
//      * DetourCreateProcessWithDlls[AW], which calls:
//      * CreateProcessInterceptRunDll32, which identifies attempts to launch rundll32 from System32/SysWOW64,
//        redirecting those attempts to either PsfRunDll32.exe or PsfRunDll64.exe, and then calls:
//      * The actual implementation of CreateProcess[AW]
//
// NOTE: Other CreateProcess variants (e.g. CreateProcessAsUser[AW]) aren't currently detoured as the Detours framework
//       does not make it easy to accomplish that at this time.
//

#include <string_view>
#include <vector>

#include <windows.h>
#include <detours.h>
#include <psf_constants.h>
#include <psf_framework.h>

#include "Config.h"

using namespace std::literals;

void Log(const char* fmt, ...);

auto CreateProcessImpl = psf::detoured_string_function(&::CreateProcessA, &::CreateProcessW);

BOOL WINAPI CreateProcessWithPsfRunDll(
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

    std::wstring cmdLine = psf::wrun_dll_name;
    cmdLine += (commandLine + 12); // +12 to get to the first space after "rundll32.exe"

    return CreateProcessImpl(
        (PackageRootPath() / psf::wrun_dll_name).c_str(),
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
BOOL WINAPI CreateProcessFixup(
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
    // access to the fixup dlls. Instead of trying to replicate the executable search logic when determining the location
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
    iwstring_view finalPackagePath(FinalPackageRootPath().native().c_str(), FinalPackageRootPath().native().length());
    iwstring_view exePath = path;
    auto fixupPath = [](iwstring_view& p)
    {
        if ((p.length() >= 4) && (p.substr(0, 4) == LR"(\\?\)"_isv))
        {
            p = p.substr(4);
        }
    };
    fixupPath(packagePath);
    fixupPath(finalPackagePath);
    fixupPath(exePath);

#if _DEBUG
    Log("\tPossible injection to process %ls %d.\n", exePath.data(), processInformation->dwProcessId);
#endif
    if (((exePath.length() >= packagePath.length()) && (exePath.substr(0, packagePath.length()) == packagePath)) ||
        ((exePath.length() >= finalPackagePath.length()) && (exePath.substr(0, finalPackagePath.length()) == finalPackagePath)))
    {
#if _DEBUG
        Log("\tInject %ls into PID=%d", psf::runtime_dll_name, processInformation->dwProcessId);
#endif
        // The target executable is in the package, so we _do_ want to fixup it

        static const auto pathToPsfRuntime = (PackageRootPath() / psf::runtime_dll_name).string();
        PCSTR targetDll = pathToPsfRuntime.c_str();
        if (!std::filesystem::exists(targetDll))
        {
            // Possibly the dll is in the folder with the exe and not at the package root.
            Log("\t%s not found at package root, try target folder.", targetDll);

            std::filesystem::path altPathToExeRuntime = exePath.data();
            static const auto altPathToPsfRuntime = (altPathToExeRuntime.parent_path() / psf::runtime_dll_name).string();
            targetDll = altPathToPsfRuntime.c_str();
#if _DEBUG
            Log("\talt target filename is now %s", altPathToPsfRuntime.c_str());
#endif
        }

        if (!std::filesystem::exists(targetDll))
        {
#if _DEBUG
            Log("\tNot present there either, try elsewhere in package.");
#endif
            // If not in those two locations, must check everywhere in package.
            // The child process might also be in another package folder, so look elsewhere in the package.
            for (auto& dentry : std::filesystem::recursive_directory_iterator(PackageRootPath()))
            {
                try
                {
                    if (dentry.path().filename().compare(psf::runtime_dll_name) == 0)
                    {
                        static const auto altDirPathToPsfRuntime = narrow(dentry.path().c_str());
#if _DEBUG
                        Log("\tFound match as %ls", dentry.path().c_str());
#endif
                        targetDll = altDirPathToPsfRuntime.c_str();
                        break;
                    }
                }
                catch (...)
                {
                    Log("Non-fatal error enumerating directories while looking for PsfRuntime.");
                }
            }
        }

        Log("\tAttempt injection into %d using %s", processInformation->dwProcessId, targetDll);
        if (!::DetourUpdateProcessWithDll(processInformation->hProcess, &targetDll, 1))
        {
            Log("\t%s not found at target folder, try PsfRunDll.", targetDll);
            // We failed to detour the created process. Assume that it the failure was due to an architecture mis-match
            // and try the launch using PsfRunDll
            if (!::DetourProcessViaHelperDllsW(processInformation->dwProcessId, 1, &targetDll, CreateProcessWithPsfRunDll))
            {
                // Could not detour the target process, so return failure
                auto err = ::GetLastError();
                Log("\tUnable to inject %ls into PID=%d err=0x%x\n", psf::runtime_dll_name, processInformation->dwProcessId, err);
                ::TerminateProcess(processInformation->hProcess, ~0u);
                ::CloseHandle(processInformation->hProcess);
                ::CloseHandle(processInformation->hThread);

                ::SetLastError(err);
                return FALSE;
            }
        }
    }

    Log("\tInjected %ls into PID=%d\n", psf::runtime_dll_name, processInformation->dwProcessId);

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

DECLARE_STRING_FIXUP(CreateProcessImpl, CreateProcessFixup);
