//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <filesystem>
#include <functional>

#include <test_config.h>

#include "common_paths.h"

static int DoReplaceFileTest(
    const std::filesystem::path& from,
    const std::filesystem::path& to,
    const std::filesystem::path& backup,
    const char* expectedContents)
{
    trace_messages(L"Replacing file: ", info_color, to.native(), new_line);
    trace_messages(L"          with: ", info_color, from.native(), new_line);
    trace_messages(L"  using backup: ", info_color, backup.native(), new_line);

    auto replacedContents = read_entire_file(to.c_str());
    trace_messages(L"Replaced file initial contents: ", info_color, replacedContents.c_str(), new_line);

    if (!::ReplaceFileW(to.c_str(), from.c_str(), backup.c_str(), 0, nullptr, nullptr))
    {
        return trace_last_error(L"Failed to replace the file");
    }

    auto contents = read_entire_file(to.c_str());
    trace_messages(L"Replaced file final contents: ", info_color, contents.c_str(), new_line);
    if (contents != expectedContents)
    {
        trace_message(L"ERROR: File contents did not match the expected value\n", error_color);
        trace_messages(error_color, L"ERROR: Expected contents: ", error_info_color, expectedContents, new_line);
        return ERROR_ASSERTION_FAILURE;
    }

    contents = read_entire_file(backup.c_str());
    trace_messages(L"Backup file contents: ", info_color, contents.c_str(), new_line);
    if (contents != replacedContents)
    {
        trace_message(L"ERROR: Backup file contents should have matched the initial file contents\n", error_color);
        return ERROR_ASSERTION_FAILURE;
    }

    return ERROR_SUCCESS;
}

int ReplaceFileTests()
{
    int result = ERROR_SUCCESS;

    auto packageFilePath = g_packageRootPath / g_packageFileName;
    auto otherFilePath = g_packageRootPath / L"MôƲèFïℓè.txt";
    auto backupFilePath = g_packageRootPath / L"ßáçƙúƥFïℓè.txt";

    // Copy from the package file
    test_begin("Replace Redirected File with Package File Test");
    clean_redirection_path();
    write_entire_file(otherFilePath.c_str(), "You are reading file contents that should have been replaced");
    auto testResult = DoReplaceFileTest(packageFilePath, otherFilePath, backupFilePath, g_packageFileContents);
    result = result ? result : testResult;
    test_end(testResult);

    // Copy to the package file
    test_begin("Replace Package File with Redirected File Test");
    clean_redirection_path();
    const char generatedFileContents[] = "You are reading from the generated file";
    write_entire_file(otherFilePath.c_str(), generatedFileContents);
    testResult = DoReplaceFileTest(otherFilePath, packageFilePath, backupFilePath, generatedFileContents);
    result = result ? result : testResult;
    test_end(testResult);

    // Copy from the redirected package file
    test_begin("Replace Redirected File with Redirected Package File Test");
    clean_redirection_path();
    const char replacedFileContents[] = "You are reading from the redirected package file";
    write_entire_file(packageFilePath.c_str(), replacedFileContents);
    write_entire_file(otherFilePath.c_str(), "You are reading file contents that should have been replaced");
    testResult = DoReplaceFileTest(packageFilePath, otherFilePath, backupFilePath, replacedFileContents);
    result = result ? result : testResult;
    test_end(testResult);

    return result;
}
