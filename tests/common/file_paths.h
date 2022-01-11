//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <string>

#include <known_folders.h>
#include <psf_utils.h>

#include "test_config.h"

inline bool wiequals(std::wstring a, std::wstring b)
{
    return std::equal(a.begin(), a.end(),
                      b.begin(), b.end(),
        [](wchar_t a, wchar_t b) {
            return tolower(a) == tolower(b);
        });
}

inline bool isPathActuallyUnder(std::filesystem::path candidatePath, const std::filesystem::path dir_path)
{
    std::wstring wCandidate = candidatePath.c_str();
    std::wstring wDir = dir_path.c_str();
    return (wCandidate.substr(0, wDir.length()).compare(wDir.c_str()) == 0);
}
inline std::uintmax_t deleteDirectoryContents(const std::filesystem::path dir_path)
{
    std::uintmax_t cnt = 0;
    std::uintmax_t num = 0;
    for (const auto& entry : std::filesystem::directory_iterator(dir_path))
    {
        try
        {
            if (isPathActuallyUnder(entry.path(), dir_path))
            {
                if (entry.is_directory())
                {
                    //trace_messages(warning_color, L"Potential directory for cleanup: ", entry.path().c_str(), new_line);
                    num = deleteDirectoryContents(entry);
                    cnt += num;
                    bool b = std::filesystem::remove(entry.path().c_str());
                    if (b)
                    {
                        cnt++;
                        //trace_message(L"directory removed.\n");
                    }
                    else
                    {
                        //trace_message(L"directory NOT removed.\n");
                    }
                }
                else
                {
                    std::wstring theName = entry.path().filename().c_str();
                    //trace_messages(warning_color, L"Potential file for cleanup: ", theName.c_str(), new_line);

                    // On windows 11, we seem to overlay the package root folder onto every level folder under writable package root, so this
                    // isn't the only bad file we can't delete.
                    if (!wiequals(theName.c_str(), L"appxblockmap.xml"))
                    {
                        bool b = std::filesystem::remove(entry);
                        if (b)
                        {
                            cnt++;
                            //trace_message(L"file removed.\n");
                        }
                        else
                        {
                            //trace_message(L"file NOT removed.\n");
                        }
                    }
                    else
                    {
                        //trace_message(L"file skipped\n");;
                    }
                }
            }
        }
        catch (...)
        {
            //trace_message(L"deleteDirectoryContents Exception\n");
        }
    }
    return cnt;
}

inline void clean_redirection_path_helper(std::filesystem::path redirectRoot)
{
	std::error_code ec; 
    std::uintmax_t num;
#if OLD
	num = std::filesystem::remove_all(redirectRoot, ec);
	if (num == static_cast<std::uintmax_t>(-1) && ec)  // Added num test as that is set to 0 when folder doesn't exist, which is OK.
	{
		trace_message(L"WARNING: Failed to clean the redirected path. Future tests may be impacted by this...\n", warning_color);
		trace_messages(warning_color, L"WARNING: ", ec.message(), new_line);
        trace_messages(warning_color, L"Path was:", redirectRoot.c_str(), new_line);
	}
    else
    {
        // The file redirection fixup makes an assumption about the existence of the VFS directory, but we just deleted
        // it above, so re-create it to avoid future issues
        if (!::CreateDirectoryW(redirectRoot.c_str(), nullptr))
        {
            trace_message(L"WARNING: Failed to re-create the VFS directory in the redirected path. Future tests may be impacted by this...\n", warning_color);
            trace_messages(warning_color, L"WARNING: ", error_message(::GetLastError()), new_line);
        }
    }
#else
    // 12/19/2021: Modified cleanup to only cleanup contents of requested folder, as OS is now protecting the folder from deletion.
    trace_messages(info_color, L"clean_redirection_path_helper: ", redirectRoot.c_str(), new_line);
    num = deleteDirectoryContents(redirectRoot);
    wchar_t wNum[16]; 
    std::wstring sNum = L"Detail: CleanupCount=";
    _itow_s((int)num, wNum, 16, 10);
    trace_messages(info_color, sNum.c_str(), wNum, new_line);
    // without deletion of the folder, no need to recreate it either.
#endif

}

inline void clean_redirection_path()
{
    // NOTE: This makes an assumption about the location of redirected files! If this path ever changes, then the logic
    //       here will need to get updated. At that time, it may just be better to add an export to PsfRuntime for
    //       identifying what the location is.
    // NOTE: In the future, if we ever do handle deletes of files in the package path, we should also handle that here,
    //       e.g. by deleting the file that's tracking the deletions, or by calling into an export in PsfRuntime to do
    //       so for us.
    static const auto redirectRoot = std::filesystem::path(LR"(\\?\)" + psf::known_folder(FOLDERID_LocalAppData).native()) / L"Packages" / psf::current_package_family_name() / LR"(LocalCache\Local\VFS)"; 
	static const auto writablePackageRoot = std::filesystem::path(LR"(\\?\)" + psf::known_folder(FOLDERID_LocalAppData).native()) / L"Packages" / psf::current_package_family_name() / LR"(LocalCache\Local\Microsoft\WritablePackageRoot)";
    trace_message(L"<<<Cleanup Redirection Paths before next test.\n");
    clean_redirection_path_helper(redirectRoot);
	clean_redirection_path_helper(writablePackageRoot);
    trace_message(L"Cleanup Redirection Paths before next test.>>>\n");
}

inline std::string read_entire_file(const wchar_t* path)
{
    std::string result;
    auto file = ::CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file != INVALID_HANDLE_VALUE)
    {
        char buffer[256];
        DWORD bytesRead;
        do
        {
            if (::ReadFile(file, buffer, sizeof(buffer), &bytesRead, nullptr))
            {
                result.append(buffer, bytesRead);
            }
            else
            {
                trace_messages(warning_color, L"WARNING: Failed to read from file: ", warning_info_color, path, new_line);
                trace_messages(warning_color, L"WARNING: Failure was: ", warning_info_color, error_message(::GetLastError()), new_line);
                break;
            }
        } while (bytesRead > 0);

        ::CloseHandle(file);
    }
    else
    {
        trace_messages(warning_color, L"WARNING: Failed to open file: ", warning_info_color, path, new_line);
        trace_messages(warning_color, L"WARNING: Failure was: ", warning_info_color, error_message(::GetLastError()), new_line);
    }

    return result;
}

inline bool write_entire_file(const wchar_t* path, const char* contents)
{
    // ensure directories
    //std::wstring wPath = path;
    //std::filesystem::create_directories(wPath.substr(0, wPath.find_last_of(L"\\")));
    
    // create file
    auto file = ::CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        trace_messages(warning_color, L"WARNING: Failed to create file: ", warning_info_color, path, new_line);
        trace_messages(warning_color, L"WARNING: Failure was: ", warning_info_color, error_message(::GetLastError()), new_line);
        return false;
    }

    // Fill file
    auto len = static_cast<DWORD>(std::strlen(contents));
    DWORD bytesWritten;
    if (!::WriteFile(file, contents, len, &bytesWritten, nullptr))
    {
        ::CloseHandle(file);
        trace_messages(warning_color, L"WARNING: Failed to write to file: ", warning_info_color, path, new_line);
        trace_messages(warning_color, L"WARNING: Failure was: ", warning_info_color, error_message(::GetLastError()), new_line);
        return false;
    }

    assert(len == bytesWritten);
    ::CloseHandle(file);
    trace_message(L"Original file written external to package.",info_color,true);
    return true;
}
