//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>
#include <windows.h>
#include <shellapi.h>
#include <combaseapi.h>
#include <ppltasks.h>
#include <ShObjIdl.h>
#include "Logger.h"
#include "StartProcessHelper.h"
#include "Telemetry.h"
#include "PsfPowershellScriptRunner.h"
#include <TraceLoggingProvider.h>
#include "psf_tracelogging.h"
#include <psf_constants.h>
#include <psf_runtime.h>
#include <wil\result.h>
#include <wil\resource.h>
#include <debug.h>
#include <string_view>
#include <vector>

#include <windows.h>
#include <detours.h>
#include <psf_constants.h>
#include <psf_framework.h>
#include "psf_tracelogging.h"

TRACELOGGING_DECLARE_PROVIDER(g_Log_ETW_ComponentProvider);
TRACELOGGING_DEFINE_PROVIDER(
    g_Log_ETW_ComponentProvider,
    "Microsoft.Windows.PSFRuntime",
    (0xf7f4e8c4, 0x9981, 0x5221, 0xe6, 0xfb, 0xff, 0x9d, 0xd1, 0xcd, 0xa4, 0xe1),
    TraceLoggingOptionMicrosoftTelemetry());

using namespace std::literals;

LARGE_INTEGER psfLoad_startCounter;

// Forward declarations
void LogApplicationAndProcessesCollection();
int launcher_main(PCWSTR args, int cmdShow) noexcept;
void GetAndLaunchMonitor(const psf::json_object& monitor, std::filesystem::path packageRoot, int cmdShow, LPCWSTR dirStr);
void LaunchInBackgroundAsAdmin(const wchar_t executable[], const wchar_t arguments[], bool wait, int cmdShow, LPCWSTR dirStr); 
void LaunchMonitorInBackground(std::filesystem::path packageRoot, const wchar_t executable[], const wchar_t arguments[], bool wait, bool asAdmin, int cmdShow, LPCWSTR dirStr);
bool IsCurrentOSRS2OrGreater();
std::wstring FixupDllBitness(std::wstring originalName, USHORT bitness);
std::wstring ReplaceVariablesInString(std::wstring inputString, bool ReplaceEnvironmentVars, bool ReplacePseudoVars);

static inline bool check_suffix_if(iwstring_view str, iwstring_view suffix) noexcept;

int __stdcall wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ PWSTR args, _In_ int cmdShow)
{    
    QueryPerformanceCounter(&psfLoad_startCounter);
    return launcher_main(args, cmdShow);
}

