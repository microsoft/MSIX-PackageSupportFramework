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
    auto result = parse_args(argc, argv);
    test_initialize("Powershell Script Tests", 1);
    test_begin("Powershell Script Test");
    test_end(result);
    test_cleanup();

    return result;
}
