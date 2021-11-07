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

std::string ReplaceRegistrySyntax(std::string regPath)
{
    std::string returnPath = regPath;

    // string returned is for pattern matching purposes, to keep consistent for the patterns,
    // revert to strings that look like HKEY_....
    if (regPath._Starts_with("=\\REGISTRY\\USER"))
    {
        if (regPath.length() > 15)
        {
            size_t offsetAfterSid = regPath.find('\\', 16);
            if (offsetAfterSid != std::string::npos)
            {
                returnPath = InterpretStringA("HKEY_CURRENT_USER") + regPath.substr(offsetAfterSid);
            }
            else
            {
                returnPath = InterpretStringA("HKEY_CURRENT_USER") + regPath.substr(15);
            }
        }
        else
        {
            returnPath = InterpretStringA("HKEY_CURRENT_USER");
        }
    }
    else if (regPath._Starts_with("=\\REGISTRY\\MACHINE"))
    {
        if (regPath.length() > 18)
        {
            size_t offsetAfterSid = regPath.find('\\', 19);
            if (offsetAfterSid != std::string::npos)
            {
                returnPath = InterpretStringA("HKEY_CURRENT_USER") + regPath.substr(offsetAfterSid);
            }
            else
            {
                returnPath = InterpretStringA("HKEY_LOCAL_MACHINE") + regPath.substr(18);
            }
        }
        else
        {
            returnPath = InterpretStringA("HKEY_LOCAL_MACHINE");
        }
    }
    return returnPath;
}
REGSAM RegFixupSam(std::string keypath, REGSAM samDesired, DWORD RegLocalInstance)
{

    REGSAM samModified = samDesired;
    std::string keystring;
    std::string altkeystring;


    Log("[%d] RegFixupSam: path=%s\n", RegLocalInstance, keypath.c_str());
    for (auto& spec : g_regRemediationSpecs)
    {
      
        for (auto& specitem: spec.remediationRecords)
        {  
            switch (specitem.remeditaionType)
            {
            case Reg_Remediation_Type_ModifyKeyAccess:
#ifdef _DEBUG
                Log("[%d] RegFixupSam: rule is Check ModifyKeyAccess...\n", RegLocalInstance);
#endif
                switch (specitem.modifyKeyAccess.hive)
                {
                case Modify_Key_Hive_Type_HKCU:
                    keystring = "HKEY_CURRENT_USER\\";
                    altkeystring = "=\\REGISTRY\\USER\\";
                    if (keypath._Starts_with(keystring) ||
                        keypath._Starts_with(altkeystring))
                    {
#ifdef _DEBUG
                        //Log("[%d] RegFixupSam: is HKCU key\n", RegLocalInstance);
#endif
                        for (auto& pattern : specitem.modifyKeyAccess.patterns)
                        {
                            size_t OffsetHkcu  = keystring.size();
                            if (keypath._Starts_with(altkeystring))
                            {
                                // Must remove both the pattern and the S-1-5-...\ that follows.
                                OffsetHkcu = keypath.find_first_of('\\', altkeystring.size())+1;
                            }
#ifdef _DEBUG
                            Log("[%d] RegFixupSam: Check %LS\n", RegLocalInstance, widen(keypath.substr(OffsetHkcu)).c_str());
                            Log("[%d] RegFixupSam: using %LS\n", RegLocalInstance, pattern.c_str());
#endif
                            try
                            {
                                if (std::regex_match(widen(keypath.substr(OffsetHkcu)), std::wregex(pattern)))
                                {
#ifdef _DEBUG
                                    Log("[%d] RegFixupSam: is HKCU pattern match on type=0x%x.\n", RegLocalInstance, specitem.modifyKeyAccess.access);
#endif
                                    switch (specitem.modifyKeyAccess.access)
                                    {
                                    case Modify_Key_Access_Type_Full2RW:
                                        //(samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        if ((samDesired & (KEY_ALL_ACCESS)) == (KEY_ALL_ACCESS) || 
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_CREATE_SUB_KEY)) != 0)
                                        {
                                            //samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK);
                                            samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_CREATE_SUB_KEY);
#ifdef _DEBUG
                                            Log("[%d] RegFixupSam: Full2RW\n", RegLocalInstance);
#endif
                                        }
                                        break;
                                    case Modify_Key_Access_Type_Full2MaxAllowed:
                                        //(samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        if ((samDesired & (KEY_ALL_ACCESS)) == (KEY_ALL_ACCESS) ||
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER |  KEY_CREATE_LINK | KEY_CREATE_SUB_KEY)) != 0)
                                        {
                                            samModified = MAXIMUM_ALLOWED;
#ifdef _DEBUG
                                            Log("[%d] RegFixupSam: Full2MaxAllowed\n", RegLocalInstance);
#endif                                    
                                        }
                                        break;
                                    case Modify_Key_Access_Type_Full2R:
                                        if ((samDesired & (KEY_ALL_ACCESS)) == (KEY_ALL_ACCESS) ||
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK )) != 0 ||
                                            (samDesired & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_WRITE)) != 0)
                                        {
                                            samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_WRITE);
