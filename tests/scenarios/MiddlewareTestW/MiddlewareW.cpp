//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <test_config.h>
#include <appmodel.h>
#include <algorithm>
#include <ShlObj.h>
#include <filesystem>
#include <dependency_helper.h>

using namespace std::literals;

#define Registry_Key_1                 L"Software\\random_key_2\\path\\latest"
#define Registry_Key_2(v)              L"Software\\random_key\\abc\\" + v

#define Registry_Value_Version         L"Version"
#define Registry_Value_Full_Path       L"FullPath"

#define Environment_Variable_1         L"LIB_PATH"
#define Environment_Variable_2         L"PKG_VER"

int wmain(int argc, const wchar_t** argv)
{
    auto result = parse_args(argc, argv);

    test_initialize("MiddlewareW Tests", 4);

    if (auto vclib_package = query_package_with_name(VCLibsName))
    {
        {
            test_begin("Middleware test RegQueryValue able to fetch HKLM data created by the fixup with proper replacements");

            HKEY res;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, Registry_Key_1, 0, KEY_ALL_ACCESS, &res) == ERROR_SUCCESS)
            {
                DWORD dataSize;
                if (RegQueryValueExW(res, Registry_Value_Full_Path, NULL, NULL, NULL, &dataSize) == ERROR_SUCCESS)
                {
                    trace_message(L"RegQueryValueEx successfully got size", console::color::blue, true);

                    wchar_t* data = new wchar_t[dataSize];

                    if (RegQueryValueExW(res, Registry_Value_Full_Path, NULL, NULL, (LPBYTE) data, &dataSize) == ERROR_SUCCESS)
                    {
                        std::wstring expected = L"\\\\?\\" + vclib_package->pkg_install_path + L"\\bin";

                        if (expected.compare(data) == 0) {
                            trace_message(L"RegQueryValueEx successfully got expected data", console::color::blue, true);
                            result = ERROR_SUCCESS;
                        }
                        else {
                            std::wstring msg = L"RegQueryValueEx got incorrect data. Expected: \"" + expected + L"\" Found: \"" + data + L"\"";
                            trace_message(msg, console::color::red, true);
                            result = 1;
                        }
                    }
                    else
                    {
                        trace_message(L"RegQueryValueEx failed to get value data", console::color::red, true);
                        result = ERROR_FILE_NOT_FOUND;
                    }
                    delete data;
                }
                else 
                {
                    trace_message(L"RegQueryValueEx failed to get value data size", console::color::red, true);
                    result = 2;
                }

                RegCloseKey(res);
            }
            else
            {
                trace_message(L"RegOpenKeyEx HKLM on a generated key failed", console::color::red, true);
                result = ERROR_FILE_NOT_FOUND;
            }

            test_end(result);
        }

        {
            test_begin("Middleware test RegGetValue able to fetch HKCU data created by the fixup with proper replacements");

            std::wstring keyName = Registry_Key_2(vclib_package->pkg_version);

            DWORD dataSize;
            if (RegGetValueW(HKEY_CURRENT_USER, keyName.c_str(), Registry_Value_Version, RRF_RT_ANY, NULL, NULL, &dataSize) == ERROR_SUCCESS)
            {
                trace_message(L"RegGetValue successfully got size", console::color::blue, true);

                wchar_t* data = new wchar_t[dataSize];

                if (RegGetValueW(HKEY_CURRENT_USER, keyName.c_str(), Registry_Value_Version, RRF_RT_ANY, NULL, data, &dataSize) == ERROR_SUCCESS)
                {
                    std::wstring expected = vclib_package->pkg_version + L"-SNAPSHOT";

                    if (expected.compare(data) == 0)
                    {
                        trace_message(L"RegGetValue successfully got expected data", console::color::blue, true);
                        result = ERROR_SUCCESS;
                    }
                    else
                    {
                        std::wstring msg = L"RegGetValue got incorrect data. Expected: \"" + expected + L"\" Found: \"" + data + L"\"";
                        trace_message(msg, console::color::red, true);
                        result = 1;
                    }
                }
                else
                {
                    trace_message(L"RegGetValue failed to get value data", console::color::red, true);
                    result = ERROR_FILE_NOT_FOUND;
                }

                delete data;
            }
            else
            {
                trace_message(L"RegGetValue failed to get value data size", console::color::red, true);
                result = 2;
            }
            test_end(result);
        }

        {
            test_begin("Middleware test GetEnvironmentVariable able to get data created by the fixup with proper replacements");

            wchar_t data[512];

            DWORD dataSize = GetEnvironmentVariableW(Environment_Variable_1, data, 512);
            if (dataSize > 0)
            {
                std::wstring expected = vclib_package->pkg_install_path + L"\\lib";

                if (expected.compare(data) == 0)
                {
                    trace_message(L"GetEnvironmentVariable successfully got expected data", console::color::blue, true);
                    result = ERROR_SUCCESS;
                }
                else
                {
                    std::wstring msg = L"GetEnvironmentVariable got incorrect data. Expected: \"" + expected + L"\" Found: \"" + data + L"\"";
                    trace_message(msg, console::color::red, true);
                    result = 1;
                }
            }
            else
            {
                trace_message(L"GetEnvironmentVariable failed to get data", console::color::red, true);
                result = 2;
            }
            
            test_end(result);
        }

        {
            test_begin("Middleware test GetEnvironmentVariable able to get data created by the fixup with proper replacements with dynamic buffer");

            DWORD dataSize = GetEnvironmentVariableW(Environment_Variable_2, NULL, 0);
            if (dataSize != 0)
            {
                trace_message(L"GetEnvironmentVariable successfully got size ", console::color::blue, true);

                wchar_t* data = new wchar_t[dataSize];

                if (GetEnvironmentVariableW(Environment_Variable_2, data, dataSize) != 0)
                {
                    std::wstring expected = L"v" + vclib_package->pkg_version;

                    if (expected.compare(data) == 0)
                    {
                        trace_message(L"GetEnvironmentVariable successfully got expected data", console::color::blue, true);
                        result = ERROR_SUCCESS;
                    }
                    else
                    {
                        std::wstring msg = L"GetEnvironmentVariable got incorrect data. Expected: \"" + expected + L"\" Found: \"" + data + L"\"";
                        trace_message(msg, console::color::red, true);
                        result = 1;
                    }
                }
                else
                {
                    trace_message(L"GetEnvironmentVariable failed to get value data", console::color::red, true);
                    result = ERROR_FILE_NOT_FOUND;
                }

                delete data;
            }
            else
            {
                trace_message(L"GetEnvironmentVariable failed to get value data size", console::color::red, true);
                result = 2;
            }
            
            test_end(result);
        }
    }
    else
    {
        trace_message(L"VCLibs package not found", console::color::red, true);
        result = ERROR_FILE_NOT_FOUND;
    }

    test_cleanup();
    return result;
}
