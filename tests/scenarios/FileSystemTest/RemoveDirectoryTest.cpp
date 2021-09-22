//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <filesystem>
#include <functional>

#include <test_config.h>

#include "common_paths.h"

extern void Log(const char* fmt, ...);

static int DoRemoveDirectoryTest(const std::filesystem::path& path, bool expectSuccess)
{
    trace_messages(L"Removing directory: ", info_color, path.native(), new_line);
    trace_messages(L"   Expected result: ", info_color, (expectSuccess ? L"Success" : L"Failure"), new_line);

    if (::RemoveDirectoryW(path.c_str()))
    {
        if (!expectSuccess)
        {
            trace_message(L"ERROR: Successfully removed directory, but we were expecting failure\n", error_color);
            return ERROR_ASSERTION_FAILURE;
        }
    }
    else if (expectSuccess)
    {
        return trace_last_error(L"Failed to remove directory, but we were expecting success");
    }

    return ERROR_SUCCESS;
}

int RemoveDirectoryTests()
{
    int result = ERROR_SUCCESS;

    // We shouldn't be able to delete the VFS directory, particularly because it isn't redirected
    test_begin("Remove Non-Configured Directory Test");
    Log("<<<<<Remove Non-Configured Directory Test HERE");
    auto testResult = DoRemoveDirectoryTest(L"VFS", false);
    Log("Remove Non-Configured Directory Test >>>>>");
    result = result ? result : testResult;
    test_end(testResult);

    test_begin("Remove Empty Redirected Directory Test");
    Log("<<<<<Remove Empty Directory Test HERE");
    testResult = []()
    {
        clean_redirection_path();
        trace_message(L"Creating a directory that we can then validate that we can remove\n");
        if (!::CreateDirectoryW(L"TèƨƭÐïřèçƭôř¥", nullptr))
        {
            return trace_last_error(L"Failed to create test directory");
        }

        return DoRemoveDirectoryTest(L"TèƨƭÐïřèçƭôř¥", true);
    }();
    Log("Remove Empty Directory Test >>>>>");
    result = result ? result : testResult;
    test_end(testResult);

    test_begin("Remove Non-Empty Directory Test");
    Log("<<<<<Remove Non-Empty Directory Test HERE");
    testResult = []()
    {
        clean_redirection_path();
        trace_message(L"Creating another directory, but this time it won't be empty\n");
        if (!::CreateDirectoryW(L"TèƨƭÐïřèçƭôř¥", nullptr))
        {
            return trace_last_error(L"Failed to create test directory");
        }
        else
        {
            if (!std::filesystem::exists(L"TèƨƭÐïřèçƭôř¥"))
            {
                return trace_last_error(L"Failed to create test directory without returned error");
            }
            else
            {
                if (!write_entire_file(L"TèƨƭÐïřèçƭôř¥\\file.txt", "This file's presence will cause RemoveDirectory to fail"))
                {
                    return trace_last_error(L"Failed to create file");
                }
            }
        }

        return DoRemoveDirectoryTest(L"TèƨƭÐïřèçƭôř¥", false);
    }();
    Log("Remove Non-Empty Directory Test >>>>>");
    result = result ? result : testResult;
    test_end(testResult);

    // NOTE: "Tèƨƭ" is a non-empty directory in the package. Under normal circumstances, without attempts to remove the
    //       contents of the directory, we'd expect that the call would fail. However, due to the limitations around
    //       file/directory deletion, we explicitly ensure that the opposite is true. That is, we give the application
    //       the benefit of the doubt that if it were to try and delete the directory "Tèƨƭ," it had previously tried to
    //       delete the contents of the directory first
    test_begin("Remove Package Directory Test");
    clean_redirection_path();
    Log("<<<<<Remove Package Directory Test HERE");
    testResult = DoRemoveDirectoryTest(L"Tèƨƭ", true);
    Log("Remove Package Directory Test >>>>>");
    result = result ? result : testResult;
    test_end(testResult);

    return result;
}
