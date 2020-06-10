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

DWORD g_RegIntceptInstance = 0;

REGSAM RegFixupSam(std::string keypath, REGSAM samDesired, DWORD RegLocalInstance)
{

    REGSAM samModified = samDesired;
    std::string keystring;

    Log("[%d] RegFixupSam: path=%s\n", RegLocalInstance, keypath.c_str());
    for (auto& spec : g_regRemediationSpecs)
    {
#ifdef _DEBUG
        Log("[%d] RegFixupSam: spec.type=%d\n", RegLocalInstance, spec.remeditaionType);
#endif
        switch (spec.remeditaionType)
        {
        case Reg_Remediation_Type_ModifyKeyAccess:
#ifdef _DEBUG
            Log("[%d] RegFixupSam: is Check ModifyKeyAccess...\n", RegLocalInstance);
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
                        Log("[%d] RegFixupSam: is HKCU key\n", RegLocalInstance);
#endif
                        for (auto& pattern : rem.modifyKeyAccess.patterns)
                        {
#ifdef _DEBUG
                            Log("[%d] RegFixupSam: Check %LS\n", RegLocalInstance, widen(keypath.substr(keystring.size())).c_str());
                            Log("[%d] RegFixupSam: using %LS\n", RegLocalInstance, pattern.c_str());
#endif
                            if (std::regex_match(widen(keypath.substr(keystring.size())), std::wregex(pattern)))
                            {
#ifdef _DEBUG
                                Log("[%d] RegFixupSam: is HKCU pattern match.\n", RegLocalInstance);
#endif
                                switch (rem.modifyKeyAccess.access)
                                {
                                case Modify_Key_Access_Type_Full2RW:
                                    if ((samDesired & (KEY_ALL_ACCESS|KEY_CREATE_LINK)) != 0)
                                    {
                                        samModified = samDesired & ~(DELETE|KEY_CREATE_LINK);
#ifdef _DEBUG
                                        Log("[%d] RegFixupSam: Full2RW\n", RegLocalInstance);
#endif
                                    }
                                    break;
                                case Modify_Key_Access_Type_Full2R:
                                    if ((samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_CREATE_LINK)) != 0)
                                    {
                                        samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_CREATE_LINK);
#ifdef _DEBUG
                                        Log("[%d] RegFixupSam: Full2R\n", RegLocalInstance);
#endif
                                    }
                                case Modify_Key_Access_Type_RW2R:
                                    if ((samDesired & (KEY_CREATE_LINK|KEY_CREATE_SUB_KEY|KEY_SET_VALUE))  != 0)
                                    {
                                        samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_CREATE_LINK);
#ifdef _DEBUG
                                        Log("[%d] RegFixupSam: RW2R\n", RegLocalInstance);
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
                        Log("[%d] RegFixupSam:  is HKLM key\n", RegLocalInstance);
#endif
                        for (auto& pattern : rem.modifyKeyAccess.patterns)
                        {
                            if (std::regex_match(widen(keypath.substr(keystring.size())), std::wregex(pattern)))
                            {
#ifdef _DEBUG
                                Log("[%d] RegFixupSam: HKLM pattern match.\n", RegLocalInstance);
#endif
                                switch (rem.modifyKeyAccess.access)
                                {
                                case Modify_Key_Access_Type_Full2RW:
                                    if ((samDesired & (KEY_ALL_ACCESS | KEY_CREATE_LINK)) != 0)
                                    {
                                        samModified = samDesired & ~(DELETE|KEY_CREATE_LINK);
#ifdef _DEBUG
                                        Log("[%d] RegFixupSam: Full2RW\n", RegLocalInstance);
#endif
                                    }
                                    break;
                                case Modify_Key_Access_Type_Full2R:
                                    if ((samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_CREATE_LINK)) != 0)
                                    {
                                        samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_CREATE_LINK);
#ifdef _DEBUG
                                        Log("[%d] RegFixupSam: Full2R\n", RegLocalInstance);
#endif
                                    }
                                case Modify_Key_Access_Type_RW2R:
                                    if ((samDesired & (KEY_CREATE_LINK | KEY_CREATE_SUB_KEY | KEY_SET_VALUE)) != 0)
                                    {
                                        samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_CREATE_LINK);
#ifdef _DEBUG
                                        Log("[%d] RegFixupSam: RW2R\n", RegLocalInstance);
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
    DWORD RegLocalInstance = ++g_RegIntceptInstance;

    auto entry = LogFunctionEntry();
    Log("[%d] RegCreateKeyEx:\n", RegLocalInstance);


    std::string keypath = InterpretKeyPath(key) + "\\" + InterpretStringA(subKey);
    REGSAM samModified = RegFixupSam(keypath, samDesired, RegLocalInstance);

    auto result = RegCreateKeyExImpl(key, subKey, reserved, classType, options, samModified, securityAttributes, resultKey, disposition);
    QueryPerformanceCounter(&TickEnd);

#if _DEBUG
    auto functionResult = from_win32(result);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        try
        {
            LogKeyPath(key);
            LogString("\nSub Key", subKey);
            Log("[%d]Reserved=%d\n", RegLocalInstance, reserved);
            if (classType) LogString("\nClass", classType);
            LogRegKeyFlags(options);
            Log("\n[%d] samDesired=%s\n", RegLocalInstance, InterpretRegKeyAccess(samDesired).c_str());
            if (samDesired != samModified)
            {
                Log("[%d] ModifiedSam=%s\n", RegLocalInstance, InterpretRegKeyAccess(samModified).c_str());
            }
            LogSecurityAttributes(securityAttributes, RegLocalInstance);

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
            Log("[%d] RegCreateKeyEx logging failure.\n", RegLocalInstance);
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
    DWORD RegLocalInstance = ++g_RegIntceptInstance;

    auto entry = LogFunctionEntry();

    Log("[%d] RegOpenKeyEx:\n", RegLocalInstance);


    std::string keypath = InterpretKeyPath(key) + "\\" + InterpretStringA(subKey);
    REGSAM samModified = RegFixupSam(keypath, samDesired, RegLocalInstance);

    auto result = RegOpenKeyExImpl(key, subKey, options, samModified,  resultKey);
    QueryPerformanceCounter(&TickEnd);

#if _DEBUG
    auto functionResult = from_win32(result);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        try
        {
            LogKeyPath(key);
            LogString("\nSub Key", subKey);
            LogRegKeyFlags(options);
            Log("\n[%d] samDesired=%s\n", RegLocalInstance, InterpretRegKeyAccess(samDesired).c_str());
            if (samDesired != samModified)
            {
                Log("[%d] ModifiedSam=%s\n", RegLocalInstance, InterpretRegKeyAccess(samModified).c_str());
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
            Log("[%d] RegOpenKeyEx logging failure.\n", RegLocalInstance);
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
    DWORD RegLocalInstance = ++g_RegIntceptInstance;
    auto entry = LogFunctionEntry();

#if _DEBUG
    Log("[%d] RegOpenKeyTransacted:\n", RegLocalInstance);
#endif

    std::string keypath = InterpretKeyPath(key) + "\\" + InterpretStringA(subKey);
    REGSAM samModified = RegFixupSam(keypath, samDesired, RegLocalInstance);

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
            Log("\n[%d] SamDesired=%s\n", RegLocalInstance, InterpretRegKeyAccess(samDesired).c_str());
            if (samDesired != samModified)
            {
                Log("[%d] ModifiedSam=%s\n", RegLocalInstance, InterpretRegKeyAccess(samModified).c_str());
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
            Log("[%d] RegOpenKeyTransacted logging failure.\n", RegLocalInstance);
        }
    }
#endif
    return result;
}
DECLARE_STRING_FIXUP(RegOpenKeyTransactedImpl, RegOpenKeyTransactedFixup);




// Also needed (but we need a Psf detour for apis other than string function pairs):
//  RegOpenClassesRoot
//  RegOpenCurrentUser
