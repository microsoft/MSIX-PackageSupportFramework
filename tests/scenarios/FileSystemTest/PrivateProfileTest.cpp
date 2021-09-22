//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan - TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <filesystem>
#include <functional>

#include <test_config.h>

#include "common_paths.h"
#include <iostream>
#include <fstream>

extern void Log(const char* fmt, ...);

int PrivateProfileTests()
{
	int result = ERROR_SUCCESS;
    int testResult;
	auto packageFilePath = g_packageRootPath / g_packageFileName;
	//auto otherFilePathIntl = g_packageRootPath / L"TèƨƭFïℓè.ini";
    auto otherFilePath = g_packageRootPath / L"TestIniFile.ini";


    std::wstring badValue = L"BadValue";
    std::wstring updatedValue = L"UpdatedValue";
    int badValueInt = 666;

    clean_redirection_path();

    int   testLen = 2048*sizeof(wchar_t);
    wchar_t* buffer = (wchar_t *)malloc(testLen);
    if (buffer != NULL)
    {
        int rLen;

    	// Read from a file in the package path
	    test_begin("GetPrivateProfileSectionNames From Package Test");
        rLen = GetPrivateProfileSectionNames(buffer, testLen, otherFilePath.c_str());

        if (rLen > 0)
        {
            testResult = ERROR_SUCCESS;
        }
        else
        {
            testResult = GetLastError();
        }
    	result = result ? result : testResult;
	    test_end(testResult);

	    test_begin("GetPrivateProfileSection From Package Test");

        rLen = GetPrivateProfileSection(L"Section1",buffer, testLen, otherFilePath.native().c_str());

        if (rLen > 0)
        {
            testResult = ERROR_SUCCESS;
        }
        else
        {
            testResult = GetLastError();
        }
    	result = result ? result : testResult;
	    test_end(testResult);


	    test_begin("GetPrivateProfileString From Package Test");
        rLen = GetPrivateProfileString(L"Section1", L"ItemString", badValue.c_str(), buffer, testLen, otherFilePath.c_str());

        if (rLen > 0)
        {
            if (_wcsicmp(badValue.c_str(), buffer) == 0)
                testResult = -1;
            else
                testResult = ERROR_SUCCESS;
        }
        else
        {
            testResult = GetLastError();
        }
    	result = result ? result : testResult;
	    test_end(testResult);

	    test_begin("GetPrivateProfileInt From Package Test");

        int rVal = GetPrivateProfileInt(L"Section1",L"ItemInt", badValueInt , otherFilePath.c_str());

        if (rVal != badValueInt)
        {
            testResult = ERROR_SUCCESS;
        }
        else
        {
            testResult = GetLastError();
            if (testResult == ERROR_SUCCESS)
                testResult = -1;
        }
    	result = result ? result : testResult;
	    test_end(testResult);

        test_begin("WritePrivateProfileString to Package(Intl) Test");
        Log("<<<<<WritePrivateProfileString to Package(Intl) Test");
        BOOL rbool = WritePrivateProfileString(L"Section2", L"UnusedItem", updatedValue.c_str(), otherFilePath.c_str());
        if (rbool != 0)
        {
            testResult = ERROR_SUCCESS;
        }
        else
        {
            testResult = GetLastError();
        }
        Log("WritePrivateProfileString to Package(Intl) Test>>>>>");
        result = result ? result : testResult;
        test_end(testResult);



        test_begin("GetPrivateProfileString From Redirection (Intl) Test");
        rLen = GetPrivateProfileString(L"Section2", L"UnusedItem", badValue.c_str(), buffer, testLen, otherFilePath.c_str());
        if  (rLen  > 0)
        {
            if (_wcsicmp(updatedValue.c_str(), buffer) == 0)
                testResult = ERROR_SUCCESS;
            else
                testResult = -1;
        }
        else
        {
            testResult = GetLastError();
        }
        result = result ? result : testResult;
        test_end(testResult);
        free(buffer);
    }
    return result;
}