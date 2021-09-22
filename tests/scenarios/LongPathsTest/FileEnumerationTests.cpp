//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <set>

#include <test_config.h>
#include <utilities.h>

#include "paths.h"
extern void Log(const char* fmt, ...);

static int DoFileEnumerationTest()
{
    clean_redirection_path();
    trace_messages(L"Enumerating the txt files under: ", info_color, g_testPath.native(), new_line);

    // First, create a new file so that FindNextFile actually does something
    write_entire_file((g_testPath / L"other.txt").c_str(), "This is just a dummy file");
	write_entire_file((g_testPath / L"file.txt").c_str(), "This is just a dummy file");

    WIN32_FIND_DATAW data;
    auto findHandle = ::FindFirstFileW((g_testPath / L"*.txt").c_str(), &data);
    if (findHandle == INVALID_HANDLE_VALUE)
    {
        return trace_last_error(L"FindFirstFile failed");
    }

    std::set<iwstring> expected = { L"file.txt", L"other.txt" };
    while (findHandle != INVALID_HANDLE_VALUE)
    {
        auto itr = expected.find(data.cFileName);
        if (itr == expected.end())
        {
            trace_messages(error_color, L"ERROR: Unexpected file found: ", error_info_color, data.cFileName, new_line);
            ::FindClose(findHandle);
            return ERROR_ASSERTION_FAILURE;
        }
        trace_messages(L"    Found file: ", info_color, data.cFileName, new_line);
        expected.erase(itr);

        if (!::FindNextFileW(findHandle, &data))
        {
            if (::GetLastError() != ERROR_NO_MORE_FILES)
            {
                ::FindClose(findHandle);
                return trace_last_error(L"FindNextFile failed");
            }

            ::FindClose(findHandle);
            findHandle = INVALID_HANDLE_VALUE;
        }
    }

    if (!expected.empty())
    {
        trace_message(L"ERROR: Failed to find files:\n", error_color);
        for (auto& file : expected)
        {
            trace_messages(error_info_color, L"    ", file.c_str(), new_line);
        }
        return ERROR_ASSERTION_FAILURE;
    }

    return ERROR_SUCCESS;
}

int FileEnumerationTest()
{
    test_begin("File Enumeration Test");
    Log("<<<<<LongPathsTest File Enumeration");
    auto result = DoFileEnumerationTest();
    Log("LongPathsTest File Enumeration>>>>>");
    test_end(result);
    return result;
}