int launcher_main(PCWSTR args, int cmdShow) noexcept try
{
    Log("\tIn Launcher_main()");

    TraceLoggingRegister(g_Log_ETW_ComponentProvider);
    auto appConfig = PSFQueryCurrentAppLaunchConfig(true);
    THROW_HR_IF_MSG(ERROR_NOT_FOUND, !appConfig, "Error: could not find matching appid in config.json and appx manifest");

#ifdef _DEBUG 
    if (appConfig) 
    { 
        auto waitSignalPtr = appConfig->try_get("waitForDebugger"); 
        if (waitSignalPtr) 
        { 
            bool waitSignal = waitSignalPtr->as_boolean().get(); 
            if (waitSignal) 
            { 
                Log("PsfLauncher waiting for debugger to attach to process...\n"); 
                psf::wait_for_debugger(); 
            } 
        } 
    } 
#endif

    std::string currentExeFixes;
    psf::TraceLogApplicationsConfigdata(currentExeFixes);

    auto dirPtr = appConfig->try_get("workingDirectory");
    auto dirStr = dirPtr ? dirPtr->as_string().wide() : L"";

    // At least for now, configured launch paths are relative to the package root
    std::filesystem::path packageRoot = PSFQueryPackageRootPath();
    std::wstring dirWstr = dirStr;
    dirWstr = ReplaceVariablesInString(dirWstr, true, true);
    std::filesystem::path currentDirectory;

    if (dirWstr.size() < 2 || dirWstr[1] != L':')
    {
        currentDirectory = (packageRoot / dirWstr);
    }
    else
    {
        currentDirectory = dirWstr;
    }

    PsfPowershellScriptRunner powershellScriptRunner;

    if (IsCurrentOSRS2OrGreater())
    {
        powershellScriptRunner.Initialize(appConfig, currentDirectory, packageRoot);

        // Launch the starting PowerShell script if we are using one.
        powershellScriptRunner.RunStartingScript();
    }

    // Launch monitor if we are using one.
    auto monitor = PSFQueryAppMonitorConfig();
    if (monitor != nullptr)
    {
        THROW_IF_FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE));
        GetAndLaunchMonitor(*monitor, packageRoot, cmdShow, dirStr);
    }

    // Launch underlying application.
    auto exeName = appConfig->get("executable").as_string().wide();
    std::wstring exeWName = exeName;
    exeWName = ReplaceVariablesInString(exeWName, true, true);
    std::filesystem::path exePath;
    if (exeWName[1] != L':')
    {
        exePath = packageRoot / exeWName;
    }
    else
    {
        exePath = exeWName;
    }
    
    auto exeArgs = appConfig->try_get("arguments"); 
    std::wstring exeArgString = exeArgs ? exeArgs->as_string().wide() : (wchar_t*)L"";
    exeArgString = ReplaceVariablesInString(exeArgString, true, true);

    // Keep these quotes here.  StartProcess assumes there are quotes around the exe file name
    if (check_suffix_if(exeName, L".exe"_isv))
    {
        std::wstring fullargs;
        if (!exeArgString.empty())
        {
            fullargs = (exeArgString + L" " + args);
        }
        else
        {
            fullargs = args;
        }
        LogString("Process Launch: ", exePath.c_str());
        LogString("     Arguments: ", fullargs.data());
        LogString("Working Directory: ", currentDirectory.c_str());

        HRESULT hr = S_OK;
        bool createProcessesInAppContext = false;
        auto createProcessesInAppContextPtr = appConfig->try_get("inPackageContext");
        ProcThreadAttributeList m_AttributeList;
        if (createProcessesInAppContextPtr)
        {
            createProcessesInAppContext = createProcessesInAppContextPtr->as_boolean().get();
        }
        if (createProcessesInAppContext)
        {
            hr = StartProcess(exePath.c_str(), (L"\"" + exePath.filename().native() + L"\" " + exeArgString + L" " + args).data(), (packageRoot / dirStr).c_str(), cmdShow, INFINITE, m_AttributeList.get());
        }
        else
        {
            hr = StartProcess(exePath.c_str(), (L"\"" + exePath.filename().native() + L"\" " + exeArgString + L" " + args).data(), (packageRoot / dirStr).c_str(), cmdShow, INFINITE);
        }
        if (hr != ERROR_SUCCESS)
        {
            Log("Error return from launching process second try, try again 0x%x.", GetLastError());
            LaunchInBackgroundAsAdmin(exePath.c_str(), args, true, cmdShow, currentDirectory.c_str());
        }
    }
    else
    {
        LogString("Shell Launch", exePath.c_str());
        LogString("   Arguments", exeArgString.c_str());
        LogString("Working Directory: ", currentDirectory.c_str());
        StartWithShellExecute(packageRoot, exePath, exeArgString, currentDirectory.c_str(), cmdShow, INFINITE);
    }

    if (IsCurrentOSRS2OrGreater())
    {
        Log("Process Launch Ready to run any end scripts.");
        // Launch the end PowerShell script if we are using one.
        powershellScriptRunner.RunEndingScript();
        Log("Process Launch complete.");
    }
    LARGE_INTEGER psfLoad_endCounter, frequency;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&psfLoad_endCounter);
    double elapsedTime = (psfLoad_endCounter.QuadPart - psfLoad_startCounter.QuadPart) / (double)frequency.QuadPart;

    psf::TraceLogPerformance(currentExeFixes,elapsedTime);

    TraceLoggingUnregister(g_Log_ETW_ComponentProvider);
    return 0;
}
catch (...)
{
    ::PSFReportError(widen(message_from_caught_exception()).c_str());
    psf::TraceLogExceptions("PSFLauncherException", widen(message_from_caught_exception()).c_str());;
    return win32_from_caught_exception();
}




