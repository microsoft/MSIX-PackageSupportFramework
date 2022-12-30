//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <windows.h>
#include <ShlObj.h>

#include <test_config.h>
#include <known_folders.h>
#include <psf_utils.h>

int wmain(int argc, const wchar_t** argv)
{
    auto result = parse_args(argc, argv);
    if (result != ERROR_SUCCESS)
    {
        return result;
    }
    test_initialize("Argument Redirection Tests", 2);

    test_begin("Arg Redirection Test - CreateProcessA, LocalAppData");
    {
        result = ERROR_SUCCESS;
        auto newAppFolder_Local = psf::known_folder(FOLDERID_LocalAppData) / std::filesystem::path(L"Packages") / psf::current_package_family_name() / LR"(LocalCache\Local\ArgRedirectionTest)";
        CreateDirectoryW(newAppFolder_Local.c_str(), NULL);

        // create a file in per user per app data
        auto newFile_packageLocalAppDataPath = psf::known_folder(FOLDERID_LocalAppData) / std::filesystem::path(L"Packages") / psf::current_package_family_name() / LR"(LocalCache\Local\ArgRedirectionTest\newFile.txt)";
        HANDLE hFile = ::CreateFileW(newFile_packageLocalAppDataPath.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_WRITE,
            NULL,
            CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        CloseHandle(hFile);

        // Launch a cmd.exe application to rename newFile.txt in app data location
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        char cmdLineArg[MAX_PATH] = "C:\\Windows\\System32\\cmd.exe /c ren ";
        char appDataPath[MAX_PATH];
        SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appDataPath);
        strcat_s(appDataPath, MAX_PATH, "\\ArgRedirectionTest\\newFile.txt"); // dest
        strcat_s(cmdLineArg, appDataPath);
        strcat_s(cmdLineArg, " newFile_renamed.txt");
        BOOL procRes = ::CreateProcessA(NULL, (char*)cmdLineArg, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

        if (procRes != 0)
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        Sleep(1000); // sleep for 1sec for renamed file to be reflected

        // check if the renamed file exists in per user per app data location
        auto dest_file = psf::known_folder(FOLDERID_LocalAppData) / std::filesystem::path(L"Packages") / psf::current_package_family_name() / LR"(LocalCache\Local\ArgRedirectionTest\newFile_renamed.txt)";
        if (GetFileAttributesW(dest_file.c_str()) == INVALID_FILE_ATTRIBUTES)
        {
            result = ERROR_FILE_NOT_FOUND;
        }
        else
        {
            DeleteFileW(dest_file.c_str());
            RemoveDirectory(newAppFolder_Local.c_str());
        }
    }
    test_end(result);

    test_begin("Arg Redirection Test - CreateProcessW, RoamingAppData");
    {
        result = ERROR_SUCCESS;
        auto newAppFolder_Roaming = psf::known_folder(FOLDERID_LocalAppData) / std::filesystem::path(L"Packages") / psf::current_package_family_name() / LR"(LocalCache\Roaming\ArgRedirectionTest)";
        CreateDirectoryW(newAppFolder_Roaming.c_str(), NULL);

        // create a file in per user per app data
        auto newFile_packageRoamingAppDataPath = psf::known_folder(FOLDERID_LocalAppData) / std::filesystem::path(L"Packages") / psf::current_package_family_name() / LR"(LocalCache\Roaming\ArgRedirectionTest\newFile.txt)";
        HANDLE hFile = ::CreateFileW(newFile_packageRoamingAppDataPath.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_WRITE,
            NULL,
            CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        CloseHandle(hFile);

        // Launch a cmd.exe application to rename notepad.exe in app data location
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        wchar_t cmdLineArg[MAX_PATH] = L"C:\\Windows\\System32\\cmd.exe /c ren ";
        wchar_t appDataPath[MAX_PATH];
        SHGetFolderPath(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, appDataPath);
        wcscat_s(appDataPath, MAX_PATH, L"\\ArgRedirectionTest\\newFile.txt"); // dest
        wcscat_s(cmdLineArg, appDataPath);
        wcscat_s(cmdLineArg, L" newFile_renamed.txt");
        BOOL procRes = ::CreateProcessW(NULL, cmdLineArg, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

        if (procRes != 0)
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        Sleep(1000); // sleep for 1sec for renamed file to be reflected

        // check if the renamed file exists in per user per app data location
        auto dest_file = psf::known_folder(FOLDERID_LocalAppData) / std::filesystem::path(L"Packages") / psf::current_package_family_name() / LR"(LocalCache\Roaming\ArgRedirectionTest\newFile_renamed.txt)";
        if (GetFileAttributesW(dest_file.c_str()) == INVALID_FILE_ATTRIBUTES)
        {
            result = ERROR_FILE_NOT_FOUND;
        }
        else
        {
            DeleteFileW(dest_file.c_str());
            RemoveDirectory(newAppFolder_Roaming.c_str());
        }
    }
    test_end(result);

    test_cleanup();
    return result;
}
