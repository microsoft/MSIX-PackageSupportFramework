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
#include "ArgRedirection.h"
#include "psf_tracelogging.h"

using namespace std::literals;

void Log(const char* fmt, ...);

// Function to determine if this process should get 32 or 64bit dll injections
// Returns 32 or 64 (or 0 for error)
typedef BOOL(WINAPI* LPFN_ISWOW64PROCESS2) (HANDLE, PUSHORT, PUSHORT);
USHORT ProcessBitness(HANDLE hProcess)
{
    USHORT pProcessMachine;
    USHORT pMachineNative;
    LPFN_ISWOW64PROCESS2 fnIsWow64Process2 = (LPFN_ISWOW64PROCESS2)GetProcAddress(
        GetModuleHandle(TEXT("kernel32")), "IsWow64Process2");

    if (fnIsWow64Process2 != NULL)
    {
        BOOL bRet = fnIsWow64Process2(hProcess, &pProcessMachine, &pMachineNative);
        if (bRet == 0)
        {
            return 0;
        }
        if (pProcessMachine == IMAGE_FILE_MACHINE_UNKNOWN)
        {
            if (pMachineNative == IMAGE_FILE_MACHINE_AMD64)
            {
                return 64;
            }
            if (pMachineNative == IMAGE_FILE_MACHINE_I386)
            {
                return 32;
            }
        }
        else
        {
            if (pProcessMachine == IMAGE_FILE_MACHINE_AMD64)
            {
                return 64;
            }
            if (pProcessMachine == IMAGE_FILE_MACHINE_I386)
            {
                return 32;
            }
        }
    }
    return 0;
}

