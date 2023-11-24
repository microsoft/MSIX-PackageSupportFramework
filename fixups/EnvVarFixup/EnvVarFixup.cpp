//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <regex>
#include <psf_framework.h>
#include "FunctionImplementations.h"
#include "EnvVar_spec.h"
#include "psf_tracelogging.h"
#include "pch.h"

extern std::vector<env_var_spec> g_envvar_envVarSpecs;

using namespace winrt::Windows::Management::Deployment;
using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::Foundation::Collections;

DWORD g_EnvVarInterceptInstance = 0;


//ExpandEnvironmentStrings
//  [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)]
//  public static extern int ExpandEnvironmentStrings([MarshalAs(UnmanagedType.LPTStr)] String source, [Out] StringBuilder destination, int size);


//GetEnvironmentVariable
//      DllImport("kernel32.dll")]
//  static extern uint GetEnvironmentVariable(string lpName,
//      [Out] StringBuilder lpBuffer, uint nSize);
//
auto GetEnvironmentVariableImpl = psf::detoured_string_function(&::GetEnvironmentVariableA, &::GetEnvironmentVariableW);
template <typename CharC, typename CharT>
DWORD __stdcall GetEnvironmentVariableFixup(_In_ const CharC* lpName, _Inout_ CharT* lpValue, _In_ DWORD lenBuf)
{
    DWORD GetEnvVarInstance = ++g_EnvVarInterceptInstance;
    LogString(GetEnvVarInstance,"GetEnvironmentVariableFixup called for", lpName);
    auto guard = g_reentrancyGuard.enter();
    DWORD result;

    if (guard)
    {
        Log("[%d] GetEnvironmentVariableFixup unguarded.", GetEnvVarInstance);
        std::wstring eName;
        if constexpr (psf::is_ansi<CharT>)
        {
            eName = widen(lpName);
        }
        else
        {
            eName =lpName;
        }

        for (env_var_spec spec : g_envvar_envVarSpecs)
        {
            try
            {
                if (std::regex_match(eName, spec.variablename))
                {
                    size_t valuelen = spec.variablevalue.length();
                    if (spec.remediationType == Env_Remediation_Type_Registry)
                    {
                        if (spec.useregistry == true)
                        {
                            Log("[%d] GetEnvironmentVariableFixup: Registry supplied case.", GetEnvVarInstance);
                            // Check app registry for an answer instead of the Json. Note: this allows value to be modified possibly also.
                            HKEY hKeyCU;
                            if constexpr (psf::is_ansi<CharT>)
                            {
                                if (RegOpenKeyExA(HKEY_CURRENT_USER, "Environment",
                                    0, KEY_ENUMERATE_SUB_KEYS | KEY_READ | KEY_QUERY_VALUE, &hKeyCU) == ERROR_SUCCESS)
                                {
                                    Log("[%d] GetEnvironmentVariableFixup:(A) HKCU key found.", GetEnvVarInstance);
                                    DWORD type = RRF_RT_REG_SZ;
                                    DWORD dLen = (DWORD)lenBuf;
                                    auto ret = RegGetValueA(HKEY_CURRENT_USER, "Environment", lpName,
                                        RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ | RRF_ZEROONFAILURE, &type, lpValue, &dLen);
                                    if (ret == ERROR_SUCCESS)
                                    {
                                        Log("[%d] GetEnvironmentVariableFixup:(A) HKCU value found %s", GetEnvVarInstance, lpValue);
                                        result = dLen;
                                        RegCloseKey(hKeyCU);
                                        return result;
                                    }
                                    else
                                    {
                                        Log("[%d] GetEnvironmentVariableFixup:(A) HKCU value Failed 0x%x.", GetEnvVarInstance, ret);
                                    }
                                    RegCloseKey(hKeyCU);
                                }
                            }
                            else
                            {
                                if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Environment",
                                    0, KEY_ENUMERATE_SUB_KEYS | KEY_READ | KEY_QUERY_VALUE, &hKeyCU) == ERROR_SUCCESS)
                                {
                                    Log("[%d] GetEnvironmentVariableFixup:(W) HKCU key found.", GetEnvVarInstance);
                                    LogString(GetEnvVarInstance, "GetEnvironmentVariableFixup:(W) Looking for ", lpName);
                                    DWORD type = RRF_RT_REG_SZ;
                                    DWORD dLen = lenBuf;
                                    auto ret = RegGetValueW(HKEY_CURRENT_USER, L"Environment", lpName,
                                        RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ | RRF_ZEROONFAILURE, &type, lpValue, &dLen);
                                    if (ret == ERROR_SUCCESS)
                                    {
                                        Log("[%d] GetEnvironmentVariableFixup:(W) HKCU value found %ls", GetEnvVarInstance, lpValue);
                                        result = dLen;
                                        RegCloseKey(hKeyCU);
                                        return result;
                                    }
                                    else
                                    {
                                        Log("[%d] GetEnvironmentVariableFixup:(W) HKCU value Failed 0x%x.", GetEnvVarInstance, ret);
                                    }
                                    RegCloseKey(hKeyCU);
                                }
                            }
                            result = 0;

                            HKEY hKeyLM;
                            if constexpr (psf::is_ansi<CharT>)
                            {
                                if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",
                                    0, KEY_ENUMERATE_SUB_KEYS | KEY_READ | KEY_QUERY_VALUE, &hKeyLM) == ERROR_SUCCESS)
                                {
                                    Log("[%d] GetEnvironmentVariableFixup:(A) HKLM key found.", GetEnvVarInstance);

                                    LONG dLen0 = (LONG)lenBuf;
                                    auto ret00 = RegQueryValueA(hKeyLM, lpName, lpValue, &dLen0);
                                    if (ret00 == ERROR_SUCCESS)
                                    {
                                        Log("[%d] GetEnvironmentVariableFixup:(A) HKLM value queried! %s", GetEnvVarInstance, lpValue);
                                        result = (DWORD)dLen0;
                                        RegCloseKey(hKeyLM);
                                        return result;
                                    }
                                    else
                                    {
                                        Log("[%d] GetEnvironmentVariableFixup:(W) HKCU value Failed 0x%x.", GetEnvVarInstance, ret00);
                                    }

                                    RegCloseKey(hKeyLM);
                                }
                            }
                            else
                            {
                                if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",
                                    0, KEY_ENUMERATE_SUB_KEYS | STANDARD_RIGHTS_READ | KEY_QUERY_VALUE, &hKeyLM) == ERROR_SUCCESS)
                                {
                                    Log("[%d] GetEnvironmentVariableFixup:(W) HKLM key found.", GetEnvVarInstance);
                                    DWORD type = RRF_RT_REG_SZ;
                                    DWORD dLen = lenBuf;
                                    auto ret = RegGetValueW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment", lpName,
                                        RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ | RRF_ZEROONFAILURE, &type, lpValue, &dLen);
                                    if (ret == ERROR_SUCCESS)
                                    {
                                        Log("[%d] GetEnvironmentVariableFixup:(W) HKLM value found %ls", GetEnvVarInstance, lpValue);
                                        result = dLen;
                                        RegCloseKey(hKeyLM);
                                        return result;
                                    }
                                    else
                                    {
                                        Log("[%d] GetEnvironmentVariableFixup:(W) HKLM value Failed 0x%x.", GetEnvVarInstance, ret);
                                    }
                                    RegCloseKey(hKeyLM);
                                }
                            }
                            // allow to fall through to system
                            //return result;
                            result = 0;
                        }

                        // Sometimes HKLM\System reg items from the package are not visible to the app.  We think this is a bug, so
                        // allow a check to see if there is a value field in the JSON and use that.
                        // Of course, we should always look at the json if useregistry was set to false anyway.
                        Log("[%d] GetEnvironmentVariableFixup: Json supplied case.", GetEnvVarInstance);

                        if (valuelen < lenBuf)
                        {
                            Log("[%d] GetEnvironmentVariableFixup: Match to be returned.", GetEnvVarInstance);
                            // copy into lpValue, but might need form conversion
                            if constexpr (psf::is_ansi<CharT>)
                            {
                                std::string sval = narrow(spec.variablevalue);
                                LogString(GetEnvVarInstance, "GetEnvironmentVariableFixup:(A) HKCU value is ", sval.c_str());
                                ZeroMemory(lpValue, lenBuf);
                                sval.copy(lpValue, lenBuf, 0);
                                //strcpy_s(lpValue, lenBuf, sval.c_str());
                                LogString(GetEnvVarInstance, "GetEnvironmentVariableFixup:(A) HKCU value copied is ", lpValue);
                                result = (DWORD)sval.length();
                            }
                            else
                            {
                                LogString(GetEnvVarInstance, "GetEnvironmentVariableFixup:(W) HKCU value is ", spec.variablevalue.data());
                                ZeroMemory(lpValue, lenBuf);
                                spec.variablevalue.copy(lpValue, lenBuf, 0);
                                LogString(GetEnvVarInstance, "GetEnvironmentVariableFixup:(W) HKCU value copied is ", lpValue);
                                result = (DWORD)valuelen;
                            }
                            Log("GetEnvironmentVariableFixup: Value saved.");
                            SetLastError(ERROR_SUCCESS);
                            return(result);
                        }
                        else
                        {
                            Log("GetEnvironmentVariableFixup: Match returns bufferoverflow.");
                            // return buffer overflow
                            result = ERROR_BUFFER_OVERFLOW;
                            SetLastError(ERROR_BUFFER_OVERFLOW);
                            return(result);
                        }

                    }
                    else if (spec.remediationType == Env_Remediation_Type_Dependency)
                    {
                        Package pkg = Package::Current();
                        IVectorView<Package> dependencies = pkg.Dependencies();

                        Package depedency = nullptr;

#ifdef _DEBUG
                        Log("Filtering required dependency\n");
#endif
                        for (auto&& dep : dependencies) {
                            std::wstring_view name = dep.Id().Name();
#ifdef _DEBUG
                            Log("Checking package dependency %.*LS\n", name.length(), name.data());
#endif

                            if (name.find(spec.dependency) != std::wstring::npos) {
#ifdef _DEBUG
                                Log("Found required dependency\n");
#endif
                                depedency = dep;
                                break;
                            }
                        }
                        if (depedency == nullptr)
                        {
                            Log("No package with name %LS found\n", spec.dependency.c_str());
                            continue;
                        }

                        if constexpr (psf::is_ansi<CharT>)
                        {
                            std::string dependency_path(narrow(depedency.EffectivePath()));
                            auto version = depedency.Id().Version();
                            std::string dependency_version = std::to_string(version.Major) + "." + std::to_string(version.Minor) + "." + std::to_string(version.Build) + "." + std::to_string(version.Revision);
                            Log("Dependency path: %S, version: %S\n", dependency_path.c_str(), dependency_version.c_str());

                            std::regex dependency_version_regex = std::regex("%dependency_version%");
                            std::regex dependency_path_regex = std::regex("%dependency_root_path%");

                            std::string value_data = std::regex_replace(narrow(spec.variablevalue), dependency_path_regex, dependency_path);
                            value_data = std::regex_replace(value_data, dependency_version_regex, dependency_version);

                            if (lenBuf > value_data.size())
                            {
                                ZeroMemory(lpValue, lenBuf);
                                value_data.copy(lpValue, lenBuf);
                                return (DWORD)value_data.size();
                            }
                            else
                            {
                                SetLastError(ERROR_BUFFER_OVERFLOW);
                                return (DWORD)value_data.size() + 1 /* To include NULL terminator */;
                            }
                        }
                        else
                        {
                            std::wstring dependency_path(depedency.EffectivePath());
                            auto version = depedency.Id().Version();
                            std::wstring dependency_version = std::to_wstring(version.Major) + L"." + std::to_wstring(version.Minor) + L"." + std::to_wstring(version.Build) + L"." + std::to_wstring(version.Revision);
                            Log("Dependency path: %LS, version: %LS\n", dependency_path.c_str(), dependency_version.c_str());

                            std::wregex dependency_version_regex = std::wregex(L"%dependency_version%");
                            std::wregex dependency_path_regex(L"%dependency_root_path%");

                            std::wstring value_data = std::regex_replace(std::wstring(spec.variablevalue), dependency_path_regex, dependency_path);
                            value_data = std::regex_replace(value_data, dependency_version_regex, dependency_version);

                            if (lenBuf > value_data.size())
                            {
                                ZeroMemory(lpValue, lenBuf * sizeof(wchar_t));
                                value_data.copy(lpValue, lenBuf);
                                return (DWORD)value_data.size();
                            }
                            else
                            {
                                SetLastError(ERROR_BUFFER_OVERFLOW);
                                return (DWORD)value_data.size() + 1 /* To include NULL terminator */;
                            }
                        }
                    }
                    else
                    {
                        Log("[%d] Unknown EnvVarFixup.\n", GetEnvVarInstance);
                    }
                }
            }
            catch (...)
            {
                psf::TraceLogExceptions("EnvVarFixupException", "GetEnvironmentVariableFixup: Bad Regex pattern ignored in EnvVarFixup");
                Log("[%d] Bad Regex pattern ignored in EnvVarFixup.\n", GetEnvVarInstance);
            }
        }

        // If still here, make original call
        Log("[%d] GetEnvironmentVariableFixup: No match - fall through.", GetEnvVarInstance);
    }
    result = GetEnvironmentVariableImpl(lpName, lpValue, lenBuf);
    return result;
}
DECLARE_STRING_FIXUP(GetEnvironmentVariableImpl, GetEnvironmentVariableFixup);


