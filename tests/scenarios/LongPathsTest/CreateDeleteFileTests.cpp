//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <test_config.h>

#include "paths.h"

extern void Log(const char* fmt, ...);

static int DoCreateDeleteFileTest()
{
    clean_redirection_path();

    trace_messages(L"Opening existing file: ", info_color, g_testFilePath.native(), new_line);

    auto file = ::CreateFileW(
        g_testFilePath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        return trace_last_error(L"CreateFile failed");
    }

    // The file should have successfully gotten copied over
    char buffer[256];
    DWORD bytesRead;
    if (!::ReadFile(file, buffer, sizeof(buffer) - 1, &bytesRead, nullptr))
    {
        ::CloseHandle(file);
        return trace_last_error(L"ReadFile failed");
    }
    else if (buffer[bytesRead] = '\0'; std::strcmp(g_expectedFileContents, buffer))
    {
        ::CloseHandle(file);
        trace_message(L"ERROR: Copied file contents do not match expected contents\n", error_color);
        trace_messages(error_color, L"ERROR: Expected contents: ", error_info_color, g_expectedFileContents, new_line);
        trace_messages(error_color, L"ERROR: File contents: ", error_info_color, buffer, new_line);
        return ERROR_ASSERTION_FAILURE;
    }

    ::CloseHandle(file);

    trace_message(L"File successfully opened/read. Modifying its contents to validate that we don't copy it again on the next access\n");
    const char newContents[] = "You are reading from the redirected location";
    write_entire_file(g_testFilePath.c_str(), newContents);
    if (auto contents = read_entire_file(g_testFilePath.c_str()); contents != newContents)
    {
        trace_message(L"ERROR: File contents do not match expected contents\n", error_color);
        trace_messages(error_color, L"ERROR: Expected contents: ", error_info_color, newContents, new_line);
        trace_messages(error_color, L"ERROR: File contents: ", error_info_color, contents, new_line);
        return ERROR_ASSERTION_FAILURE;
    }

    auto path = g_testPath / L"new.txt";
    trace_messages(L"Creating new file: ", info_color, path.native(), new_line);

    file = ::CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        return trace_last_error(L"CreateFile failed");
    }
    ::CloseHandle(file);

    trace_message(L"File successfully created... Now trying to delete it\n");
    if (!::DeleteFileW(path.c_str()))
    {
        return trace_last_error(L"DeleteFile failed");
    }
    else if (std::filesystem::exists(path))
    {
        trace_message(L"ERROR: DeleteFile succeeded, but the file still exists\n", error_color);
        return ERROR_ASSERTION_FAILURE;
    }

    return ERROR_SUCCESS;
}

int CreateDeleteFileTest()
{
    test_begin("Create/Delete File Test");
    Log("<<<<<LongPathsTest Create/Delete File");
    auto result = DoCreateDeleteFileTest();
    Log("LongPathsTest Create/Delete File>>>>>");
    test_end(result);
    return result;

}