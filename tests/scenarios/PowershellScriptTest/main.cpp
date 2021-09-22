//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <iostream>
#include <sstream>

#include <Windows.h>

#include <test_config.h>
#include <appmodel.h>
#include <algorithm>
#include <ShlObj.h>
#include <filesystem>

using namespace std::literals;

namespace details
{
    inline std::wstring appmodel_string(LONG(__stdcall *AppModelFunc)(UINT32*, PWSTR))
    {
        // NOTE: `length` includes the null character both as input and output, hence the +1/-1 everywhere
        UINT32 length = MAX_PATH + 1;
        std::wstring result(length - 1, '\0');

        const auto err = AppModelFunc(&length, result.data());
        if ((err != ERROR_SUCCESS) && (err != ERROR_INSUFFICIENT_BUFFER))
        {
            throw_win32(err, "could not retrieve AppModel string");
        }

        assert(length > 0);
        result.resize(length - 1);
        if (err == ERROR_INSUFFICIENT_BUFFER)
        {
            check_win32(AppModelFunc(&length, result.data()));
            result.resize(length - 1);
        }

        return result;
    }
}


void Log(const wchar_t* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::wstring str;
    str.resize(256);
    try
    {
        std::size_t count = std::vswprintf(str.data(), str.size() + 1, fmt, args);
        assert(count >= 0);
        va_end(args);

        if (count > str.size())
        {
            str.resize(count);

            va_list args2;
            va_start(args2, fmt);
            count = std::vswprintf(str.data(), str.size() + 1, fmt, args2);
            assert(count >= 0);
            va_end(args2);
        }

        str.resize(count);
    }
    catch (...)
    {
        str = fmt;
    }
    ::OutputDebugStringW(str.c_str());
}

bool DoesFileExist(std::filesystem::path localAppData, LPCWSTR fileName)
{
    std::filesystem::path pathToFile = localAppData / fileName;
    return std::filesystem::exists(pathToFile);
}

void RemoveFile(std::filesystem::path localAppData, LPCWSTR fileName)
{
    std::filesystem::remove(localAppData / fileName);
}

int wmain(int argc, const wchar_t** argv)
{
    auto result = parse_args(argc, argv);
    std::wstring aumid = details::appmodel_string(&::GetCurrentApplicationUserModelId);
    test_initialize("Powershell Script Tests", 1);
    test_begin("Powershell Script Test");

    //get rid of the !
    std::wstring testType = aumid.substr(aumid.find('!') + 1);
    std::transform(testType.begin(), testType.end(), testType.begin(), towlower);

    Log(L"<<<<<Powershell Script Test %ls", testType.c_str());

    TCHAR localAppDataPath[MAX_PATH];
    SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, localAppDataPath);
    bool doesHelloExist = DoesFileExist(localAppDataPath, L"Hello.txt");
    bool doesArgumentExist = DoesFileExist(localAppDataPath, L"Argument.txt");


    if (testType.compare(L"psonlystart") == 0)
    {
        if (!doesHelloExist)
        {
            result = ERROR_FILE_NOT_FOUND;
        }
    }
    else if (testType.compare(L"psbothstartingfirst") == 0)
    {
        if (!doesHelloExist)
        {
            result = ERROR_FILE_NOT_FOUND;
        }
    }
    else if (testType.compare(L"psscriptwitharg") == 0)
    {
        if (!doesArgumentExist)
        {
            result = ERROR_FILE_NOT_FOUND;
        }
    }


    RemoveFile(localAppDataPath, L"Hello.txt");
    RemoveFile(localAppDataPath, L"Argument.txt");

    test_end(result);
    Log(L"Powershell Script Test>>>>>");

    test_cleanup();

    return result;
}
