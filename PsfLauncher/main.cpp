//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>

#include <windows.h>
#include <psf_constants.h>
#include <psf_runtime.h>
#include <shellapi.h>
#include <combaseapi.h>
#include <TraceLoggingProvider.h>
#include "Telemetry.h"

TRACELOGGING_DECLARE_PROVIDER(g_Log_ETW_ComponentProvider);
TRACELOGGING_DEFINE_PROVIDER(
	g_Log_ETW_ComponentProvider,
	"Microsoft.Windows.PSFRuntime",
	(0x61F777A1, 0x1E59, 0x4BFC, 0xA6, 0x1A, 0xEF, 0x19, 0xC7, 0x16, 0xDD, 0xC0),
	TraceLoggingOptionMicrosoftTelemetry());

using namespace std::literals;

// Forward declaration
void LaunchMonitorInBackground(std::filesystem::path packageRoot, const wchar_t * executable, const wchar_t * arguments, bool wait, bool asadmin);
void LogApplicationAndProcessesCollection();

static inline bool check_suffix_if(iwstring_view str, iwstring_view suffix)
{
    if ((str.length() >= suffix.length()) && (str.substr(str.length() - suffix.length()) == suffix))
    {
        return true;
    }

    return false;
}

void Log(const char* fmt, ...)
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
void LogString(const char* name, const char* value)
{
    Log("\t%s=%s\n", name, value);
}
void LogStringW(const char* name, const wchar_t* value)
{
    Log("\t%s=%ls\n", name, value);
}
int launcher_main(PWSTR args, int cmdShow) noexcept try
{
    Log("\tIn Launcher_main()");
    auto appConfig = PSFQueryCurrentAppLaunchConfig(true);
    if (!appConfig)
    {
        throw_win32(ERROR_NOT_FOUND, "Error: could not find matching appid in config.json and appx manifest");
    }

    auto exeName = appConfig->get("executable").as_string().wide();
    auto dirPtr = appConfig->try_get("workingDirectory");
    auto dirStr = dirPtr ? dirPtr->as_string().wide() : nullptr;

    auto exeArgs = appConfig->try_get("arguments");
    auto exeArgString = exeArgs ? exeArgs->as_string().wide() : (wchar_t*)L"";
    auto monitor = PSFQueryAppMonitorConfig();

	LogApplicationAndProcessesCollection();

    // At least for now, configured launch paths are relative to the package root
    std::filesystem::path packageRoot = PSFQueryPackageRootPath();
    auto exePath = packageRoot / exeName;

    // Allow arguments to be specified in config.json now also.
    std::wstring cmdLine = L"\"" + exePath.filename().native() + L"\" " + exeArgString + L" " + args;

    if (monitor != nullptr )
    {
        // A monitor is an optional additional program to run, such as the PSFShimMonitor. This program is run prior to the "main application".		
        bool asadmin = false;
        bool wait = false;
        auto monitor_executable = monitor->try_get("executable");
        auto monitor_arguments = monitor->try_get("arguments");
        auto monitor_asadmin = monitor->try_get("asadmin");
        auto monitor_wait = monitor->try_get("wait");
        if (monitor_asadmin)
            asadmin = monitor_asadmin->as_boolean().get();
        if (monitor_wait)
            wait = monitor_wait->as_boolean().get();
        Log("\tCreating the monitor: %ls", monitor_executable->as_string().wide());
        LaunchMonitorInBackground(packageRoot, monitor_executable->as_string().wide(), monitor_arguments->as_string().wide(), wait, asadmin);
    }

    // Fix-up for no working directory
    // By default, we should use the directory of the executable.
    std::wstring wd;
    if (dirStr == nullptr)
    {
        std::wstring wdwd =  exePath.parent_path().native() ; // force working directory to exe's folder
        wdwd.resize(wdwd.size() - 1); // remove trailing slash
        wd = L"\"" + wdwd + L"\"";
    }
    else
    {
        // Use requested path, relative to the package root folder.
        std::wstring wdwd = (packageRoot / dirStr).native();
        wdwd.resize(wdwd.size() - 1); // remove trailing slash
        wd =  wdwd ;
    }
    std::wstring quotedapp = exePath.native(); // L"\"" + exePath.native() + L"\"";

    if (check_suffix_if(exeName, L".exe"_isv))
    {
        STARTUPINFO startupInfo = { sizeof(startupInfo) };
        startupInfo.dwFlags = STARTF_USESHOWWINDOW;
        startupInfo.wShowWindow = static_cast<WORD>(cmdShow);

        Log("\tCreating process %ls", cmdLine.data());
        // NOTE: PsfRuntime, which we dynamically link against, detours this call
        PROCESS_INFORMATION processInfo;
        if (!::CreateProcessW(
            exePath.c_str(),
            cmdLine.data(),
            nullptr, nullptr, // Process/ThreadAttributes
            true, // InheritHandles
            0, // CreationFlags
            nullptr, // Environment
            dirStr ? (packageRoot / dirStr).c_str() : nullptr, // NOTE: extended-length path not supported
            &startupInfo,
            &processInfo))
        {
            std::wostringstream ss;
            auto err{ ::GetLastError() };
            // Remove the ".\r\n" that gets added to all messages
            auto msg = widen(std::system_category().message(err));
            msg.resize(msg.length() - 3);
            ss << L"ERROR: Failed to create detoured process\n  Path: \"" << exeName << "\"\n  Error: " << msg << " (" << err << ")";
            ::PSFReportError(ss.str().c_str());
            return err;
        }

        // Propagate exit code to caller, in case they care
        switch (::WaitForSingleObject(processInfo.hProcess, INFINITE))
        {
        case WAIT_OBJECT_0:
            break; // Success case... handle at the end of main

        case WAIT_FAILED:
            return ::GetLastError();

        default:
            return ERROR_INVALID_HANDLE;
        }

        DWORD exitCode;
        if (!::GetExitCodeProcess(processInfo.hProcess, &exitCode))
        {
            return ::GetLastError();
        }
        return exitCode;
    }
    else
    {
        // Non Exe case, use shell launching to pick up local FTA
        auto nonexePath = packageRoot / exeName;

        CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        SHELLEXECUTEINFO shex;
        memset(&shex, 0, sizeof(SHELLEXECUTEINFO));
        shex.cbSize = sizeof(shex);
        shex.fMask = SEE_MASK_NOCLOSEPROCESS;
        shex.hwnd = (HWND)NULL;
        shex.lpVerb = NULL;  // just use the default action for the fta
        shex.lpFile = nonexePath.c_str();
        shex.lpParameters = exeArgString;
        shex.lpDirectory = dirStr ? (packageRoot / dirStr).c_str() : nullptr;
        shex.nShow = static_cast<WORD>(cmdShow);
        

        Log("\tUsing Shell launch: %ls %ls", shex.lpFile, shex.lpParameters);
        if (!ShellExecuteEx(&shex))
        {
            std::wostringstream ss;
            auto err{ ::GetLastError() };
            // Remove the ".\r\n" that gets added to all messages
            auto msg = widen(std::system_category().message(err));
            msg.resize(msg.length() - 3);
            ss << L"ERROR: Failed to create detoured shell process\n  Path: \"" << exeName << "\"\n  Error: " << msg << " (" << err << ")";
            ::PSFReportError(ss.str().c_str());
            return err;
        }
        else
        { 
            // Propagate exit code to caller, in case they care
            switch (::WaitForSingleObject(shex.hProcess, INFINITE))
            {
            case WAIT_OBJECT_0: // SUCCESS
                if (((unsigned long long)(shex.hInstApp)) > 32)
                    return 0;
                else
                    return (int)(unsigned long long)(shex.hInstApp);
            case WAIT_FAILED:
                return ::GetLastError();

            default:
                return ERROR_INVALID_HANDLE;
            }			
        }
    }
}
catch (...)
{
    ::PSFReportError(widen(message_from_caught_exception()).c_str());
    return win32_from_caught_exception();

}


