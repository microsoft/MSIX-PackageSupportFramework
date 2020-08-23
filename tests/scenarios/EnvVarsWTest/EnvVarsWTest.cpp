// EnvVarsTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
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
    inline std::wstring appmodel_string(LONG(__stdcall* AppModelFunc)(UINT32*, PWSTR))
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


int wmain(int argc, const wchar_t** argv)
{
    auto result = parse_args(argc, argv);
    result = 0;
    UINT32 len = 512;
    UINT32 rlen;

    test_initialize("EnvVarsW Tests", 4);


    test_begin("EnvVars GetEnvironmentW Test#1 Name Test1 from Json");
    try
    {
        wchar_t* buffer = new wchar_t[len];
        trace_messages(L"Testing: ", info_color, L"GetEnvironmentW Test#1", new_line);
        rlen = GetEnvironmentVariableW(L"Test1", buffer, len);
        if (rlen > 0)
        {
            if (wcscmp(buffer, L"Value1") == 0)
            {
                result = 0;
            }
            else
            {
                trace_messages(L"Testing: Incorrect value read ", info_color, buffer, new_line);
                result = 1;
            }
        }
        else
        {
            result = GetLastError();
            print_last_error("Failed to getenvW on Test#1");
            if (result == 0)
                result = ERROR_UNHANDLED_EXCEPTION;
        }
    }
    catch (...)
    {
        result = GetLastError();
        print_last_error("Failed to get envW from json");
        if (result == 0)
            result = ERROR_UNHANDLED_EXCEPTION;
    }
    test_end(result);


    test_begin("EnvVars GetEnvW Test#2 Name Test2 from AppReg HKCU");
    try
    {
        wchar_t* buffer = new wchar_t[len];
        trace_messages(L"Testing: ", info_color, L"GetEnvironmentW Test#2", new_line);
        rlen = GetEnvironmentVariableW(L"Test2", buffer, len);
        if (rlen > 0)
        {
            if (wcscmp(buffer, L"Value2Reg") == 0)
            {
                result = 0;
            }
            else
            {
                trace_messages(L"Testing: Incorrect value read ", info_color, buffer, new_line);
                result = 1;
            }
        }
        else
        {
            result = GetLastError();
            print_last_error("Failed to getenvW on Test#2");
            if (result == 0)
                result = ERROR_UNHANDLED_EXCEPTION;
        }
    }
    catch (...)
    {
        result = GetLastError();
        print_last_error("Failed to get envW from reg");
        if (result == 0)
            result = ERROR_UNHANDLED_EXCEPTION;
    }
    test_end(result);


    bool Test3Success = false;
    test_begin("EnvVars SetEnvironmentW Test#3 Name Test2 to AppReg HKCU");
    try
    {
        trace_messages(L"Testing: ", info_color, L"SetEnvironmentW Test#3", new_line);
        BOOL rb = SetEnvironmentVariableW(L"Test2", L"Value2RegModified");
        if (rb != 0)
        {
            result = 0;
            Test3Success = true;
        }
        else
        {
            trace_messages(L"Testing: Unable to write value ", info_color, "", new_line);
            result = GetLastError();
            print_last_error("Failed to setenvW on Test#3");
            if (result == 0)
                result = ERROR_UNHANDLED_EXCEPTION;
        }
    }
    catch (...)
    {
        result = GetLastError();
        print_last_error("Failed to set envW to reg");
        if (result == 0)
            result = ERROR_UNHANDLED_EXCEPTION;
    }
    test_end(result);


    test_begin("EnvVars GetEnvironmentW Test#4 Name Test2 from AppReg HKCU modified");
    try
    {
        wchar_t* buffer = new wchar_t[len];
        trace_messages(L"Testing: ", info_color, L"GetEnvironmentW Test#4", new_line);
        rlen = GetEnvironmentVariableW(L"Test2", buffer, len);
        if (rlen > 0)
        {
            if (wcscmp(buffer, L"Value2RegModified") == 0)
            {
                result = 0;
            }
            else if (wcscmp(buffer, L"Value2Reg") == 0)
            {
                trace_messages(L"Testing: Incorrect old value read ", info_color, buffer, new_line);
                result = 1;
            }
            else
            {
                trace_messages(L"Testing: Incorrect value read ", info_color, buffer, new_line);
                result = 2;
            }
        }
        else
        {
            result = GetLastError();
            print_last_error("Failed to getenvW on Test#4");
            if (result == 0)
                result = ERROR_UNHANDLED_EXCEPTION;
        }
    }
    catch (...)
    {
        result = GetLastError();
        print_last_error("Failed to get envW modified from reg");
        if (result == 0)
            result = ERROR_UNHANDLED_EXCEPTION;
    }
    test_end(result);

    if (Test3Success)
    {
        // Need to restore value as next test may occur without package removal/add
        trace_messages(L"Cleanup: Resetting Name Test2 ", info_color, L"", new_line);
        SetEnvironmentVariableW(L"Test2", L"Value2Reg");
    }

    test_cleanup();
    return result;

}
