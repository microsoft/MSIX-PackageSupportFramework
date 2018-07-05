
#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>

#include <windows.h>
#include <shim_constants.h>
#include <shim_runtime.h>

using namespace std::literals;

int launcher_main(PWSTR args, int cmdShow) noexcept try
{
    auto appConfig = ShimQueryCurrentAppLaunchConfig();
    if (!appConfig)
    {
        throw_win32(ERROR_NOT_FOUND, "Error: could not find matching appid in config.json and appx manifest");
    }

    auto exeName = appConfig->get("executable").as_string().wide();
    auto dirPtr = appConfig->try_get("workingDirectory");
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
        dirStr ? (packageRoot / dirStr).c_str() : nullptr, // NOTE: extended-length path not supported
        &startupInfo,
        &processInfo))
    {
        auto error = std::wstring{ L"ERROR: Failed to create detoured process\nERROR: Path: \"" } + exeName + L"\"";
        ShimReportError(error.c_str());
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
catch (...)
{
    std::wostringstream ss;
    auto error_message{ widen(message_from_caught_exception()) };
    auto error_code{ win32_from_caught_exception() };
    ss << error_message << " (" << std::hex << error_code << ")";
    ::ShimReportError(ss.str().c_str());
    return error_code;
}

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR args, int cmdShow)
{
    return launcher_main(args, cmdShow);
}
