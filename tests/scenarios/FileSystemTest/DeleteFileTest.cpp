//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <filesystem>
#include <functional>

#include <test_config.h>

#include "common_paths.h"

int DoDeleteFileTest()
{
    auto packageFilePath = g_packageRootPath / g_packageFileName;

    // NOTE: Since we don't have any mechanism in place for remembering deletes to files in the package path, this test
    //       is rather mundane. That said, the initial attempt to delete the file should at the very least appear as
    //       though it succeeded.
    clean_redirection_path();
    trace_messages(L"Attempting to delete the package file: ", info_color, packageFilePath.native(), new_line);
    if (!::DeleteFileW(packageFilePath.c_str()))
    {
        return trace_last_error(L"Attempt to delete package file failed");
    }

    trace_message(L"Writing data to the file so that it exists in the redirected location...\n");
    write_entire_file(packageFilePath.c_str(), "Redirected file contents");
    auto actualPath = actual_path(packageFilePath.c_str());
    trace_message(L"Deleting the file again...\n");
    if (!::DeleteFileW(packageFilePath.c_str()))
    {
        return trace_last_error(L"Attempt to delete redirected file failed");
    }
    else if (std::filesystem::exists(actualPath))
    {
        trace_message(L"ERROR: DeleteFile returned success, but the redirected file still exists\n", error_color);
        trace_messages(error_color, L"ERROR: Redirected file path: ", error_info_color, actualPath.native(), new_line);
        return ERROR_ASSERTION_FAILURE;
    }

    return ERROR_SUCCESS;
}

int DeleteFileTests()
{
    test_begin("DeleteFile Test");
    auto result = DoDeleteFileTest();
    test_end(result);

    return result;
}
