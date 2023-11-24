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

#define Registry_Key_1                 "Software\\random_key_2\\path\\latest"
#define Registry_Key_2(v)              "Software\\random_key\\abc\\" + v

#define Registry_Value_Version         "Version"
#define Registry_Value_Full_Path       "FullPath"

#define Environment_Variable_1         "LIB_PATH"
#define Environment_Variable_2         "PKG_VER"

int main(int argc, const char** argv)
{
    auto result = parse_args(argc, argv);

    test_initialize("MiddlewareW Tests", 4);

    if (auto vclib_package = query_package_with_name(VCLibsName))
    {
        {
            test_begin("Middleware test RegQueryValue able to fetch HKLM data created by the fixup with proper replacements");

            HKEY res;
            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, Registry_Key_1, 0, KEY_ALL_ACCESS, &res) == ERROR_SUCCESS)
            {
                DWORD dataSize;
                if (RegQueryValueExA(res, Registry_Value_Full_Path, NULL, NULL, NULL, &dataSize) == ERROR_SUCCESS)
                {
                    trace_message("RegQueryValueEx successfully got size", console::color::blue, true);

                    char* data = new char[dataSize];

                    if (RegQueryValueExA(res, Registry_Value_Full_Path, NULL, NULL, (LPBYTE) data, &dataSize) == ERROR_SUCCESS)
                    {
                        std::string expected = "\\\\?\\" + narrow(vclib_package->pkg_install_path) + "\\bin";

                        if (expected.compare(data) == 0) {
                            trace_message("RegQueryValueEx successfully got expected data", console::color::blue, true);
                            result = ERROR_SUCCESS;
                        }
                        else {
                            std::string msg = "RegQueryValueEx got incorrect data. Expected: \"" + expected + "\" Found: \"" + data + "\"";
                            trace_message(msg, console::color::red, true);
                            result = 1;
                        }
                    }
                    else
                    {
                        trace_message("RegQueryValueEx failed to get value data", console::color::red, true);
                        result = ERROR_FILE_NOT_FOUND;
                    }
                    delete data;
                }
                else 
                {
                    trace_message("RegQueryValueEx failed to get value data size", console::color::red, true);
                    result = 2;
                }

                RegCloseKey(res);
            }
            else
            {
                trace_message("RegOpenKeyEx HKLM on a generated key failed", console::color::red, true);
                result = ERROR_FILE_NOT_FOUND;
            }

            test_end(result);
        }

        {
            test_begin("Middleware test RegGetValue able to fetch HKCU data created by the fixup with proper replacements");

            std::string keyName = Registry_Key_2(narrow(vclib_package->pkg_version));

            DWORD dataSize;
            if (RegGetValueA(HKEY_CURRENT_USER, keyName.c_str(), Registry_Value_Version, RRF_RT_ANY, NULL, NULL, &dataSize) == ERROR_SUCCESS)
            {
                trace_message("RegGetValue successfully got size", console::color::blue, true);

                char* data = new char[dataSize];

                if (RegGetValueA(HKEY_CURRENT_USER, keyName.c_str(), Registry_Value_Version, RRF_RT_ANY, NULL, data, &dataSize) == ERROR_SUCCESS)
                {
                    std::string expected = narrow(vclib_package->pkg_version) + "-SNAPSHOT";

                    if (expected.compare(data) == 0)
                    {
                        trace_message("RegGetValue successfully got expected data", console::color::blue, true);
                        result = ERROR_SUCCESS;
                    }
                    else
                    {
                        std::string msg = "RegGetValue got incorrect data. Expected: \"" + expected + "\" Found: \"" + data + "\"";
                        trace_message(msg, console::color::red, true);
                        result = 1;
                    }
                }
                else
                {
                    trace_message("RegGetValue failed to get value data", console::color::red, true);
                    result = ERROR_FILE_NOT_FOUND;
                }

                delete data;
            }
            else
            {
                trace_message("RegGetValue failed to get value data size", console::color::red, true);
                result = 2;
            }
            test_end(result);
        }

        {
            test_begin("Middleware test GetEnvironmentVariable able to get data created by the fixup with proper replacements");

            char data[512];

            DWORD dataSize = GetEnvironmentVariableA(Environment_Variable_1, data, 512);
            if (dataSize > 0)
            {
                std::string expected = narrow(vclib_package->pkg_install_path) + "\\lib";

                if (expected.compare(data) == 0)
                {
                    trace_message("GetEnvironmentVariable successfully got expected data", console::color::blue, true);
                    result = ERROR_SUCCESS;
                }
                else
                {
                    std::string msg = "GetEnvironmentVariable got incorrect data. Expected: \"" + expected + "\" Found: \"" + data + "\"";
                    trace_message(msg, console::color::red, true);
                    result = 1;
                }
            }
            else
            {
                trace_message("GetEnvironmentVariable failed to get data", console::color::red, true);
                result = 2;
            }
            
            test_end(result);
        }

        {
            test_begin("Middleware test GetEnvironmentVariable able to get data created by the fixup with proper replacements with dynamic buffer");

            DWORD dataSize = GetEnvironmentVariableA(Environment_Variable_2, NULL, 0);
            if (dataSize != 0)
            {
                trace_message("GetEnvironmentVariable successfully got size ", console::color::blue, true);

                char* data = new char[dataSize];

                if (GetEnvironmentVariableA(Environment_Variable_2, data, dataSize) != 0)
                {
                    std::string expected = "v" + narrow(vclib_package->pkg_version);

                    if (expected.compare(data) == 0)
                    {
                        trace_message("GetEnvironmentVariable successfully got expected data", console::color::blue, true);
                        result = ERROR_SUCCESS;
                    }
                    else
                    {
                        std::string msg = "GetEnvironmentVariable got incorrect data. Expected: \"" + expected + "\" Found: \"" + data + "\"";
                        trace_message(msg, console::color::red, true);
                        result = 1;
                    }
                }
                else
                {
                    trace_message("GetEnvironmentVariable failed to get value data", console::color::red, true);
                    result = ERROR_FILE_NOT_FOUND;
                }

                delete data;
            }
            else
            {
                trace_message("GetEnvironmentVariable failed to get value data size", console::color::red, true);
                result = 2;
            }
            
            test_end(result);
        }
    }
    else
    {
        trace_message("VCLibs package not found", console::color::red, true);
        result = ERROR_FILE_NOT_FOUND;
    }

    test_cleanup();
    return result;
}
