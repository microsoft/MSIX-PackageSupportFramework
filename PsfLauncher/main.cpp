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
#include <ppltasks.h>
#include <ShObjIdl.h>
#include "ErrorInformation.h"
#include <wil\resource.h>
#include <wil\result.h>
#include <TraceLoggingProvider.h>
#include "Telemetry.h"

TRACELOGGING_DECLARE_PROVIDER(g_Log_ETW_ComponentProvider);
TRACELOGGING_DEFINE_PROVIDER(
    g_Log_ETW_ComponentProvider,
    "Microsoft.Windows.PSFRuntime",
    (0xf7f4e8c4, 0x9981, 0x5221, 0xe6, 0xfb, 0xff, 0x9d, 0xd1, 0xcd, 0xa4, 0xe1),
    TraceLoggingOptionMicrosoftTelemetry());

//These two macros don't exist in RS1.  Define them here to prevent build
//failures when building for RS1.
#ifndef PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_DISABLE_PROCESS_TREE
#    define PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_DISABLE_PROCESS_TREE PROCESS_CREATION_DESKTOP_APPX_OVERRIDE
#endif

#ifndef PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY
#    define PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY PROC_THREAD_ATTRIBUTE_CHILD_PROCESS_POLICY
#endif

using namespace std::literals;

void LogApplicationAndProcessesCollection();

struct ExecutionInformation
{
    LPCWSTR ApplicationName;
    std::wstring CommandLine;
    LPCWSTR CurrentDirectory;
};

//Forward declarations
ErrorInformation StartProcess(ExecutionInformation execInfo, int cmdShow, bool runInVirtualEnvironment) noexcept;
int launcher_main(PCWSTR args, int cmdShow) noexcept;
ErrorInformation RunScript(const psf::json_object &scriptInformation, std::filesystem::path packageRoot, LPCWSTR dirStr, int cmdShow) noexcept;
ErrorInformation GetAndLaunchMonitor(const psf::json_object &monitor, std::filesystem::path packageRoot, int cmdShow, LPCWSTR dirStr) noexcept;
ErrorInformation LaunchMonitorInBackground(std::filesystem::path packageRoot, const wchar_t executable[], const wchar_t arguments[], bool wait, bool asAdmin, int cmdShow, LPCWSTR dirStr) noexcept;
static inline bool check_suffix_if(iwstring_view str, iwstring_view suffix) noexcept;
void LogString(const char name[], const wchar_t value[]) noexcept;
void LogString(const char name[], const char value[]) noexcept;
void Log(const char fmt[], ...) noexcept;
ErrorInformation StartWithShellExecute(std::filesystem::path packageRoot, std::filesystem::path exeName, std::wstring exeArgString, LPCWSTR dirStr, int cmdShow) noexcept;
ErrorInformation CheckIfPowershellIsInstalled(bool& isPowershellInstalled) noexcept;
std::wstring GetApplicationNameFromExecInfo(ExecutionInformation execInfo);
ErrorInformation GetExitCodeForProcess(ExecutionInformation execInfo, HANDLE hProcess);

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR args, int cmdShow)
{
    return launcher_main(args, cmdShow);
}