//SetEnvironmentVariable
//  [DllImport("kernel32.dll", SetLastError=true)]
//  static extern bool SetEnvironmentVariable(string lpName, string lpValue);
auto SetEnvironmentVariableImpl = psf::detoured_string_function(&::SetEnvironmentVariableA, &::SetEnvironmentVariableW);
template <typename CharT>
BOOL __stdcall SetEnvironmentVariableFixup(_In_ const CharT* lpName, _In_ const CharT* lpValue)
{
    DWORD SetEnvVarInstance = ++g_EnvVarInterceptInstance;
    LogString(SetEnvVarInstance, "SetEnvironmentVariableFixup called for", lpName);

    auto guard = g_reentrancyGuard.enter();
    BOOL result;

    if (guard)
    {
        Log("[%d] SetEnvironmentVariableFixup unguarded.", SetEnvVarInstance);

        std::wstring eName;
        if constexpr (psf::is_ansi<CharT>)
        {
            eName = widen(lpName);
        }
        else
        {
            eName = lpName;
        }
        for (env_var_spec spec : g_envvar_envVarSpecs)
        {
            try
            {
                if (std::regex_match(eName, spec.variablename))
                {
                    if (spec.remediationType == Env_Remediation_Type_Registry)
                    {
                        if (spec.useregistry == true)
                        {
                            Log("[%d] GetEnvironmentVariableFixup: registry case.", SetEnvVarInstance);

                            HKEY hKeyCU;
                            if constexpr (psf::is_ansi<CharT>)
                            {
                                if (RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, MAXIMUM_ALLOWED, &hKeyCU) == ERROR_SUCCESS)
                                {
                                    DWORD dLen = (DWORD)((strlen(lpValue) + 1) * sizeof(CharT));
                                    auto ret = RegSetValueExA(hKeyCU, lpName, NULL, REG_SZ, (BYTE*)lpValue, dLen);
                                    if (ret == ERROR_SUCCESS)
                                    {
                                        Log("[%d] SetEnvironmentVariableFixup: success. %s=%s", SetEnvVarInstance, lpName, lpValue);
                                        result = 1;
                                        return result;
                                    }
                                    else
                                    {
                                        Log("[%d] SetEnvironmentVariableFixup: Failure 0x%x.", SetEnvVarInstance, GetLastError());
                                        result = 0;
                                        return result;
                                    }
                                }
                            }
                            else
                            {
                                if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Environment", 0, MAXIMUM_ALLOWED, &hKeyCU) == ERROR_SUCCESS)
                                {
                                    DWORD dLen = (DWORD)((wcslen(lpValue) + 1) * sizeof(CharT));
                                    auto ret = RegSetValueExW(hKeyCU, lpName, NULL, REG_SZ, (BYTE*)lpValue, dLen);
                                    if (ret == ERROR_SUCCESS)
                                    {
                                        Log("[%d] SetEnvironmentVariableFixup: success. %ls=%ls", SetEnvVarInstance, lpName, lpValue);
                                        result = 1;
                                        return result;
                                    }
                                    else
                                    {
                                        Log("[%d] SetEnvironmentVariableFixup: Failure 0x%x.", SetEnvVarInstance, GetLastError());
                                        result = 0;
                                        return result;
                                    }
                                }
                            }
                        }
                        else
                        {
                            Log("[%d] GetEnvironmentVariableFixup: JSON case - return ACCESS_DENIED.", SetEnvVarInstance);
                            // Unable to overwrite json, return ACCESS_DENIED
                            SetLastError(ERROR_ACCESS_DENIED);
                            result = 0;
                            return result;
                        }
                    }
                }
            }
            catch (...)
            {
                psf::TraceLogExceptions("EnvVarFixupException", "SetEnvironmentVariableFixup: Bad Regex pattern ignored in EnvVarFixup");
#ifdef _DEBUG
                Log("[%d] Bad Regex pattern ignored in EnvVarFixup.\n", SetEnvVarInstance);
#endif
            }
        }
    }

    // If still here, make original call
    Log("[%d] SetEnvironmentVariableFixup: No match - fall through.", SetEnvVarInstance);

    result = SetEnvironmentVariableImpl(lpName, lpValue);
    return result;
}
DECLARE_STRING_FIXUP(SetEnvironmentVariableImpl, SetEnvironmentVariableFixup);
