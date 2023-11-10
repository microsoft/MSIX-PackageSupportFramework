#pragma region Include
#include <test_config.h>
#include <appmodel.h>
#include <algorithm>
#include <filesystem>
#include < Ktmw32.h>
#pragma endregion

#pragma region Using
using namespace std::literals;
#pragma endregion

#pragma region Define
// The folllowing strings must match with registry keys present in the appropriate section of the package Registry.dat file.
#define TestKeyName_HKCU_Covered        L"Software\\Vendor_Covered\\"
#define TestKeyName_HKCU_NotCovered     L"Software\\Vendor_NotCovered"

#define TestKeyName_HKLM_SYSTEM         L"SYSTEM\\"
#define TestKeyName_HKLM_Covered        L"SOFTWARE\\Vendor_Covered"        // Registry contains both regular and wow entries so this works.
#define TestKeyName_HKLM_NotCovered     L"SOFTWARE\\Vendor_NotCovered"
#define TestKeyName_Covered_SubKey      L"SOFTWARE\\Vendor_Covered\\SubKey"
#define TestKeyName_HKLM_ControlSet     L"SYSTEM\\CurrentControlSet"
#define TestKeyName_CurrentControlSet   L"CurrentControlSet"

#define TestSubSubKey                   L"SubKey"
#define TestSubItem                     L"SubItem"
#define TestSubItem_FILENOTFOUND        L"DeletionMarker"
#pragma endregion

