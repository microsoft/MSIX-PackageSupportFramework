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
#include "StartProcessHelper.h"
#include "Telemetry.h"
#include "PsfPowershellScriptRunner.h"
#include "Globals.h"
#include <TraceLoggingProvider.h>
#include <psf_constants.h>
#include <psf_runtime.h>
#include <wil\result.h>
#include <wil\resource.h>
#include <debug.h>
#include <shlwapi.h>
#include <WinUser.h>

TRACELOGGING_DECLARE_PROVIDER(g_Log_ETW_ComponentProvider);
TRACELOGGING_DEFINE_PROVIDER(
    g_Log_ETW_ComponentProvider,
    "Microsoft.Windows.PSFRuntime",
    (0xf7f4e8c4, 0x9981, 0x5221, 0xe6, 0xfb, 0xff, 0x9d, 0xd1, 0xcd, 0xa4, 0xe1),
    TraceLoggingOptionMicrosoftTelemetry());

using namespace std::literals;

// Forward declarations
void LogApplicationAndProcessesCollection();
int launcher_main(PCWSTR args, int cmdShow) noexcept;
void GetAndLaunchMonitor(const psf::json_object& monitor, std::filesystem::path packageRoot, int cmdShow, LPCWSTR dirStr);
void LaunchMonitorInBackground(std::filesystem::path packageRoot, const wchar_t executable[], const wchar_t arguments[], bool wait, bool asAdmin, int cmdShow, LPCWSTR dirStr);
void LaunchInBackgroundAsAdmin(const wchar_t executable[], const wchar_t arguments[], bool wait, int cmdShow, LPCWSTR dirStr);
bool IsCurrentOSRS2OrGreater();
std::wstring ReplaceVariablesInString(std::wstring inputString, bool ReplaceEnvironmentVars, bool ReplacePseudoVars);

static inline bool check_suffix_if(iwstring_view str, iwstring_view suffix) noexcept;

#if INECTIONFROMHEREREADY
// From PsfRuntime:
USHORT ProcessBitness(HANDLE hProcess);
std::wstring FixDllBitness(std::wstring originalName, USHORT bitness);
#endif


int __stdcall wWinMain(_In_ HINSTANCE , _In_opt_ HINSTANCE, _In_ PWSTR args, _In_ int cmdShow)
{ 
    int ret = launcher_main(args, cmdShow);
    
    return ret;
}

int launcher_main(PCWSTR args, int cmdShow) noexcept try
{
    Log(L"PSFLauncher started.");

    
    //Log(L"DEBUG TEMP PsfLauncher waiting for debugger to attach to process...\n");
    //psf::wait_for_debugger();

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
                Log(L"PsfLauncher waiting for debugger to attach to process...\n"); 
                psf::wait_for_debugger(); 
            } 
        } 
    } 
