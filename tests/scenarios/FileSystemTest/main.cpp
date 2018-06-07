
#include <conio.h>

#include <known_folders.h>
#include <shim_runtime.h>

#include <error_logging.h>

#include "common_paths.h"

void InitializeFolderMappings()
{
    g_packageRootPath = shims::current_package_path();
    std::wcout << L"Package Root: " << console::change_foreground(console::color::dark_gray) << g_packageRootPath.native() << L"\n";

    auto initMapping = [](vfs_mapping& mapping, const KNOWNFOLDERID& knownFolder, const wchar_t* vfsRelativePath, const wchar_t* knownFolderRelativePath = nullptr)
    {
        mapping.package_path = g_packageRootPath / vfsRelativePath;
        mapping.path = shims::known_folder(knownFolder);
        if (knownFolderRelativePath)
        {
            mapping.path /= knownFolderRelativePath;
        }

        std::wcout << L"VFS Mapping: " << console::change_foreground(console::color::dark_gray) << mapping.package_path.native();
        std::wcout << L" -> " << console::change_foreground(console::color::dark_gray) << mapping.path.native() << L"\n";
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
int EnumerateFilesTests();

int run()
{
    auto result = ModifyFileTests();
    if (result) return result;

    result = EnumerateFilesTests();
    if (result) return result;

    return ERROR_SUCCESS;
}

int main()
{
    InitializeFolderMappings();

    auto result = run();

    std::cout << "Press any key to continue . . . ";
    _getch();
    return result;
}
