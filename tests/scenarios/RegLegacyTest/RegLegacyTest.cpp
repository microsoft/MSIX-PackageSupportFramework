
#include <test_config.h>
#include <appmodel.h>
#include <algorithm>
#include <ShlObj.h>
#include <filesystem>
#include "DeletionMarkerTest.cpp"

using namespace DeletionMarkerTest;
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

// The RegLegacyFixup supports only a few intercepts.
// The tests here make routine registry calls that might have once worked but do not when running under MSIX without remediation.




#define FULL_RIGHTS_ACCESS_REQUEST   KEY_ALL_ACCESS
#define RW_ACCESS_REQUEST            KEY_READ | KEY_WRITE

void NotCoveredTests()
{
    DWORD retval = 0;
    test_begin("RegLegacy Test without changes HKCU");

    REGSAM samFull = FULL_RIGHTS_ACCESS_REQUEST;
    REGSAM sam2R = samFull & ~(DELETE|WRITE_DAC|WRITE_OWNER|KEY_CREATE_SUB_KEY|KEY_CREATE_LINK| KEY_SET_VALUE);
    REGSAM samRW = READ_CONTROL | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_CREATE_SUB_KEY;
    REGSAM samR = READ_CONTROL | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE;
    DWORD Dispo;


    HKEY HKCU_Attempt;
    HKEY HKLM_Attempt;
    HKEY HKCU_Verify;
    HKEY HKLM_Verify;

    trace_message(L"The following tests avoid using the fixup and are allowed to fail. The results are dependent on OS version you run on.", console::color::blue, true);
    if (RegOpenKey(HKEY_CURRENT_USER, TestKeyName_HKCU_NotCovered, &HKCU_Verify) == ERROR_SUCCESS)
    {
        RegCloseKey(HKCU_Verify);

        if (RegOpenKeyEx(HKEY_CURRENT_USER, TestKeyName_HKCU_NotCovered, 0, samFull , &HKCU_Attempt) == ERROR_SUCCESS)
        {
            trace_message(L"OpenKeyEx HKCU full rights SUCCESS", console::color::blue, true);

            RegCloseKey(HKCU_Attempt);
        }
        else
        {
            trace_message(L"OpenKeyEx HKCU full rights FAIL", console::color::blue, true);

        }
        if (RegOpenKeyEx(HKEY_CURRENT_USER, TestKeyName_HKCU_NotCovered, 0, sam2R, &HKCU_Attempt) == ERROR_SUCCESS)
        {
            trace_message(L"OpenKeyEx HKCU full rights-Delete SUCCESS", console::color::blue, true);

            RegCloseKey(HKCU_Attempt);
        }
        else
        {
            trace_message(L"OpenKeyEx HKCU full rights-Delete FAIL", console::color::blue, true);

        }
        if (RegCreateKeyEx(HKEY_CURRENT_USER, TestKeyName_HKCU_NotCovered, 0, NULL, 0, samFull, NULL,&HKCU_Attempt, &Dispo) == ERROR_SUCCESS)
        {
            trace_message(L"CreateKeyEx HKCU full rights SUCCESS", console::color::blue, true);

            RegCloseKey(HKCU_Attempt);
        }
        else
        {
            trace_message(L"CreateKeyEx HKCU full rights FAIL", console::color::blue, true);

        }
        if (RegCreateKeyEx(HKEY_CURRENT_USER, TestKeyName_HKCU_NotCovered, 0, NULL, 0, sam2R, NULL, &HKCU_Attempt, &Dispo) == ERROR_SUCCESS)
        {
            trace_message(L"CreateKeyEx HKCU full rights-Delete SUCCESS", console::color::blue, true);

            RegCloseKey(HKCU_Attempt);
        }
        else
        {
            trace_message(L"CreateKeyEx HKCU full rights-Delete FAIL", console::color::blue, true);

        }
    }
    else
    {
        trace_message(L"Test2CU key not found", console::color::red, true);
        retval = 2;
    }

    if (RegOpenKey(HKEY_LOCAL_MACHINE, TestKeyName_HKLM_NotCovered, &HKLM_Verify) == ERROR_SUCCESS)
    {
        RegCloseKey(HKLM_Verify);

        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, TestKeyName_HKLM_NotCovered, 0, NULL, 0, samFull, NULL, &HKLM_Attempt, &Dispo) == ERROR_SUCCESS)
        {
            trace_message(L"CreateKeyEx HKLM full rights SUCCESS", console::color::blue, true);

            RegCloseKey(HKLM_Attempt);
        }
        else
        {
            trace_message(L"CreateKeyEx HKLM full rights FAIL", console::color::blue, true);

        }
        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, TestKeyName_HKLM_NotCovered, 0, NULL, 0, sam2R, NULL, &HKLM_Attempt, &Dispo) == ERROR_SUCCESS)
        {
            trace_message(L"CreateKeyEx HKLM full rights-Delete SUCCESS", console::color::blue, true);

            RegCloseKey(HKLM_Attempt);
        }
        else
        {
            trace_message(L"CreateKeyEx HKLM full rights-Delete FAIL", console::color::blue, true);

        }

        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, TestKeyName_HKLM_NotCovered, 0, NULL, 0, samRW, NULL, &HKLM_Attempt, &Dispo) == ERROR_SUCCESS)
        {
            trace_message(L"CreateKeyEx HKLM RW SUCCESS", console::color::blue, true);

            RegCloseKey(HKLM_Attempt);
        }
        else
        {
            trace_message(L"CreateKeyEx HKLM RW FAIL", console::color::blue, true);

        }
        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, TestKeyName_HKLM_NotCovered, 0, NULL, 0, samR, NULL, &HKLM_Attempt, &Dispo) == ERROR_SUCCESS)
        {
            trace_message(L"CreateKeyEx HKLM R SUCCESS", console::color::blue, true);

            RegCloseKey(HKLM_Attempt);
        }
        else
        {
            trace_message(L"CreateKeyEx HKLM R FAIL", console::color::blue, true);

        }
    }
    else
    {
        trace_message(L"Test2LM key not found", console::color::red, true);
        retval = 2;
    }
    test_end(retval);
}

