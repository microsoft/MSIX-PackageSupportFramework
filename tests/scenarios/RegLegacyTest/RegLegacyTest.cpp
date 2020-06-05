
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

// The RegLegacyFixup supports only a few intercepts.
// The tests here make routine registry calls that might have once worked but do not when running under MSIX without remediation.

// THe folllowing strings must match with registry keus present in the appropriate section of the package Registry.dat file.
#define TestKeyName_HKCU  L"Software\\Vendor"
#define TestKeyName_HKLM  L"SOFTWARE\\Vendor"
#define TestSubSubKey L"SubKey"
#define TestSubItem  L"SubItem"

#define FULL_RIGHTS_ACCESS_REQUEST   KEY_ALL_ACCESS
#define RW_ACCESS_REQUEST            KEY_READ | KEY_WRITE
int wmain(int argc, const wchar_t** argv)
{
    auto result = parse_args(argc, argv);
    //std::wstring aumid = details::appmodel_string(&::GetCurrentApplicationUserModelId);
    test_initialize("RegLegacy Tests", 2);


    test_begin("RegLegacy Test ModifyKeyAccess HKCU");
    try
    {
        HKEY HKCU_Verify;
        if (RegOpenKey(HKEY_CURRENT_USER, TestKeyName_HKCU, &HKCU_Verify) == ERROR_SUCCESS)
        {
            RegCloseKey(HKCU_Verify);
            HKEY HKCU_Attempt;
            if (RegOpenKeyEx(HKEY_CURRENT_USER, TestKeyName_HKCU, 0, FULL_RIGHTS_ACCESS_REQUEST, &HKCU_Attempt) == ERROR_SUCCESS)
            {
                DWORD size = 128;  // must be big enough for the test registry item string in the registry file.
                wchar_t* data = new wchar_t[size];
                data[0] = 0;
                DWORD type;
                if (RegGetValue(HKCU_Attempt, L"", TestSubItem, RRF_RT_REG_SZ, &type, data, &size) == ERROR_SUCCESS)
                {
                    trace_message(data, console::color::gray, true);
                    print_last_error("NO ERROR OCCURED");
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
        trace_message("try", console::color::gray, true);
        HKEY HKLM_Verify;
        if (RegOpenKey(HKEY_LOCAL_MACHINE, TestKeyName_HKLM, &HKLM_Verify) == ERROR_SUCCESS)
        {
            RegCloseKey(HKLM_Verify);
            HKEY HKLM_Attempt;
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TestKeyName_HKLM, 0, RW_ACCESS_REQUEST, &HKLM_Attempt) == ERROR_SUCCESS)
            {
                DWORD size = 128;  // must be big enough for the test registry item string in the registry file.
                wchar_t* data = new wchar_t[size];
                data[0] = 0;
                DWORD type;
                if (RegGetValue(HKLM_Attempt, L"", TestSubItem, RRF_RT_REG_SZ, &type, data, &size) == ERROR_SUCCESS)
                {
                    trace_message("success vvv", console::color::gray, true);
                    trace_message(data, console::color::gray, true);
                    trace_message("success ^^^", console::color::gray, true);
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
        print_last_error("Failed to MOdify HKCU Full Access case");
    }
    trace_message("completed", console::color::gray, true);

    test_end(result);



    test_cleanup();
    return result;
}