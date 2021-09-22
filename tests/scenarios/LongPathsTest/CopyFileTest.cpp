//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <test_config.h>

#include "paths.h"


void Log(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::string str;
    str.resize(256);
    try
    {
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
    }
    catch (...)
    {
        str = fmt;
    }
    ::OutputDebugStringA(str.c_str());
}


static int DoCopyFileTest()
{
    clean_redirection_path();

    auto copyPath = g_testPath / L"copy.txt";
    trace_messages(L"Copying from: ", info_color, g_testFilePath.native(), new_line);
    trace_messages(L"          to: ", info_color, copyPath.native(), new_line);
	write_entire_file(g_testFilePath.c_str(), g_expectedFileContents);
    if (!::CopyFileW(g_testFilePath.c_str(), copyPath.c_str(), false))
    {
        // The source file does  exist
        ////if (::GetLastError() == ERROR_PATH_NOT_FOUND)
        ////{
        ////    trace_message("Success (PATH_NOT_FOUND).", info_color, true);
        ////    return ERROR_SUCCESS;
        ///}
        return trace_last_error(L"CopyFile failed");
    }
    else if (auto contents = read_entire_file(copyPath.c_str()); contents != g_expectedFileContents)
    {
        trace_message(L"ERROR: Copied file contents do not match expected contents\n", error_color);
        trace_messages(error_color, L"ERROR: Expected contents: ", error_info_color, g_expectedFileContents, new_line);
        trace_messages(error_color, L"ERROR: File contents: ", error_info_color, contents, new_line);
        return ERROR_ASSERTION_FAILURE;
    }

    return ERROR_SUCCESS;
}

int CopyFileTest()
{
    test_begin("CopyFile Test");
    Log("<<<<<LongPathsTest CopyFile");
    auto result = DoCopyFileTest();
    Log("LongPathsTest CopyFile>>>>>");
    test_end(result);
    return result;
}
