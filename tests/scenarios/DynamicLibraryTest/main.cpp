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

// The DynamicLibraryFixup supports only two intercepts. It helps an app find dlls that are in the package in different folders.
// Traditional apps might use a Path Environment Variable or AppPath registration to solve this, but those remedies are not
// available under MSIX at present. With code access, a developer could solve this by adding folders to the dll search programatically, 
// but when repackaging an app without source, the DynamicLibraryFixup can help.
// The tests here will call each once, loading an otherwise unused PSF dll copied into a subfolder of the package.
int wmain(int argc, const wchar_t** argv)
{
    auto result = parse_args(argc, argv);
    //std::wstring aumid = details::appmodel_string(&::GetCurrentApplicationUserModelId);
    test_initialize("DynamicLibrary Tests", 2);
 

    test_begin("Dynamic Library Test LoadLibrary");
    try
    {
        trace_messages(L"Testing: ", info_color, L"LoadLibrary", new_line);
        result = 0;
        HMODULE mod1 = NULL;
#if _WinX86
        mod1 = LoadLibrary(L"TraceFixup32.dll");
#else
        mod1 = LoadLibrary(L"TraceFixup64.dll");
#endif
        if (mod1 == NULL)
        {
            result = GetLastError();
            print_last_error("Failed to load system dll");
        }
        else
        {
            result = 0;
        }
        FreeLibrary(mod1);
    }
    catch (...)
    {
        result = GetLastError();
        print_last_error("Failed to LOAD system dll");
    }
    test_end(result);


    test_begin("Dynamic Library Test LoadLibraryEx");
    try
    {
        trace_messages(L"Testing: ", info_color, L"LoadLibraryEx", new_line);
        result = 0;
       
        HMODULE mod2 = NULL;
#if _WinX86
        mod2 = LoadLibraryEx(L"TraceFixup32.dll", NULL, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
#else
        mod2 = LoadLibraryEx(L"TraceFixup64.dll", NULL, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
#endif
        if (mod2 == NULL)
        {
            result = GetLastError();
            print_last_error("Failed to load system dll");
        }
        else
        {
            result = 0;
        }
        FreeLibrary(mod2);
        
    }
    catch (...)
    {
        result = GetLastError();
        print_last_error("Failed to LOADEX system dll");
    }
    test_end(result);
    
    
    test_cleanup();
    return result;
}
