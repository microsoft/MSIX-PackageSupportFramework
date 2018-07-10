//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <filesystem>
#include <set>
#include <vector>

#include <test_config.h>

#include "common_paths.h"

using namespace std::literals;

static int EnumerateFilesTest(const vfs_mapping& mapping)
{
    auto doEnumerate = [](const std::filesystem::path& dir, int& error)
    {
        trace_messages(L"Enumerating .txt files in directory: ", info_color, dir.native(), new_line);
        std::set<std::wstring> result;

        WIN32_FIND_DATAW findData;
        for (auto findHandle = ::FindFirstFileW((dir / L"*.txt").c_str(), &findData);
            findHandle != INVALID_HANDLE_VALUE;
            )
        {
            trace_messages(L"    Found: ", info_color, findData.cFileName, new_line);
            if (auto [itr, added] = result.emplace(findData.cFileName); !added)
            {
                trace_message(L"ERROR: Same file found twice!\n", error_color);
                error = ERROR_ASSERTION_FAILURE;
                break;
            }

            if (!::FindNextFileW(findHandle, &findData))
            {
                ::FindClose(findHandle);
                findHandle = INVALID_HANDLE_VALUE;
            }
        }

        return result;
    };

    auto performTest = [&](const std::filesystem::path& packagePath, const std::filesystem::path& path) -> int
    {
        int result = ERROR_SUCCESS;
        clean_redirection_path();

        // The files found in the package path should also be found in the mapped directory
        auto files = doEnumerate(path, result);
        if (result) return result;
        auto packageFiles = doEnumerate(packagePath, result);
        if (result) return result;

        if (!std::includes(files.begin(), files.end(), packageFiles.begin(), packageFiles.end()))
        {
            trace_message(L"ERROR: Found files did not match!\n", error_color);
            return ERROR_ASSERTION_FAILURE;
        }

        // Create a new txt file in both directories that we should later find
        trace_message(L"Creating new files that we should later find\n");
        auto createFile = [](const std::filesystem::path& dir, std::wstring_view name) -> int
        {
            auto path = dir / name;
            trace_messages(L"Creating file: ", info_color, path.native(), new_line);
            auto file = ::CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (file == INVALID_HANDLE_VALUE)
            {
                return trace_last_error(L"Failed to create file");
            }

            ::CloseHandle(file);
            return ERROR_SUCCESS;
        };
        result = createFile(path, L"á.txt"sv);
        if (result) return result;
        result = createFile(packagePath, L"β.txt"sv);
        if (result) return result;

        // We should find both of these files enumerating either directory
        auto newFiles = doEnumerate(path, result);
        if (result) return result;
        auto newPackageFiles = doEnumerate(packagePath, result);
        if (result) return result;

        // NOTE: The non-shimmed version of this function has access to some of the folders (e.g. Common AppData), so
        //       we may hit spurious failures if we try and run the shimmed test after the non-shimmed test, so try and
        //       delete the files. This is best effort
        ::DeleteFileW((path / L"á.txt"sv).c_str());
        ::DeleteFileW((packagePath / L"β.txt"sv).c_str());

        if ((newFiles.size() != files.size() + 2) || (newPackageFiles.size() != packageFiles.size() + 2))
        {
            trace_message(L"ERROR: Did not find the newly created files\n", error_color);
            return ERROR_ASSERTION_FAILURE;
        }

        if (!std::includes(newFiles.begin(), newFiles.end(), newPackageFiles.begin(), newPackageFiles.end()))
        {
            trace_message(L"ERROR: Found files did not match!\n", error_color);
            return ERROR_ASSERTION_FAILURE;
        }

        return ERROR_SUCCESS;
    };

    // Test with full paths
    auto result = performTest(mapping.package_path, mapping.path);
    if (result) return result;

    // Test with relative paths
    result = performTest(
        mapping.package_path.lexically_relative(std::filesystem::current_path()),
        mapping.path.lexically_relative(std::filesystem::current_path()));
    if (result) return result;

    // Test with paths containing forward slashes
    auto packagePath = mapping.package_path.native();
    auto path = mapping.path.native();
    std::replace(packagePath.begin(), packagePath.end(), L'\\', L'/');
    std::replace(path.begin(), path.end(), L'\\', L'/');
    result = performTest(packagePath, path);
    if (result) return result;

    // Test with root-local device paths
    packagePath = LR"(\\?\)"s + mapping.package_path.native();
    path = LR"(\\?\)"s + mapping.path.native();
    result = performTest(packagePath, path);
    if (result) return result;

    return ERROR_SUCCESS;
}

int EnumerateFilesTests()
{
    int result = ERROR_SUCCESS;

    for (auto& mapping : g_vfsMappings)
    {
        std::string testName = "Enumerate " + mapping.get().package_path.filename().string() + " Test";
        test_begin(testName);

        auto testResult = EnumerateFilesTest(mapping);
        result = result ? result : testResult;

        test_end(testResult);
    }

    return result;
}
