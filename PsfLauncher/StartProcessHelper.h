#pragma once
#include <windows.h>
#include "Logger.h"
#include <wil\resource.h>


void MakeAttributeList(LPPROC_THREAD_ATTRIBUTE_LIST &attributeList)
{
    SIZE_T AttributeListSize{};

    InitializeProcThreadAttributeList(nullptr, 1, 0, &AttributeListSize);
    attributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(
        GetProcessHeap(),
        0,
        AttributeListSize);

    THROW_LAST_ERROR_IF_MSG(
        !InitializeProcThreadAttributeList(
            attributeList,
            1,
            0,
            &AttributeListSize),
        "Could not initialize the proc thread attribute list.");

    DWORD attribute = 0x02;
    THROW_LAST_ERROR_IF_MSG(
        !UpdateProcThreadAttribute(
            attributeList,
            0,
            ProcThreadAttributeValue(18, FALSE, TRUE, FALSE),
            &attribute,
            sizeof(attribute),
            nullptr,
            nullptr),
        "Could not update Proc thread attribute.");
}

HRESULT StartProcess(LPCWSTR applicationName, LPWSTR commandLine, LPCWSTR currentDirectory, int cmdShow, DWORD timeout)
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
    LPPROC_THREAD_ATTRIBUTE_LIST attributeList;
    MakeAttributeList(attributeList);
    startupInfoEx.lpAttributeList = attributeList;

    RETURN_LAST_ERROR_IF_MSG(
        !::CreateProcessW(
            applicationName,
            commandLine,
            nullptr, nullptr, // Process/ThreadAttributes
            true, // InheritHandles
            EXTENDED_STARTUPINFO_PRESENT, // CreationFlags
            nullptr, // Environment
            currentDirectory,
            (LPSTARTUPINFO)&startupInfoEx,
            &processInfo),
        "ERROR: Failed to create a process for %ws",
        applicationName);

    RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE), processInfo.hProcess == INVALID_HANDLE_VALUE);
    DWORD waitResult = ::WaitForSingleObject(processInfo.hProcess, timeout);
    RETURN_LAST_ERROR_IF_MSG(waitResult != WAIT_OBJECT_0, "Waiting operation failed unexpectedly.");
    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    return ERROR_SUCCESS;
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
    THROW_LAST_ERROR_IF_MSG(
        !ShellExecuteEx(&shex),
        "ERROR: Failed to create detoured shell process");

    THROW_LAST_ERROR_IF(shex.hProcess == INVALID_HANDLE_VALUE);
    DWORD exitCode{};
    THROW_IF_WIN32_ERROR(GetExitCodeProcess(shex.hProcess, &exitCode));
    THROW_IF_WIN32_ERROR(exitCode);
    CloseHandle(shex.hProcess);
}