int launcher_main(PCWSTR args, int cmdShow) noexcept try
{
    Log("\tIn Launcher_main()");
    auto appConfig = PSFQueryCurrentAppLaunchConfig(true);
    if (!appConfig)
    {
        throw_win32(ERROR_NOT_FOUND, "Error: could not find matching appid in config.json and appx manifest");
    }

    LogApplicationAndProcessesCollection();

    auto dirPtr = appConfig->try_get("workingDirectory");
    auto dirStr = dirPtr ? dirPtr->as_string().wide() : L"";
    auto exeArgs = appConfig->try_get("arguments");

    // At least for now, configured launch paths are relative to the package root
    std::filesystem::path packageRoot = PSFQueryPackageRootPath();

    ErrorInformation error;

    //Launch the starting PowerShell script if we are using one.
    auto startScriptInformation = PSFQueryStartScriptInfo();
    if (startScriptInformation)
    {
        auto isPowershellInstalled = false;
        error = CheckIfPowershellIsInstalled(isPowershellInstalled);

        if (error.IsThereAnError())
        {
            ::PSFReportError(error.Print().c_str());
            return error.GetErrorNumber();
        }

        if (!isPowershellInstalled)
        {
            ::PSFReportError(L"PowerShell is not installed.  Please install PowerShell to run scripts in PSF");
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }

        error = RunScript(*startScriptInformation, packageRoot, dirStr, cmdShow);

        if (error.IsThereAnError())
        {
            ::PSFReportError(error.Print().c_str());
            return error.GetErrorNumber();
        }
    }

    //If we get here we know that starting script did NOT encounter an error
    //Launch monitor if we are using one.
    auto monitor = PSFQueryAppMonitorConfig();
    if (monitor != nullptr)
    {        
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        error = GetAndLaunchMonitor(*monitor, packageRoot, cmdShow, dirStr);
    }

    if (!error.IsThereAnError())
    {
        //Launch underlying application.
        auto exeName = appConfig->get("executable").as_string().wide();
        auto exePath = packageRoot / exeName;
        auto exeArgString = exeArgs ? exeArgs->as_string().wide() : (wchar_t*)L"";

        //Keep these quotes here.  StartProcess assums there are quptes around the exe file name
        std::wstring cmdLine = L"\"" + exePath.filename().native() + L"\" " + exeArgString + L" " + args;
        if (check_suffix_if(exeName, L".exe"_isv))
        {
            std::wstring workingDirectory = (packageRoot / dirStr).native();

            ExecutionInformation execInfo = { exePath.c_str(), cmdLine };

            auto currentDirectory = (packageRoot / dirStr);
            execInfo.CurrentDirectory = currentDirectory.c_str();
            error = StartProcess(execInfo, cmdShow, false);
            error.AddExeName(exeName);
        }
        else
        {
            error = StartWithShellExecute(packageRoot, exeName, exeArgString, dirStr, cmdShow);
        }
    }

    //Launch the end PowerShell script if we are using one.
    auto endScriptInformation = PSFQueryEndScriptInfo();
    if (endScriptInformation)
    {
        ErrorInformation endingScriptError = RunScript(*endScriptInformation, packageRoot, dirStr, cmdShow);
        error.AddExeName(L"PowerShell.exe -file ");

        //If there is an existing error from Monitor or the packaged exe
        if (error.IsThereAnError())
        {
            ::PSFReportError(error.Print().c_str());
            return error.GetErrorNumber();
        }
        else if (endingScriptError.IsThereAnError())
        {
            ::PSFReportError(endingScriptError.Print().c_str());
            return endingScriptError.GetErrorNumber();
        }
    }

    return 0;
}
catch (...)
{
    ::PSFReportError(widen(message_from_caught_exception()).c_str());
    return win32_from_caught_exception();
}

ErrorInformation RunScript(const psf::json_object &scriptInformation, std::filesystem::path packageRoot, LPCWSTR dirStr, int cmdShow) noexcept
{
    //Generate the command string that we will use to call powershell
    std::wstring powershellCommandString(L"Powershell.exe -file ");

    std::wstring scriptPath = scriptInformation.get("scriptPath").as_string().wide();
    powershellCommandString.append(scriptPath);
    powershellCommandString.append(L" ");

    //Script arguments are optional.
    auto scriptArgumentsJObject = scriptInformation.try_get("scriptArguments");
    if (scriptArgumentsJObject)
    {
        powershellCommandString.append(scriptArgumentsJObject->as_string().wide());
    }

    auto currentDirectory = (packageRoot / dirStr);
    std::filesystem::path powershellScriptPath(scriptPath);

    //The file might be on a network drive.
    auto doesFileExist = std::filesystem::exists(powershellScriptPath);

    //Check on local computer.
    if (!doesFileExist)
    {
        doesFileExist = std::filesystem::exists(currentDirectory / powershellScriptPath);
    }

    if (doesFileExist)
    {
        //runInVirtualEnvironment is optional.
        auto runInVirtualEnvironmentJObject = scriptInformation.try_get("runInVirtualEnvironment");
        auto runInVirtualEnvironment = false;

        if (runInVirtualEnvironmentJObject)
        {
            runInVirtualEnvironment = runInVirtualEnvironmentJObject->as_boolean().get();
        }

        ExecutionInformation execInfo = { nullptr, powershellCommandString , currentDirectory.c_str() };
        return StartProcess(execInfo, cmdShow, runInVirtualEnvironment);
    }
    else
    {
        std::wstring errorMessage = L"The PowerShell file ";
        errorMessage.append(currentDirectory / powershellScriptPath);
        errorMessage.append(L" can't be found");
        ErrorInformation error = { errorMessage, ERROR_FILE_NOT_FOUND };

        return error;
    }
}

