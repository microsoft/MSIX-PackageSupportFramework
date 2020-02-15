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
#include <psf_constants.h>
#include <psf_runtime.h>
#include <wil\result.h>
#include <wil\resource.h>

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
bool LaunchViaCreateProcessInContainer(const std::wstring app, const std::wstring fullCmd, const std::wstring dir, bool cmdShow);
void GetAndLaunchMonitor(const psf::json_object &monitor, std::filesystem::path packageRoot, int cmdShow, LPCWSTR dirStr);
void LaunchMonitorInBackground(std::filesystem::path packageRoot, const wchar_t executable[], const wchar_t arguments[], bool wait, bool asAdmin, int cmdShow, LPCWSTR dirStr);

static inline bool check_suffix_if(iwstring_view str, iwstring_view suffix) noexcept;

int __stdcall wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ PWSTR args, _In_ int cmdShow)
{
    return launcher_main(args, cmdShow);
}

int launcher_main(PCWSTR args, int cmdShow) noexcept try
{
    Log("\tIn Launcher_main()");
    auto appConfig = PSFQueryCurrentAppLaunchConfig(true);
    THROW_HR_IF_MSG(ERROR_NOT_FOUND, !appConfig, "Error: could not find matching appid in config.json and appx manifest");

    LogApplicationAndProcessesCollection();

    auto dirPtr = appConfig->try_get("workingDirectory");
    auto dirStr = dirPtr ? dirPtr->as_string().wide() : L"";
    auto exeArgs = appConfig->try_get("arguments");

    // At least for now, configured launch paths are relative to the package root
    std::filesystem::path packageRoot = PSFQueryPackageRootPath();
	auto currentDirectory = (packageRoot / dirStr);

	PsfPowershellScriptRunner powershellScriptRunner;
	powershellScriptRunner.Initialize(appConfig, currentDirectory);

    // Launch the starting PowerShell script if we are using one.
	powershellScriptRunner.RunStartingScript();

    // Launch monitor if we are using one.
    auto monitor = PSFQueryAppMonitorConfig();
    if (monitor != nullptr)
    {        
        THROW_IF_FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE));
        GetAndLaunchMonitor(*monitor, packageRoot, cmdShow, dirStr);
    }

    // Launch underlying application.
    auto exeName = appConfig->get("executable").as_string().wide();
    auto exePath = packageRoot / exeName;
    std::wstring exeArgString = exeArgs ? exeArgs->as_string().wide() : (wchar_t*)L"";
    
    //Allow for a substitution in the arguments for a new pseudo variable %PackageRoot% so that arguments can point to files
    //inside the package using a syntax relative to the package root rather than rely on VFS pathing which can't kick in yet.
    std::wstring::size_type pos = 0u;
    std::wstring var2rep = L"%MsixPackageRoot%";
    std::wstring repargs = packageRoot.c_str();
    while ((pos = exeArgString.find(var2rep, pos)) != std::string::npos) {
        exeArgString.replace(pos, var2rep.length(), repargs);
        pos += repargs.length();
    }

    // Keep these quotes here.  StartProcess assumes there are quotes around the exe file name
    if (check_suffix_if(exeName, L".exe"_isv))
    {
        std::wstring fullargs = (L"\"" + exePath.filename().native() + L"\" " + exeArgString + L" " + args); 
        LogString("Process Launch: ", fullargs.data());
        StartProcess(exePath.c_str(), fullargs.data(), (packageRoot / dirStr).c_str(), cmdShow, false, INFINITE);
    }
    else if (check_suffix_if(exeName, L".cmd"_isv) ||
             check_suffix_if(exeName, L".bat"_isv)    )
    {
        // These don't work through shell launch, so patch it up here.
        std::wstring cmdexe = L"cmd.exe";
        std::wstring cmdargs = (L"cmd.exe /c \"" + exePath.filename().native() + L"\" " + exeArgString + L" " + args);
        bool UsedCreateProcess = false;
        UsedCreateProcess = LaunchViaCreateProcessInContainer(cmdexe, cmdargs, (packageRoot / dirStr), cmdShow);
        if (!UsedCreateProcess)
        {
            LogString("Process Launch via CMD: ", repargs.data());
            StartProcess(cmdexe.c_str(), cmdargs.data(), (packageRoot / dirStr).c_str(), cmdShow, false, INFINITE);
        }
    }
    else if (check_suffix_if(exeName, L".ps1"_isv))
    {
        // These don't work through shell launch, so patch it up here.
        std::wstring cmdexe = L"powershell.exe";
        std::wstring cmdargs = (L"powershell.exe -executionpolicy ByPass -file \"" + exePath.filename().native() + L"\" " + exeArgString + L" " + args);
        bool UsedCreateProcess = false;
        UsedCreateProcess = LaunchViaCreateProcessInContainer(cmdexe, cmdargs, (packageRoot / dirStr), cmdShow);
        if (!UsedCreateProcess)
        {
            LogString("Process Launch via powershell: ", repargs.data());
            StartProcess(cmdexe.c_str(), cmdargs.data(), (packageRoot / dirStr).c_str(), cmdShow, false, INFINITE);
        }
    }
    else
    {
        LogString("Shell Launch", exeName);
        LogString("   Arguments", exeArgString.c_str());
        StartWithShellExecute(packageRoot, exeName, exeArgString.c_str(), dirStr, cmdShow);
    }

    Log("Process Launch Ready to run any end scripts.");
    // Launch the end PowerShell script if we are using one.
	powershellScriptRunner.RunEndingScript();
    Log("Process Launch complete.");
    return 0;
}
catch (...)
{
    ::PSFReportError(widen(message_from_caught_exception()).c_str());
    return win32_from_caught_exception();
}

