//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <iostream>
#include <sstream>

#include <Windows.h>

#include <test_config.h>

using namespace std::literals;

int wmain(int argc, const wchar_t** argv)
{
    std::map<std::wstring_view, std::wstring> allowedArgs;
    allowedArgs.emplace(L"/launchChild"sv, L"true");
    auto result = parse_args(argc, argv, allowedArgs);

#ifdef _M_IX86
    constexpr const wchar_t targetExe[] = L"PowershellScriptTest64.exe";
    constexpr const char name[] = "x64 Test";
#else
    constexpr const wchar_t targetExe[] = L"PowershellScriptTest32.exe";
    constexpr const char name[] = "x86 Test";
#endif

    auto isInitialTest = allowedArgs[L"/launchChild"] == L"true";
    if (result == ERROR_SUCCESS)
    {
        if (isInitialTest)
        {
            test_initialize("Powershell Script Tests", 2);
        }

        test_begin(name);

        test_end(result);

        test_cleanup();
    }

    return result;
}
