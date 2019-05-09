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
int launcher_main(PCWSTR args, int cmdShow) noexcept;
ErrorInformation RunScript(const psf::json_object *scriptInformation, std::filesystem::path packageRoot, LPCWSTR dirStr, int cmdShow);
ErrorInformation GetAndLaunchMonitor(const psf::json_object *monitor, std::filesystem::path packageRoot, int cmdShow, LPCWSTR dirStr);
ErrorInformation LaunchMonitorInBackground(std::filesystem::path packageRoot, const wchar_t executable[], const wchar_t arguments[], bool wait, bool asAdmin, int cmdShow, LPCWSTR dirStr);
static inline bool check_suffix_if(iwstring_view str, iwstring_view suffix);
void LogString(const char name[], const wchar_t value[]);
void LogString(const char name[], const char value[]);
void Log(const char fmt[], ...);
ErrorInformation StartWithShellExecute(std::filesystem::path packageRoot, std::filesystem::path exeName, std::wstring exeArgString, LPCWSTR dirStr, int cmdShow);
bool CheckIfPowershellIsInstalled();

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

    auto dirPtr = appConfig->try_get("workingDirectory");
    auto dirStr = dirPtr ? dirPtr->as_string().wide() : L"";
    auto exeArgs = appConfig->try_get("arguments");

    // At least for now, configured launch paths are relative to the package root
    std::filesystem::path packageRoot = PSFQueryPackageRootPath();

    ErrorInformation error;
    bool isPowershellInstalled = CheckIfPowershellIsInstalled();
    //Launch the starting PowerShell script if we are using one.
    auto startScriptInformation = PSFQueryStartScriptInfo();
    if (startScriptInformation)
    {
        if (!isPowershellInstalled)
        {
            ::PSFReportError(L"PowerShell is not installed.  Please install PowerShell to run scripts in PSF");
            return E_APPLICATION_NOT_REGISTERED;
        }

        error = RunScript(startScriptInformation, packageRoot, dirStr, cmdShow);

        if (error.IsThereAnError())
        {
            ::PSFReportError(error.Print().c_str());
            return error.GetErrorNumber();
        }
    }

    //If we get here we know that starting script did NOT encounter an error
    //run the monitor.
    //Launch monitor if we are using one.
    auto monitor = PSFQueryAppMonitorConfig();
    if (monitor != nullptr)
    {
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
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

    //Launch the end PowerShell script if we are using one.
    auto endScriptInformation = PSFQueryEndScriptInfo();
    if (endScriptInformation)
    {
        ErrorInformation endingScriptError = RunScript(endScriptInformation, packageRoot, dirStr, cmdShow);
        error.AddExeName(L"PowerShell.exe");

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

ErrorInformation RunScript(const psf::json_object *scriptInformation, std::filesystem::path packageRoot, LPCWSTR dirStr, int cmdShow)
{
    //Script arguments are optional.
    auto scriptArgumentsJObject = scriptInformation->try_get("scriptArguments");
    std::wstring scriptArguments;
    if (scriptArgumentsJObject)
    {
        scriptArguments = scriptArgumentsJObject->as_string().wide();
    }

    std::wstring scriptPath = scriptInformation->get("scriptPath").as_string().wide();
    std::filesystem::path PowershellScriptPath(scriptPath);

    std::wstring PowershellCommandString(L"PowerShell.exe -file ");

    PowershellCommandString.append(scriptPath);
    PowershellCommandString.append(L" ");
    PowershellCommandString.append(scriptArguments);

    auto currentDirectory = (packageRoot / dirStr);
    auto doesFileExist = std::filesystem::exists(currentDirectory / PowershellScriptPath);

    if (doesFileExist)
    {
        ExecutionInformation execInfo;
        execInfo.ApplicationName = nullptr;
        execInfo.CommandLine = PowershellCommandString;
        execInfo.CurrentDirectory = currentDirectory.c_str();

        //runInVirtualEnvironment is optional.
        auto runInVirtualEnviormentJObject = scriptInformation->try_get("runInVirtualEnvironment");
        auto runInVirtualEnviorment = false;

        if (runInVirtualEnviormentJObject)
        {
            runInVirtualEnviorment = runInVirtualEnviormentJObject->as_string().wide();
        }

        return StartProcess(execInfo, cmdShow, runInVirtualEnviorment);
    }
    else
    {
        std::wstring errorMessage = L"The PowerShell file ";
        errorMessage.append(currentDirectory / PowershellScriptPath);
        errorMessage.append(L" can't be found");
        ErrorInformation error(errorMessage, ERROR_FILE_NOT_FOUND);

        return error;
    }
}

ErrorInformation GetAndLaunchMonitor(const psf::json_object *monitor, std::filesystem::path packageRoot, int cmdShow, LPCWSTR dirStr)
{
    bool asadmin = false;
    bool wait = false;
    auto monitorExecutable = monitor->try_get("executable");
    auto monitorArguments = monitor->try_get("arguments");
    auto monitorAsAdmin = monitor->try_get("asadmin");
    auto monitorWait = monitor->try_get("wait");
    if (monitorAsAdmin)
    {
        asadmin = monitorAsAdmin->as_boolean().get();
    }

    if (monitorWait)
    {
        wait = monitorWait->as_boolean().get();
    }

    Log("\tCreating the monitor: %ls", monitorExecutable->as_string().wide());
    ErrorInformation error = LaunchMonitorInBackground(packageRoot, monitorExecutable->as_string().wide(), monitorArguments->as_string().wide(), wait, asadmin, cmdShow, dirStr);

    return error;
}

ErrorInformation LaunchMonitorInBackground(std::filesystem::path packageRoot, const wchar_t executable[], const wchar_t arguments[], bool wait, bool asAdmin, int cmdShow, LPCWSTR dirStr)
{
    std::wstring cmd = L"\"" + (packageRoot / executable).native() + L"\"";

    if (asAdmin)
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
            std::wstring errorString = L"error starting monitor using ShellExecuteEx";
            auto err = ::GetLastError();
            ErrorInformation error(errorString, err, executable);

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
        InitializeProcThreadAttributeList(nullptr, 1, 0, &AttributeListSize);
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
            std::wstring errorString = L"Could not initilize the proc thread attribute list.";

            ErrorInformation error(errorString, err);
            return error;
        }

        DWORD attribute = PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_DISABLE_PROCESS_TREE;
        if (UpdateProcThreadAttribute(StartupInfoEx.lpAttributeList,
            0,
            PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY,
            &attribute,
            sizeof(attribute),
            nullptr,
            nullptr) == FALSE)
        {
            auto err{ ::GetLastError() };
            std::wstring errorString = L"Could not update Proc thread attribute.";

            ErrorInformation error(errorString, err);
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
            std::wstring errorString = L"Running process failed.";

            ErrorInformation error(errorString, err);
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

    SHELLEXECUTEINFO shex = { 0 };
    shex.cbSize = sizeof(shex);
    shex.fMask = SEE_MASK_NOCLOSEPROCESS;
    shex.hwnd = (HWND)nullptr;
    shex.lpVerb = nullptr;  // just use the default action for the fta
    shex.lpFile = nonexePath.c_str();
    shex.lpParameters = exeArgString.c_str();
    shex.lpDirectory = dirStr ? (packageRoot / dirStr).c_str() : nullptr;
    shex.nShow = static_cast<WORD>(cmdShow);


    Log("\tUsing Shell launch: %ls %ls", shex.lpFile, shex.lpParameters);
    if (!ShellExecuteEx(&shex))
    {
        auto err{ ::GetLastError() };
        std::wstring errorString = L"ERROR: Failed to create detoured shell process";

        ErrorInformation error(errorString, err);
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

void Log(const char fmt[], ...)
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
void LogString(const char name[], const char value[])
{
    Log("\t%s=%s\n", name, value);
}
void LogString(const char name[], const wchar_t value[])
{
    Log("\t%s=%ls\n", name, value);
}

bool CheckIfPowershellIsInstalled()
{
    HKEY registryHandle;
    DWORD statusOfRegistryKey;
    LSTATUS createResult = RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\PowerShell\\1", 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_READ, nullptr, &registryHandle, &statusOfRegistryKey);

    if (createResult != ERROR_SUCCESS)
    {
        ErrorInformation error(L"Error with getting the key to see if PowerShell is installed. ", createResult);
        ::PSFReportError(error.Print().c_str());
    }

    DWORD valueFromRegistry = 0;
    DWORD bufferSize = sizeof(DWORD);
    DWORD type = REG_DWORD;
    auto getResult = RegQueryValueExW(registryHandle, L"Install", nullptr, &type, reinterpret_cast<BYTE*>(&valueFromRegistry), &bufferSize);

    if (getResult != ERROR_SUCCESS)
    {
        ErrorInformation error(L"Error with querying the key to see if PowerShell is installed. ", createResult);
        ::PSFReportError(error.Print().c_str());
    }

    return valueFromRegistry;
}