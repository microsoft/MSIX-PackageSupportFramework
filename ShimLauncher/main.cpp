
#include <filesystem>
#include <fstream>
#include <string>

#include <windows.h>
#include <shim_constants.h>
#include <shim_runtime.h>

using namespace std::literals;

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR args, int cmdShow)
{
    auto appConfig = ShimQueryCurrentAppLaunchConfig();
    if (!appConfig)
    {
        return ERROR_NOT_FOUND;
    }

    auto exeName = appConfig->get("executable").as_string().wide();
    auto dirPtr = appConfig->try_get("working_directory");
    auto dirStr = dirPtr ? dirPtr->as_string().wide() : nullptr;

    // At least for now, configured launch paths are relative to the package root
    std::filesystem::path packageRoot = ShimQueryPackageRootPath();
    auto exePath = packageRoot / exeName;
    std::wstring cmdLine = L"\"" + exePath.filename().native() + L"\" " + args;

    STARTUPINFO startupInfo = { sizeof(startupInfo) };
    startupInfo.dwFlags = STARTF_USESHOWWINDOW;
    startupInfo.wShowWindow = static_cast<WORD>(cmdShow);

    // NOTE: ShimRuntime, which we dynamically link against, detours this call
    PROCESS_INFORMATION processInfo;
    if (!::CreateProcessW(
        exePath.c_str(),
        cmdLine.data(),
        nullptr, nullptr, // Process/ThreadAttributes
        true, // InheritHandles
        0, // CreationFlags
        nullptr, // Environment
        dirStr ? (packageRoot / dirStr).c_str() : nullptr,
        &startupInfo,
        &processInfo))
    {
        ::OutputDebugStringW(L"ERROR: Failed to create detoured process\nERROR: Path: \"");
        ::OutputDebugStringW(exeName);
        ::OutputDebugStringW(L"\"\n");
        return ::GetLastError();
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
