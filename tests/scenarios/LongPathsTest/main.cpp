//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <cstdlib>

#include <windows.h>

#include <shim_utils.h>
#include <test_config.h>

#include "paths.h"

// NOTE: Not every function is tested here. We only try and ensure that all of the "interesting" scenarios are covered,
//       which in general includes:
//  * Copying an existing file to the redirected path (i.e. a copy-on-read operation), which is covered by (at least)
//    calling CreateFile with the path to an existing file
//  * Shouldn't copy-on-read if the file has already been copied
//  * An operation that checks for file presence instead of doing a copy-on-read, which is covered by (at least) calling
//    CopyFile with the path to a file that exists in the package.
//  * Deleting a file/directory that exists in the redirected path
//  * Enumerating files both in the package path and redirected path
int CopyFileTest();
int CreateRemoveDirectoryTest();
int CreateDeleteFileTest();
int FileEnumerationTest();

int RunTests()
{
    int result = ERROR_SUCCESS;
    int testResult;

    testResult = CopyFileTest();
    result = result ? result : testResult;

    testResult = CreateRemoveDirectoryTest();
    result = result ? result : testResult;

    testResult = CreateDeleteFileTest();
    result = result ? result : testResult;

    testResult = FileEnumerationTest();
    result = result ? result : testResult;

    return result;
}

int wmain(int argc, const wchar_t** argv)
{
    auto result = parse_args(argc, argv);
    if (result == ERROR_SUCCESS)
    {
        test_initialize("Long Paths Tests", 4);

        // The whole purpose of this test is that the application should _think_ that it is accessing files with path
        // lengths that are less than MAX_PATH, when in reality it isn't. Validate that first since otherwise the remainder
        // of the test may be pointless
        if (g_testFilePath.native().length() >= MAX_PATH)
        {
            std::wcout << error_text() << L"ERROR: Non-virtualized test file path is longer than MAX_PATH\n";
            std::wcout << error_text() << L"ERROR: Full path is: " << error_info_text() << g_testFilePath.native() << L"\n";
            return ERROR_ASSERTION_FAILURE;
        }
        else if (auto pkgPath = shims::current_package_path() / LR"(VFS\ProgramFilesX??)" / g_programFilesRelativeTestPath / L"file.txt";
            pkgPath.native().length() <= MAX_PATH)
        {
            std::wcout << error_text() << L"ERROR: Package-relative test file path is not longer than MAX_PATH\n";
            std::wcout << error_text() << L"ERROR: Full path is: " << error_info_text() << g_testFilePath.native() << L"\n";
            return ERROR_ASSERTION_FAILURE;
        }

        result = RunTests();

        test_cleanup();
    }

    if (!g_testRunnerPipe)
    {
        system("pause");
    }

    return result;
}
