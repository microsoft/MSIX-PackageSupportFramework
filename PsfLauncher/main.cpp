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

//These two macros don't exist in RS1.  Define them here to prevent build
//failures when building for RS1.
#ifndef PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_DISABLE_PROCESS_TREE
#    define PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_DISABLE_PROCESS_TREE PROCESS_CREATION_DESKTOP_APPX_OVERRIDE
#endif

#ifndef PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY
#    define PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY PROC_THREAD_ATTRIBUTE_CHILD_PROCESS_POLICY
#endif

using namespace std::literals;

struct ExecutionInformation
{
    LPCWSTR ApplicationName;
    std::wstring CommandLine;
    LPCWSTR CurrentDirectory;
};

//Forward declarations
ErrorInformation StartProcess(ExecutionInformation execInfo, int cmdShow, bool runInAppContainer);
int launcher_main(PWSTR args, int cmdShow) noexcept;
ErrorInformation RunScript(const psf::json_object * scriptInformation, std::filesystem::path packageRoot, LPCWSTR dirStr, int cmdShow);
ErrorInformation GetAndLaunchMonitor(const psf::json_object * monitor, std::filesystem::path packageRoot, int cmdShow, LPCWSTR dirStr);
ErrorInformation LaunchMonitorInBackground(std::filesystem::path packageRoot, const wchar_t * executable, const wchar_t * arguments, bool wait, bool asadmin, int cmdShow, LPCWSTR dirStr);
static inline bool check_suffix_if(iwstring_view str, iwstring_view suffix);
void LogStringW(const char* name, const wchar_t* value);
void LogString(const char* name, const char* value);
void Log(const char* fmt, ...);
ErrorInformation StartWithShellExecute(std::filesystem::path packageRoot, std::filesystem::path exeName, std::wstring exeArgString, LPCWSTR dirStr, int cmdShow);
bool CheckIfPowerShellIsInstalled();

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR args, int cmdShow)
{
    return launcher_main(args, cmdShow);
}