namespace Helper
{
    /// <summary>
    /// Test for RegQueryValueExW
    /// hive : HKCU
    /// Deletion Marker Not Found
    /// </summary>
    /// <param name="result"></param>
    inline void RegQueryValueEx_SUCCESS_HKCU(int result)
    {
        test_begin("RegLegacy Test DeletionMarker - RegQueryValueEx HKCU (SUCCESS)");

        try
        {
            HKEY hKey;

            if (RegOpenKeyEx(HKEY_CURRENT_USER, TestKeyName_HKCU_Covered, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
            {
                DWORD dwType;
                TCHAR szData[256];
                DWORD dwDataSize = sizeof(szData);

                auto response = RegQueryValueExW(hKey, TestSubItem, NULL, &dwType, (LPBYTE)szData, &dwDataSize);

                if (response == ERROR_SUCCESS)
                {
                    trace_message(szData, console::color::gray, true);
                    trace_messages("HKCU Deletion Marker Not Found");
                    result = 0;
                }
                else if (response == ERROR_FILE_NOT_FOUND)
                {
                    trace_messages("HKCU Deletion Marker Found");
                    result = ERROR_FILE_NOT_FOUND;
                }
                else
                {
                    trace_message("Failed to find read subItem.", console::color::red, true);
                    result = GetLastError();
                    if (result == 0)
                        result = ERROR_FILE_NOT_FOUND;
                    print_last_error("Failed to find read subItem");
                }
                RegCloseKey(hKey);
            }
            else
            {
                trace_message("Failed to find key. Most likely a bug in the testing tool.", console::color::red, true);
                result = GetLastError();
                if (result == 0)
                    result = ERROR_PATH_NOT_FOUND;
                print_last_error("Failed to find key");
            }
        }
        catch (...)
        {
            trace_message("Unexpected error.", console::color::red, true);
            result = GetLastError();
            print_last_error("Failed Deletion Marker HKCU case (SUCCESS)");
        }
        test_end(result);
    }

    /// <summary>
    /// Test for RegQueryValueExW
    /// hive : HKLM
    /// Deletion Marker Not Found
    /// </summary>
    /// <param name="result"></param>
    inline void RegQueryValueEx_SUCCESS_HKLM(int result)
    {
        test_begin("RegLegacy Test DeletionMarker - RegQueryValueEx HKLM (SUCCESS)");

        try
        {
            HKEY hKey;

            if (RegOpenKey(HKEY_LOCAL_MACHINE, TestKeyName_HKLM_Covered, &hKey) == ERROR_SUCCESS)
            {
                DWORD dwType;
                TCHAR szData[256];
                DWORD dwDataSize = sizeof(szData);

                auto response = RegQueryValueExW(hKey, TestSubItem, NULL, &dwType, (LPBYTE)szData, &dwDataSize);

                if (response == ERROR_SUCCESS)
                {
                    trace_message(szData, console::color::gray, true);
                    trace_messages("HKLM Deletion Marker Not Found");
                    result = 0;
                }
                else if (response == ERROR_FILE_NOT_FOUND)
                {
                    trace_messages("HKLM Deletion Marker Found");
                    result = ERROR_FILE_NOT_FOUND;
                }
                else
                {
                    trace_message("Failed to find read subItem.", console::color::red, true);
                    result = GetLastError();
                    if (result == 0)
                        result = ERROR_FILE_NOT_FOUND;
                    print_last_error("Failed to find read subItem");
                }
                RegCloseKey(hKey);
            }
            else
            {
                trace_message("Failed to find key. Most likely a bug in the testing tool.", console::color::red, true);
                result = GetLastError();
                if (result == 0)
                    result = ERROR_PATH_NOT_FOUND;
                print_last_error("Failed to find key");
            }
        }
        catch (...)
        {
            trace_message("Unexpected error.", console::color::red, true);
            result = GetLastError();
            print_last_error("Failed Deletion Marker HKLM case (SUCCESS)");
        }
        test_end(result);
    }

    /// <summary>
    /// Test for RegQueryValueExW
    /// hive : HKCU
    /// Deletion Marker Found
    /// </summary>
    /// <param name="result"></param>
    inline void RegQueryValueEx_FILENOTFOUND_HKCU(int result)
    {
        test_begin("RegLegacy Test DeletionMarker - RegQueryValueEx HKCU (FILE_NOT_FOUND)");

        try
        {
            HKEY hKey;

            if (RegOpenKeyEx(HKEY_CURRENT_USER, TestKeyName_Covered_SubKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
            {
                DWORD dwType;
                TCHAR szData[256];
                DWORD dwDataSize = sizeof(szData);

                auto response = RegQueryValueExW(hKey, TestSubItem_FILENOTFOUND, NULL, &dwType, (LPBYTE)szData, &dwDataSize);

                if (response == ERROR_SUCCESS)
                {
                    trace_message(szData, console::color::gray, true);
                    trace_messages("HKCU Deletion Marker Not Found");
                    result = ERROR_CURRENT_DIRECTORY;
                }
                else if (response == ERROR_FILE_NOT_FOUND)
                {
                    trace_messages("HKCU Deletion Marker Found");
                    result = 0;
                }
                else
                {
                    trace_message("Failed to find read subItem.", console::color::red, true);
                    result = GetLastError();
                    if (result == 0)
                        result = ERROR_CURRENT_DIRECTORY;
                    print_last_error("Failed to find read subItem");
                }
                RegCloseKey(hKey);
            }
            else
            {
                trace_message("Failed to find key. Most likely a bug in the testing tool.", console::color::red, true);
                result = GetLastError();
                if (result == 0)
                    result = ERROR_PATH_NOT_FOUND;
                print_last_error("Failed to find key");
            }
        }
        catch (...)
        {
            trace_message("Unexpected error.", console::color::red, true);
            result = GetLastError();
            print_last_error("Failed Deletion Marker HKCU case (FILE_NOT_FOUND)");
        }
        test_end(result);
    }

    /// <summary>
    /// Test for RegQueryValueExW
    /// hive : HKLM
    /// Deletion Marker Found
    /// </summary>
    /// <param name="result"></param>
    inline void RegQueryValueEx_FILENOTFOUND_HKLM(int result)
    {
        test_begin("RegLegacy Test DeletionMarker - RegQueryValueEx HKLM (FILE_NOT_FOUND)");

        try
        {
            HKEY hKey;

            if (RegOpenKey(HKEY_LOCAL_MACHINE, TestKeyName_HKLM_Covered, &hKey) == ERROR_SUCCESS)
            {
                DWORD dwType;
                TCHAR szData[256];
                DWORD dwDataSize = sizeof(szData);

                auto response = RegQueryValueExW(hKey, TestSubItem_FILENOTFOUND, NULL, &dwType, (LPBYTE)szData, &dwDataSize);

                if (response == ERROR_SUCCESS)
                {
                    trace_message(szData, console::color::gray, true);
                    trace_messages("HKLM Deletion Marker Not Found");
                    result = ERROR_CURRENT_DIRECTORY;
                }
                else if (response == ERROR_FILE_NOT_FOUND)
                {
                    trace_messages("HKLM Deletion Marker Found");
                    result = 0;
                }
                else
                {
                    trace_message("Failed to find read subItem.", console::color::red, true);
                    result = GetLastError();
                    if (result == 0)
                        result = ERROR_CURRENT_DIRECTORY;
                    print_last_error("Failed to find read subItem");
                }
                RegCloseKey(hKey);
            }
            else
            {
                trace_message("Failed to find key. Most likely a bug in the testing tool.", console::color::red, true);
                result = GetLastError();
                if (result == 0)
                    result = ERROR_PATH_NOT_FOUND;
                print_last_error("Failed to find key");
            }
        }
        catch (...)
        {
            trace_message("Unexpected error.", console::color::red, true);
            result = GetLastError();
            print_last_error("Failed Deletion Marker HKLM case (FILE_NOT_FOUND)");
        }
        test_end(result);
    }

    /// <summary>
    /// Test for RegGetValue
    /// hive : HKCU
    /// Deletion Marker Not Found
    /// </summary>
    /// <param name="result"></param>
    inline void RegGetValue_SUCCESS_HKCU(int result)
    {
        test_begin("RegLegacy Test DeletionMarker - RegGetValue HKCU (SUCCESS)");

        try
        {
            HKEY hKey;
            if (RegOpenKeyEx(HKEY_CURRENT_USER, TestKeyName_HKCU_Covered, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
            {
                DWORD dwType;
                TCHAR szData[256];
                DWORD dwDataSize = sizeof(szData);
                auto response = RegGetValue(hKey, TestSubSubKey, TestSubItem, RRF_RT_REG_SZ, &dwType, (LPBYTE)szData, &dwDataSize);

                if (response == ERROR_SUCCESS)
                {
                    trace_message(szData, console::color::gray, true);
                    trace_messages("HKCU Deletion Marker Not Found");
                    result = 0;
                }
                else if (response == ERROR_FILE_NOT_FOUND)
                {
                    trace_messages("HKCU Deletion Marker Found");
                    result = ERROR_FILE_NOT_FOUND;
                }
                else
                {
                    trace_message("Failed to find read subItem.", console::color::red, true);
                    result = GetLastError();
                    if (result == 0)
                        result = ERROR_FILE_NOT_FOUND;
                    print_last_error("Failed to find read subItem");
                }
                RegCloseKey(hKey);
            }
            else
            {
                trace_message("Failed to find key. Most likely a bug in the testing tool.", console::color::red, true);
                result = GetLastError();
                if (result == 0)
                    result = ERROR_PATH_NOT_FOUND;
                print_last_error("Failed to find key");
            }
        }
        catch (...)
        {
            trace_message("Unexpected error.", console::color::red, true);
            result = GetLastError();
            print_last_error("Failed Deletion Marker HKCU case (SUCCESS)");
        }
        test_end(result);
    }

    /// <summary>
    /// Test for RegGetValue
    /// hive : HKLM
    /// Deletion Marker Found
    /// </summary>
    /// <param name="result"></param>
    inline void RegGetValue_FILENOTFOUND_HKLM(int result)
    {
        test_begin("RegLegacy Test DeletionMarker - RegGetValue HKLM (FILENOTFOUND)");

        try
        {
            HKEY hKey;
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TestKeyName_HKLM_Covered, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
            {
                DWORD dwType;
                TCHAR szData[256];
                DWORD dwDataSize = sizeof(szData);
                auto response = RegGetValue(hKey, L"", TestSubItem_FILENOTFOUND, RRF_RT_REG_SZ, &dwType, (LPBYTE)szData, &dwDataSize);

                if (response == ERROR_SUCCESS)
                {
                    trace_message(szData, console::color::gray, true);
                    trace_messages("HKLM Deletion Marker Not Found");
                    result = ERROR_CURRENT_DIRECTORY;
                }
                else if (response == ERROR_FILE_NOT_FOUND)
                {
                    trace_messages("HKLM Deletion Marker Found");
                    result = 0;
                }
                else
                {
                    trace_message("Failed to find read subItem.", console::color::red, true);
                    result = GetLastError();
                    if (result == 0)
                        result = ERROR_CURRENT_DIRECTORY;
                    print_last_error("Failed to find read subItem");
                }
                RegCloseKey(hKey);
            }
            else
            {
                trace_message("Failed to find key. Most likely a bug in the testing tool.", console::color::red, true);
                result = GetLastError();
                if (result == 0)
                    result = ERROR_PATH_NOT_FOUND;
                print_last_error("Failed to find key");
            }
        }
        catch (...)
        {
            trace_message("Unexpected error.", console::color::red, true);
            result = GetLastError();
            print_last_error("Failed Deletion Marker HKLM case (FILENOTFOUND)");
        }
        test_end(result);
    }

    /// <summary>
    /// Test for RegQueryMultipleValues
    /// hive : HKCU
    /// Deletion Marker Not Found
    /// </summary>
    /// <param name="result"></param>
    inline void RegQueryMultipleValues_SUCCESS_HKCU(int result)
    {
        test_begin("RegLegacy Test DeletionMarker - RegQueryMultipleValues HKCU (SUCCESS)");

        try
        {
            HKEY hKey;
            if (RegOpenKeyEx(HKEY_CURRENT_USER, TestKeyName_HKCU_Covered, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
            {

                VALENTW value;
                
                DWORD num_vals = 1;
                TCHAR szData[256];
                DWORD dwDataSize = sizeof(szData);
                value.ve_valuename = (LPWSTR) TestSubItem;
                value.ve_valuelen = (sizeof(value.ve_valuename) + 1) * sizeof(WCHAR);
                value.ve_valueptr = (DWORD_PTR)szData;
                value.ve_type = REG_SZ;
                
                auto response = RegQueryMultipleValuesW(hKey, &value, num_vals, szData, &dwDataSize);

                if (response == ERROR_SUCCESS)
                {
                    trace_message(szData, console::color::gray, true);
                    trace_messages("HKCU Deletion Marker Not Found");
                    result = 0;
                }
                else if (response == ERROR_FILE_NOT_FOUND)
                {
                    trace_messages("HKCU Deletion Marker Found");
                    result = ERROR_FILE_NOT_FOUND;
                }
                else
                {
                    trace_message("Failed to find read subItem.", console::color::red, true);
                    result = GetLastError();
                    if (result == 0)
                        result = ERROR_FILE_NOT_FOUND;
                    print_last_error("Failed to find read subItem");
                }
                RegCloseKey(hKey);
            }
            else
            {
                trace_message("Failed to find key. Most likely a bug in the testing tool.", console::color::red, true);
                result = GetLastError();
                if (result == 0)
                    result = ERROR_PATH_NOT_FOUND;
                print_last_error("Failed to find key");
            }
        }
        catch (...)
        {
            trace_message("Unexpected error.", console::color::red, true);
            result = GetLastError();
            print_last_error("Failed Deletion Marker HKCU case (SUCCESS)");
        }
        test_end(result);
    }

    /// <summary>
    /// Test for RegQueryMultipleValues
    /// hive : HKLM
    /// Deletion Marker Found
    /// </summary>
    /// <param name="result"></param>
    inline void RegQueryMultipleValues_FILENOTFOUND_HKLM(int result)
    {
        test_begin("RegLegacy Test DeletionMarker - RegQueryMultipleValues HKLM (FILENOTFOUND)");

        try
        {
            HKEY hKey;
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TestKeyName_HKLM_Covered, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
            {
                VALENTW value;
                DWORD num_vals = 1;
                TCHAR szData[256];
                DWORD dwDataSize = sizeof(szData);
                value.ve_valuename = (LPWSTR)TestSubItem_FILENOTFOUND;
                value.ve_valuelen = (sizeof(value.ve_valuename) + 1) * sizeof(WCHAR);
                value.ve_valueptr = (DWORD_PTR)szData;
                value.ve_type = REG_SZ;

                auto response = RegQueryMultipleValuesW(hKey, &value, num_vals, szData, &dwDataSize);

                if (response == ERROR_SUCCESS)
                {
                    trace_message(szData, console::color::gray, true);
                    trace_messages("HKLM Deletion Marker Not Found");
                    result = ERROR_CURRENT_DIRECTORY;
                }
                else if (response == ERROR_FILE_NOT_FOUND)
                {
                    trace_messages("HKLM Deletion Marker Found");
                    result = 0;
                }
                else
                {
                    trace_message("Failed to find read subItem.", console::color::red, true);
                    result = GetLastError();
                    if (result == 0)
                        result = ERROR_CURRENT_DIRECTORY;
                    print_last_error("Failed to find read subItem");
                }
                RegCloseKey(hKey);
            }
            else
            {
                trace_message("Failed to find key. Most likely a bug in the testing tool.", console::color::red, true);
                result = GetLastError();
                if (result == 0)
                    result = ERROR_PATH_NOT_FOUND;
                print_last_error("Failed to find key");
            }
        }
        catch (...)
        {
            trace_message("Unexpected error.", console::color::red, true);
            result = GetLastError();
            print_last_error("Failed Deletion Marker HKLM case (SUCCESS)");
        }
        test_end(result);
    }

    /// <summary>
    /// Test for RegEnumKeyEx
    /// hive : HKLM
    /// Deletion Marker Found
    /// </summary>
    /// <param name="result"></param>
    inline void RegEnumKeyEx_FILENOTFOUND_HKLM(int result)
    {
        test_begin("RegLegacy Test DeletionMarker - RegEnumKeyEx HKLM (Deletion Marker Found)");

        try
        {
            HKEY hKey;
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TestKeyName_HKLM_SYSTEM, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
            {
                TCHAR szSubKey[INT16_MAX];
                DWORD cbName = INT16_MAX;
                DWORD Index = 0;

                auto response = RegEnumKeyEx(hKey, Index, szSubKey, &cbName, NULL, NULL, NULL, NULL);

                if (response == ERROR_SUCCESS)
                {
                    trace_message(szSubKey, console::color::gray, true);
                    result = ERROR_CURRENT_DIRECTORY;
                }
                else if (response == ERROR_NO_MORE_ITEMS)
                {
                    trace_messages("Index item doesn't exists");
                    result = 0;
                }
                else
                {
                    trace_message("Failed to find read subKey.", console::color::red, true);
                    result = GetLastError();
                    if (result == 0)
                        result = ERROR_CURRENT_DIRECTORY;
                    print_last_error("Failed to find read subKey");
                }
                RegCloseKey(hKey);
            }
            else
            {
                trace_message("Failed to find key. Most likely a bug in the testing tool.", console::color::red, true);
                result = GetLastError();
                if (result == 0)
                    result = ERROR_PATH_NOT_FOUND;
                print_last_error("Failed to find key");
            }
        }
        catch (...)
        {
            trace_message("Unexpected error.", console::color::red, true);
            result = GetLastError();
            print_last_error("Failed Deletion Marker HKLM case (Deletion Marker Found)");
        }
        test_end(result);
    }

    /// <summary>
    /// Test for RegEnumValue
    /// hive : HKLM
    /// Deletion Marker Found
    /// </summary>
    /// <param name="result"></param>
    inline void RegEnumValue_DeletionMarkerFound_HKLM(int result)
    {
        test_begin("RegLegacy Test DeletionMarker - RegEnumValue HKLM (Deletion Marker Found)");

        try
        {
            HKEY hKey;
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TestKeyName_HKLM_SYSTEM, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
            {
                DWORD dwIndex = 0;
                WCHAR lpValueName[INT16_MAX];
                DWORD lpcValueName = INT16_MAX;

                auto response = RegEnumValue(hKey, dwIndex, lpValueName, &lpcValueName, NULL, NULL, NULL, NULL);

                if (response == ERROR_SUCCESS)
                {
                    trace_message(lpValueName, console::color::gray, true);
                    result = ERROR_CURRENT_DIRECTORY;
                }
                else if (response == ERROR_NO_MORE_ITEMS)
                {
                    trace_messages("Index item doesn't exists");
                    result = 0;
                }
                else
                {
                    trace_message("Failed to find read subItem.", console::color::red, true);
                    result = GetLastError();
                    if (result == 0)
                        result = ERROR_CURRENT_DIRECTORY;
                    print_last_error("Failed to find read subItem");
                }
                RegCloseKey(hKey);
            }
            else
            {
                trace_message("Failed to find key. Most likely a bug in the testing tool.", console::color::red, true);
                result = GetLastError();
                if (result == 0)
                    result = ERROR_PATH_NOT_FOUND;
                print_last_error("Failed to find key");
            }
        }
        catch (...)
        {
            trace_message("Unexpected error.", console::color::red, true);
            result = GetLastError();
            print_last_error("Failed Deletion Marker HKLM case (Deletion Marker Found)");
        }
        test_end(result);
    }

    /// <summary>
    /// Test for RegOpenKeyEx
    /// hive : HKLM
    /// Deletion Marker Found
    /// </summary>
    /// <param name="result"></param>
    inline void RegOpenKeyEx_FILENOTFOUND_HKLM(int result)
    {
        test_begin("RegLegacy Test DeletionMarker - RegOpenKeyEx HKLM (Deletion Marker Found)");

        try
        {
            HKEY hKey;
            auto response = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TestKeyName_HKLM_SYSTEM, 0, KEY_READ, &hKey);
            if (response == ERROR_SUCCESS)
            {
                response = RegOpenKeyEx(hKey, TestKeyName_HKLM_ControlSet, 0, KEY_READ, &hKey);
                if (response == ERROR_SUCCESS)
                {
                    trace_message("Key Opened", console::color::gray, true);
                    result = ERROR_CURRENT_DIRECTORY;
                    RegCloseKey(hKey);
                }
                else if (response == ERROR_FILE_NOT_FOUND)
                {
                    trace_messages("Key doesn't exists");
                    result = 0;
                }
                else
                {
                    trace_message("Failed to find key ControlSet . Most likely a bug in the testing tool.", console::color::red, true);
                    result = GetLastError();
                    if (result == 0)
                        result = ERROR_PATH_NOT_FOUND;
                    print_last_error("Failed to find key");
                }
            }
            else
            {
                trace_message("Failed to find key System . Most likely a bug in the testing tool.", console::color::red, true);
                result = GetLastError();
                if (result == 0)
                    result = ERROR_PATH_NOT_FOUND;
                print_last_error("Failed to find key");
            }
        }
        catch (...)
        {
            trace_message("Unexpected error.", console::color::red, true);
            result = GetLastError();
            print_last_error("Failed Deletion Marker HKLM case (Deletion Marker Found)");
        }
        test_end(result);
    }

    /// <summary>
    /// Test for RegOpenKeyTransacted
    /// hive : HKLM
    /// Deletion Marker Found
    /// </summary>
    /// <param name="result"></param>
    inline void RegOpenKeyTransacted_FILENOTFOUND_HKLM(int result)
    {
        test_begin("RegLegacy Test DeletionMarker - RegOpenKeyTransacted HKLM (Deletion Marker Found)");

        try
        {
            HKEY hKey;
            HANDLE hTransaction = CreateTransaction(NULL, 0, 0, 0, 0, 0, NULL);
            auto response = RegOpenKeyTransacted(HKEY_LOCAL_MACHINE, TestKeyName_HKLM_ControlSet, 0, KEY_READ, &hKey, hTransaction, NULL);
            if (response == ERROR_SUCCESS)
            {
                trace_message("Key Opened", console::color::gray, true);
                result = ERROR_CURRENT_DIRECTORY;
                RegCloseKey(hKey);
            }
            else if (response == ERROR_FILE_NOT_FOUND)
            {
                trace_messages("Key doesn't exists");
                result = 0;
            }
            else
            {
                trace_message("Failed to find key. Most likely a bug in the testing tool.", console::color::red, true);
                result = GetLastError();
                if (result == 0)
                    result = ERROR_PATH_NOT_FOUND;
                print_last_error("Failed to find key");
            }
        }
        catch (...)
        {
            trace_message("Unexpected error.", console::color::red, true);
            result = GetLastError();
            print_last_error("Failed Deletion Marker HKLM case (Deletion Marker Found)");
        }
        test_end(result);
    }

    /// <summary>
    /// Test for RegQueryInfoKey
    /// hive : HKCU
    /// </summary>
    /// <param name="result"></param>
    void inline RegQueryInfoKey_SUCCESS_HKCU(int result)
    {
        test_begin("RegLegacy Test DeletionMarker - RegQueryInfoKey HKLM (SUCCESS)");

        try
        {
            HKEY hkey;
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TestKeyName_HKLM_SYSTEM, 0, KEY_ALL_ACCESS, &hkey) == ERROR_SUCCESS)
            {
                DWORD dwSubKeyCount = 0;
                DWORD dwValueCount = 0;
                DWORD dwMaxValueNameLen = 0;
                DWORD dwMaxValueLen = 0;

                auto response = RegQueryInfoKeyW(hkey, NULL, NULL, NULL, &dwSubKeyCount, NULL, NULL, &dwValueCount, &dwMaxValueNameLen, &dwMaxValueLen, NULL, NULL);
                if (response == ERROR_SUCCESS)
                {
                    trace_message("Query Key Success", console::color::gray, true);
                    result = 0;
                    if ((dwSubKeyCount || dwValueCount || dwMaxValueNameLen || dwMaxValueLen) != 0)
                    {
                        trace_message("Data does not match");
                        result = ERROR_CURRENT_DIRECTORY;
                    }
                    
                    RegCloseKey(hkey);
                }
                else if (response == ERROR_MORE_DATA)
                {
                    trace_messages("Buffer is too small to receive the name of the class,");
                    result = response;
                }
                else
                {
                    trace_message("Function Failed.", console::color::red, true);
                    result = GetLastError();
                    if (result == 0)
                        result = ERROR_PATH_NOT_FOUND;
                    print_last_error("Failed to find key");
                }
            }
            else
            {
                trace_message("Failed to find key. Most likely a bug in the testing tool.", console::color::red, true);
                result = GetLastError();
                if (result == 0)
                    result = ERROR_PATH_NOT_FOUND;
                print_last_error("Failed to find key");
            }
        }
        catch (...)
        {
            trace_message("Unexpected error.", console::color::red, true);
            result = GetLastError();
            print_last_error("Failed Deletion Marker HKLM case (SUCCESS)");
        }
        test_end(result);
    }
}