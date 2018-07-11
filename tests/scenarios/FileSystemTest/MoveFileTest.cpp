//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <filesystem>
#include <functional>

#include <test_config.h>

#include "common_paths.h"

auto MoveFileFunc = &::MoveFileW;
auto MoveFileExFunc = [](LPCWSTR from, LPCWSTR to)
{
    return ::MoveFileExW(from, to, MOVEFILE_WRITE_THROUGH);
};

static int DoMoveFileTest(
    const std::function<BOOL(PCWSTR, PCWSTR)>& moveFn,
    const char* moveFnName,
    const std::filesystem::path& from,
    const std::filesystem::path& to,
    const char* expectedContents)
{
    trace_messages(L"Moving file from: ", info_color, from.native(), new_line);
    trace_messages(L"              to: ", info_color, to.native(), new_line);
    trace_messages(L"  Using function: ", info_color, moveFnName, new_line);

    // The initial move should succeed
    if (!moveFn(from.c_str(), to.c_str()))
    {
        return trace_last_error(L"Initial move of the file failed");
    }

    auto contents = read_entire_file(to.c_str());
    trace_messages(L"Moved file contents: ", info_color, contents.c_str(), new_line);
    if (contents != expectedContents)
    {
        trace_message(L"ERROR: Moved file contents did not match the expected contents\n", error_color);
        trace_messages(error_color, L"ERROR: Expected contents: ", error_info_color, expectedContents, new_line);
        return ERROR_ASSERTION_FAILURE;
    }

    // Trying to move again should fail. Note that we might need to re-create the moved-from file since it should have
    // been deleted
    write_entire_file(from.c_str(), "You are reading the re-created file contents");
    trace_message(L"Moving the file again to ensure that the call fails due to the destination already existing...\n");
    if (moveFn(from.c_str(), to.c_str()))
    {
        trace_message(L"ERROR: Second move succeeded\n", error_color);
        return ERROR_ASSERTION_FAILURE;
    }
    else if (::GetLastError() != ERROR_ALREADY_EXISTS)
    {
        return trace_last_error(L"The second move failed as expected, but the error was not set to ERROR_ALREADY_EXISTS");
    }

    return ERROR_SUCCESS;
}

static int DoMoveToReplaceFileTest(const std::filesystem::path& from, const std::filesystem::path& to)
{
    const char newContents[] = "You are reading modified text";
    write_entire_file(from.c_str(), newContents);
    trace_message(L"Validating that MoveFileEx can replace the target file...\n");
    if (!::MoveFileExW(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        return trace_last_error(L"Failed to replace the file");
    }

    auto contents = read_entire_file(to.c_str());
    trace_messages(L"File contents are now: ", info_color, contents.c_str(), new_line);
    if (contents != newContents)
    {
        trace_message(L"ERROR: Moved file contents did not match the expected contents\n", error_color);
        trace_messages(error_color, L"ERROR: Expected contents: ", error_info_color, newContents, new_line);
        return ERROR_ASSERTION_FAILURE;
    }

    return ERROR_SUCCESS;
}

int MoveFileTests()
{
    int result = ERROR_SUCCESS;

    auto packageFilePath = g_packageRootPath / g_packageFileName;
    auto otherFilePath = g_packageRootPath / L"MôƲèFïℓè.txt";

    // Move from a file in the package path
    test_begin("MoveFile From Package Path Test");
    clean_redirection_path();
    auto testResult = DoMoveFileTest(MoveFileFunc, "MoveFile", packageFilePath, otherFilePath, g_packageFileContents);
    result = result ? result : testResult;
    test_end(testResult);

    test_begin("MoveFileEx From Package Path Test");
    clean_redirection_path();
    testResult = DoMoveFileTest(MoveFileExFunc, "MoveFileEx", packageFilePath, otherFilePath, g_packageFileContents);
    result = result ? result : testResult;
    test_end(testResult);

    // MoveFileEx has the ability to replace an existing file, so go ahead and test that here
    test_begin("MoveFileEx Replace Redirected File Test");
    testResult = DoMoveToReplaceFileTest(packageFilePath, otherFilePath);
    result = result ? result : testResult;
    test_end(testResult);

    // Move from a created file to a file in the package path
    // NOTE: Ideally the move with would fail, but due to the limitations around file deletion, we explicitly ensure
    //       that the call won't fail. If this is ever changes, this test will start to fail, at which point we'll need
    //       to update this assumption
    const char otherFileContents[] = "You are reading from the generated file";

    test_begin("MoveFile Replace Package File Test");
    clean_redirection_path();
    write_entire_file(otherFilePath.c_str(), otherFileContents);
    testResult = DoMoveFileTest(MoveFileFunc, "MoveFile", otherFilePath, packageFilePath, otherFileContents);
    result = result ? result : testResult;
    test_end(testResult);

    test_begin("MoveFileEx Replace Package File Test");
    clean_redirection_path();
    write_entire_file(otherFilePath.c_str(), otherFileContents);
    testResult = DoMoveFileTest(MoveFileExFunc, "MoveFileEx", otherFilePath, packageFilePath, otherFileContents);
    result = result ? result : testResult;
    test_end(testResult);

    // Again, validate that MoveFileEx has the ability to replace an existing file
    test_begin("MoveFileEx Replace Redirected Package File Test");
    testResult = DoMoveToReplaceFileTest(otherFilePath, packageFilePath);
    result = result ? result : testResult;
    test_end(testResult);

    return result;
}
