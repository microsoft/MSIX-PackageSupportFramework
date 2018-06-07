
#include <filesystem>

#include <error_logging.h>

#include "common_paths.h"

using namespace std::literals;

int ModifyFileTest(const std::wstring_view filename, const vfs_mapping& mapping)
{
    auto modifyFile = [](const std::filesystem::path& filePath, const char* expectedContents, const char* newContents) -> int
    {
        std::wcout << L"Opening File: " << console::change_foreground(console::color::dark_gray) << filePath.native() << L"\n";
        auto file = ::CreateFileW(filePath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE)
        {
            return print_last_error("Failed to open file");
        }
        std::wcout << L"Actual Path:  " << console::change_foreground(console::color::dark_gray) << actual_path(file).native() << L"\n";

        char buffer[256];
        DWORD size;
        OVERLAPPED overlapped = {};
        overlapped.Offset = 0; // Always read from the start of the file
        if (!::ReadFile(file, buffer, sizeof(buffer) - 1, &size, &overlapped))
        {
            ::CloseHandle(file);
            return print_last_error("Failed to read from file");
        }
        else
        {
            buffer[size] = '\0';
            std::cout << "Contents:     " << console::change_foreground(console::color::dark_gray) << buffer << "\n";
            if (std::strcmp(buffer, expectedContents) != 0)
            {
                std::cout << error_text() << "ERROR: File contents did not match the expected value\n";
                std::cout << error_text() << "ERROR: Expected contents: " << console::change_foreground(console::color::dark_red) << expectedContents << "\n";
                std::cout << error_text() << "ERROR: Actual contents:   " << console::change_foreground(console::color::dark_red) << buffer << "\n";
                ::CloseHandle(file);
                return ERROR_ASSERTION_FAILURE;
            }
        }

        if (!::WriteFile(file, newContents, static_cast<DWORD>(std::strlen(newContents)), nullptr, &overlapped))
        {
            ::CloseHandle(file);
            return print_last_error("Failed to write to file");
        }

        if (!::SetEndOfFile(file))
        {
            std::cout << warning_text() << "WARNING: Failed to set end of file. Future tests may fail because of this\n";
        }

        ::CloseHandle(file);
        return ERROR_SUCCESS;
    };

    static const char* initial_contents = "You are reading from the package path";
    static const char* first_modify_contents = "You are reading the first write to the redirected file";
    static const char* second_modify_contents = "You are reading the second write to the redirected file";

    auto performTest = [&](const std::filesystem::path& packagePath, const std::filesystem::path& path) -> int
    {
        auto result = modifyFile(packagePath / filename, initial_contents, first_modify_contents);
        if (result) return result;
        std::cout << "\n";

        result = modifyFile(path / filename, first_modify_contents, second_modify_contents);
        if (result) return result;
        std::cout << "\n";

        // TODO: Should probably just delete the redirected file?
        result = modifyFile(packagePath / filename, second_modify_contents, initial_contents);
        if (result) return result;

        std::cout << "--------------------------------------------------\n";
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
    if (!mapping.path.is_relative())
    {
        packagePath = LR"(\\?\)"s + mapping.package_path.native();
        path = LR"(\\?\)"s + mapping.path.native();
        result = performTest(packagePath, path);
        if (result) return result;
    }

    return ERROR_SUCCESS;
}

int ModifyFileTests()
{
    std::cout << console::change_foreground(console::color::cyan) << "==================================================\n";
    std::cout << console::change_foreground(console::color::cyan) << "Modify File Tests\n";
    std::cout << console::change_foreground(console::color::cyan) << "==================================================\n";

    for (auto& mapping : g_vfsMappings)
    {
        auto result = ModifyFileTest(test_filename(mapping), mapping);
        if (result) return result;
    }

    // Modify a file in the package path. This doesn't have a redirected path, but we do set the working directory to be
    // the package root, so use that for fun
    vfs_mapping packageMapping{ L"", g_packageRootPath };
    packageMapping.package_path = g_packageRootPath;
    auto result = ModifyFileTest(L"PackageFile.txt", packageMapping);
    if (result) return result;

    std::cout << success_text() << "SUCCESS: All file modifications properly redirected!\n";

    return ERROR_SUCCESS;
}