#endif

    LogApplicationAndProcessesCollection();

    auto dirPtr = appConfig->try_get("workingDirectory");
    auto dirStr = dirPtr ? dirPtr->as_string().wide() : L"";

    // At least for now, configured launch paths are relative to the package root
    std::filesystem::path packageRoot = PSFQueryPackageRootPath();
    std::wstring dirWstr = dirStr;
    dirWstr = ReplaceVariablesInString(dirWstr, true, true);
    std::filesystem::path currentDirectory;

    if (dirWstr.size() < 2 || dirWstr[1] != L':')
    {
        if (dirWstr.size() == 0)
            currentDirectory = packageRoot;
        else
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
    bool isHttp = false;
    if (exeWName[1] != L':')
    {
        if (_wcsnicmp(exeWName.c_str(), L"http://", 7) == 0 ||
            _wcsnicmp(exeWName.c_str(), L"https://", 8) == 0)
        {
            isHttp = true;
            exePath = exeWName;
        }
        else
        {
            exePath = packageRoot / exeWName;
        }
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
        LogString(L"Process Launch: ", exePath.c_str());
        LogString(L"     Arguments: ", fullargs.data());
        LogString(L"Working Directory: ", currentDirectory.c_str());

        HRESULT hr = StartProcess(exePath.c_str(), (L"\"" + exePath.filename().native() + L"\" " + exeArgString + L" " + args).data(), currentDirectory.c_str(), cmdShow, INFINITE);
        if (hr != ERROR_SUCCESS)
        {
            Log(L"Error return from launching process 0x%x.", GetLastError());
            LaunchInBackgroundAsAdmin(exePath.c_str(), args, true, cmdShow, currentDirectory.c_str());
        }
    }
    else
    {
        LogString(L"Shell Launch", exePath.c_str());
        LogString(L"   Arguments", exeArgString.c_str());
        LogString(L"Working Directory: ", currentDirectory.c_str());
        
        if (check_suffix_if(exeName, L".cmd"_isv) || check_suffix_if(exeName, L".bat"_isv))
        {
            Log(L"Shell Launch special case for cmd/bat files");
            // To get the cmd process that runs this script we need to start it via a powershell process that uses Invoke-CommandInDesktopPackage with the -PreventBreakaway option.
            // This is currently done by using a powershell wrapper script that is part of the PSF.
            // Why another ps1 file?  
            //    cmd files are wonky in how they work when launched from in the bubble.  Even if we try to control them by using 
            //    extended attributes.  So we use a powershell script.
            // But we also find that powershell is not happy if the launcher injects PSFRuntime into it and shuts down.  So we
            // start powershell differently, and have the powershell script use inject-commandindesktoppackage to run the cmd script
            // inside of our package container.
            //
            // There is an issue that inject-cidp is asynchronous and returns before the script is run.  So our Launcher will run any
            // end scripts and shut down, possibly before the cmd ends.  We probably aren't going to see any end scripts on a shortcut to
            // a cmd script, and the launcher shutting down isn't noticable since it is hidden and causes no problem.
            std::filesystem::path SSCmdWrapper = packageRoot / L"StartMenuCmdScriptWrapper.ps1";
            if (!std::filesystem::exists(SSCmdWrapper))
            {
                // The wrapper isn't in this folder, so we should search for it elewhere in the package.
                for (const auto& file : std::filesystem::recursive_directory_iterator(packageRoot))
                {
                    if (file.path().filename().compare(SSCmdWrapper.filename()) == 0)
                    {
                        SSCmdWrapper = file.path();
                        break;
                    }
                }
            }

            //std::wstring wArgs = L"powershell.exe -ExecutionPolicy Bypass";
            std::wstring wArgs = L"powershell.exe";

            wArgs.append(L" -File \"");
            wArgs.append(SSCmdWrapper.c_str());
            wArgs.append(L"\" ");

            wArgs.append(L" ");
            ///wArgs.append(L"-PackageFamilyName ");
            wArgs.append(PSFQueryPackageFamilyName());
            wArgs.append(L" ");

            wArgs.append(L" ");
            ///wArgs.append(L"-AppID ");
            wArgs.append(PSFQueryApplicationId());
            wArgs.append(L" ");

            wArgs.append(L" \"");
            wArgs.append(exePath.c_str());
            wArgs.append(L"\" ");

            wArgs.append(exeArgString);

            powershellScriptRunner.RunOtherScript(SSCmdWrapper.c_str(), currentDirectory.c_str(), exePath.c_str(), exeArgString.c_str(), false);
        }
        else
        {
            // Previously we had issues with this: StartWithShellExecute(nullptr, packageRoot, exePath, exeArgString, currentDirectory.c_str(), cmdShow, INFINITE);
            std::wstring ext = exePath.extension().c_str();
            if (isHttp)
            {
                // Let the default browser handle it.
                ext = L".html";
            }
            Log(L"Looking for default command for FTA %ls", ext.c_str());
            wchar_t szBuf[1024];
            DWORD cbBufSize = sizeof(szBuf);

            cbBufSize = sizeof(szBuf);  // resetting for second call...
            HRESULT hr = AssocQueryString(0, ASSOCSTR_EXECUTABLE,
                ext.c_str(), NULL, szBuf, &cbBufSize);
            if (FAILED(hr))
            {
                Log(L"Failed to get an FTA default command 0x0x", GetLastError());
                StartWithShellExecute(nullptr, packageRoot, exePath, exeArgString, currentDirectory.c_str(), cmdShow, INFINITE);
            }
            else
            {
                // We are going to assume this FTA takes the file as an unadorned argument.  Not perfect but should cover most of our cases.
                // In a pinch, one could add command line arguments to the json.  
                //      So the jason says executable: file.ext  and Arguments: /xxx
                //      We construct a command that is:    DefaultExeForFTA.exe /xxx  file.ext
                // Note: If there is no FTA, the query returns with OpenWith.exe, which means the user will be prompted with what they want to do.  
                //       That is probably the best we can do.
                Log(L"Default command for FTA is %ls, use StartMenuShellLaunchWrapperScript.ps1 to inject into container, if possible.", szBuf);
                std::wstring newcmd = szBuf;

                std::filesystem::path SSShellWrapper = packageRoot / L"StartMenuShellLaunchWrapperScript.ps1";
                if (!std::filesystem::exists(SSShellWrapper))
                {
                    // The wrapper isn't in this folder, so we should search for it elewhere in the package.
                    for (const auto& file : std::filesystem::recursive_directory_iterator(packageRoot))
                    {
                        if (file.path().filename().compare(SSShellWrapper.filename()) == 0)
                        {
                            SSShellWrapper = file.path();
                            break;
                        }
                    }
                }

                std::wstring wArgs = args;
                wArgs.append(L" \"");
                wArgs.append(exePath.c_str());
                wArgs.append(L"\"");
                powershellScriptRunner.RunOtherScript(SSShellWrapper.c_str(), currentDirectory.c_str(), newcmd.c_str(), wArgs.c_str(), false);
            }
        }
    }

    if (IsCurrentOSRS2OrGreater())
    {
        Log(L"Process Launch Ready to run any end scripts.");
        // Launch the end PowerShell script if we are using one.
        powershellScriptRunner.RunEndingScript();
        Log(L"Process Launch complete.");
    }

    return 0;
}
catch (...)
{
    ::PSFReportError(widen(message_from_caught_exception()).c_str());
    return win32_from_caught_exception();
}