bool LaunchViaCreateProcessInContainer(std::wstring app, std::wstring fullCmd, std::wstring dir, bool cmdShow)
{
    // An attempt is made to get the new process to run inside the container.
    // Currently the attempt launches outside the container, but unless the script
    // needed access to container registry, for example, should be OK.  
    // Perhaps this can get more attention at a later time.

    bool bRet = false;
    Log("CreateProcess needed:");
    DWORD ProtectionLevel = PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_DISABLE_PROCESS_TREE; // PROTECTION_LEVEL_SAME;
    SIZE_T AttributeListSize;
    PROCESS_INFORMATION ProcessInformation = { 0 };
    STARTUPINFOEXW StartupInfoEx = { 0 };
    StartupInfoEx.StartupInfo.cb = sizeof(StartupInfoEx);
    InitializeProcThreadAttributeList(NULL, 1, 0, &AttributeListSize);
    StartupInfoEx.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), 0, AttributeListSize);
    if (InitializeProcThreadAttributeList(StartupInfoEx.lpAttributeList, 1, 0, &AttributeListSize) == FALSE)
    {
        Log("Unable to initialize thread attribute list for CreateProcess.");
    }
    else
    {
        if (UpdateProcThreadAttribute(StartupInfoEx.lpAttributeList, 0,
            PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY, //PROC_THREAD_ATTRIBUTE_PROTECTION_LEVEL,
            &ProtectionLevel, sizeof(ProtectionLevel),
            NULL, NULL) == FALSE)
        {
            Log("Unable to update thread attribute list for CreateProcess.");
        }
        else
        {
            DWORD CreationFlag = EXTENDED_STARTUPINFO_PRESENT; // EXTENDED_STARTUPINFO_PRESENT | CREATE_PROTECTED_PROCESS;
            if (!cmdShow)
                CreationFlag |= CREATE_NO_WINDOW;
#ifdef UseCreateProcess
            // This is what the script runner is doing
            LogString("CreateProcess used for CMD: ", fullCmd.data());
            if (CreateProcessW(app.c_str(),
                fullCmd.data(),
                NULL,
                NULL,
                false,
                CreationFlag,
                NULL,
                dir.c_str(),
                (LPSTARTUPINFOW)&StartupInfoEx,
                &ProcessInformation) == TRUE)
            {
                bRet = true;
                WaitForSingleObject(ProcessInformation.hProcess, INFINITE);
            }
            else
            {
                Log("CreateProcess failure.");
            }
#else
            // This was suggested as a replacement
            LogString("CreateProcessAsUser used for CMD: ", fullCmd.data());
            HANDLE pHnd = GetCurrentProcess();
            if (pHnd != NULL)
            {
                HANDLE pToken;
                if (OpenProcessToken(pHnd, TOKEN_ALL_ACCESS, &pToken))
                {
                    if (CreateProcessAsUserW(pToken,
                        app.c_str(),
                        fullCmd.data(),
                        NULL,
                        NULL,
                        false,
                        CreationFlag,
                        NULL,
                        dir.c_str(),
                        (LPSTARTUPINFOW)&StartupInfoEx,
                        &ProcessInformation) == TRUE)
                    {
                        bRet = true;
                        WaitForSingleObject(ProcessInformation.hProcess, INFINITE);
                    }
                    else
                    {
                        Log("CreateProcessAsUserW failure.");
                    }
                }
                else
                {
                    Log("OpenProcessToken failure.");
                }
            }
            else
            {
                Log("GetCurrentProcess failure.");
            }
#endif
        }
    }
    return bRet;
}

void GetAndLaunchMonitor(const psf::json_object &monitor, std::filesystem::path packageRoot, int cmdShow, LPCWSTR dirStr)
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

    Log("\tCreating the monitor: %ls", monitorExecutable->as_string().wide());
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
        StartProcess(executable, (cmd + L" " + arguments).data(), (packageRoot / dirStr).c_str(), cmdShow, false, INFINITE);
    }
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