
#include <filesystem>
#include <set>
#include <vector>

#include <error_logging.h>

#include "common_paths.h"

using namespace std::literals;

int EnumerateFilesTest(const vfs_mapping& mapping)
{
    auto doEnumerate = [](const std::filesystem::path& dir, int& error)
    {
        std::wcout << L"Enumerating .txt files in directory: " << console::change_foreground(console::color::dark_gray) << dir.native() << "\n";
        std::set<std::wstring> result;

        WIN32_FIND_DATAW findData;
        for (auto findHandle = ::FindFirstFileW((dir / L"*.txt").c_str(), &findData);
            findHandle != INVALID_HANDLE_VALUE;
            )
        {
            std::wcout << L"    Found: " << console::change_foreground(console::color::dark_gray) << findData.cFileName << "\n";
            if (auto [itr, added] = result.emplace(findData.cFileName); !added)
            {
                std::cout << error_text() << "ERROR: Same file found twice!\n";
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

        // We should find the same files when enumerating the package and the mapped directory
        auto files = doEnumerate(path, result);
        if (result) return result;
        auto packageFiles = doEnumerate(packagePath, result);
        if (result) return result;

        if (files != packageFiles)
        {
            std::cout << error_text() << "ERROR: Found files did not match!\n";
            return ERROR_ASSERTION_FAILURE;
        }

        // Create a new txt file in both directories that we should later find
        std::cout << "Creating new files that we should later find\n";
        auto createFile = [](const std::filesystem::path& dir, std::wstring_view name) -> int
        {
            auto path = dir / name;
            std::wcout << L"Creating file: " << console::change_foreground(console::color::dark_gray) << path.native() << "\n";
            auto file = ::CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (file == INVALID_HANDLE_VALUE)
            {
                return print_last_error("Failed to create file");
            }

            ::CloseHandle(file);
            return ERROR_SUCCESS;
        };
        result = createFile(path, L"a.txt"sv);
        if (result) return result;
        result = createFile(packagePath, L"b.txt"sv);
        if (result) return result;

        // We should find both of these files enumerating either directory
        auto newFiles = doEnumerate(path, result);
        if (result) return result;
        auto newPackageFiles = doEnumerate(packagePath, result);
        if (result) return result;

        if ((newFiles.size() != files.size() + 2) || (newPackageFiles.size() != packageFiles.size() + 2))
        {
            std::cout << error_text() << "ERROR: Did not find the newly created files\n";
            return ERROR_ASSERTION_FAILURE;
        }

        if (newFiles != newPackageFiles)
        {
            std::cout << error_text() << "ERROR: Found files did not match!\n";
            return ERROR_ASSERTION_FAILURE;
        }

        std::cout << "--------------------------------------------------\n";
        return ERROR_SUCCESS;
    };

    auto deleteFiles = [&]()
    {
        // Make sure that we delete any files that the test may have created
        ::DeleteFileW((mapping.path / L"a.txt"sv).c_str());
        ::DeleteFileW((mapping.package_path / L"b.txt"sv).c_str());
    };

    // Test with full paths
    auto result = performTest(mapping.package_path, mapping.path);
    deleteFiles();
    if (result) return result;

    // Test with relative paths
    result = performTest(
        mapping.package_path.lexically_relative(std::filesystem::current_path()),
        mapping.path.lexically_relative(std::filesystem::current_path()));
    deleteFiles();
    if (result) return result;

    // Test with paths containing forward slashes
    auto packagePath = mapping.package_path.native();
    auto path = mapping.path.native();
    std::replace(packagePath.begin(), packagePath.end(), L'\\', L'/');
    std::replace(path.begin(), path.end(), L'\\', L'/');
    result = performTest(packagePath, path);
    deleteFiles();
    if (result) return result;

    // Test with root-local device paths
    packagePath = LR"(\\?\)"s + mapping.package_path.native();
    path = LR"(\\?\)"s + mapping.path.native();
    result = performTest(packagePath, path);
    deleteFiles();
    if (result) return result;

    return ERROR_SUCCESS;
}

int EnumerateFilesTests()
{
    std::cout << console::change_foreground(console::color::cyan) << "==================================================\n";
    std::cout << console::change_foreground(console::color::cyan) << "Enumerate Files Tests\n";
    std::cout << console::change_foreground(console::color::cyan) << "==================================================\n";

    for (auto& mapping : g_vfsMappings)
    {
        auto result = EnumerateFilesTest(mapping);
        if (result) return result;
    }

    std::cout << success_text() << "SUCCESS: All file enumerations properly redirected!\n";

    return ERROR_SUCCESS;
}