ErrorInformation GetAndLaunchMonitor(const psf::json_object &monitor, std::filesystem::path packageRoot, int cmdShow, LPCWSTR dirStr) noexcept
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
    ErrorInformation error = LaunchMonitorInBackground(packageRoot, monitorExecutable->as_string().wide(), monitorArguments->as_string().wide(), wait, asAdmin, cmdShow, dirStr);

    return error;
}

ErrorInformation LaunchMonitorInBackground(std::filesystem::path packageRoot, const wchar_t executable[], const wchar_t arguments[], bool wait, bool asAdmin, int cmdShow, LPCWSTR dirStr) noexcept
{
    std::wstring cmd = L"\"" + (packageRoot / executable).native() + L"\"";

    if (asAdmin)
    {
        // This happens when the program is requested for elevation.
        SHELLEXECUTEINFOW shExInfo =
        {
            sizeof(shExInfo) //cbSize
            , wait ? (ULONG)SEE_MASK_NOCLOSEPROCESS : (ULONG)(SEE_MASK_NOCLOSEPROCESS | SEE_MASK_WAITFORINPUTIDLE) // fmask
            , 0 //hwnd
            , L"runas" //lpVerb
            , cmd.c_str() // lpFile
            , arguments //lpParameters
            , nullptr //lpDirectory
            , 1 //nShow
            , 0 //hInstApp
        };


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

            return GetExitCodeForProcess({ executable }, shExInfo.hProcess);
        }
        else
        {
            auto err = ::GetLastError();
            ErrorInformation error = { L"Error starting monitor using ShellExecuteEx", err, executable };
        }
    }
    else
    {
        ExecutionInformation execInfo = { executable, (cmd + L" " + arguments) };

        auto currentDirectory = (packageRoot / dirStr);
        execInfo.CurrentDirectory = currentDirectory.c_str();

        ErrorInformation error = StartProcess(execInfo, cmdShow, false);
        error.AddExeName(executable);

        return error;
    }

    return {};
}

ErrorInformation StartProcess(ExecutionInformation execInfo, int cmdShow, bool runInVirtualEnvironment) noexcept
{
    STARTUPINFOEXW startupInfoEx =
    {
        {
        sizeof(startupInfoEx)
        , nullptr //lpReserved
        , nullptr // lpDesktop
        , nullptr // lpTitle
        , 0 //dwX
        , 0 //dwY
        , 0 //dwXSize
        , 0 //swYSize
        , 0 //dwXCountChar
        , 0 //dwYCountChar
        , 0 // dwFillAttribute
        , STARTF_USESHOWWINDOW //dwFlags
        , static_cast<WORD>(cmdShow) // wShowWindow
        }
    };

    if (runInVirtualEnvironment)
    {
        SIZE_T AttributeListSize;
        InitializeProcThreadAttributeList(nullptr, 1, 0, &AttributeListSize);
        startupInfoEx.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(
            GetProcessHeap(),
            0,
            AttributeListSize
        );

        if (InitializeProcThreadAttributeList(startupInfoEx.lpAttributeList,
            1,
            0,
            &AttributeListSize) == FALSE)
        {
            auto err{ ::GetLastError() };
            ErrorInformation error = { L"Could not initialize the proc thread attribute list.", err };
        }

        DWORD attribute = PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_DISABLE_PROCESS_TREE;
        if (UpdateProcThreadAttribute(startupInfoEx.lpAttributeList,
            0,
            PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY,
            &attribute,
            sizeof(attribute),
            nullptr,
            nullptr) == FALSE)
        {
            auto err{ ::GetLastError() };
            ErrorInformation error{ L"Could not update Proc thread attribute.", err };
        }
    }

    PROCESS_INFORMATION processInfo = { 0 };
    if (::CreateProcessW(
        execInfo.ApplicationName,
        execInfo.CommandLine.data(),
        nullptr, nullptr, // Process/ThreadAttributes
        true, // InheritHandles
        EXTENDED_STARTUPINFO_PRESENT, // CreationFlags
        nullptr, // Environment
        execInfo.CurrentDirectory,
        (LPSTARTUPINFO)&startupInfoEx,
        &processInfo))
    {
        DWORD waitResult = ::WaitForSingleObject(processInfo.hProcess, INFINITE);

        if (waitResult != WAIT_OBJECT_0)
        {
            auto err{ ::GetLastError() };
            return { L"Waiting operation failed unexpectedly.", err };
        }
    }
    else
    {
        std::wostringstream ss;
        auto err{ ::GetLastError() };
        ss << L"ERROR: Failed to create a process for " << GetApplicationNameFromExecInfo(execInfo);

        return { ss.str(), err };
    }

    return {};
}

