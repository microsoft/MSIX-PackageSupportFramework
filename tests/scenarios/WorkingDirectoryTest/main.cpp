
#include <filesystem>
#include <iostream>

#include <Windows.h>

#include <error_logging.h>
#include <shim_utils.h>
#include <test_config.h>

using namespace std::literals;

int run()
{
    test_begin("File Presence Test");

    auto requiredSize = ::GetCurrentDirectoryW(0, nullptr);
    std::wstring currentDir(requiredSize - 1, L'\0');
    currentDir.resize(::GetCurrentDirectoryW(requiredSize, currentDir.data()));
    trace_messages(L"Current directory: ", info_color, currentDir, new_line);

    int result = std::filesystem::exists(L"foo.txt") ? ERROR_SUCCESS : ERROR_FILE_NOT_FOUND;
    if (!result)
    {
        trace_message(L"SUCCESS: Working directory set correctly!\n", success_color);
    }
    else
    {
        trace_message(L"ERROR: Working directory not set correctly!\n", error_color);
    }

    test_end(result);
    return result;
}

int wmain(int argc, const wchar_t** argv)
{
    auto result = parse_args(argc, argv);
    if (result == ERROR_SUCCESS)
    {
        test_initialize("Working Directory Tests", 1);
        result = run();

        test_cleanup();
    }

    if (!g_testRunnerPipe)
    {
        system("pause");
    }

    return result;
}
