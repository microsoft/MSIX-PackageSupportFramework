//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include <known_folders.h>

constexpr int MAX_CMDLINE_PATH = 32767;
auto GetFileAttributesImpl = psf::detoured_string_function(&::GetFileAttributesA, &::GetFileAttributesW);

template <typename CharT>
bool IsUnderUserAppDataAndReplace(const CharT* fileName, CharT* cmdLine, bool AppDataLocal)
{
    bool isUnderAppData = false;
    bool result = false;

    if (!fileName)
    {
        return result;
    }
    constexpr wchar_t root_local_device_prefix[] = LR"(\\?\)";
    constexpr wchar_t root_local_device_prefix_dot[] = LR"(\\.\)";

    GUID appDataFolderId = AppDataLocal ? FOLDERID_LocalAppData : FOLDERID_RoamingAppData;

    if (std::equal(root_local_device_prefix, root_local_device_prefix + wcslen(root_local_device_prefix), fileName))
    {
        isUnderAppData = is_path_relative(fileName + wcslen(root_local_device_prefix), psf::known_folder(appDataFolderId));
    }
    else if (std::equal(root_local_device_prefix_dot, root_local_device_prefix_dot + wcslen(root_local_device_prefix_dot), fileName))
    {
        isUnderAppData = is_path_relative(fileName + wcslen(root_local_device_prefix_dot), psf::known_folder(appDataFolderId));
    }
    else
    {
        isUnderAppData = is_path_relative(fileName, psf::known_folder(appDataFolderId));
    }

    size_t remBufSize = MAX_CMDLINE_PATH - strlenImpl(cmdLine);
    if (isUnderAppData)
    {
        std::filesystem::path strLocalRoaming = AppDataLocal ? std::filesystem::path(L"Local") : std::filesystem::path(L"Roaming");
        std::filesystem::path appDataPath = psf::known_folder(appDataFolderId);
        auto packageAppDataPath = psf::known_folder(FOLDERID_LocalAppData) / std::filesystem::path(L"Packages") / psf::current_package_family_name() / LR"(LocalCache)" / strLocalRoaming;
        std::wstring relativePath = widen(fileName + (appDataPath.native().length()));
        std::wstring fullPath = packageAppDataPath.native() + relativePath;

        if (GetFileAttributesImpl(fullPath.c_str()) != INVALID_FILE_ATTRIBUTES)
        {
            result = true;
            if (std::is_same<CharT, char>::value)
            {
                size_t fullPathLen = fullPath.length();
                size_t bytes_written;
                std::unique_ptr<char[]> fullPath_char(new char[fullPathLen + 1]);
                wcstombs_s(&bytes_written, fullPath_char.get(), fullPathLen + 1, fullPath.c_str(), fullPathLen);
                strcatImpl(cmdLine, remBufSize, (const CharT*)(fullPath_char.get()));
            }
            else
            {
                strcatImpl(cmdLine, remBufSize, (CharT*)(fullPath.c_str()));
            }
        }
    }
    return result;
}

// checks each space separated command line parameter and changes from native local app data folder to per user per app data folder if the file referred in command line parameter is present in per user per app data folder, else it remains the same
template <typename CharT>
bool convertCmdLineParameters(CharT* inputCmdLine, CharT* cnvtCmdLine)
{
    CharT* next_token;
    size_t cmdLinelen = strlenImpl(inputCmdLine);
    std::unique_ptr<CharT[]> cmdLine(new CharT[cmdLinelen+1]);
    if (cmdLine.get())
    {
        memset(cmdLine.get(), 0, cmdLinelen+1);
        strcatImpl(cmdLine.get(), cmdLinelen+1, inputCmdLine);
    }
    else
    {
        strcatImpl(cnvtCmdLine, MAX_CMDLINE_PATH, inputCmdLine);
        return false;
    }

    CharT* token = strtokImpl(cmdLine.get(), (CharT*)L" ", &next_token);
    bool redirectResult = false;
    size_t remBufSize; 
    bool isCmdLineRedirected = false;

    while (token != nullptr)
    {
        redirectResult = IsUnderUserAppDataAndReplace(token, cnvtCmdLine, true);
        if (redirectResult == false)
        {
            redirectResult = IsUnderUserAppDataAndReplace(token, cnvtCmdLine, false);
        }

        if (!redirectResult)
        {
            remBufSize = MAX_CMDLINE_PATH - strlenImpl(cnvtCmdLine);
            strcatImpl(cnvtCmdLine, remBufSize, token);
        }
        else
        {
            isCmdLineRedirected = true;
        }
        token = strtokImpl((CharT*)nullptr, (CharT*)L" ", &next_token);
        if (token != nullptr)
        {
            remBufSize = MAX_CMDLINE_PATH - strlenImpl(cnvtCmdLine);
            strcatImpl(cnvtCmdLine, remBufSize, (CharT*)L" ");
        }
    }
    return isCmdLineRedirected;
}