void LaunchMonitorInBackground(std::filesystem::path packageRoot, const wchar_t * executable, const wchar_t * arguments, bool wait, bool asadmin )
{
    std::wstring cmd = L"\"" + (packageRoot / executable).native() + L"\"";

    if (asadmin)
    {
        // This happens when the program is requested for elevation.
        SHELLEXECUTEINFOW shExInfo = { 0 };
        shExInfo.cbSize = sizeof(shExInfo);
        if (wait)
            shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        else
            shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS|SEE_MASK_WAITFORINPUTIDLE;  // make sure we wait a bit for the monitor to be running before continuing on.
        shExInfo.hwnd = 0;
        shExInfo.lpVerb = L"runas";                // Operation to perform
        shExInfo.lpFile = cmd.c_str();       // Application to start    
        shExInfo.lpParameters = arguments;                  // Additional parameters
        shExInfo.lpDirectory = 0;
        shExInfo.nShow = 1;
        shExInfo.hInstApp = 0;
        

        if (ShellExecuteEx(&shExInfo))
        {
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
        else
        {
            //Log("error starting monitor using SellExecuteEx also. Error=0x%x\n", ::GetLastError());
        }
    }
    else
    {
        STARTUPINFO startupInfo = { sizeof(startupInfo) };
        startupInfo.dwFlags = STARTF_USESHOWWINDOW;
        startupInfo.wShowWindow = static_cast<WORD>(1);

        PROCESS_INFORMATION processInfo;

        std::wstring cmdarg = cmd + L" " + arguments;

        if (!::CreateProcessW(
            nullptr, //quotedapp.data(),
            (wchar_t *)cmdarg.c_str(),
            nullptr, nullptr, // Process/ThreadAttributes
            true, // InheritHandles
            0, // CreationFlags
            nullptr, // Environment
            nullptr,
            &startupInfo,
            &processInfo))
        {
            if (::GetLastError() == ERROR_ELEVATION_REQUIRED)
                Log("error starting monitor using CreateProcessW. You must specify 'monitor/asadmin' in config.json\n");
            else
                Log("error starting monitor using CreateProcessW. Error=0x%x\n", ::GetLastError());
        }
        else
        {
            if (wait)
                WaitForSingleObject(processInfo.hProcess, INFINITE);
        }
    }
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
						TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage),
						TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));					
				}
			}
		}
	}
}

void LogJsonConfig()
{
#pragma warning(suppress:4996) // Nonsense warning; _wfopen is perfectly safe
	auto file = _wfopen((std::filesystem::path(PSFQueryPackageRootPath()) / L"config.json").c_str(), L"rb, ccs=UTF-8");
	if (file)
	{
		fseek(file, 0, SEEK_END);
		auto size = ftell(file);
		auto fileContents = std::make_unique<char[]>(static_cast<int>(size + 1));
		rewind(file);
		fread(fileContents.get(), sizeof(char), size, file);
		fileContents[size] = 0;


		TraceLoggingWrite(
			g_Log_ETW_ComponentProvider,
			"JsonConfig",
			TraceLoggingValue(fileContents.get(), "config_data"),
			TraceLoggingBoolean(TRUE, "UTCReplace_AppSessionGuid"),
			TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage),
			TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
		fclose(file);
	}
}

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR args, int cmdShow)
{
	TraceLoggingRegister(g_Log_ETW_ComponentProvider);
	LogJsonConfig();
	int result = launcher_main(args, cmdShow);
	TraceLoggingUnregister(g_Log_ETW_ComponentProvider);
	return result;
}
