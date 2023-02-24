#pragma once
#include <windows.h>
#include "Logger.h"
#include <wil\resource.h>
#include "proc_helper.h"

HRESULT StartProcess(LPCWSTR applicationName, LPWSTR commandLine, LPCWSTR currentDirectory, int cmdShow, DWORD timeout, LPPROC_THREAD_ATTRIBUTE_LIST attributeList = nullptr, _Out_ DWORD *exitCode = nullptr)
{

    STARTUPINFOEXW startupInfoEx =
    {
        {
        sizeof(startupInfoEx)
        , nullptr // lpReserved
        , nullptr // lpDesktop
        , nullptr // lpTitle
        , 0 // dwX
        , 0 // dwY
        , 0 // dwXSize
        , 0 // swYSize
        , 0 // dwXCountChar
        , 0 // dwYCountChar
        , 0 // dwFillAttribute
        , STARTF_USESHOWWINDOW // dwFlags
        , static_cast<WORD>(cmdShow) // wShowWindow
        }
    };
    PROCESS_INFORMATION processInfo{};

    startupInfoEx.lpAttributeList = attributeList;
    DWORD CreationFlags = 0;
    if (attributeList != nullptr)
    {
        CreationFlags = EXTENDED_STARTUPINFO_PRESENT;
    }
    
    RETURN_LAST_ERROR_IF_MSG(
        !::CreateProcessW(
            applicationName,
            commandLine,
            nullptr, nullptr, // Process/ThreadAttributes
            true, // InheritHandles
            CreationFlags,
            nullptr, // Environment
            currentDirectory,
            (LPSTARTUPINFO)&startupInfoEx,
            &processInfo),
        "ERROR: Failed to create a process for %ws",
        applicationName);


    RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE), processInfo.hProcess == INVALID_HANDLE_VALUE);
    DWORD waitResult = ::WaitForSingleObject(processInfo.hProcess, timeout);
    RETURN_LAST_ERROR_IF_MSG(waitResult != WAIT_OBJECT_0, "Waiting operation failed unexpectedly.");
    if (exitCode != nullptr)
    {
        GetExitCodeProcess(processInfo.hProcess, exitCode);
    }
    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    return ERROR_SUCCESS;
}

void StartWithShellExecute(std::filesystem::path packageRoot, std::filesystem::path exeName, std::wstring exeArgString, LPCWSTR dirStr, int cmdShow, DWORD timeout)
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
	THROW_LAST_ERROR_IF_MSG(
		!ShellExecuteEx(&shex),
		"ERROR: Failed to create detoured shell process");

	THROW_LAST_ERROR_IF(shex.hProcess == INVALID_HANDLE_VALUE);
	DWORD exitCode = ::WaitForSingleObject(shex.hProcess, timeout);

    // Don't throw an error as we should assume that the process would have appropriately made indications to the user.  Log for debug purposes only.
    Log("PsfLauncher: Shell Launch: process returned exit code 0x%x", exitCode);

	CloseHandle(shex.hProcess);
}

