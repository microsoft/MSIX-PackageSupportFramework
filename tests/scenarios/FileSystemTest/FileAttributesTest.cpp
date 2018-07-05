
#include <filesystem>
#include <functional>

#include <test_config.h>

#include "attributes.h"
#include "common_paths.h"

auto GetFileAttributesFunc = &::GetFileAttributesW;
auto GetFileAttributesExFunc = [](PCWSTR path)
{
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!::GetFileAttributesExW(path, GetFileExInfoStandard, &data))
    {
        return INVALID_FILE_ATTRIBUTES;
    }

    return data.dwFileAttributes;
};

static int DoFileAttributesTest(const std::function<DWORD(PCWSTR)>& getFn, const char* getFnName, const std::filesystem::path& path)
{
    trace_messages(L"Querying the attributes for file: ", info_color, path.native(), new_line);
    trace_messages(L"                  Using function: ", info_color, getFnName, new_line);
    auto attr = getFn(path.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES)
    {
        return trace_last_error(L"Failed to query the file attributes");
    }

    trace_messages(L"Initial file attributes: ", info_color, file_attributes(attr), new_line);
    if (attr == FILE_ATTRIBUTE_HIDDEN)
    {
        trace_message(L"WARNING: File already has the FILE_ATTRIBUTE_HIDDEN attribute\n", warning_color);
        trace_message(L"WARNING: This may cause false negatives\n", warning_color);
    }

    trace_messages(L"Setting the file's attributes to: ", info_color, L"FILE_ATTRIBUTE_HIDDEN\n");
    if (!::SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_HIDDEN))
    {
        return trace_last_error(L"Failed to set the file's attributes");
    }

    attr = getFn(path.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES)
    {
        return trace_last_error(L"Failed to query the file attributes");
    }

    trace_messages(L"New file attributes: ", info_color, file_attributes(attr), new_line);
    if (attr != FILE_ATTRIBUTE_HIDDEN)
    {
        trace_message(L"ERROR: File attributes set incorrectly\n", error_color);
        return ERROR_ASSERTION_FAILURE;
    }

    return ERROR_SUCCESS;
}

int FileAttributesTests()
{
    int result = ERROR_SUCCESS;

    auto packageFilePath = g_packageRootPath / g_packageFileName;

    test_begin("GetFileAttributes Test");
    clean_redirection_path();
    auto testResult = DoFileAttributesTest(GetFileAttributesFunc, "GetFileAttributes", packageFilePath);
    result = result ? result : testResult;
    test_end(testResult);

    test_begin("GetFileAttributesEx Test");
    clean_redirection_path();
    testResult = DoFileAttributesTest(GetFileAttributesExFunc, "GetFileAttributesEx", packageFilePath);
    result = result ? result : testResult;
    test_end(testResult);

    return result;
}
