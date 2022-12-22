//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <windows.h>
#include <psapi.h>
#include <tchar.h>

#include <test_config.h>

int wmain(int argc, const wchar_t** argv)
{
    auto result = parse_args(argc, argv);
    if (result != ERROR_SUCCESS)
    {
        return result;
    }
    test_initialize("Prevent BreakAway Tests", 1);

    test_begin("Prevent Break Away test");
    {
        result = ERROR_FILE_NOT_FOUND;
        // Test loading PSF dlls on C:\Windows\System32\cmd.exe process whih is possible only when cmd.exe is run in package context
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));
        wchar_t cmdLineArg[MAX_PATH] = L"C:\\Windows\\System32\\cmd.exe";
        BOOL procRes = ::CreateProcessW(NULL, cmdLineArg, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        if (procRes != 0)
        {
            // check if psfRuntime dll is loaded in cmd process
            Sleep(500);
            HMODULE hModules[1024];
            DWORD cbNeeded;
            TCHAR dllName[] = _T("PsfRuntime");
            if (EnumProcessModules(pi.hProcess, hModules, sizeof(hModules), &cbNeeded))
            {
                for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
                {
                    TCHAR szModuleName[MAX_PATH];
                    if (GetModuleFileNameEx(pi.hProcess, hModules[i], szModuleName, sizeof(szModuleName) / sizeof(TCHAR)))
                    {
                        if (_tcsstr(szModuleName, dllName) != NULL)
                        {
                            result = ERROR_SUCCESS;
                            break;
                        }
                    }
                }
            }
            TerminateProcess(pi.hProcess, 0);
        }
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    test_end(result);

    test_cleanup();
    return result;
}
