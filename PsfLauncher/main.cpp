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

using namespace std::literals;

//Forward declarations
DWORD StartProcess(LPCWSTR applicationName, std::wstring commandLine, std::wstring currentDirectory, std::wstring exeName, int cmdShow);
int launcher_main(PWSTR args, int cmdShow) noexcept;
DWORD RunScript(const psf::json_object * scriptInformation, LPCWSTR applicationName, std::filesystem::path packageRoot, std::filesystem::path dirStr);
void GetAndLaunchMonitor(const psf::json_object * monitor, std::filesystem::path packageRoot);
void LaunchMonitorInBackground(std::filesystem::path packageRoot, const wchar_t * executable, const wchar_t * arguments, bool wait, bool asadmin);
static inline bool check_suffix_if(iwstring_view str, iwstring_view suffix);
void LogStringW(const char* name, const wchar_t* value);
void LogString(const char* name, const char* value);
void Log(const char* fmt, ...);
DWORD StartWithShellExecute(std::filesystem::path packageRoot, std::filesystem::path exeName, std::wstring exeArgString, LPCWSTR dirStr, int cmdShow);

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR args, int cmdShow)
{
    return launcher_main(args, cmdShow);
}

int launcher_main(PWSTR args, int cmdShow) noexcept try
{
    //TEST
    MessageBoxEx(NULL, L"In here", L"In here", 0, 0);
    Log("\tIn Launcher_main()");
    auto appConfig = PSFQueryCurrentAppLaunchConfig(true);
    if (!appConfig)
    {
        throw_win32(ERROR_NOT_FOUND, "Error: could not find matching appid in config.json and appx manifest");
    }

    auto dirPtr = appConfig->try_get("workingDirectory");
    auto dirStr = dirPtr ? dirPtr->as_string().wide() : nullptr;
    auto exeArgs = appConfig->try_get("arguments");


    // At least for now, configured launch paths are relative to the package root
    std::filesystem::path packageRoot = PSFQueryPackageRootPath();


    //Launch monitor if we are using one.
    auto monitor = PSFQueryAppMonitorConfig();
    if (monitor != nullptr)
    {
        GetAndLaunchMonitor(monitor, packageRoot);
    }


    //Launch the starting powershell script if we are using one.
    auto startScriptInformation = PSFQueryStartScriptInfo();
    if(startScriptInformation)
    { 
        RunScript(startScriptInformation, nullptr, packageRoot, dirStr);
    }

    //Launch underlying application.
    auto exeName = appConfig->get("executable").as_string().wide();
    auto exePath = packageRoot / exeName;
    auto exeArgString = exeArgs ? exeArgs->as_string().wide() : (wchar_t*)L"";
    // Allow arguments to be specified in config.json now also.
    std::wstring cmdLine = L"\"" + exePath.filename().native() + L"\" " + exeArgString + L" " + args;
    if (check_suffix_if(exeName, L".exe"_isv))
    {
        auto directory = dirStr ? (packageRoot / dirStr).c_str() : nullptr;
        StartProcess(exePath.c_str(), cmdLine.data(), directory, exeName, cmdShow);
    }
    else
    {
        StartWithShellExecute(packageRoot, exeName, exeArgString, dirStr, cmdShow);
    }

    //Launch the end powershell script if we are using one.
    auto endScriptInformation = PSFQueryEndScriptInfo();
    if (endScriptInformation)
    {
        RunScript(endScriptInformation, nullptr, packageRoot, dirStr);
    }

    return 0;
}
catch (...)
{
    ::PSFReportError(widen(message_from_caught_exception()).c_str());
    return win32_from_caught_exception();
}

DWORD RunScript(const psf::json_object * scriptInformation, LPCWSTR applicationName, std::filesystem::path packageRoot, std::filesystem::path dirStr)
{
    //auto startScript = PSFQueryStartScriptInfo();
    auto scriptPath = scriptInformation->get("scriptPath").as_string().wide();
    auto scriptArguments = scriptInformation->get("scriptArguments").as_string().wide();
    auto currentDirectory = (packageRoot / dirStr);

    std::filesystem::path powershellScriptPath(scriptPath);

    std::wstring powershellCommandString(L"powershell.exe -file ");

    powershellCommandString.append(scriptPath);
    powershellCommandString.append(L" ");
    powershellCommandString.append(scriptArguments);
    
    auto doesFileExist = std::filesystem::exists(currentDirectory / powershellScriptPath);

    if (doesFileExist)
    {
        //TODO Change this to an enum, not a magic number
        return StartProcess(applicationName, powershellCommandString, dirStr, L"Powershell", 5);
    }
    else
    {
        return ERROR_FILE_NOT_FOUND;
    }
}

