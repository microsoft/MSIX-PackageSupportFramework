// Copyright (C) Tim Mangan. All rights reserved
//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <psf_framework.h>

#include "FunctionImplementations.h"
#include "Framework.h"
#include "Reg_Remediation_Spec.h"
#include "Logging.h"
#include <regex>

REGSAM RegFixupSam(std::string keypath, REGSAM samDesired)
{
    REGSAM samModified = samDesired;
    std::string keystring;
#ifdef _DEBUG
        Log("RegFixupSam: %s", keypath.c_str());
#endif
    for (auto& spec : g_regRemediationSpecs)
    {
#ifdef _DEBUG
        Log("spec.type=%d", spec.remeditaionType);
#endif
        switch (spec.remeditaionType)
        {
        case Reg_Remediation_Type_ModifyKeyAccess:
#ifdef _DEBUG
            Log("Check ModifyKeyAccess...");
#endif
            for (auto& rem : spec.remediationRecords)
            {
                switch (rem.modifyKeyAccess.hive)
                {
                case Modify_Key_Hive_Type_HKCU:
                    keystring = "HKEY_CURRENT_USER\\";
                    if (keypath._Starts_with(keystring))
                    {
#ifdef _DEBUG
                        Log("HKCU key");
#endif
                        for (auto& pattern : rem.modifyKeyAccess.patterns)
                        {
#ifdef _DEBUG
                            Log("Check %LS", widen(keypath.substr(keystring.size())).c_str());
                            Log("using %LS", pattern.c_str());
#endif
                            if (std::regex_match(widen(keypath.substr(keystring.size())), std::wregex(pattern)))
                            {
#ifdef _DEBUG
                                Log("HKCU pattern match");
#endif
                                switch (rem.modifyKeyAccess.access)
                                {
                                case Modify_Key_Access_Type_Full2RW:
                                    if ((samDesired & (KEY_ALL_ACCESS|KEY_CREATE_LINK)) != 0)
                                    {
                                        samModified = samDesired & ~KEY_CREATE_LINK;
#ifdef _DEBUG
                                        Log("Full2RW");
#endif
                                    }
                                    break;
                                case Modify_Key_Access_Type_Full2R:
                                    if ((samDesired & (KEY_ALL_ACCESS | KEY_CREATE_LINK | KEY_CREATE_SUB_KEY)) != 0)
                                    {
                                        samModified = samDesired & ~(KEY_CREATE_LINK | KEY_CREATE_SUB_KEY);
#ifdef _DEBUG
                                        Log("Full2R");
#endif
                                    }
                                case Modify_Key_Access_Type_RW2R:
                                    if ((samDesired & (KEY_CREATE_SUB_KEY|KEY_SET_VALUE))  != 0)
                                    {
                                        samModified = samDesired & ~(KEY_CREATE_SUB_KEY | KEY_SET_VALUE);
#ifdef _DEBUG
                                        Log("RW2R");
#endif
                                    }
                                default:
                                    break;
                                }
                                return samModified;
                            }
                        }
                    }
                    break;
                case Modify_Key_Hive_Type_HKLM:
                    keystring = "HKEY_LOCAL_MACHINE\\";
                    if (keypath._Starts_with(keystring))
                    {
#ifdef _DEBUG
                        Log("HKLM key");
#endif
                        for (auto& pattern : rem.modifyKeyAccess.patterns)
                        {
                            if (std::regex_match(widen(keypath.substr(keystring.size())), std::wregex(pattern)))
                            {
#ifdef _DEBUG
                                Log("HKLM pattern match");
#endif
                                switch (rem.modifyKeyAccess.access)
                                {
                                case Modify_Key_Access_Type_Full2RW:
                                    if ((samDesired & (KEY_ALL_ACCESS | KEY_CREATE_LINK)) != 0)
                                    {
                                        samModified = samDesired & ~KEY_CREATE_LINK;
#ifdef _DEBUG
                                        Log("Full2RW");
#endif
                                    }
                                    break;
                                case Modify_Key_Access_Type_Full2R:
                                    if ((samDesired & (KEY_ALL_ACCESS| KEY_CREATE_LINK | KEY_CREATE_SUB_KEY | KEY_SET_VALUE)) != 0)
                                    {
                                        samModified = samDesired & ~(KEY_CREATE_LINK | KEY_CREATE_SUB_KEY | KEY_SET_VALUE);
#ifdef _DEBUG
                                        Log("Full2R");
#endif
                                    }
                                case Modify_Key_Access_Type_RW2R:
                                    if ((samDesired & (KEY_CREATE_LINK | KEY_CREATE_SUB_KEY | KEY_SET_VALUE)) != 0)
                                    {
                                        samModified = samDesired & ~(KEY_CREATE_LINK | KEY_CREATE_SUB_KEY | KEY_SET_VALUE);
#ifdef _DEBUG
                                        Log("RW2R");
#endif
                                    }
                                default:
                                    break;
                                }
                                return samModified;
                            }
                        }
                    }
                    break;
                case Modify_Key_Hive_Type_Unknown:
                default:
                    break;
                }
            }
            break;
        case Reg_Remediation_Type_Unknown:
        default:
            break;
        }
    }
    return samModified;
}



