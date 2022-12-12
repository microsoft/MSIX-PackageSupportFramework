//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


#define MAX_CMDLINE_PATH 32767
auto GetFileAttributesImpl = psf::detoured_string_function(&::GetFileAttributesA, &::GetFileAttributesW);

template <typename CharT>
errno_t strcatImpl(CharT* dest, rsize_t destBufSize, CharT const* src)
{
    if (std::is_same<CharT, char>::value)
    {
        return strcat_s((char*)dest, destBufSize, (const char*)src);
    }
    else if (std::is_same<CharT, wchar_t>::value)
    {
        return wcscat_s((wchar_t*)dest, destBufSize, (const wchar_t*)src);
    }
}

template <typename CharT>
CharT* strtokImpl(CharT* inpStr, CharT const* delim, CharT** token)
{
    if (std::is_same<CharT, char>::value)
    {
        return (CharT*)(strtok_s((char*)inpStr, (const char*)delim, (char**)token));
    }
    else if (std::is_same<CharT, wchar_t>::value)
    {
        return (CharT*)(wcstok_s((wchar_t*)inpStr, (const wchar_t*)delim, (wchar_t**)token));
    }
}

template <typename CharT>
size_t strlenImpl(CharT const* inpStr)
{
    if (std::is_same<CharT, char>::value)
    {
        return strlen((const char*)inpStr);
    }
    else if (std::is_same<CharT, wchar_t>::value)
    {
        return wcslen((const wchar_t*)inpStr);
    }
}

template <typename CharT>
bool is_path_relative(const CharT* path, const std::filesystem::path& basePath)
{
    return std::equal(basePath.native().begin(), basePath.native().end(), path, psf::path_compare{});
}

template <typename CharT>
bool IsUnderUserAppDataRoamingandReplace(const CharT* fileName, CharT* cnvtCmdLine)
{
    bool isUnderAppDataRoaming = false;
    bool result = false;

    if (fileName == NULL)
    {
        return result;
    }
    constexpr wchar_t root_local_device_prefix[] = LR"(\\?\)";
    constexpr wchar_t root_local_device_prefix_dot[] = LR"(\\.\)";

    if (std::equal(root_local_device_prefix, root_local_device_prefix + 4, fileName))
    {
        isUnderAppDataRoaming = is_path_relative(fileName + 4, psf::known_folder(FOLDERID_RoamingAppData));
    }
    else if (std::equal(root_local_device_prefix_dot, root_local_device_prefix_dot + 4, fileName))
    {
        isUnderAppDataRoaming = is_path_relative(fileName + 4, psf::known_folder(FOLDERID_RoamingAppData));
    }
    else
    {
        isUnderAppDataRoaming = is_path_relative(fileName, psf::known_folder(FOLDERID_RoamingAppData));
    }

    size_t remBufSize = MAX_CMDLINE_PATH - strlenImpl(cnvtCmdLine);
    if (isUnderAppDataRoaming)
    {
        std::filesystem::path roamingAppData = psf::known_folder(FOLDERID_RoamingAppData);
        auto packageRoamingAppDataPath = psf::known_folder(FOLDERID_LocalAppData) / std::filesystem::path(L"Packages") / psf::current_package_family_name() / LR"(LocalCache\Roaming)";
        std::wstring relativePath = widen(fileName + (roamingAppData.native().length()));
        std::wstring fullPath = packageRoamingAppDataPath.native() + relativePath;

        if (GetFileAttributesImpl(fullPath.c_str()) != INVALID_FILE_ATTRIBUTES)
        {
            result = true;
            strcatImpl(cnvtCmdLine, remBufSize, (CharT*)(packageRoamingAppDataPath.c_str()));
            strcatImpl(cnvtCmdLine, remBufSize, fileName + (roamingAppData.native().length()));
        }
    }
    return result;
}

template <typename CharT>
bool IsUnderUserAppDataLocalandReplace(const CharT* fileName, CharT* cnvtCmdLine)
{
    bool isUnderAppDataLocal = false;
    bool result = false;

    if (fileName == NULL)
    {
        return result;
    }
    constexpr wchar_t root_local_device_prefix[] = LR"(\\?\)";
    constexpr wchar_t root_local_device_prefix_dot[] = LR"(\\.\)";

    if (std::equal(root_local_device_prefix, root_local_device_prefix + 4, fileName))
    {
        isUnderAppDataLocal = is_path_relative(fileName + 4, psf::known_folder(FOLDERID_LocalAppData));
    }
    else if (std::equal(root_local_device_prefix_dot, root_local_device_prefix_dot + 4, fileName))
    {
        isUnderAppDataLocal = is_path_relative(fileName + 4, psf::known_folder(FOLDERID_LocalAppData));
    }
    else
    {
        isUnderAppDataLocal = is_path_relative(fileName, psf::known_folder(FOLDERID_LocalAppData));
    }

    size_t remBufSize = MAX_CMDLINE_PATH - strlenImpl(cnvtCmdLine);
    if (isUnderAppDataLocal)
    {
        std::filesystem::path localAppdata = psf::known_folder(FOLDERID_LocalAppData);
        auto packageLocalAppDataPath = psf::known_folder(FOLDERID_LocalAppData) / std::filesystem::path(L"Packages") / psf::current_package_family_name() / LR"(LocalCache\Local)";
        std::wstring relativePath = widen(fileName + (localAppdata.native().length()));
        std::wstring fullPath = packageLocalAppDataPath.native() + relativePath;

        if (GetFileAttributesImpl(fullPath.c_str()) != INVALID_FILE_ATTRIBUTES)
        {
            result = true;
            strcatImpl(cnvtCmdLine, remBufSize, (CharT*)(packageLocalAppDataPath.c_str()));
            strcatImpl(cnvtCmdLine, remBufSize, fileName + (localAppdata.native().length()));
        }
    }
    return result;
}

template <typename CharT>
void convertCmdLineParameters(CharT* inpCmdLine, CharT* cnvtCmdLine)
{
    CharT* next_token;
    size_t cmdLinelen = strlenImpl(inpCmdLine);
    CharT* cmdLine = new CharT[cmdLinelen+1];
    if (cmdLine)
    {
        cmdLine[0] = (CharT)nullptr;
        strcatImpl(cmdLine, cmdLinelen+1, inpCmdLine);
    }
    else
    {
        strcatImpl(cnvtCmdLine, MAX_CMDLINE_PATH, inpCmdLine);
        return;
    }

    CharT* token = strtokImpl(cmdLine, (CharT*)L" ", &next_token);
    bool redirectResult = false;
    size_t remBufSize; 

    while (token != nullptr)
    {
        redirectResult = IsUnderUserAppDataLocalandReplace(token, cnvtCmdLine);
        if (redirectResult == false)
        {
            redirectResult = IsUnderUserAppDataRoamingandReplace(token, cnvtCmdLine);
        }

        if (!redirectResult)
        {
            remBufSize = MAX_CMDLINE_PATH - strlenImpl(cnvtCmdLine);
            strcatImpl(cnvtCmdLine, remBufSize, token);
        }
        token = strtokImpl((CharT*)nullptr, (CharT*)L" ", &next_token);
        if (token != nullptr)
        {
            remBufSize = MAX_CMDLINE_PATH - strlenImpl(cnvtCmdLine);
            strcatImpl(cnvtCmdLine, remBufSize, (CharT*)L" ");
        }
    }
    delete[] cmdLine;
    return;
}