void GetAndLaunchMonitor(const psf::json_object& monitor, std::filesystem::path packageRoot, int cmdShow, LPCWSTR dirStr)
{
    std::wstringstream traceDataStream;
    traceDataStream << " config:\n";
    bool asAdmin = false;
    bool wait = false;
    auto monitorExecutable = monitor.try_get("executable");
    traceDataStream << " executable: " << monitorExecutable->as_string().wide() << " ;";
    auto monitorArguments = monitor.try_get("arguments");
    traceDataStream << " arguments: " << monitorArguments->as_string().wide() << " ;";
    auto monitorAsAdmin = monitor.try_get("asadmin");
    auto monitorWait = monitor.try_get("wait");
    if (monitorAsAdmin)
    {
        asAdmin = monitorAsAdmin->as_boolean().get();
    }
    traceDataStream << " asadmin: " << (asAdmin ? "true" : "false") << " ;";

    if (monitorWait)
    {
        wait = monitorWait->as_boolean().get();
    }
    traceDataStream << " wait: " << (wait ? "true" : "false") << " ;";

    psf::TraceLogPSFMonitorConfigData(traceDataStream.str().c_str());
    Log("\tCreating the monitor: %ls", monitorExecutable->as_string().wide());
    LaunchMonitorInBackground(packageRoot, monitorExecutable->as_string().wide(), monitorArguments->as_string().wide(), wait, asAdmin, cmdShow, dirStr);
}

