
#include <filesystem>
#include <functional>

#include <test_config.h>

#include "common_paths.h"

using namespace std::literals;

static int ModifyFileTest(const std::wstring_view filename, const vfs_mapping& mapping)
{
    CREATEFILE2_EXTENDED_PARAMETERS extParams = { sizeof(extParams) };
    extParams.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;

    auto CreateFileFunc = std::bind(&::CreateFileW, std::placeholders::_1, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, std::placeholders::_2, FILE_ATTRIBUTE_NORMAL, nullptr);
    auto CreateFile2Func = std::bind(&::CreateFile2, std::placeholders::_1, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, std::placeholders::_2, &extParams);

    auto modifyFile = [](const std::function<HANDLE(LPCWSTR, DWORD)>& createFunc, DWORD creationDisposition, const std::filesystem::path& filePath, const char* expectedContents, const char* newContents) -> int
    {
        trace_messages(L"Opening File: ", info_color, filePath.native(), new_line);

        // First, validate that opening with CREATE_NEW fails
        if (createFunc(filePath.c_str(), CREATE_NEW) != INVALID_HANDLE_VALUE)
        {
            trace_message(L"ERROR: Attempting to open the file with 'CREATE_NEW' should fail as the file should already exist\n", error_color);
            return ERROR_ASSERTION_FAILURE;
        }
        else if (::GetLastError() != ERROR_FILE_EXISTS)
        {
            return trace_last_error(L"Error should have been set to ERROR_FILE_EXISTS when attempting to open with 'CREATE_NEW'");
        }

        auto file = createFunc(filePath.c_str(), creationDisposition);
        if (file == INVALID_HANDLE_VALUE)
        {
            return trace_last_error(L"Failed to open file");
        }
        else if ((creationDisposition == CREATE_ALWAYS) && (::GetLastError() != ERROR_ALREADY_EXISTS))
        {
            trace_message(L"ERROR: Last error should be ERROR_ALREADY_EXISTS when opening with CREATE_ALWAYS\n", error_color);
            return ERROR_ASSERTION_FAILURE;
        }
        else if ((creationDisposition == OPEN_ALWAYS) && (::GetLastError() != ERROR_ALREADY_EXISTS))
        {
            trace_message(L"ERROR: Last error should be ERROR_ALREADY_EXISTS when opening with OPEN_ALWAYS\n", error_color);
            return ERROR_ASSERTION_FAILURE;
        }

        char buffer[256];
        DWORD size;
        OVERLAPPED overlapped = {};
        overlapped.Offset = 0; // Always read from the start of the file
        if (!::ReadFile(file, buffer, sizeof(buffer) - 1, &size, &overlapped) && (::GetLastError() != ERROR_HANDLE_EOF))
        {
            ::CloseHandle(file);
            return trace_last_error(L"Failed to read from file");
        }
        else
        {
            buffer[size] = '\0';
            if (std::strcmp(buffer, expectedContents) != 0)
            {
                trace_messages(error_color,
                    L"ERROR: File contents did not match the expected value\n",
                    L"ERROR: Expected contents: ", error_info_color, expectedContents, new_line, error_color,
                    L"ERROR: Actual contents:   ", error_info_color, buffer, new_line);
                ::CloseHandle(file);
                return ERROR_ASSERTION_FAILURE;
            }
        }

        if (!::WriteFile(file, newContents, static_cast<DWORD>(std::strlen(newContents)), nullptr, &overlapped))
        {
            ::CloseHandle(file);
            return trace_last_error(L"Failed to write to file");
        }

        if (!::SetEndOfFile(file))
        {
            trace_message(L"WARNING: Failed to set end of file. Future tests may fail because of this\n", warning_color);
        }

        ::CloseHandle(file);
        return ERROR_SUCCESS;
    };

    static const char* const initial_contents = "You are reading from the package path";
    static const char* const first_modify_contents = "You are reading the first write to the redirected file";
    static const char* const second_modify_contents = "You are reading the second write to the redirected file";
    static const char* const unexpected_contents = "This is text that you shouldn't be reading!";

    auto performTest = [&](const std::function<HANDLE(LPCWSTR, DWORD)>& createFunc, const std::filesystem::path& packagePath, const std::filesystem::path& path) -> int
    {
        // Clean up the redirected path so that existing files don't impact this test
        clean_redirection_path();

        auto result = modifyFile(createFunc, OPEN_ALWAYS, packagePath / filename, initial_contents, first_modify_contents);
        if (result) return result;

        result = modifyFile(createFunc, OPEN_EXISTING, path / filename, first_modify_contents, second_modify_contents);
        if (result) return result;

        result = modifyFile(createFunc, OPEN_EXISTING, packagePath / filename, second_modify_contents, unexpected_contents);
        if (result) return result;

        // Opening the file with TRUNCATE_EXISTING should mean that we read no contents
        result = modifyFile(createFunc, TRUNCATE_EXISTING, packagePath / filename, "", unexpected_contents);
        if (result) return result;

        // Opening the file with CREATE_ALWAYS should effectively be equivalent to TRUNCATE_EXISTING
        result = modifyFile(createFunc, CREATE_ALWAYS, packagePath / filename, "", unexpected_contents);
        if (result) return result;

        // Open again with CREATE_ALWAYS, but this time after cleaning the redirected path to ensure the correct error
        clean_redirection_path();
        result = modifyFile(createFunc, CREATE_ALWAYS, packagePath / filename, "", unexpected_contents);
        if (result) return result;

        return ERROR_SUCCESS;
    };

    // Test with full paths
    auto result = performTest(CreateFileFunc, mapping.package_path, mapping.path);
    if (result) return result;

    result = performTest(CreateFile2Func, mapping.package_path, mapping.path);
    if (result) return result;

    // Test with relative paths
    result = performTest(
        CreateFileFunc,
        mapping.package_path.lexically_relative(std::filesystem::current_path()),
        mapping.path.lexically_relative(std::filesystem::current_path()));
    if (result) return result;

    result = performTest(
        CreateFile2Func,
        mapping.package_path.lexically_relative(std::filesystem::current_path()),
        mapping.path.lexically_relative(std::filesystem::current_path()));
    if (result) return result;

    // Test with paths containing forward slashes
    auto packagePath = mapping.package_path.native();
    auto path = mapping.path.native();
    std::replace(packagePath.begin(), packagePath.end(), L'\\', L'/');
    std::replace(path.begin(), path.end(), L'\\', L'/');
    result = performTest(CreateFileFunc, packagePath, path);
    if (result) return result;

    result = performTest(CreateFile2Func, packagePath, path);
    if (result) return result;

    // Test with root-local device paths
    if (!mapping.path.is_relative())
    {
        packagePath = LR"(\\?\)"s + mapping.package_path.native();
        path = LR"(\\?\)"s + mapping.path.native();
        result = performTest(CreateFileFunc, packagePath, path);
        if (result) return result;

        result = performTest(CreateFile2Func, packagePath, path);
        if (result) return result;
    }

    return ERROR_SUCCESS;
}

int ModifyFileTests()
{
    int result = ERROR_SUCCESS;

    for (auto& mapping : g_vfsMappings)
    {
        std::string testName = "Modify " + mapping.get().package_path.filename().string() + " File Test";
        test_begin(testName);

        auto testResult = ModifyFileTest(test_filename(mapping), mapping);
        result = result ? result : testResult;

        test_end(testResult);
    }

    // Modify a file in the package path. This doesn't have a redirected path, but we do set the working directory to be
    // the package root, so use that for fun
    test_begin("Modify Package File Test");

    vfs_mapping packageMapping{ L"", g_packageRootPath };
    packageMapping.package_path = g_packageRootPath;
    auto testResult = ModifyFileTest(g_packageFileName, packageMapping);
    result = result ? result : testResult;

    test_end(testResult);

    return result;
}