#ifdef _DEBUG
                                            Log("[%d] RegFixupSam: Full2R\n", RegLocalInstance);
#endif
                                        }
                                        break;
                                    case Modify_Key_Access_Type_RW2R:
                                        if ((samDesired & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY)) != 0 ||
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK )) != 0)
                                        {
                                            samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_WRITE);
#ifdef _DEBUG
                                            Log("[%d] RegFixupSam: RW2R\n", RegLocalInstance);
#endif
                                        }
                                        break;
                                    case Modify_Key_Access_Type_RW2MaxAllowed:
                                        //(samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        if ((samDesired & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY)) != 0 ||
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_CREATE_SUB_KEY)) != 0)
                                        {
                                            samModified = MAXIMUM_ALLOWED;
#ifdef _DEBUG
                                            Log("[%d] RegFixupSam: RW2MaxAllowed\n", RegLocalInstance);
#endif                                    
                                        }
                                        break;
                                    default:
                                        break;
                                    }
                                    return samModified;
                                }
                            }
                            catch (...)
                            {
                                Log("[%d] Bad Regex pattern ignored in RegLegacyFixups.\n", RegLocalInstance);
                            }
                        }
                    }
                    else
                    {
#ifdef _DEBUG
                        //Log("[%d] RegFixupSam: is not HKCU key?\n", RegLocalInstance);
#endif
                    }
                    break;
                case Modify_Key_Hive_Type_HKLM:
                    keystring = "HKEY_LOCAL_MACHINE\\";
                    altkeystring = "=\\REGISTRY\\MACHINE\\";
                    if (keypath._Starts_with(keystring) ||
                        keypath._Starts_with(altkeystring))
                    {
                        size_t OffsetHklm = keystring.size();
                        if (keypath._Starts_with(altkeystring))
                        {
                            // Must remove both the pattern and the S-1-5-...\ that follows.
                            OffsetHklm = keypath.find_first_of('\\', altkeystring.size()) + 1;
                        }
#ifdef _DEBUG
                        //Log("[%d] RegFixupSam:  is HKLM key\n", RegLocalInstance);
#endif
                        for (auto& pattern : specitem.modifyKeyAccess.patterns)
                        {
                            try
                            {
                                if (std::regex_match(widen(keypath.substr(OffsetHklm)), std::wregex(pattern)))
                                {
#ifdef _DEBUG
                                    Log("[%d] RegFixupSam: is HKLM pattern match on type=0x%x.\n", RegLocalInstance, specitem.modifyKeyAccess.access);
#endif
                                    switch (specitem.modifyKeyAccess.access)
                                    {
                                    case Modify_Key_Access_Type_Full2RW:
                                        //(samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        if ((samDesired & (KEY_ALL_ACCESS)) == (KEY_ALL_ACCESS) ||
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_CREATE_SUB_KEY)) != 0)
                                        {
                                            //samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK );
                                            samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_CREATE_SUB_KEY);
#ifdef _DEBUG
                                            Log("[%d] RegFixupSam: Full2RW\n", RegLocalInstance);
#endif
                                        }
                                        break;
                                    case Modify_Key_Access_Type_Full2R:
                                        if ((samDesired & (KEY_ALL_ACCESS)) == (KEY_ALL_ACCESS) ||
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK )) != 0 ||
                                            (samDesired & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_WRITE)) != 0)
                                        {
                                            samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_WRITE);
#ifdef _DEBUG
                                            Log("[%d] RegFixupSam: Full2R\n", RegLocalInstance);
#endif
                                        }
                                        break;
                                    case Modify_Key_Access_Type_Full2MaxAllowed:
                                        //(samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        if ((samDesired & (KEY_ALL_ACCESS)) == (KEY_ALL_ACCESS) ||
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK| KEY_CREATE_SUB_KEY)) != 0)
                                        {
                                            samModified = MAXIMUM_ALLOWED;
#ifdef _DEBUG
                                            Log("[%d] RegFixupSam: Full2MaxAllowed\n", RegLocalInstance);
#endif                                    
                                        }
                                        break;
                                    case Modify_Key_Access_Type_RW2R:
                                        //(samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        if ((samDesired & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY)) != 0 ||
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_CREATE_SUB_KEY)) != 0)
                                        {
                                            samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_WRITE);
