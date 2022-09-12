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
    else if (testType.compare(L"pswaitforscripttofinish") == 0)
    {
       result = ERROR_ASSERTION_FAILURE;
       if (!doesHelloExist) 
       {
           // wait for 5sec and check if file exists
           Sleep(5000);
           doesHelloExist = DoesFileExist(localAppDataPath, L"Hello.txt");
           if (!doesHelloExist)
           {
               result = ERROR_FILE_NOT_FOUND;
           }
           else
           {
               result = ERROR_SUCCESS;
           }
       }
       else
       {
           std::wcout << error_text() << L"ERROR: Script executed before application start. \n";
       }
    }


    RemoveFile(localAppDataPath, L"Hello.txt");
    RemoveFile(localAppDataPath, L"Argument.txt");

    test_end(result);

    test_cleanup();

    return result;
}
