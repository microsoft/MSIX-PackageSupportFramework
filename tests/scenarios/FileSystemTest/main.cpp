//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <conio.h>
#include <fcntl.h>
#include <io.h>

#include <known_folders.h>
#include <psf_runtime.h>

#include <test_config.h>

#include "common_paths.h"

void InitializeFolderMappings()
{
    g_packageRootPath = psf::current_package_path();
    std::wcout << "Package Root: " << info_text() << g_packageRootPath.native() << "\n";

    auto initMapping = [](vfs_mapping& mapping, const KNOWNFOLDERID& knownFolder, const wchar_t* vfsRelativePath, const wchar_t* knownFolderRelativePath = nullptr)
    {
        mapping.package_path = g_packageRootPath / vfsRelativePath;
        mapping.path = psf::known_folder(knownFolder);
        if (knownFolderRelativePath)
        {
            mapping.path /= knownFolderRelativePath;
        }

        std::wcout << "VFS Mapping: " << info_text() << mapping.package_path.native();
        std::wcout << " -> " << info_text() << mapping.path.native() << "\n";
    };

    initMapping(g_systemX86Mapping, FOLDERID_SystemX86, LR"(VFS\SystemX86)");
    initMapping(g_programFilesX86Mapping, FOLDERID_ProgramFilesX86, LR"(VFS\ProgramFilesX86)");
    initMapping(g_programFilesCommonX86Mapping, FOLDERID_ProgramFilesCommonX86, LR"(VFS\ProgramFilesCommonX86)");
#if !_M_IX86
    initMapping(g_systemX64Mapping, FOLDERID_System, LR"(VFS\SystemX64)");
    initMapping(g_programFilesX64Mapping, FOLDERID_ProgramFilesX64, LR"(VFS\ProgramFilesX64)");
    initMapping(g_programFilesCommonX64Mapping, FOLDERID_ProgramFilesCommonX64, LR"(VFS\ProgramFilesCommonX64)");
#endif
    initMapping(g_windowsMapping, FOLDERID_Windows, LR"(VFS\Windows)");
    initMapping(g_commonAppDataMapping, FOLDERID_ProgramData, LR"(VFS\Common AppData)");
    initMapping(g_system32CatrootMapping, FOLDERID_System, LR"(VFS\AppVSystem32Catroot)", LR"(catroot)");
    initMapping(g_system32Catroot2Mapping, FOLDERID_System, LR"(VFS\AppVSystem32Catroot2)", LR"(catroot2)");
    initMapping(g_system32DriversEtcMapping, FOLDERID_System, LR"(VFS\AppVSystem32DriversEtc)", LR"(drivers\etc)");
    initMapping(g_system32DriverStoreMapping, FOLDERID_System, LR"(VFS\AppVSystem32Driverstore)", LR"(driverstore)");
    initMapping(g_system32LogFilesMapping, FOLDERID_System, LR"(VFS\AppVSystem32Logfiles)", LR"(logfiles)");
    initMapping(g_system32SpoolMapping, FOLDERID_System, LR"(VFS\AppVSystem32Spool)", LR"(spool)");
}

int ModifyFileTests();
int CreateNewFileTests();
int EnumerateFilesTests();
int CopyFileTests();
int DeleteFileTests();
int CreateHardLinkTests();
int CreateSymbolicLinkTests();
int FileAttributesTests();
int MoveFileTests();
int ReplaceFileTests();
int CreateDirectoryTests();
int RemoveDirectoryTests();
int EnumerateDirectoriesTests();
int EnumerateDirectoriesTests2();
int PrivateProfileTests();

int run()
{
    int result = ERROR_SUCCESS;
    int testResult;


    testResult = ModifyFileTests();
    result = result ? result : testResult;

    testResult = CreateNewFileTests();
    result = result ? result : testResult;

    //testResult = EnumerateFilesTests();
    //result = result ? result : testResult;

    //testResult = CopyFileTests();
    //result = result ? result : testResult;

    //testResult = DeleteFileTests();
    //result = result ? result : testResult;

    //testResult = CreateHardLinkTests();
    //result = result ? result : testResult;

    //testResult = CreateSymbolicLinkTests();
    //result = result ? result : testResult;

    //testResult = FileAttributesTests();
    //result = result ? result : testResult;

    //testResult = MoveFileTests();
    //result = result ? result : testResult;

    //testResult = ReplaceFileTests();
    //result = result ? result : testResult;

    //testResult = CreateDirectoryTests();
    //result = result ? result : testResult;

    //testResult = RemoveDirectoryTests();
    //result = result ? result : testResult;

    //testResult = EnumerateDirectoriesTests();
    //result = result ? result : testResult;

    //testResult = EnumerateDirectoriesTests2();
    //result = result ? result : testResult;

    //testResult = PrivateProfileTests();
    //result = result ? result : testResult;

    return result;
}

int wmain(int argc, const wchar_t** argv)
{
    // Display UTF-16 correctly...
    _setmode(_fileno(stdout), _O_U16TEXT);

    auto result = parse_args(argc, argv);
    if (result == ERROR_SUCCESS)
    {
        // The number of file mappings is different in 32-bit vs 64-bit
#if !_M_IX86
        test_initialize("File System Tests", 89);
#else
        test_initialize("File System Tests", 80);
#endif

        InitializeFolderMappings();
        result = run();

        test_cleanup();
    }

    if (!g_testRunnerPipe)
    {
        system("pause");
    }

    return result;
}