#ifdef _DEBUG
                                            Log("[%d] RegFixupSam: RW2R\n", RegLocalInstance);
#endif
                                        }
                                        break;
                                    case Modify_Key_Access_Type_RW2MaxAllowed:
                                        //(samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        if ((samDesired & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY)) != 0 ||
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_CREATE_SUB_KEY)) != 0)
                                        {
                                            samModified = MAXIMUM_ALLOWED;
#ifdef _DEBUG
                                            Log("[%d] RegFixupSam: RW2MaxAllowed\n", RegLocalInstance);
#endif                                    
                                        }
                                        break;
                                    default:
                                        break;
                                    }
                                    return samModified;
                                }
                            }
                            catch (...)
                            {
                                Log("[%d] Bad Regex pattern ignored in RegLegacyFixups.\n", RegLocalInstance);
                            }
                        }
                    }
                    else
                    {
#ifdef _DEBUG
                        //Log("[%d] RegFixupSam: is not HKLM key?\n", RegLocalInstance);
#endif
                    }
                    break;
                case Modify_Key_Hive_Type_Unknown:
#ifdef _DEBUG
                    Log("[%d] RegFixupSam: is UNKNOWN type key?\n", RegLocalInstance);
#endif
                    break;
                default:
#ifdef _DEBUG
                    Log("[%d] RegFixupSam: is OTHER type key?\n", RegLocalInstance);
#endif                    
                    break;
                }
                break;
            default:
                // other rule type
                break;
            }
        }
    }
    return samModified;
}