std::wstring GetApplicationNameFromExecInfo(ExecutionInformation execInfo)
{
    if (execInfo.ApplicationName)
    {
        return execInfo.ApplicationName;
    }
    else
    {
        std::wstring applicationName;
        //If the application name contains spaces, the application needs to be surrounded in quotes.
        if (execInfo.CommandLine[0] == '"')
        {
            //Skip the first quote and don't include the last quote.
            applicationName = execInfo.CommandLine.substr(1, execInfo.CommandLine.find('"', 1) - 1);
        }
        else
        {
            applicationName = execInfo.CommandLine.substr(0, execInfo.CommandLine.find(' '));

        }

        return applicationName;
    }
}

ErrorInformation GetExitCodeForProcess(ExecutionInformation execInfo, HANDLE hProcess)
{
    DWORD exitCode = ERROR_SUCCESS;
    if (GetExitCodeProcess(hProcess, &exitCode))
    {
        if (exitCode != ERROR_SUCCESS)
        {
            return { L"Exit code for " + GetApplicationNameFromExecInfo(execInfo), exitCode };
        }
    }
    else
    {
        return { L"Could not get the exit code for " + GetApplicationNameFromExecInfo(execInfo), ::GetLastError() };
    }

    return {};
}

ErrorInformation StartWithShellExecute(std::filesystem::path packageRoot, std::filesystem::path exeName, std::wstring exeArgString, LPCWSTR dirStr, int cmdShow) noexcept
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
    if (!ShellExecuteEx(&shex))
    {
        auto err{ ::GetLastError() };
        return { L"ERROR: Failed to create detoured shell process", err };
    }

    return GetExitCodeForProcess({ exeName.native().c_str() }, shex.hProcess);
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

ErrorInformation CheckIfPowershellIsInstalled(bool& isPowershellInstalled) noexcept
{
    wil::unique_hkey registryHandle;
    LSTATUS createResult = RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\PowerShell\\1", 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_READ, nullptr, &registryHandle, nullptr);

    if (createResult == ERROR_FILE_NOT_FOUND)
    {
        // If the key cannot be found, powershell is not installed
        isPowershellInstalled = false;
        return {};
    }
    else if (createResult != ERROR_SUCCESS)
    {
        return { L"Error with getting the key to see if PowerShell is installed. ", (DWORD)createResult };
    }

    DWORD valueFromRegistry = 0;
    DWORD bufferSize = sizeof(DWORD);
    DWORD type = REG_DWORD;
    auto getResult = RegQueryValueExW(registryHandle.get(), L"Install", nullptr, &type, reinterpret_cast<BYTE*>(&valueFromRegistry), &bufferSize);

    if (getResult != ERROR_SUCCESS)
    {
        //Don't set isPowershellInstalled since we can't figure it out.
        return { L"Error with querying the key to see if PowerShell is installed. ", (DWORD)getResult };
    }

    if (valueFromRegistry == 1)
    {
        isPowershellInstalled = true;
    }
    else
    {
        isPowershellInstalled = false;
    }

    return {};
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