void GetAndLaunchMonitor(const psf::json_object& monitor, std::filesystem::path packageRoot, int cmdShow, LPCWSTR dirStr)
{
    bool asAdmin = false;
    bool wait = false;
    auto monitorExecutable = monitor.try_get("executable");
    auto monitorArguments = monitor.try_get("arguments");
    auto monitorAsAdmin = monitor.try_get("asadmin");
    auto monitorWait = monitor.try_get("wait");
    if (monitorAsAdmin)
    {
        asAdmin = monitorAsAdmin->as_boolean().get();
    }

    if (monitorWait)
    {
        wait = monitorWait->as_boolean().get();
    }

    Log(L"\tCreating the monitor: %ls", monitorExecutable->as_string().wide());
    LaunchMonitorInBackground(packageRoot, monitorExecutable->as_string().wide(), monitorArguments->as_string().wide(), wait, asAdmin, cmdShow, dirStr);
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

void LaunchInBackgroundAsAdmin( const wchar_t executable[], const wchar_t arguments[], bool wait, int cmdShow, LPCWSTR dirStr)
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
#if INECTIONFROMHEREREADY
    try
    {
        BOOL b;
        BOOL res = IsProcessInJob(shExInfo.hProcess, nullptr, &b);
        if (res != 0)
        {
            if (b == true)
            {
                Log(L"\tPsfLaunch: New process is in a job, allow injection.");
                // Fix for issue #167: allow subprocess to be a different bitness than this process.
                USHORT bitness = ProcessBitness(shExInfo.hProcess);
                DWORD procId = GetProcessId(shExInfo.hProcess);
                std::filesystem::path packageRoot = PSFQueryPackageRootPath();
#if _DEBUG
                Log(L"\tPsfLaunch: Injection for PID=%d Bitness=%d", procId, bitness);
#endif  
                std::wstring wtargetDllName = FixDllBitness(std::wstring(psf::runtime_dll_name), bitness);
#if _DEBUG
                Log(L"\tPsfLaunch: Use runtime %ls", wtargetDllName.c_str());
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
                    Log(L"\tPsfLaunch: %ls not found at package root, try target folder.", wtargetDllName.c_str());

                    std::filesystem::path altPathToExeRuntime = executable;
                    static const auto altPathToPsfRuntime = (altPathToExeRuntime.parent_path() / pathToPsfRuntime.c_str()).string();
#if _DEBUG
                    Log(L"\tPsfLaunch: alt target filename is now %ls", altPathToPsfRuntime.c_str());
#endif
                    if (std::filesystem::exists(altPathToPsfRuntime))
                    {
                        targetDllPath = altPathToPsfRuntime.c_str();
                    }
                    else
                    {
#if _DEBUG
                        Log(L"\tPsfLaunch: Not present there either, try elsewhere in package.");
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
                                    Log(L"\tPsfLaunch: Found match as %ls", dentry.path().c_str());
#endif
                                    targetDllPath = altDirPathToPsfRuntime.c_str();
                                    break;
                                }
                            }
                            catch (...)
                            {
                                Log(L"\tPsfLaunch: Non-fatal error enumerating directories while looking for PsfRuntime.");
                            }
                        }

                    }
                }

                if (targetDllPath != NULL)
                {
                    Log("\tPsfLaunch: Attempt injection into %d using %s", procId, targetDllPath);
                    if (!::DetourUpdateProcessWithDll(processInformation->hProcess, &targetDllPath, 1))
                    {
                        Log("\tPsfLaunch: %s unable to inject, err=0x%x.", targetDllPath, ::GetLastError());
                        if (!::DetourProcessViaHelperDllsW(processInformation->dwProcessId, 1, &targetDllPath, CreateProcessWithPsfRunDll))
                        {
                            Log("\tPsfLaunch: %s unable to inject with RunDll either (Skipping), err=0x%x.", targetDllPath, ::GetLastError());
                        }
                    }
                    else
                    {
                        Log(L"\tPsfLaunch: Injected %ls into PID=%d\n", wtargetDllName.c_str(), processInformation->dwProcessId);
                    }
                }
                else
                {
                    Log(L"\tPsfLaunch: %ls not found, skipping.", wtargetDllName.c_str());
                }
            }
        }
    }
    catch (...)
    {
#if _DEBUG
        Log(L"\tPsfLaunch: Exception while trying to determine job status of new process. Do not inject.");
#endif

    }