auto RegCreateKeyExImpl = psf::detoured_string_function(&::RegCreateKeyExA, &::RegCreateKeyExW);
template <typename CharT>
LSTATUS __stdcall RegCreateKeyExFixup(
    _In_ HKEY key,
    _In_ const CharT* subKey,
    _Reserved_ DWORD reserved,
    _In_opt_ CharT* classType,
    _In_ DWORD options,
    _In_ REGSAM samDesired,
    _In_opt_ CONST LPSECURITY_ATTRIBUTES securityAttributes,
    _Out_ PHKEY resultKey,
    _Out_opt_ LPDWORD disposition)
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();

    Log("RegCreateKeyEx:\n");


    std::string keypath = InterpretKeyPath(key) + "\\" + InterpretStringA(subKey);
    REGSAM samModified = RegFixupSam(keypath, samDesired);

    auto result = RegCreateKeyExImpl(key, subKey, reserved, classType, options, samModified, securityAttributes, resultKey, disposition);
    QueryPerformanceCounter(&TickEnd);

#if _DEBUG
    auto functionResult = from_win32(result);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        try
        {
            LogKeyPath(key);
            LogString("Sub Key", subKey);
            if (classType) LogString("Class", classType);
            LogRegKeyFlags(options);
            Log("samDesired=%s", InterpretRegKeyAccess(samDesired).c_str());
            if (samDesired != samModified)
            {
                Log("ModifiedSam=%s",InterpretRegKeyAccess(samModified).c_str());
            }
            LogFunctionResult(functionResult);
            if (function_failed(functionResult))
            {
                LogWin32Error(result);
            }
            else if (disposition)
            {
                LogRegKeyDisposition(*disposition);
            }
            LogCallingModule();
        }
        catch (...)
        {
            Log("RegCreateKeyEx logging failure");
        }
    }
#endif
    return result;
}
DECLARE_STRING_FIXUP(RegCreateKeyExImpl, RegCreateKeyExFixup);


auto RegOpenKeyExImpl = psf::detoured_string_function(&::RegOpenKeyExA, &::RegOpenKeyExW);
template <typename CharT>
LSTATUS __stdcall RegOpenKeyExFixup(
    _In_ HKEY key,
    _In_ const CharT* subKey,
    _In_ DWORD options,
    _In_ REGSAM samDesired,
    _Out_ PHKEY resultKey)
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();

    Log("RegOpenKeyEx:\n");


    std::string keypath = InterpretKeyPath(key) + "\\" + InterpretStringA(subKey);
    REGSAM samModified = RegFixupSam(keypath, samDesired);

    auto result = RegOpenKeyExImpl(key, subKey, options, samModified,  resultKey);
    QueryPerformanceCounter(&TickEnd);

#if _DEBUG
    auto functionResult = from_win32(result);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        try
        {
            LogKeyPath(key);
            LogString("Sub Key", subKey);
            LogRegKeyFlags(options);
            Log("samDesired=%s", InterpretRegKeyAccess(samDesired).c_str());
            if (samDesired != samModified)
            {
                Log("ModifiedSam=%s", InterpretRegKeyAccess(samModified).c_str());
            }
            LogFunctionResult(functionResult);
            if (function_failed(functionResult))
            {
                LogWin32Error(result);
            }
            LogCallingModule();
        }
        catch (...)
        {
            Log("RegOpenKeyEx logging failure");
        }
    }
#endif
    return result;
}
DECLARE_STRING_FIXUP(RegOpenKeyExImpl, RegOpenKeyExFixup);



auto RegOpenKeyTransactedImpl = psf::detoured_string_function(&::RegOpenKeyTransactedA, &::RegOpenKeyTransactedW);
template <typename CharT>
LSTATUS __stdcall RegOpenKeyTransactedFixup(
    _In_ HKEY key,
    _In_opt_ const CharT* subKey,
    _In_opt_ DWORD options,         // reserved
    _In_ REGSAM samDesired,
    _Out_ PHKEY resultKey,
    _In_ HANDLE hTransaction,
    _In_ PVOID  pExtendedParameter)  // reserved
{


    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();

#if _DEBUG
    Log("RegOpenKeyTransacted:\n");
#endif

    std::string keypath = InterpretKeyPath(key) + "\\" + InterpretStringA(subKey);
    REGSAM samModified = RegFixupSam(keypath, samDesired);

    auto result = RegOpenKeyTransactedImpl(key, subKey, options, samModified, resultKey, hTransaction, pExtendedParameter);
    QueryPerformanceCounter(&TickEnd);

#if _DEBUG
    auto functionResult = from_win32(result);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        try
        {
            LogKeyPath(key);
            if (subKey) LogString("Sub Key", subKey);
            LogRegKeyFlags(options);
            Log("SamDesired=%s", InterpretRegKeyAccess(samDesired).c_str());
            if (samDesired != samModified)
            {
                Log("ModifiedSam=%s", InterpretRegKeyAccess(samModified).c_str());
            }
            LogFunctionResult(functionResult);
            if (function_failed(functionResult))
            {
                LogWin32Error(result);
            }
            LogCallingModule();
        }
        catch (...)
        {
            Log("RegOpenKeyTransacted logging failure");
        }
    }
#endif
    return result;
}
DECLARE_STRING_FIXUP(RegOpenKeyTransactedImpl, RegOpenKeyTransactedFixup);




// Also needed (but we need a Psf detour for apis other than string function pairs):
//  RegOpenClassesRoot
//  RegOpenCurrentUser