void LaunchInBackgroundAsAdmin(const wchar_t executable[], const wchar_t arguments[], bool wait, int cmdShow, LPCWSTR dirStr)
{
    std::wstring cmd = L"\"";
    cmd.append(executable);
    cmd.append(L"\"");


    // This happens when the program is requested for elevation.  For now just launch this way.  We don't get to inject,
    // but it is better than nothing.
    SHELLEXECUTEINFOW shExInfo =
    {
        sizeof(shExInfo) // bSize
        , wait ? (ULONG)SEE_MASK_NOCLOSEPROCESS : (ULONG)(SEE_MASK_NOCLOSEPROCESS | SEE_MASK_WAITFORINPUTIDLE) // fmask
        , 0           // hwnd
        , L"runas"    // lpVerb
        , cmd.c_str() // lpFile
        , arguments   // lpParameters
        , dirStr     // lpDirectory
        , cmdShow           // nShow
        , 0           // hInstApp
    };
    Log("PsfLaunch: Unable to launch with injection (possibly due to elevation).  Launch using shell launch without injection.");
    THROW_LAST_ERROR_IF_MSG(!ShellExecuteEx(&shExInfo), "Error starting monitor using ShellExecuteEx");
    THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE), shExInfo.hProcess == INVALID_HANDLE_VALUE);

    // possibly inject the runtime now?
    try
    {
        BOOL b;
        BOOL res = IsProcessInJob(shExInfo.hProcess, nullptr, &b);
        if (res != 0)
        {
            if (b == TRUE)
            {
                Log("\tPsfLaunch: New process is in a job, allow injection.");
                // Fix for issue #167: allow subprocess to be a different bitness than this process.
                USHORT bitness = ProcessBitness(shExInfo.hProcess);
                DWORD procId = GetProcessId(shExInfo.hProcess);
                std::filesystem::path packageRoot = PSFQueryPackageRootPath();
#if _DEBUG
                Log("\tPsfLaunch: Injection for PID=%d Bitness=%d", procId, bitness);
#endif  
                std::wstring wtargetDllName = FixupDllBitness(std::wstring(psf::runtime_dll_name), bitness);
#if _DEBUG
                Log("\tPsfLaunch: Use runtime %ls", wtargetDllName.c_str());
#endif
                static const auto pathToPsfRuntime = (packageRoot / wtargetDllName.c_str()).string();
                const char* targetDllPath = NULL;
#if _DEBUG
                Log("\tPsfLaunch: Inject %s into PID=%d", pathToPsfRuntime.c_str(), procId);
#endif

                if (std::filesystem::exists(pathToPsfRuntime))
                {
                    targetDllPath = pathToPsfRuntime.c_str();
                }
                else
                {
                    // Possibly the dll is in the folder with the exe and not at the package root.
                    Log("\tPsfLaunch: %ls not found at package root, try target folder.", wtargetDllName.c_str());

                    std::filesystem::path altPathToExeRuntime = executable;
                    static const auto altPathToPsfRuntime = (altPathToExeRuntime.parent_path() / pathToPsfRuntime.c_str()).string();
#if _DEBUG
                    Log("\tPsfLaunch: alt target filename is now %ls", altPathToPsfRuntime.c_str());
#endif
                    if (std::filesystem::exists(altPathToPsfRuntime))
                    {
                        targetDllPath = altPathToPsfRuntime.c_str();
                    }
                    else
                    {
#if _DEBUG
                        Log("\tPsfLaunch: Not present there either, try elsewhere in package.");
#endif
                        // If not in those two locations, must check everywhere in package.
                        // The child process might also be in another package folder, so look elsewhere in the package.
                        for (auto& dentry : std::filesystem::recursive_directory_iterator(packageRoot))
                        {
                            try
                            {
                                if (dentry.path().filename().compare(wtargetDllName) == 0)
                                {
                                    static const auto altDirPathToPsfRuntime = narrow(dentry.path().c_str());
#if _DEBUG
                                    Log("\tPsfLaunch: Found match as %ls", dentry.path().c_str());
#endif
                                    targetDllPath = altDirPathToPsfRuntime.c_str();
                                    break;
                                }
                            }
                            catch (...)
                            {
                                Log("\tPsfLaunch: Non-fatal error enumerating directories while looking for PsfRuntime.");
                            }
                        }

                    }
                }

                if (targetDllPath != NULL)
                {
                    Log("\tPsfLaunch: Attempt injection into %d using %s", procId, targetDllPath);
                    if (!::DetourUpdateProcessWithDll(shExInfo.hProcess, &targetDllPath, 1))
                    {
                        Log("\tPsfLaunch: %s unable to inject, err=0x%x.", targetDllPath, ::GetLastError());
                        if (!::DetourProcessViaHelperDllsW(procId, 1, &targetDllPath, CreateProcessWithPsfRunDll))
                        {
                            Log("\tPsfLaunch: %s unable to inject with RunDll either (Skipping), err=0x%x.", targetDllPath, ::GetLastError());
                        }
                    }
                    else
                    {
                        Log("\tPsfLaunch: Injected %ls into PID=%d\n", wtargetDllName.c_str(), procId);
                    }
                }
                else
                {
                    Log("\tPsfLaunch: %ls not found, skipping.", wtargetDllName.c_str());
                }
            }
        }
    }
    catch (...)
    {
#if _DEBUG
        Log("\tPsfLaunch: Exception while trying to determine job status of new process. Do not inject.");
#endif

    }


    if (wait)
    {
        WaitForSingleObject(shExInfo.hProcess, INFINITE);
        CloseHandle(shExInfo.hProcess);
    }
    else
    {
        WaitForInputIdle(shExInfo.hProcess, 1000);
        // Due to elevation, the process starts, relaunches, and the main process ends in under 1ms.
        // So we'll just toss in an ugly sleep here for now.
        Sleep(5000);
    }

}