int wmain(int argc, const wchar_t** argv)
{
    auto result = parse_args(argc, argv);
    //std::wstring aumid = details::appmodel_string(&::GetCurrentApplicationUserModelId);
    test_initialize("RegLegacy Tests", 16);
    NotCoveredTests();

    DeletionMarkerTests(result);

    test_begin("RegLegacy Test ModifyKeyAccess HKCU");
    try
    {
        HKEY HKCU_Verify;
        if (RegOpenKey(HKEY_CURRENT_USER, TestKeyName_HKCU_Covered, &HKCU_Verify) == ERROR_SUCCESS)
        {
            RegCloseKey(HKCU_Verify);
            HKEY HKCU_Attempt;
            if (RegOpenKeyEx(HKEY_CURRENT_USER, TestKeyName_HKCU_Covered, 0, FULL_RIGHTS_ACCESS_REQUEST, &HKCU_Attempt) == ERROR_SUCCESS)
            {
                DWORD size = 128;  // must be big enough for the test registry item string in the registry file.
                wchar_t* data = new wchar_t[size];
                data[0] = 0;
                DWORD type;
                if (RegGetValue(HKCU_Attempt, L"", TestSubItem, RRF_RT_REG_SZ, &type, data, &size) == ERROR_SUCCESS)
                {
                    trace_message(data, console::color::gray, true);
                    trace_messages("HKCU Full Access Rights Request: NO ERROR OCCURED");
                    //print_last_error("NO ERROR OCCURED");
                    result = 0;
                }
                else
                {
                    trace_message("Failed to find read subItem.", console::color::red, true);
                    result = GetLastError();
                    if (result == 0)
                        result = ERROR_FILE_NOT_FOUND;
                    print_last_error("Failed to find read subItem");
                }
                RegCloseKey(HKCU_Attempt);
            }
            else
            {
                trace_message("Fail to open key. Remediation did not work.", console::color::red, true);
                result = GetLastError();
                if (result == 0)
                    result = ERROR_ACCESS_DENIED;
                print_last_error("Failed to open key");
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
        print_last_error("Failed to MOdify HKCU Full Access case");
    }
    test_end(result);

    test_begin("RegLegacy Test ModifyKeyAccess HKLM");
    try
    {
        HKEY HKLM_Verify;
        if (RegOpenKey(HKEY_LOCAL_MACHINE, TestKeyName_HKLM_Covered, &HKLM_Verify) == ERROR_SUCCESS)
        {
            RegCloseKey(HKLM_Verify);
            HKEY HKLM_Attempt;
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TestKeyName_HKLM_Covered, 0, RW_ACCESS_REQUEST, &HKLM_Attempt) == ERROR_SUCCESS)
            {
                DWORD size = 128;  // must be big enough for the test registry item string in the registry file.
                wchar_t* data = new wchar_t[size];
                data[0] = 0;
                DWORD type;
                if (RegGetValue(HKLM_Attempt, L"", TestSubItem, RRF_RT_REG_SZ, &type, data, &size) == ERROR_SUCCESS)
                {
                    trace_message(data, console::color::gray, true);
                    trace_messages("HKLM RW Access Test: NO ERROR OCCURED");
                    result = 0;
                }
                else
                {
                    trace_message("Failed to find read subItem.", console::color::red, true);
                    result =  GetLastError();
                    if (result == 0)
                        result = ERROR_FILE_NOT_FOUND; 
                    print_last_error("Failed to find read subItem");
                }
                RegCloseKey(HKLM_Attempt);
            }
            else
            {
                trace_message("Fail to open key. Remediation did not work.", console::color::red, true);
                result = GetLastError();
                if (result == 0)
                    result = ERROR_ACCESS_DENIED;
                print_last_error("Failed to open key");
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
        print_last_error("Failed to MOdify HKCU RW Access case");
    }

    test_end(result);



    test_cleanup();
    Sleep(1000);
    return result;
}