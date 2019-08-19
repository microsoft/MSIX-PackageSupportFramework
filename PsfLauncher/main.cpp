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
#include <TraceLoggingProvider.h>
#include <psf_constants.h>
#include <psf_runtime.h>
#include <future>
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
void GetAndLaunchMonitor(const psf::json_object &monitor, std::filesystem::path packageRoot, int cmdShow, LPCWSTR dirStr);
void LaunchMonitorInBackground(std::filesystem::path packageRoot, const wchar_t executable[], const wchar_t arguments[], bool wait, bool asAdmin, int cmdShow, LPCWSTR dirStr);

static inline bool check_suffix_if(iwstring_view str, iwstring_view suffix) noexcept;
void LogString(const char name[], const wchar_t value[]) noexcept;
void LogString(const char name[], const char value[]) noexcept;
void Log(const char fmt[], ...) noexcept;

void StartWithShellExecute(std::filesystem::path packageRoot, std::filesystem::path exeName, std::wstring exeArgString, LPCWSTR dirStr, int cmdShow);
bool CheckIfPowershellIsInstalled();

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

	bool stopOnScriptError = false;

	auto stopOnScriptErrorObject = appConfig->try_get("stopOnScriptError");
	if (stopOnScriptErrorObject)
	{
		stopOnScriptError = stopOnScriptErrorObject->as_boolean().get();
	}

    // At least for now, configured launch paths are relative to the package root
    std::filesystem::path packageRoot = PSFQueryPackageRootPath();
	auto currentDirectory = (packageRoot / dirStr);
    // Launch the starting PowerShell script if we are using one.
	auto startScriptInformation = PSFQueryStartScriptInfo();
    if (startScriptInformation)
	{
        THROW_HR_IF_MSG(ERROR_NOT_SUPPORTED, !CheckIfPowershellIsInstalled(), "PowerShell is not installed.  Please install PowerShell to run scripts in PSF");
		PsfPowershellScriptRunner startingScript(*startScriptInformation, currentDirectory);

		//Don't run if the users wants the starting script to run asynch
		//AND to stop running PSF is the starting script encounters an error.
		//We stop here because we don't want to crash the application if the
		//starting script encountered an error.
		if (stopOnScriptError && !startingScript.GetWaitForScriptToFinish())
		{
			THROW_HR_MSG(ERROR_BAD_CONFIGURATION, "PSF does not allow stopping on a script error and running asynchronously.  Please either remove stopOnScriptError or add a wait");
		}

		startingScript.RunPfsScript(stopOnScriptError, currentDirectory);
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
    auto exePath = packageRoot / exeName;
    auto exeArgString = exeArgs ? exeArgs->as_string().wide() : (wchar_t*)L"";

    // Keep these quotes here.  StartProcess assumes there are quotes around the exe file name
    if (check_suffix_if(exeName, L".exe"_isv))
    {
        StartProcess(exePath.c_str(), (L"\"" + exePath.filename().native() + L"\" " + exeArgString + L" " + args).data(), (packageRoot / dirStr).c_str(), cmdShow, false, INFINITE);
    }
    else
    {
        StartWithShellExecute(packageRoot, exeName, exeArgString, dirStr, cmdShow);
    }

    // Launch the end PowerShell script if we are using one.
    auto endScriptInformation = PSFQueryEndScriptInfo();
    if (endScriptInformation)
    {
		PsfPowershellScriptRunner endingScript(*endScriptInformation, currentDirectory);
		endingScript.RunPfsScript(true, currentDirectory);
    }

    return 0;
}
catch (...)
{
    ::PSFReportError(widen(message_from_caught_exception()).c_str());
    return win32_from_caught_exception();
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

        DWORD exitCode{};
        THROW_LAST_ERROR_IF_MSG(!GetExitCodeProcess(shExInfo.hProcess, &exitCode), "Could not get error for process");
        THROW_IF_WIN32_ERROR(exitCode);
    }
    else
    {
        StartProcess(executable, (cmd + L" " + arguments).data(), (packageRoot / dirStr).c_str(), cmdShow, false, INFINITE);
    }
}

void StartWithShellExecute(std::filesystem::path packageRoot, std::filesystem::path exeName, std::wstring exeArgString, LPCWSTR dirStr, int cmdShow)
{
    // Non Exe case, use shell launching to pick up local FTA
    auto nonExePath = packageRoot / exeName;

    SHELLEXECUTEINFO shex = {
        sizeof(shex)
        , SEE_MASK_NOCLOSEPROCESS
        , (HWND)nullptr
        , nullptr
        , nonExePath.c_str()
        , exeArgString.c_str()
        , dirStr ? (packageRoot / dirStr).c_str() : nullptr
        , static_cast<WORD>(cmdShow)
    };

    Log("\tUsing Shell launch: %ls %ls", shex.lpFile, shex.lpParameters);
    THROW_LAST_ERROR_IF_MSG (
        !ShellExecuteEx(&shex),
        "ERROR: Failed to create detoured shell process");

    THROW_LAST_ERROR_IF(shex.hProcess == INVALID_HANDLE_VALUE);
    DWORD exitCode{};
    THROW_IF_WIN32_ERROR(GetExitCodeProcess(shex.hProcess, &exitCode));
    THROW_IF_WIN32_ERROR(exitCode);
    CloseHandle(shex.hProcess);
}

static inline bool check_suffix_if(iwstring_view str, iwstring_view suffix) noexcept
{
    return ((str.length() >= suffix.length()) && (str.substr(str.length() - suffix.length()) == suffix));
}

void Log(const char fmt[], ...) noexcept
{
    std::string str;
    str.resize(256);

    va_list args;
    va_start(args, fmt);
    std::size_t count = std::vsnprintf(str.data(), str.size() + 1, fmt, args);
    assert(count >= 0);
    va_end(args);

    if (count > str.size())
    {
        str.resize(count);

        va_list args2;
        va_start(args2, fmt);
        count = std::vsnprintf(str.data(), str.size() + 1, fmt, args2);
        assert(count >= 0);
        va_end(args2);
    }

    str.resize(count);
    ::OutputDebugStringA(str.c_str());
}
void LogString(const char name[], const char value[]) noexcept
{
    Log("\t%s=%s\n", name, value);
}
void LogString(const char name[], const wchar_t value[]) noexcept
{
    Log("\t%s=%ls\n", name, value);
}

bool CheckIfPowershellIsInstalled()
{
    wil::unique_hkey registryHandle;
    LSTATUS createResult = RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\PowerShell\\1", 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_READ, nullptr, &registryHandle, nullptr);

    if (createResult == ERROR_FILE_NOT_FOUND)
    {
        // If the key cannot be found, powershell is not installed
        return false;
    }
    else if (createResult != ERROR_SUCCESS)
    {
        THROW_HR_MSG(createResult, "Error with getting the key to see if PowerShell is installed.");
    }

    DWORD valueFromRegistry = 0;
    DWORD bufferSize = sizeof(DWORD);
    DWORD type = REG_DWORD;
    THROW_IF_WIN32_ERROR_MSG(RegQueryValueExW(registryHandle.get(), L"Install", nullptr, &type, reinterpret_cast<BYTE*>(&valueFromRegistry), &bufferSize), 
                             "Error with querying the key to see if PowerShell is installed.");

    if (valueFromRegistry != 1)
    {
        return false;
    }

    return true;
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