void GetAndLaunchMonitor(const psf::json_object * monitor, std::filesystem::path packageRoot)
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
    LaunchMonitorInBackground(packageRoot, monitor_executable->as_string().wide(), monitor_arguments->as_string().wide(), wait, asadmin);
}

void LaunchMonitorInBackground(std::filesystem::path packageRoot, const wchar_t * executable, const wchar_t * arguments, bool wait, bool asadmin)
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
            shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_WAITFORINPUTIDLE;  // make sure we wait a bit for the monitor to be running before continuing on.
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
        std::wstring cmdarg = cmd + L" " + arguments;

        StartProcess(nullptr, cmdarg.data(), nullptr, executable, 1);

        //PROCESS_INFORMATION processInfo;
        //STARTUPINFO startupInfo = { sizeof(startupInfo) };
        //startupInfo.dwFlags = STARTF_USESHOWWINDOW;
        //startupInfo.wShowWindow = static_cast<WORD>(1);

        //if (!::CreateProcessW(
        //    nullptr, //quotedapp.data(),
        //    (wchar_t *)cmdarg.c_str(),
        //    nullptr, nullptr, // Process/ThreadAttributes
        //    true, // InheritHandles
        //    0, // CreationFlags
        //    nullptr, // Environment
        //    nullptr,
        //    &startupInfo,
        //    &processInfo))
        //{
        //    if (::GetLastError() == ERROR_ELEVATION_REQUIRED)
        //        Log("error starting monitor using CreateProcessW. You must specify 'monitor/asadmin' in config.json\n");
        //    else
        //        Log("error starting monitor using CreateProcessW. Error=0x%x\n", ::GetLastError());
        //}
        //else
        //{
        //    if (wait)
        //        WaitForSingleObject(processInfo.hProcess, INFINITE);
        //}
    }
}

DWORD StartProcess(LPCWSTR applicationName, std::wstring commandLine, std::wstring currentDirectory, std::wstring exeName, int cmdShow)
{
    STARTUPINFO startupInfo = { sizeof(startupInfo) };
    startupInfo.dwFlags = STARTF_USESHOWWINDOW;
    startupInfo.wShowWindow = static_cast<WORD>(cmdShow);

    PROCESS_INFORMATION processInfo;
    if (::CreateProcessW(
        applicationName,
        commandLine.data(),
        nullptr, nullptr, // Process/ThreadAttributes
        true, // InheritHandles
        0, // CreationFlags
        nullptr, // Environment
        currentDirectory.c_str(),
        &startupInfo,
        &processInfo))
    {
        // Propagate exit code to caller, in case they care
        switch (::WaitForSingleObject(processInfo.hProcess, INFINITE))
        {
        case WAIT_OBJECT_0:
            break;

        case WAIT_FAILED:
            return ::GetLastError();

        default:
            return ERROR_INVALID_HANDLE;
        }
    }
    else
    {
        std::wostringstream ss;
        auto err{ ::GetLastError() };
        // Remove the ".\r\n" that gets added to all messages
        auto msg = widen(std::system_category().message(err));
        msg.resize(msg.length() - 3);
        ss << L"ERROR: Failed to create a process for " << applicationName 
            << " Path: \"" << currentDirectory << "\\" << exeName << "\" "
            << "Error: " << msg << " (" << err << ")";
        ::PSFReportError(ss.str().c_str());
        return err;
    }

    DWORD exitCode;
    if (!::GetExitCodeProcess(processInfo.hProcess, &exitCode))
    {
        return ::GetLastError();
    }

    return exitCode;
}

DWORD StartWithShellExecute(std::filesystem::path packageRoot, std::filesystem::path exeName, std::wstring exeArgString, LPCWSTR dirStr, int cmdShow)
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