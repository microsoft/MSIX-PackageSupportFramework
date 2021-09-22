//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <filesystem>
#include <functional>

#include <test_config.h>

#include "common_paths.h"

using namespace std::literals;

static const char* creation_disposition_text(DWORD creationDisposition)
{
    switch (creationDisposition)
    {
    case CREATE_ALWAYS: return "CREATE_ALWAYS";
    case CREATE_NEW: return "CREATE_NEW";
    case OPEN_ALWAYS: return "OPEN_ALWAYS";
    case OPEN_EXISTING: return "OPEN_EXISTING";
    case TRUNCATE_EXISTING: return "TRUNCATE_EXISTING";
    }

    assert(false);
    return "UNKNOWN";
}

extern void Log(const char* fmt, ...);

int CreateFileTest(const vfs_mapping& mapping)
{
    CREATEFILE2_EXTENDED_PARAMETERS extParams = { sizeof(extParams) };
    extParams.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;

    auto CreateFileFunc = std::bind(&::CreateFileW, std::placeholders::_1, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, std::placeholders::_2, FILE_ATTRIBUTE_NORMAL, nullptr);
    auto CreateFile2Func = std::bind(&::CreateFile2, std::placeholders::_1, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, std::placeholders::_2, &extParams);

    auto createFile = [&](const std::function<HANDLE(PCWSTR, DWORD)>& createFunc, DWORD creationDisposition, const std::filesystem::path& directory) -> int
    {
        // Use a file name that'll be unique to this test running
        auto path = directory / L"ÇřèáƭèFïℓèTèƨƭFïℓè.txt";
        trace_messages(L"Creating File: ", info_color, path.native(), console::color::gray, L" with ", info_color, creation_disposition_text(creationDisposition), new_line);

        // Since we expect each creation function to produce no error, we need to ensure that the file does not exist
        clean_redirection_path();
        Log("<<<<<CreateNew with CreateFile HERE");
        bool expectSuccess = (creationDisposition != OPEN_EXISTING) && (creationDisposition != TRUNCATE_EXISTING);
        auto file = createFunc(path.c_str(), creationDisposition);
        Log("CreateNew with CreateFile >>>>>");
        if (file == INVALID_HANDLE_VALUE)
        {
            if (!expectSuccess)
            {
                if (::GetLastError() != ERROR_FILE_NOT_FOUND)
                {
                    return trace_last_error(L"Failure code should have been ERROR_FILE_NOT_FOUND");
                }

                return ERROR_SUCCESS;
            }

            return trace_last_error(L"Failed to open file");
        }

        // NOTE: The non-fixed version of this function has access to some of the folders (e.g. Common AppData), so
        //       we may hit spurious failures if we try and run the fixed test after the non-fixed test, so try and
        //       delete the file by its path. This is best effort
        ::CloseHandle(file);
        BOOL deleteFileResult = ::DeleteFileW(path.c_str());

        if (!expectSuccess)
        {
            trace_message(L"ERROR: Expected a failure of ERROR_FILE_NOT_FOUND\n", error_color);
            return ERROR_ASSERTION_FAILURE;
        }
        else if (!deleteFileResult)
        {
            return trace_last_error(L"Creating a new file should produce no error");
        }

        return ERROR_SUCCESS;
    };

    auto performTest = [&](const std::function<HANDLE(PCWSTR, DWORD)>& createFunc, const std::filesystem::path& packagePath, const std::filesystem::path& path) -> int
    {
        auto result = createFile(createFunc, CREATE_ALWAYS, packagePath);
        if (result) return result;

        result = createFile(createFunc, CREATE_NEW, path);
        if (result) return result;

        result = createFile(createFunc, OPEN_ALWAYS, packagePath);
        if (result) return result;

        result = createFile(createFunc, OPEN_EXISTING, path);
        if (result) return result;

        result = createFile(createFunc, TRUNCATE_EXISTING, packagePath);
        if (result) return result;

        return ERROR_SUCCESS;
    };

    auto result = performTest(CreateFileFunc, mapping.package_path, mapping.path);
    if (result) return result;

    result = performTest(CreateFile2Func, mapping.package_path, mapping.path);
    if (result) return result;

    return ERROR_SUCCESS;
}

int CreateNewFileTests()
{
    int result = ERROR_SUCCESS;

    for (auto& mapping : g_vfsMappings)
    {
        std::string testName = "Create " + mapping.get().package_path.filename().string() + " File Test";
        test_begin(testName);
        Log("<<<<<Create %s", testName.c_str());
        auto testResult = CreateFileTest(mapping);
        result = result ? result : testResult;
        Log("Create %s>>>>>", testName.c_str());

        test_end(testResult);
    }

    // Modify a file in the package path. This doesn't have a redirected path, but we do set the working directory to be
    // the package root, so use that for fun
    test_begin("Create Package File Test");

    vfs_mapping packageMapping{ L"", g_packageRootPath };
    auto testResult = CreateFileTest(packageMapping);
    result = result ? result : testResult;

    test_end(testResult);

    return result;
}