std::wstring FixupDllBitness(std::wstring originalName, USHORT bitness)
{
    std::wstring targetDll = originalName.substr(0, originalName.length() - 6);
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

void LaunchMonitorInBackground(std::filesystem::path packageRoot, const wchar_t executable[], const wchar_t arguments[], bool wait, bool asAdmin, int cmdShow, LPCWSTR dirStr)
{
    std::wstring cmd = L"\"" + (packageRoot / executable).native() + L"\"";

    if (asAdmin)
    {
        // This happens when the program is requested for elevation.
        SHELLEXECUTEINFOW shExInfo =
        {
            sizeof(shExInfo) // bSize
            , wait ? (ULONG)SEE_MASK_NOCLOSEPROCESS : (ULONG)(SEE_MASK_NOCLOSEPROCESS | SEE_MASK_WAITFORINPUTIDLE) // fmask
            , 0           // hwnd
            , L"runas"    // lpVerb
            , cmd.c_str() // lpFile
            , arguments   // lpParameters
            , nullptr     // lpDirectory
            , 1           // nShow
            , 0           // hInstApp
        };

        THROW_LAST_ERROR_IF_MSG(!ShellExecuteEx(&shExInfo), "Error starting monitor using ShellExecuteEx");
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE), shExInfo.hProcess == INVALID_HANDLE_VALUE);

        if (wait)
        {
            WaitForSingleObject(shExInfo.hProcess, INFINITE);
            CloseHandle(shExInfo.hProcess);
        }
        else
        {
            WaitForInputIdle(shExInfo.hProcess, 1000);
            // Due to elevation, the process starts, relaunches, and the main process ends in under 1ms.
            // So we'll just toss in an ugly sleep here for now.
            Sleep(5000);
        }

        // Should not kill the intended app because the monitor elevated.
        //DWORD exitCode{};
        //THROW_LAST_ERROR_IF_MSG(!GetExitCodeProcess(shExInfo.hProcess, &exitCode), "Could not get error for process");
        //THROW_IF_WIN32_ERROR(exitCode);
    }
    else
    {
        THROW_IF_FAILED(StartProcess(executable, (cmd + L" " + arguments).data(), (packageRoot / dirStr).c_str(), cmdShow, INFINITE));
    }
}


// Replace all occurrences of requested environment and/or pseudo-environment variables in a string.
std::wstring ReplaceVariablesInString(std::wstring inputString, bool ReplaceEnvironmentVars, bool ReplacePseudoVars)
{
    std::wstring outputString = inputString;
    if (ReplacePseudoVars)
    {
        std::wstring::size_type pos = 0u;
        std::wstring var2rep = L"%MsixPackageRoot%";
        std::wstring repargs = PSFQueryPackageRootPath();
        while ((pos = outputString.find(var2rep, pos)) != std::string::npos) {
            outputString.replace(pos, var2rep.length(), repargs);
            pos += repargs.length();
        }

        pos = 0u;
        var2rep = L"%MsixWritablePackageRoot%";
        std::filesystem::path writablePackageRootPath = psf::known_folder(FOLDERID_LocalAppData) / std::filesystem::path(L"Packages") / psf::current_package_family_name() / LR"(LocalCache\Local\Microsoft\WritablePackageRoot)";
        repargs = writablePackageRootPath.c_str();
        while ((pos = outputString.find(var2rep, pos)) != std::string::npos) {
            outputString.replace(pos, var2rep.length(), repargs);
            pos += repargs.length();
        }
    }
    if (ReplaceEnvironmentVars)
    {
        // Potentially an environment variable that needs replacing. For Example: "%HomeDir%\\Documents"
        DWORD nSizeBuff = 256;
        LPWSTR buff = new wchar_t[nSizeBuff];
        DWORD nSizeRet = ExpandEnvironmentStrings(outputString.c_str(), buff, nSizeBuff);
        if (nSizeRet > 0)
        {
            outputString = std::wstring(buff);
        }

    }
    return outputString;
}


static inline bool check_suffix_if(iwstring_view str, iwstring_view suffix) noexcept
{
    return ((str.length() >= suffix.length()) && (str.substr(str.length() - suffix.length()) == suffix));
}

bool IsCurrentOSRS2OrGreater()
{
    OSVERSIONINFOEXW osvi = { sizeof(osvi), 0, 0, 0, 0, {0}, 0, 0 };
    DWORDLONG const dwlConditionMask = VerSetConditionMask(0, VER_BUILDNUMBER, VER_GREATER_EQUAL);
    osvi.dwBuildNumber = 15063;

    return VerifyVersionInfoW(&osvi, VER_BUILDNUMBER, dwlConditionMask);
}