#endif


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

void LogApplicationAndProcessesCollection()
{
    auto configRoot = PSFQueryConfigRoot();

    if (auto applications = configRoot->as_object().try_get("applications"))
    {
        for (auto& applicationsConfig : applications->as_array())
        {
            auto exeStr = applicationsConfig.as_object().try_get("executable")->as_string().wide();
            auto idStr = applicationsConfig.as_object().try_get("id")->as_string().wide();
            TraceLoggingWrite(
                g_Log_ETW_ComponentProvider,
                "ApplicationsConfigdata",
                TraceLoggingWideString(exeStr, "applications_executable"),
                TraceLoggingWideString(idStr, "applications_id"),
                TraceLoggingBoolean(TRUE, "UTCReplace_AppSessionGuid"),
                TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
        }
    }

    if (auto processes = configRoot->as_object().try_get("processes"))
    {
        for (auto& processConfig : processes->as_array())
        {
            auto exeStr = processConfig.as_object().get("executable").as_string().wide();
            TraceLoggingWrite(
                g_Log_ETW_ComponentProvider,
                "ProcessesExecutableConfigdata",
                TraceLoggingWideString(exeStr, "processes_executable"),
                TraceLoggingBoolean(TRUE, "UTCReplace_AppSessionGuid"),
                TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));

            if (auto fixups = processConfig.as_object().try_get("fixups"))
            {
                for (auto& fixupConfig : fixups->as_array())
                {
                    auto dllStr = fixupConfig.as_object().try_get("dll")->as_string().wide();
                    TraceLoggingWrite(
                        g_Log_ETW_ComponentProvider,
                        "ProcessesFixUpConfigdata",
                        TraceLoggingWideString(dllStr, "processes_fixups"),
                        TraceLoggingBoolean(TRUE, "UTCReplace_AppSessionGuid"),
                        TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage),
                        TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
                }
            }
        }
    }
}

bool IsCurrentOSRS2OrGreater()
{
    OSVERSIONINFOEXW osvi = { sizeof(osvi), 0, 0, 0, 0, {0}, 0, 0 };
    DWORDLONG const dwlConditionMask = VerSetConditionMask(0, VER_BUILDNUMBER, VER_GREATER_EQUAL);
    osvi.dwBuildNumber = 15063;

    return VerifyVersionInfoW(&osvi, VER_BUILDNUMBER, dwlConditionMask);
}