int launcher_main(PWSTR args, int cmdShow) noexcept try
{
    Log("\tIn Launcher_main()");
    auto appConfig = PSFQueryCurrentAppLaunchConfig(true);
    if (!appConfig)
    {
        throw_win32(ERROR_NOT_FOUND, "Error: could not find matching appid in config.json and appx manifest");
    }

    auto dirPtr = appConfig->try_get("workingDirectory");
    auto dirStr = dirPtr ? dirPtr->as_string().wide() : L"";
    auto exeArgs = appConfig->try_get("arguments");

    // At least for now, configured launch paths are relative to the package root
    std::filesystem::path packageRoot = PSFQueryPackageRootPath();

    ErrorInformation error;
    bool isPowershellInstalled = CheckIfPowerShellIsInstalled();
    //Launch the starting powershell script if we are using one.
    auto startScriptInformation = PSFQueryStartScriptInfo();
    if (startScriptInformation)
    {
        if (!isPowershellInstalled)
        {
            ::PSFReportError(L"Powershell is not installed.  Please install powershell to run scripts in PSF");
            return E_APPLICATION_NOT_REGISTERED;
        }

        error = RunScript(startScriptInformation, packageRoot, dirStr, cmdShow);

        if (error.IsThereAnError())
        {
            ::PSFReportError(error.Print());
            return error.GetErrorNumber();
        }
    }

    //If we get here we know that starting script did NOT encounter an error
    //run the monitor.
    //Launch monitor if we are using one.
    auto monitor = PSFQueryAppMonitorConfig();
    if (monitor != nullptr)
    {
        error = GetAndLaunchMonitor(monitor, packageRoot, cmdShow, dirStr);
    }

    if (!error.IsThereAnError())
    {
        //Launch underlying application.
        auto exeName = appConfig->get("executable").as_string().wide();
        auto exePath = packageRoot / exeName;
        auto exeArgString = exeArgs ? exeArgs->as_string().wide() : (wchar_t*)L"";
        // Allow arguments to be specified in config.json now also.
        std::wstring cmdLine = L"\"" + exePath.filename().native() + L"\" " + exeArgString + L" " + args;
        if (check_suffix_if(exeName, L".exe"_isv))
        {
            std::wstring workingDirectory;

            if (dirStr)
            {
                workingDirectory = (packageRoot / dirStr).native();
            }

            ExecutionInformation execInfo;
            execInfo.ApplicationName = exePath.c_str();
            execInfo.CommandLine = cmdLine;

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

    //Launch the end powershell script if we are using one.
    auto endScriptInformation = PSFQueryEndScriptInfo();
    if (endScriptInformation)
    {
        ErrorInformation endingScriptError = RunScript(startScriptInformation, packageRoot, dirStr, cmdShow);
        error.AddExeName(L"Powershell.exe");

        //If there is an existing error from Monitor or the packaged exe
        if (error.IsThereAnError())
        {
            ::PSFReportError(error.Print());
            return error.GetErrorNumber();
        }
        else if (endingScriptError.IsThereAnError())
        {
            ::PSFReportError(endingScriptError.Print());
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

ErrorInformation RunScript(const psf::json_object * scriptInformation, std::filesystem::path packageRoot, LPCWSTR dirStr, int cmdShow)
{
    std::wstring scriptPath = scriptInformation->get("scriptPath").as_string().wide();
    auto scriptArguments = scriptInformation->get("scriptArguments").as_string().wide();
    auto currentDirectory = (packageRoot / dirStr);

    //RunInAppEnviorment is optional.
    auto runInAppEnviormentJObject = scriptInformation->try_get("runInAppEnviorment");
    auto runInAppEnviorment = false;

    if (runInAppEnviormentJObject)
    {
        runInAppEnviorment = runInAppEnviormentJObject->as_string().wide();
    }

    std::filesystem::path powershellScriptPath(scriptPath);

    std::wstring powershellCommandString(L"powershell.exe -file ");

    powershellCommandString.append(scriptPath);
    powershellCommandString.append(L" ");
    powershellCommandString.append(scriptArguments);

    Log("Looking for the script here: ");
    LogStringW("Script Path", scriptPath.c_str());
    auto doesFileExist = std::filesystem::exists(currentDirectory / powershellScriptPath);

    if (doesFileExist)
    {
        ExecutionInformation execInfo;
        execInfo.ApplicationName = nullptr;
        execInfo.CommandLine = powershellCommandString;
        execInfo.CurrentDirectory = currentDirectory.c_str();
        return StartProcess(execInfo, cmdShow, runInAppEnviorment);
    }
    else
    {
        std::wstring errorMessage = L"The powershell file ";
        errorMessage.append(currentDirectory / powershellScriptPath);
        errorMessage.append(L" can't be found");
        ErrorInformation error(errorMessage, ERROR_FILE_NOT_FOUND);

        return error;
    }
}

ErrorInformation GetAndLaunchMonitor(const psf::json_object * monitor, std::filesystem::path packageRoot, int cmdShow, LPCWSTR dirStr)
{
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
    ErrorInformation error = LaunchMonitorInBackground(packageRoot, monitor_executable->as_string().wide(), monitor_arguments->as_string().wide(), wait, asadmin, cmdShow, dirStr);

    return error;
}

ErrorInformation LaunchMonitorInBackground(std::filesystem::path packageRoot, const wchar_t * executable, const wchar_t * arguments, bool wait, bool asadmin, int cmdShow, LPCWSTR dirStr)
{
    std::wstring cmd = L"\"" + (packageRoot / executable).native() + L"\"";

    if (asadmin)
    {
        // This happens when the program is requested for elevation.
        SHELLEXECUTEINFOW shExInfo = { 0 };
        shExInfo.cbSize = sizeof(shExInfo);
        if (wait)
        {
            shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        }
        else
        {
            // make sure we wait a bit for the monitor to be running before continuing on.
            shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_WAITFORINPUTIDLE;
        }
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
            std::wostringstream ss;
            ss << L"error starting monitor using SellExecuteEx";
            auto err = ::GetLastError();
            ErrorInformation error(ss.str(), err, executable);

            return error;
        }
    }
    else
    {
        std::wstring cmdarg = cmd + L" " + arguments;

        ExecutionInformation execInfo;
        execInfo.ApplicationName = nullptr;
        execInfo.CommandLine = cmdarg;

        auto currentDirectory = (packageRoot / dirStr);
        execInfo.CurrentDirectory = currentDirectory.c_str();
        ErrorInformation error = StartProcess(execInfo, cmdShow, false);
        error.AddExeName(executable);

        return error;
    }

    ErrorInformation noError;
    return noError;
}

ErrorInformation StartProcess(ExecutionInformation execInfo, int cmdShow, bool runInAppContainer)
{
    STARTUPINFOEXW StartupInfoEx = { 0 };

    StartupInfoEx.StartupInfo.cb = sizeof(StartupInfoEx);
    StartupInfoEx.StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    StartupInfoEx.StartupInfo.wShowWindow = static_cast<WORD>(cmdShow);

    if (runInAppContainer)
    {
        SIZE_T AttributeListSize;
        InitializeProcThreadAttributeList(NULL, 1, 0, &AttributeListSize);
        StartupInfoEx.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(
            GetProcessHeap(),
            0,
            AttributeListSize
        );

        if (InitializeProcThreadAttributeList(StartupInfoEx.lpAttributeList,
            1,
            0,
            &AttributeListSize) == FALSE)
        {
            auto err{ ::GetLastError() };
            std::wostringstream ss;
            ss << L"Could not initilize the proc thread attribute list.";

            ErrorInformation error(ss.str(), err);
            return error;
        }

        DWORD attribute = PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_DISABLE_PROCESS_TREE;
        if (UpdateProcThreadAttribute(StartupInfoEx.lpAttributeList,
            0,
            PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY,
            &attribute,
            sizeof(attribute),
            NULL,
            NULL) == FALSE)
        {
            auto err{ ::GetLastError() };
            std::wostringstream ss;
            ss << L"Could not update Proc thread attribute.";

            ErrorInformation error(ss.str(), err);
            return error;
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
        (LPSTARTUPINFO)&StartupInfoEx,
        &processInfo))
    {
        // Propagate exit code to caller, in case they care
        DWORD waitResult = ::WaitForSingleObject(processInfo.hProcess, INFINITE);

        if (waitResult == WAIT_OBJECT_0)
        {
            ErrorInformation noError;
            return noError;
        }
        else
        {
            auto err{ ::GetLastError() };
            std::wostringstream ss;
            ss << L"Running process failed.";

            ErrorInformation error(ss.str(), err);
            return error;
        }
    }
    else
    {
        std::wostringstream ss;
        auto err{ ::GetLastError() };

        if (execInfo.ApplicationName)
        {
            ss << L"ERROR: Failed to create a process for " << execInfo.ApplicationName << " ";
        }
        else
        {
            std::wstring applicationNameForError;
            applicationNameForError = execInfo.CommandLine.substr(0, execInfo.CommandLine.find(' '));
            ss << L"ERROR: Failed to create a process for " << applicationNameForError << " ";
        }

        ErrorInformation error(ss.str(), err);
        return error;
    }
}

ErrorInformation StartWithShellExecute(std::filesystem::path packageRoot, std::filesystem::path exeName, std::wstring exeArgString, LPCWSTR dirStr, int cmdShow)
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
    shex.lpParameters = exeArgString.c_str();
    shex.lpDirectory = dirStr ? (packageRoot / dirStr).c_str() : nullptr;
    shex.nShow = static_cast<WORD>(cmdShow);


    Log("\tUsing Shell launch: %ls %ls", shex.lpFile, shex.lpParameters);
    if (!ShellExecuteEx(&shex))
    {
        auto err{ ::GetLastError() };
        std::wostringstream ss;
        ss << L"ERROR: Failed to create detoured shell process";

        ErrorInformation error(ss.str(), err);
        return error;
    }

    ErrorInformation noError;
    return noError;
}

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

bool CheckIfPowerShellIsInstalled()
{
    HKEY registryHandle;
    DWORD statusOfRegistryKey;
    LSTATUS createResult = RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\PowerShell\\1", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &registryHandle, &statusOfRegistryKey);

    if (createResult != ERROR_SUCCESS)
    {
        ErrorInformation error(L"Error with getting the key to see if powershell is installed. ", createResult);
        ::PSFReportError(error.Print());
    }

    DWORD valueFromRegistry = 0;
    DWORD bufferSize = sizeof(DWORD);
    DWORD type = REG_DWORD;
    auto getResult = RegQueryValueExW(registryHandle, L"Install", NULL, &type, reinterpret_cast<BYTE*>(&valueFromRegistry), &bufferSize);

    if (getResult != ERROR_SUCCESS)
    {
        ErrorInformation error(L"Error with querying the key to see if powershell is installed. ", createResult);
        ::PSFReportError(error.Print());
    }

    return valueFromRegistry;
}