#ifdef _DEBUG
bool RegFixupFakeDelete(std::string keypath,DWORD RegLocalInstance)
#else
bool RegFixupFakeDelete(std::string keypath)
#endif
{
#ifdef _DEBUG
    Log("[%d] RegFixupFakeDelete: path=%s\n", RegLocalInstance, keypath.c_str());
#endif
    std::string keystring;
    std::string altkeystring;
    for (auto& spec : g_regRemediationSpecs)
    {
      
        for (auto& specitem : spec.remediationRecords)
        {
#ifdef _DEBUG
            Log("[%d] RegFixupFakeDelete: specitem.type=%d\n", RegLocalInstance, specitem.remeditaionType);
#endif      
            if (specitem.remeditaionType == Reg_Remediation_type_FakeDelete)
            {
#ifdef _DEBUG
                Log("[%d] RegFixupFakeDelete: specitem.type=%d\n", RegLocalInstance, specitem.remeditaionType);
#endif 
                switch (specitem.fakeDeleteKey.hive)
                {
                case Modify_Key_Hive_Type_HKCU:                
                    keystring = "HKEY_CURRENT_USER\\";
                    altkeystring = "=\\REGISTRY\\USER\\";
                    if (keypath._Starts_with(keystring) ||
                        keypath._Starts_with(altkeystring))
                    {
                        size_t OffsetHkcu = keystring.size();
                        if (keypath._Starts_with(altkeystring))
                        {
                            // Must remove both the pattern and the S-1-5-...\ that follows.
                            OffsetHkcu = keypath.find_first_of('\\', altkeystring.size()) + 1;
                        }
                        for (auto& pattern : specitem.fakeDeleteKey.patterns)
                        {
                            try
                            {
                                if (std::regex_match(widen(keypath.substr(OffsetHkcu)), std::wregex(pattern)))
                                {
#ifdef _DEBUG
                                    Log("[%d] RegFixupFakeDelete: match hkcu\n", RegLocalInstance);
#endif                            
                                    return true;
                                }
                            }
                            catch (...)
                            {
#ifdef _DEBUG
                                Log("[%d] Bad Regex pattern ignored in RegLegacyFixups.\n", RegLocalInstance);
#endif
                            }
                        }
                    }
                    break;
                case Modify_Key_Hive_Type_HKLM:
                    keystring = "HKEY_LOCAL_MACHINE\\";
                    altkeystring = "=\\REGISTRY\\MACHINE\\";
                    if (keypath._Starts_with(keystring) ||
                        keypath._Starts_with(altkeystring))
                    {
                        size_t OffsetHklm = keystring.size();
                        if (keypath._Starts_with(altkeystring))
                        {
                            // Must remove both the pattern and the S-1-5-...\ that follows.
                            OffsetHklm = keypath.find_first_of('\\', altkeystring.size()) + 1;
                        }
                        for (auto& pattern : specitem.fakeDeleteKey.patterns)
                        {
                            try
                            {
                                if (std::regex_match(widen(keypath.substr(OffsetHklm)), std::wregex(pattern)))
                                {
#ifdef _DEBUG
                                    Log("[%d] RegFixupFakeDelete: match hklm\n", RegLocalInstance);
#endif                            
                                    return true;
                                }
                            }
                            catch (...)
                            {
#ifdef _DEBUG
                                Log("[%d] Bad Regex pattern ignored in RegLegacyFixups.\n", RegLocalInstance);
#endif
                            }

                        }
                    }
                    break;
                }
            }
        }
    }
    return false;
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
#if _DEBUG
    if constexpr (psf::is_ansi<CharT>)
    {
        Log("[%d] RegCreateKeyEx: key=0x%x subkey=%s", RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey);
    }
    else
    {
        Log("[%d] RegCreateKeyEx: key=0x%x subkey=%ls", RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey);
    }
#endif

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

#if _DEBUG
    if constexpr (psf::is_ansi<CharT>)
    {
        Log("[%d] RegOpenKeyEx: key=0x%x subkey=%s", RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey);
    }
    else
    {
        Log("[%d] RegOpenKeyEx: key=0x%x subkey=%ls", RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey);
    }
#endif

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

auto RegDeleteKeyImpl = psf::detoured_string_function(&::RegDeleteKeyA, &::RegDeleteKeyW);
template <typename CharT>
LSTATUS __stdcall RegDeleteKeyFixup(
    _In_ HKEY key,
    _In_ const CharT* subKey)
{

    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    DWORD RegLocalInstance = ++g_RegIntceptInstance;
    auto entry = LogFunctionEntry();


    auto result = RegDeleteKeyImpl(key, subKey);
    auto functionResult = from_win32(result);
    QueryPerformanceCounter(&TickEnd);

    if (functionResult != from_win32(0))
    {
        if (auto lock = acquire_output_lock(function_type::registry, functionResult))
        {
            try
            {
#if _DEBUG
                Log("[%d] RegDeleteKey:\n", RegLocalInstance);
#endif
                std::string keypath = ReplaceRegistrySyntax( InterpretKeyPath(key) + "\\" + InterpretStringA(subKey));

#ifdef _DEBUG
                Log("[%d] RegDeleteKey: Path=%s", RegLocalInstance, keypath.c_str());
                if (RegFixupFakeDelete(keypath, RegLocalInstance) == true)
#else
                if (RegFixupFakeDelete(keypath) == true)
#endif
                {
#if _DEBUG
                    Log("[%d] RegDeleteKey:Fake Success\n", RegLocalInstance);
#endif
                    result = 0;
                    LogCallingModule();
                }
            }
            catch  (...)
            {
                Log("[%d] RegDeleteKey logging failure.\n", RegLocalInstance);
            }
        }
    }
#if _DEBUG
    Log("[%d] RegDeleteKey:Fake returns %d\n", RegLocalInstance, result);
#endif
    return result;
}
DECLARE_STRING_FIXUP(RegDeleteKeyImpl, RegDeleteKeyFixup);


auto RegDeleteKeyExImpl = psf::detoured_string_function(&::RegDeleteKeyExA, &::RegDeleteKeyExW);
template <typename CharT>
LSTATUS __stdcall RegDeleteKeyExFixup(
    _In_ HKEY key,
    _In_ const CharT* subKey,
    DWORD viewDesired, // 32/64
    DWORD Reserved)
{

    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    DWORD RegLocalInstance = ++g_RegIntceptInstance;
    auto entry = LogFunctionEntry();


    auto result = RegDeleteKeyExImpl(key, subKey, viewDesired, Reserved);
    auto functionResult = from_win32(result);
    QueryPerformanceCounter(&TickEnd);

    if (functionResult != from_win32(0))
    {
        if (auto lock = acquire_output_lock(function_type::registry, functionResult))
        {
            try
            {
#if _DEBUG
                Log("[%d] RegDeleteKeyEx:\n", RegLocalInstance);
#endif
                std::string keypath = ReplaceRegistrySyntax(InterpretKeyPath(key) + "\\" + InterpretStringA(subKey));
#ifdef _DEBUG
                Log("[%d] RegDeleteKeyEx: Path=%s", RegLocalInstance, keypath.c_str());
                if (RegFixupFakeDelete(keypath, RegLocalInstance) == true)
#else
                if (RegFixupFakeDelete(keypath) == true)
#endif
                {
#if _DEBUG
                    Log("[%d] RegDeleteKeyEx:Fake Success\n", RegLocalInstance);
#endif
                    result = 0;
                    LogCallingModule();
                }
            }
            catch (...)
            {
                Log("[%d] RegDeleteKeyEx logging failure.\n", RegLocalInstance);
            }
        }
    }
#if _DEBUG
    Log("[%d] RegDeleteKeyEx:Fake returns %d\n", RegLocalInstance, result);
#endif
    return result;
}
DECLARE_STRING_FIXUP(RegDeleteKeyExImpl, RegDeleteKeyExFixup);

auto RegDeleteKeyTransactedImpl = psf::detoured_string_function(&::RegDeleteKeyTransactedA, &::RegDeleteKeyTransactedW);
template <typename CharT>
LSTATUS __stdcall RegDeleteKeyTransactedFixup(
    _In_ HKEY key,
    _In_ const CharT* subKey,
    DWORD viewDesired, // 32/64
    DWORD Reserved,
    HANDLE hTransaction,
    PVOID  pExtendedParameter)
{

    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    DWORD RegLocalInstance = ++g_RegIntceptInstance;
    auto entry = LogFunctionEntry();


    auto result = RegDeleteKeyTransactedImpl(key, subKey, viewDesired, Reserved, hTransaction, pExtendedParameter);
    auto functionResult = from_win32(result);
    QueryPerformanceCounter(&TickEnd);

    if (functionResult != from_win32(0))
    {
        if (auto lock = acquire_output_lock(function_type::registry, functionResult))
        {
            try
            {
#if _DEBUG
                Log("[%d] RegDeleteKeyTransacted:\n", RegLocalInstance);
#endif
                std::string keypath = ReplaceRegistrySyntax(InterpretKeyPath(key) + "\\" + InterpretStringA(subKey));
#ifdef _DEBUG
                Log("[%d] RegDeleteKeyTransacted: Path=%s", RegLocalInstance, keypath.c_str());
                if (RegFixupFakeDelete(keypath, RegLocalInstance) == true)
#else
                if (RegFixupFakeDelete(keypath) == true)
#endif
                {
#if _DEBUG
                    Log("[%d] RegDeleteKeyTransacted:Fake Success\n", RegLocalInstance);
#endif
                    result = 0;
                    LogCallingModule();
                }
            }
            catch (...)
            {
                Log("[%d] RegDeleteKeyTransacted logging failure.\n", RegLocalInstance);
            }
        }
    }
#if _DEBUG
    Log("[%d] RegDeleteKeyTransacted:Fake returns %d\n", RegLocalInstance, result);
#endif
    return result;
}
DECLARE_STRING_FIXUP(RegDeleteKeyTransactedImpl, RegDeleteKeyTransactedFixup);


auto RegDeleteValueImpl = psf::detoured_string_function(&::RegDeleteValueA, &::RegDeleteValueW);
template <typename CharT>
LSTATUS __stdcall RegDeleteValueFixup(
    _In_ HKEY key,
    _In_ const CharT* subValueName)
{

    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    DWORD RegLocalInstance = ++g_RegIntceptInstance;
    auto entry = LogFunctionEntry();


    auto result = RegDeleteValueImpl(key, subValueName);
    auto functionResult = from_win32(result);
    QueryPerformanceCounter(&TickEnd);

    if (functionResult != from_win32(0))
    {
        if (auto lock = acquire_output_lock(function_type::registry, functionResult))
        {
            try
            {
#if _DEBUG
                Log("[%d] RegDeleteValue:\n", RegLocalInstance);
#endif
                std::string keypath = ReplaceRegistrySyntax(InterpretKeyPath(key) + "\\" + InterpretStringA(subValueName));
#ifdef _DEBUG
                Log("[%d] RegDeleteValue: Path=%s", RegLocalInstance, keypath.c_str());
                if (RegFixupFakeDelete(keypath, RegLocalInstance) == true)
#else
                if (RegFixupFakeDelete(keypath) == true)
#endif
                {
#if _DEBUG
                    Log("[%d] RegDeleteValue:Fake Success\n", RegLocalInstance);
#endif
                    result = 0;
                    LogCallingModule();
                }
            }
            catch (...)
            {
                Log("[%d] RegDeleteValue logging failure.\n", RegLocalInstance);
            }
        }
    }
#if _DEBUG
    Log("[%d] RegDeleteValue:Fake returns %d\n", RegLocalInstance, result);
#endif
    return result;
}
DECLARE_STRING_FIXUP(RegDeleteValueImpl, RegDeleteValueFixup);