std::wstring FixDllBitness(std::wstring originalName, USHORT bitness)
{
    std::wstring targetDll =  originalName.substr(0, originalName.length() - 6);
    switch (bitness)
    {
    case 32:
        targetDll = targetDll.append(L"32.dll");
        break;
    case 64:
        targetDll = targetDll.append(L"64.dll");
        break;
    default:
        targetDll = std::wstring(originalName);
    }
    return targetDll;
}

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

    DWORD cnvtCmdLinePathLen = 0;
    if (commandLine)
    {
        size_t cmdLineLen = strlenImpl(reinterpret_cast<const CharT*>(commandLine));
        cnvtCmdLinePathLen = (cmdLineLen ? MAX_CMDLINE_PATH : 0); // avoid allocation when no cmdLine is passed
    }

    std::unique_ptr<CharT[]> cnvtCmdLine(new CharT[cnvtCmdLinePathLen]);
    if (commandLine && cnvtCmdLine.get())
    {
        // Redirect createProcess arguments if any in native app data to per user per app data
        memset(cnvtCmdLine.get(), 0, MAX_CMDLINE_PATH);
        bool cmdLineConverted = convertCmdLineParameters(commandLine, cnvtCmdLine.get());
        if (cmdLineConverted == true)
        {
            psf::TraceLogArgumentRedirection(commandLine, cnvtCmdLine.get());
        }
    }

    if (!CreateProcessImpl(
        applicationName,
        (commandLine && cnvtCmdLine.get()) ? cnvtCmdLine.get() : commandLine,
        processAttributes,
        threadAttributes,
        inheritHandles,
        creationFlags | CREATE_SUSPENDED,
        environment,
        currentDirectory,
        startupInfo,
        processInformation))
    {
        psf::TraceLogExceptions("PSFRuntimeException", "Create Process failure");
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

    auto appConfig = PSFQueryCurrentAppLaunchConfig(true);
    bool createProcessInAppContext = false;
    auto createProcessInAppContextPtr = appConfig->try_get("inPackageContext");
    if (createProcessInAppContextPtr)
    {
        createProcessInAppContext = createProcessInAppContextPtr->as_boolean().get();
    }

#if _DEBUG
    Log("\tPossible injection to process %ls %d.\n", exePath.data(), processInformation->dwProcessId);
#endif
    if (((exePath.length() >= packagePath.length()) && (exePath.substr(0, packagePath.length()) == packagePath)) ||
        ((exePath.length() >= finalPackagePath.length()) && (exePath.substr(0, finalPackagePath.length()) == finalPackagePath)) ||
        (createProcessInAppContext)) // Inject psfRuntime into an external process that is run in package context 
    {
        // The target executable is in the package, so we _do_ want to fixup it
#if _DEBUG
        Log("\tIn package, so yes");
#endif
        // Fix for issue #167: allow subprocess to be a different bitness than this process.
        USHORT bitness = ProcessBitness(processInformation->hProcess);
#if _DEBUG
        Log("\tInjection for PID=%d Bitness=%d", processInformation->dwProcessId, bitness);
#endif  
        std::wstring wtargetDllName = FixDllBitness(std::wstring(psf::runtime_dll_name), bitness);
#if _DEBUG
        Log("\tUse runtime %ls", wtargetDllName.c_str());
#endif
        static const auto pathToPsfRuntime = (PackageRootPath() / wtargetDllName.c_str()).string();
        const char * targetDllPath = NULL;
#if _DEBUG
        Log("\tInject %s into PID=%d", pathToPsfRuntime.c_str(), processInformation->dwProcessId);
#endif

        if (std::filesystem::exists(pathToPsfRuntime))
        {
            targetDllPath = pathToPsfRuntime.c_str();
        }
        else
        {
            // Possibly the dll is in the folder with the exe and not at the package root.
            Log("\t%ls not found at package root, try target folder.", wtargetDllName.c_str());

            std::filesystem::path altPathToExeRuntime = exePath.data();
            static const auto altPathToPsfRuntime = (altPathToExeRuntime.parent_path() / pathToPsfRuntime.c_str()).string();
#if _DEBUG
            Log("\talt target filename is now %s", altPathToPsfRuntime.c_str());
#endif
            if (std::filesystem::exists(altPathToPsfRuntime))
            {
                targetDllPath = altPathToPsfRuntime.c_str();
            }
            else
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
                        if (dentry.path().filename().compare(wtargetDllName) == 0)
                        {
                            static const auto altDirPathToPsfRuntime = narrow(dentry.path().c_str());
#if _DEBUG
                            Log("\tFound match as %ls", dentry.path().c_str());
#endif
                            targetDllPath = altDirPathToPsfRuntime.c_str();
                            break;
                        }
                    }
                    catch (...)
                    {
                        psf::TraceLogExceptions("PSFRuntimeException", "Non-fatal error enumerating directories while looking for PsfRuntime");
                        Log("Non-fatal error enumerating directories while looking for PsfRuntime.");
                    }
                }

            }
        }

        if (targetDllPath != NULL)
        {
            Log("\tAttempt injection into %d using %s", processInformation->dwProcessId, targetDllPath);
            if (!::DetourUpdateProcessWithDll(processInformation->hProcess, &targetDllPath, 1))
            {
                Log("\t%s not found at target folder, try PsfRunDll.", targetDllPath);
                // We failed to detour the created process. Assume that the failure was due to an architecture mis-match
                // and try the launch using PsfRunDll
                if (!::DetourProcessViaHelperDllsW(processInformation->dwProcessId, 1, &targetDllPath, CreateProcessWithPsfRunDll))
                {
                    // Could not detour the target process, so return failure
                    auto err = ::GetLastError();
                    Log("\tUnable to inject %ls into PID=%d err=0x%x\n", targetDllPath, processInformation->dwProcessId, err);
                    ::TerminateProcess(processInformation->hProcess, ~0u);
                    ::CloseHandle(processInformation->hProcess);
                    ::CloseHandle(processInformation->hThread);

                    ::SetLastError(err);
                    return FALSE;
                }
            }
            Log("\tInjected %ls into PID=%d\n", wtargetDllName.c_str(), processInformation->dwProcessId);
        }
        else
        {
            Log("\t%ls not found, skipping.", wtargetDllName.c_str());
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
    psf::TraceLogExceptions("PSFRuntimeException", "Create Process Hook failure");
    return FALSE;
}

DECLARE_STRING_FIXUP(CreateProcessImpl, CreateProcessFixup);
