// EnvVarsATest.cpp : This file contains the 'main' function. Program execution begins and ends there.
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
    inline std::string appmodel_string(LONG(__stdcall* AppModelFunc)(UINT32*, LPCSTR))
    {
        // NOTE: `length` includes the null character both as input and output, hence the +1/-1 everywhere
        UINT32 length = MAX_PATH + 1;
        std::string result(length - 1, '\0');

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



int main(int argc, const char** argv)
{
    auto result = parse_args(argc, argv);
    result = 0;
    UINT32 len = 512;
    UINT32 rlen;
    test_initialize("EnvVarsA Tests", 4);


    test_begin("EnvVarsA GetEnvironmentA Test#1 Name Test2 from Json");
    try
    {
        char* buffer = new char[len];
        trace_messages("Testing: ", info_color, "GetEnvironmentA Test#1", new_line);
        rlen = GetEnvironmentVariableA("Test1", buffer, len);
        if (rlen > 0)
        {
            if (strcmp(buffer, "Value1") == 0)
            {
                result = 0;
            }
            else
            {
                trace_messages("Testing: Incorrect value read ", info_color, buffer, new_line);
                result = 1;
            }
        }
        else
        {
            result = GetLastError();
            print_last_error("Failed to getenvA on Test#1");
            if (result == 0)
                result = ERROR_UNHANDLED_EXCEPTION;
        }
    }
    catch (...)
    {
        result = GetLastError();
        print_last_error("Failed to get envA from json");
        if (result == 0)
            result = ERROR_UNHANDLED_EXCEPTION;
    }
    test_end(result);


    test_begin("EnvVarsA GetEnvironmentA Test#2 Name Test2 from AppReg HKCU");
    try
    {
        char* buffer = new char[len];
        trace_messages("Testing: ", info_color, "GetEnvironmentA Test#2", new_line);
        rlen = GetEnvironmentVariableA("Test2", buffer, len);
        if (rlen > 0)
        {
            if (strcmp(buffer, "Value2Reg") == 0)
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
            print_last_error("Failed to getenvA on Test#2");
            if (result == 0)
                result = ERROR_UNHANDLED_EXCEPTION;
        }
    }
    catch (...)
    {
        result = GetLastError();
        print_last_error("Failed to get envA from reg");
        if (result == 0)
            result = ERROR_UNHANDLED_EXCEPTION;
    }
    test_end(result);



    bool Test3Success = false;
    test_begin("EnvVarsA SetEnvironmentA Test#3 Name Test2 to AppReg HKCU");
    try
    {
        trace_messages("Testing: ", info_color, "SetEnvironmentA Test#3", new_line);
        BOOL rb = SetEnvironmentVariableA("Test2", "Value2RegModified");
        if (rb != 0)
        {
            result = 0;
            Test3Success = true;
        }
        else
        {
            result = GetLastError();
            trace_messages(L"Testing: Unable to write value ", info_color, "", new_line);
            print_last_error("Failed to setenvA on Test#3");
            if (result == 0)
                result = ERROR_UNHANDLED_EXCEPTION;
        }
    }
    catch (...)
    {
        result = GetLastError();
        print_last_error("Failed to set envA to reg");
        if (result == 0)
            result = ERROR_UNHANDLED_EXCEPTION;
    }
    test_end(result);


    test_begin("EnvVarsA GetEnvironmentA Test#4 Name Test2 from AppReg HKCU modified");
    try
    {
        char* buffer = new char[len];
        trace_messages("Testing: ", info_color, "GetEnvironmentA Test#4", new_line);
        rlen = GetEnvironmentVariableA("Test2", buffer, len);
        if (rlen > 0)
        {
            if (strcmp(buffer, "Value2RegModified") == 0)
            {
                result = 0;
            }
            else if (strcmp(buffer, "Value2Reg") == 0)
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
            print_last_error("Failed to getenvA on Test#4");
            if (result == 0)
                result = ERROR_UNHANDLED_EXCEPTION;
        }
    }
    catch (...)
    {
        result = GetLastError();
        print_last_error("Failed to get envA modified from reg");
        if (result == 0)
            result = ERROR_UNHANDLED_EXCEPTION;
    }
    test_end(result);
    

    if (Test3Success)
    {
        // Need to restore value as next test may occur without package removal/add
        trace_messages(L"Cleanup: Resetting Name Test2 ", info_color, "", new_line);
        SetEnvironmentVariableA("Test2", "Value2Reg");
    }

    test_cleanup();
    return result;

}
