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
#include "psf_tracelogging.h"
#include "pch.h"


DWORD g_RegIntceptInstance = 0;
std::unordered_set<std::string> g_regRedirectedHivePaths;
std::vector<std::wstring> g_regCreatedKeysOrdered;


enum Action {
    Continue,
    ReturnTrue,
    ReturnFalse
};

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
            size_t offsetAfterMachine = regPath.find('\\', 18);
            if (offsetAfterMachine != std::string::npos)
            {
                returnPath = InterpretStringA("HKEY_LOCAL_MACHINE") + regPath.substr(offsetAfterMachine);
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
    keypath = ReplaceRegistrySyntax(keypath);
    REGSAM samModified = samDesired;
    std::string keystring;


    Log("[%d] RegFixupSam: path=%s\n", RegLocalInstance, keypath.c_str());
    for (auto& spec : g_regRemediationSpecs)
    {
#ifdef _DEBUG
        Log("[%d] RegFixupSam: spec\n", RegLocalInstance);
#endif        
        for (auto& specitem: spec.remediationRecords)
        {
#ifdef _DEBUG
            Log("[%d] RegFixupSam: specitem.type=%d\n", RegLocalInstance, specitem.remeditaionType);
#endif   
            switch (specitem.remeditaionType)
            {
            case Reg_Remediation_Type_ModifyKeyAccess:
#ifdef _DEBUG
                Log("[%d] RegFixupSam: is Check ModifyKeyAccess...\n", RegLocalInstance);
#endif
                switch (specitem.modifyKeyAccess.hive)
                {
                case Modify_Key_Hive_Type_HKCU:
                    keystring = "HKEY_CURRENT_USER\\";
                    if (keypath._Starts_with(keystring))
                    {
#ifdef _DEBUG
                        Log("[%d] RegFixupSam: is HKCU key\n", RegLocalInstance);
#endif
                        for (auto& pattern : specitem.modifyKeyAccess.patterns)
                        {
#ifdef _DEBUG
                            Log("[%d] RegFixupSam: Check %LS\n", RegLocalInstance, widen(keypath.substr(keystring.size())).c_str());
                            Log("[%d] RegFixupSam: using %LS\n", RegLocalInstance, pattern.c_str());
#endif
                            try
                            {
                                if (std::regex_match(widen(keypath.substr(keystring.size())), std::wregex(pattern)))
                                {
#ifdef _DEBUG
                                    Log("[%d] RegFixupSam: is HKCU pattern match.\n", RegLocalInstance);
#endif
                                    switch (specitem.modifyKeyAccess.access)
                                    {
                                    case Modify_Key_Access_Type_Full2RW:
                                        if ((samDesired & (KEY_ALL_ACCESS | KEY_CREATE_LINK)) != 0)
                                        {
                                            samModified = samDesired & ~(DELETE | KEY_CREATE_LINK);
#ifdef _DEBUG
                                            Log("[%d] RegFixupSam: Full2RW\n", RegLocalInstance);
#endif
                                        }
                                        break;
                                    case Modify_Key_Access_Type_Full2MaxAllowed:
                                        if ((samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_CREATE_LINK)) != 0)
                                        {
                                            samModified = MAXIMUM_ALLOWED;
#ifdef _DEBUG
                                            Log("[%d] RegFixupSam: Full2MaxAllowed\n", RegLocalInstance);
#endif                                    
                                        }
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
                                    case Modify_Key_Access_Type_RW2MaxAllowed:
                                        if ((samDesired & (KEY_CREATE_LINK | KEY_CREATE_SUB_KEY | KEY_SET_VALUE | WRITE_DAC | WRITE_OWNER)) != 0)
                                        {
                                            samModified = MAXIMUM_ALLOWED;
#ifdef _DEBUG
                                            Log("[%d] RegFixupSam: RW2MaxAllowed\n", RegLocalInstance);
#endif                                    
                                        }
                                    default:
                                        break;
                                    }
                                    return samModified;
                                }
                            }
                            catch (...)
                            {
                                psf::TraceLogExceptions("RegLegacyFixupException", "Bad Regex pattern ignored in RegLegacyFixups. Hive: HKCU");
                                Log("[%d] Bad Regex pattern ignored in RegLegacyFixups.\n", RegLocalInstance);
                            }
                        }
                    }
                    break;
                case Modify_Key_Hive_Type_HKLM:
                    keystring = "HKEY_LOCAL_MACHINE\\";
                    if (keypath._Starts_with(keystring))
                    {
#ifdef _DEBUG
                        Log("[%d] RegFixupSam: is HKLM key\n", RegLocalInstance);
#endif
                        for (auto& pattern : specitem.modifyKeyAccess.patterns)
                        {
                            try
                            {
                                if (std::regex_match(widen(keypath.substr(keystring.size())), std::wregex(pattern)))
                                {
#ifdef _DEBUG
                                    Log("[%d] RegFixupSam: HKLM pattern match.\n", RegLocalInstance);
#endif
                                    switch (specitem.modifyKeyAccess.access)
                                    {
                                    case Modify_Key_Access_Type_Full2RW:
                                        if ((samDesired & (KEY_ALL_ACCESS | KEY_CREATE_LINK)) != 0)
                                        {
                                            samModified = samDesired & ~(DELETE | KEY_CREATE_LINK);
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
                                    case Modify_Key_Access_Type_Full2MaxAllowed:
                                        if ((samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_CREATE_LINK)) != 0)
                                        {
                                            samModified = MAXIMUM_ALLOWED;
#ifdef _DEBUG
                                            Log("[%d] RegFixupSam: Full2MaxAllowed\n", RegLocalInstance);
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
                                    case Modify_Key_Access_Type_RW2MaxAllowed:
                                        if ((samDesired & (KEY_CREATE_LINK | KEY_CREATE_SUB_KEY | KEY_SET_VALUE | WRITE_DAC | WRITE_OWNER)) != 0)
                                        {
                                            samModified = MAXIMUM_ALLOWED;
#ifdef _DEBUG
                                            Log("[%d] RegFixupSam: RW2MaxAllowed\n", RegLocalInstance);
#endif                                    
                                        }
                                    default:
                                        break;
                                    }
                                    return samModified;
                                }
                            }
                            catch (...)
                            {
                                psf::TraceLogExceptions("RegLegacyFixupException", "Bad Regex pattern ignored in RegLegacyFixups. Hive: HKLM");
                                Log("[%d] Bad Regex pattern ignored in RegLegacyFixups.\n", RegLocalInstance);
                            }
                        }
                    }
                    break;
                case Modify_Key_Hive_Type_Unknown:
                default:
                    break;
                }
            }
        }
    }
    return samModified;
}

template <typename CharT>
const char* ReplaceRegistryQueryPath(PHKEY key, const CharT* subKey)
{
    std::string keypath = InterpretKeyPath(*key) + "\\" + InterpretStringA(subKey);

    std::string normalizedKeypath = ReplaceRegistrySyntax(keypath);
    size_t subkeyOffset = normalizedKeypath.find_first_of('\\');
    if (subkeyOffset != std::wstring_view::npos)
    {
        std::string requestedSubkey = normalizedKeypath.substr(subkeyOffset + 1);
        auto it = g_regRedirectedHivePaths.find(requestedSubkey);
        if (it != g_regRedirectedHivePaths.end())
        {
            *key = HKEY_CURRENT_USER;
            return it->c_str();
        }
    }

    return nullptr;
}


#ifdef _DEBUG
bool RegFixupFakeDelete(std::string keypath,DWORD RegLocalInstance)
#else
bool RegFixupFakeDelete(std::string keypath)
#endif
{
    keypath = ReplaceRegistrySyntax(keypath);
#ifdef _DEBUG
    Log("[%d] RegFixupFakeDelete: path=%s\n", RegLocalInstance, keypath.c_str());
#endif
    std::string keystring;
    for (auto& spec : g_regRemediationSpecs)
    {
#ifdef _DEBUG
        Log("[%d] RegFixupFakeDelete: spec\n", RegLocalInstance);
#endif        
        for (auto& specitem : spec.remediationRecords)
        {
#ifdef _DEBUG
            Log("[%d] RegFixupFakeDelete: specitem.type=%d\n", RegLocalInstance, specitem.remeditaionType);
#endif        
            switch (specitem.fakeDeleteKey.hive)
            {
            case Modify_Key_Hive_Type_HKCU:
                keystring = "HKEY_CURRENT_USER\\";
                if (keypath._Starts_with(keystring))
                {
                    for (auto& pattern : specitem.fakeDeleteKey.patterns)
                    {
                        try
                        {
                            if (std::regex_match(widen(keypath.substr(keystring.size())), std::wregex(pattern)))
                            {
#ifdef _DEBUG
                                Log("[%d] RegFixupFakeDelete: match hkcu\n", RegLocalInstance);
#endif                            
                                return true;
                            }
                        }
                        catch (...)
                        {
                            psf::TraceLogExceptions("RegLegacyFixupException", "RegFixupFakeDelete: Bad Regex pattern ignored in RegLegacyFixups. Hive: HKCU");
#ifdef _DEBUG
                            Log("[%d] Bad Regex pattern ignored in RegLegacyFixups.\n", RegLocalInstance);
#endif
                        }
                    }
                }
                break;
            case Modify_Key_Hive_Type_HKLM:
                keystring = "HKEY_LOCAL_MACHINE\\";
                if (keypath._Starts_with(keystring))
                {
                    for (auto& pattern : specitem.fakeDeleteKey.patterns)
                    {
                        try
                        {
                            if (std::regex_match(widen(keypath.substr(keystring.size())), std::wregex(pattern)))
                            {
#ifdef _DEBUG
                                Log("[%d] RegFixupFakeDelete: match hklm\n", RegLocalInstance);
#endif                            
                                return true;
                            }
                        }
                        catch (...)
                        {
                            psf::TraceLogExceptions("RegLegacyFixupException", "RegFixupFakeDelete: Bad Regex pattern ignored in RegLegacyFixups. Hive: HKLM");
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
    return false;
}



/// <summary>
/// Helper method for RegFixupDeletionMarker
/// </summary>
template <typename CharT>
#ifdef _DEBUG
Action RegFixupDeletionMarkerHelper(Reg_Remediation_Record &specitem, std::string keyPath, const CharT* keyValue, DWORD RegLocalInstance,  std::string keystring, const char* hive)
#else
Action RegFixupDeletionMarkerHelper(Reg_Remediation_Record& specitem, std::string keyPath, const CharT* keyValue, std::string keystring)
#endif
{
    if (keyPath._Starts_with(keystring))
    {
#ifdef _DEBUG
        Log("[%d] RegFixupDeletionMarker: is %s hive\n", RegLocalInstance, hive);
#endif
        try
        {
#ifdef _DEBUG
            Log("[%d] RegFixupDeletionMarker: key: %LS\n", RegLocalInstance, specitem.deletionMarker.key.c_str());
#endif
            if (std::regex_match(widen(keyPath.substr(keystring.size())), std::wregex(specitem.deletionMarker.key)))
            {
#ifdef _DEBUG
                Log("[%d] RegFixupDeletionMarker: is %s key match.\n", RegLocalInstance, hive);
#endif
                if (specitem.deletionMarker.values.empty())
                {
                    // Deletion Marker for Key Found
#ifdef _DEBUG
                    Log("[%d] RegFixupDeletionMarker: No Values\n", RegLocalInstance);
#endif
                    return ReturnTrue;
                }
                else if (keyValue == NULL)
                {
                    return ReturnFalse;
                }
                else
                {
                    // Check for Deletion Marker for Value
                    for (auto& value : specitem.deletionMarker.values)
                    {
#ifdef _DEBUG
                        Log("[%d] RegFixupDeletionMarker: value: %LS\n", RegLocalInstance, value.c_str());
#endif
                        if (std::regex_match(widen(keyValue), std::wregex(value)))
                        {
                            //Deletion Marker for Value Found
#ifdef _DEBUG
                            Log("[%d] RegFixupDeletionMarker: is %s key-value match.\n", RegLocalInstance, hive);
                            Log("[%d] RegFixupDeletionMarker: Deletion-Marker true.\n", RegLocalInstance);
#endif
                            return ReturnTrue;
                        }
                    }
                }
            }
        }
        catch (...)
        {
            psf::TraceLogExceptions("RegLegacyFixupException", "Bad Regex pattern ignored in RegLegacyFixups.");
#ifdef _DEBUG
            Log("[%d] Bad Regex pattern ignored in RegLegacyFixups.\n", RegLocalInstance);
#endif
        }
    }
    return Continue;
}

/// <summary>
/// Method to check if given keyValue is a deletion-marker
/// </summary>
/// <param name="keypath"></param>
/// <param name="keyValue"></param>
/// <param name="RegLocalInstance"></param>
/// <returns></returns>

template <typename CharT>
#ifdef _DEBUG
bool RegFixupDeletionMarker(std::string keyPath, const CharT* keyValue, DWORD RegLocalInstance)
#else
bool RegFixupDeletionMarker(std::string keyPath, const CharT* keyValue)
#endif 
{
    std::string keystring;
#ifdef _DEBUG
    Log("[%d] RegFixupDeletionMarker: keyPath=%s keyValue=%s \n", RegLocalInstance, keyPath.c_str(), keyValue);
#endif  

    for (auto& spec : g_regRemediationSpecs)
    {
#ifdef _DEBUG
        Log("[%d] RegFixupDeletionMarker: spec\n", RegLocalInstance);
#endif        
        for (auto& specitem : spec.remediationRecords)
        {
#ifdef _DEBUG
            Log("[%d] RegFixupDeletionMarker: specitem.type=%d\n", RegLocalInstance, specitem.remeditaionType);
#endif   
            Action result = Continue;
            switch (specitem.remeditaionType)
            {
            case Reg_Remediation_type_DeletionMarker:
#ifdef _DEBUG
                Log("[%d] RegFixupDeletionMarker: is Check DeletionMarker...\n", RegLocalInstance);
#endif  
                switch (specitem.deletionMarker.hive)
                {  
                case Modify_Key_Hive_Type_HKCU:
                    keystring = "HKEY_CURRENT_USER\\";
#ifdef _DEBUG
                    result = RegFixupDeletionMarkerHelper(specitem, keyPath, keyValue, RegLocalInstance, keystring, "HKCU");
#else
                    result = RegFixupDeletionMarkerHelper(specitem, keyPath, keyValue, keystring);
#endif
                    break;
                case Modify_Key_Hive_Type_HKLM:
                    keystring = "HKEY_LOCAL_MACHINE\\";
#ifdef _DEBUG
                    result = RegFixupDeletionMarkerHelper(specitem, keyPath, keyValue, RegLocalInstance, keystring, "HKLM");
#else
                    result = RegFixupDeletionMarkerHelper(specitem, keyPath, keyValue, keystring);
#endif
                    break;
                default:
                    break;
                }

                if (result == ReturnTrue) {
                    return true;
                }
                else if (result == ReturnFalse) {
                    return false;
                }

            default:
                break;
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
    const char* updatedSubKey = ReplaceRegistryQueryPath(&key, subKey);
    if (updatedSubKey)
    {
        return RegOpenKeyExImpl(key, updatedSubKey, options, samDesired, resultKey);
    }

    std::string keypath = InterpretKeyPath(key) + "\\" + InterpretStringA(subKey);

    std::string registryPath = ReplaceRegistrySyntax(keypath);

#ifdef _DEBUG
    Log("[%d] RegOpenKeyEx: Path=%s", RegLocalInstance, registryPath.c_str());
    if (RegFixupDeletionMarker<char>(registryPath, NULL, RegLocalInstance) == true)
#else
    if (RegFixupDeletionMarker<char>(registryPath, NULL) == true)
#endif
    {
#ifdef _DEBUG
        Log("[%d] RegOpenKeyEx: Key Deletion Marker", RegLocalInstance);
        Log("[%d] RegOpenKeyEx: return = ERROR_FILE_NOT_FOUND", RegLocalInstance);
#endif
        return ERROR_FILE_NOT_FOUND;
    }

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

    const char* updatedSubKey = ReplaceRegistryQueryPath(&key, subKey);
    if (updatedSubKey)
    {
        return RegOpenKeyTransactedImpl(key, updatedSubKey, options, samDesired, resultKey, hTransaction, pExtendedParameter);
    }

    std::string keypath = InterpretKeyPath(key) + "\\" + InterpretStringA(subKey);

    std::string registryPath = ReplaceRegistrySyntax(keypath);

#ifdef _DEBUG
    Log("[%d] RegOpenKeyTransacted: Path=%s", RegLocalInstance, registryPath.c_str());
    if (RegFixupDeletionMarker<char>(registryPath,NULL, RegLocalInstance) == true)
#else
    if (RegFixupDeletionMarker<char>(registryPath, NULL) == true)
#endif
    {
#ifdef _DEBUG
        Log("[%d] RegOpenKeyTransacted: Key Deletion Marker", RegLocalInstance);
        Log("[%d] RegOpenKeyTransacted: return = ERROR_FILE_NOT_FOUND", RegLocalInstance);
#endif
        return ERROR_FILE_NOT_FOUND;
    }

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



/// <summary>
/// detour RegEnumKeyExA & RegEnumKeyExW
/// for deletion marker fixup
/// </summary>
auto RegEnumKeyExImpl = psf::detoured_string_function(&::RegEnumKeyExA, &::RegEnumKeyExW);
template <typename CharT>
LSTATUS __stdcall RegEnumKeyExFixup(
    _In_ HKEY hKey,
    _In_ DWORD dwIndex,
    _Out_writes_to_opt_(*lpcchName, *lpcchName + 1) CharT* lpName,
    _Inout_ LPDWORD lpcchName,
    _Reserved_ LPDWORD lpReserved,
    _Out_writes_to_opt_(*lpcchClass, *lpcchClass + 1) CharT* lpClass,
    _Inout_opt_ LPDWORD lpcchClass,
    _Out_opt_ PFILETIME lpftLastWriteTime
)
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    DWORD RegLocalInstance = ++g_RegIntceptInstance;
    auto entry = LogFunctionEntry();
    auto response = ERROR_NO_MORE_ITEMS;

    try
    {
#ifdef _DEBUG
        Log("[%d] RegEnumKeyEx:\n", RegLocalInstance);
#endif
        DWORD Index = 0;
        DWORD CountDeletionMarkerKeys = 0;

        do
        {
            CharT KeyName[INT16_MAX];
            DWORD lpdKeyName = INT16_MAX;
            response = RegEnumKeyExImpl(hKey, Index, KeyName, &lpdKeyName, nullptr, nullptr, nullptr, nullptr);

            if (response == ERROR_SUCCESS)
            {
                //Get Registry Path from hkey
                std::string keyPath = InterpretKeyPath(hKey) + "\\" + InterpretStringA(KeyName);
                keyPath = ReplaceRegistrySyntax(keyPath);
#ifdef _DEBUG
                Log("[%d] RegEnumKeyEx: path=%s", RegLocalInstance, keyPath.c_str());
                if (RegFixupDeletionMarker<char>(keyPath, NULL, RegLocalInstance) == true)
#else
                if (RegFixupDeletionMarker<char>(keyPath, NULL) == true)
#endif
                {
#ifdef _DEBUG
                    Log("[%d] RegEnumKeyEx:Deletion Marker Success\n", RegLocalInstance);
#endif 
                    CountDeletionMarkerKeys++;
                }

                if (Index - CountDeletionMarkerKeys == dwIndex)
                {
#ifdef _DEBUG
                    Log("[%d] RegEnumKeyEx:Index Found\n", RegLocalInstance);
#endif 
                    return RegEnumKeyExImpl(hKey, Index, lpName, lpcchName, lpReserved, lpClass, lpcchClass, lpftLastWriteTime);
                }
            }
            else
            {
#ifdef _DEBUG
                Log("[%d] RegEnumKeyEx: function failure\n", RegLocalInstance);
#endif 
                return response;
            }

            Index++;
        } while (response == ERROR_SUCCESS);
    }
    catch (...)
    {
        Log("[%d] RegEnumKeyEx logging failure.\n", RegLocalInstance);
    }
    QueryPerformanceCounter(&TickEnd);
#if _DEBUG
    Log("[%d] RegEnumKeyEx: returns %d\n", RegLocalInstance, response);
#endif
    return response;
}
DECLARE_STRING_FIXUP(RegEnumKeyExImpl, RegEnumKeyExFixup);



/// <summary>
/// detour RegEnumValueA & RegEnumValueW
/// for deletion marker fixup
/// </summary>
auto RegEnumValueImpl = psf::detoured_string_function(&::RegEnumValueA, &::RegEnumValueW);
template <typename CharT>
LSTATUS __stdcall RegEnumValueFixup(
    _In_ HKEY hKey,
    _In_ DWORD dwIndex,
    _Out_writes_to_opt_(*lpcchValueName, *lpcchValueName + 1)  CharT* lpValueName,
    _Inout_ LPDWORD lpcchValueName,
    _Reserved_ LPDWORD lpReserved,
    _Out_opt_ LPDWORD lpType,
    _Out_writes_bytes_to_opt_(*lpcbData, *lpcbData) __out_data_source(REGISTRY) LPBYTE lpData,
    _Inout_opt_ LPDWORD lpcbData
)
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    DWORD RegLocalInstance = ++g_RegIntceptInstance;
    auto entry = LogFunctionEntry();
    auto response = ERROR_NO_MORE_ITEMS;

    try
    {
#ifdef _DEBUG
        Log("[%d] RegEnumValue:\n", RegLocalInstance);
#endif
        DWORD Index = 0;
        DWORD CountDeletionMarkerValues = 0;
        
        do
        {
            CharT ValueName[INT16_MAX];
            DWORD lpdValueName = INT16_MAX;
            response = RegEnumValueImpl(hKey, Index, ValueName, &lpdValueName, nullptr, nullptr, nullptr, nullptr);
            
            if (response == ERROR_SUCCESS)
            {
                //Get Registry Path from hkey
                std::string keyPath = InterpretKeyPath(hKey);
                keyPath = ReplaceRegistrySyntax(keyPath);
#ifdef _DEBUG
                Log("[%d] RegEnumValue: path=%s", RegLocalInstance, keyPath.c_str());
                if (RegFixupDeletionMarker(keyPath, ValueName, RegLocalInstance) == true)
#else
                if (RegFixupDeletionMarker(keyPath, ValueName) == true)
#endif
                {
#ifdef _DEBUG
                    Log("[%d] RegEnumValue:Deletion Marker Success\n", RegLocalInstance);
#endif 
                    CountDeletionMarkerValues++;
                }

                if (Index - CountDeletionMarkerValues == dwIndex)
                {
#ifdef _DEBUG
                    Log("[%d] RegEnumValue:Index Found\n", RegLocalInstance);
#endif 
                    return RegEnumValueImpl(hKey, Index, lpValueName, lpcchValueName, lpReserved, lpType, lpData, lpcbData);
                }
            }
            else
            {
#ifdef _DEBUG
                Log("[%d] RegEnumValue: function failure\n", RegLocalInstance);
#endif 
                return response;
            }
            
            Index++;
        } while (response == ERROR_SUCCESS);
    }
    catch (...)
    {
        Log("[%d] RegEnumValue logging failure.\n", RegLocalInstance);
    }
    QueryPerformanceCounter(&TickEnd);
#if _DEBUG
    Log("[%d] RegEnumValue: returns %d\n", RegLocalInstance, response);
#endif
    return response;
}
DECLARE_STRING_FIXUP(RegEnumValueImpl, RegEnumValueFixup);


/// <summary>
/// detour RegQueryInfoKeyA & RegQueryInfoKeyW
/// for deletion marker fixup
/// </summary>
auto RegQueryInfoKeyImpl = psf::detoured_string_function(&::RegQueryInfoKeyA, &::RegQueryInfoKeyW);
template <typename CharT>
LSTATUS __stdcall RegQueryInfoKeyFixup(
    _In_ HKEY hKey,
    _Out_writes_to_opt_(*lpcchClass, *lpcchClass + 1) CharT* lpClass,
    _Inout_opt_ LPDWORD lpcchClass,
    _Reserved_ LPDWORD lpReserved,
    _Out_opt_ LPDWORD lpcSubKeys,
    _Out_opt_ LPDWORD lpcbMaxSubKeyLen,
    _Out_opt_ LPDWORD lpcbMaxClassLen,
    _Out_opt_ LPDWORD lpcValues,
    _Out_opt_ LPDWORD lpcbMaxValueNameLen,
    _Out_opt_ LPDWORD lpcbMaxValueLen,
    _Out_opt_ LPDWORD lpcbSecurityDescriptor,
    _Out_opt_ PFILETIME lpftLastWriteTime
)
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();

#if _DEBUG
    DWORD RegLocalInstance = ++g_RegIntceptInstance;
    Log("[%d] RegQueryInfoKey:\n", RegLocalInstance);
#endif
    auto result = RegQueryInfoKeyImpl(hKey, lpClass, lpcchClass, lpReserved, lpcSubKeys, lpcbMaxSubKeyLen, lpcbMaxClassLen, lpcValues, lpcbMaxValueNameLen, lpcbMaxValueLen, lpcbSecurityDescriptor, lpftLastWriteTime);

    if (result == ERROR_SUCCESS)
    {
        // Modify lpcValues & lpcbMaxValueNameLen , lpcbMaxValueLen
        // Since Deletion Markers for values may be present
        try
        {
            DWORD Index = 0;
            DWORD CountValues = 0;
            DWORD MaxValueNameLen = 0;
            DWORD MaxDataLen = 0;
            auto response = ERROR_SUCCESS;
            
            while (response == ERROR_SUCCESS)
            {
                CharT ValueName[INT16_MAX];
                DWORD dValueName = INT16_MAX;
                LPBYTE lpData = new BYTE[INT16_MAX];
                DWORD dData = INT16_MAX;

                if constexpr (psf::is_ansi<CharT>)
                {
                    response = RegEnumValueA(hKey, Index, ValueName, &dValueName, NULL, NULL, lpData, &dData);
                }
                else
                {
                    response = RegEnumValueW(hKey, Index, ValueName, &dValueName, NULL, NULL, lpData, &dData);
                }

                if (response == ERROR_SUCCESS)
                {
                    CountValues++;

                    MaxValueNameLen = max(MaxValueNameLen, dValueName);

                    //Remove size of null character from dData
                    if constexpr (psf::is_ansi<CharT>)
                    {
                        dData = dData - sizeof(CHAR);
                    }
                    else
                    {
                        dData = dData - sizeof(WCHAR);
                    }

                    MaxDataLen = max(MaxDataLen, dData);
                }
                else if (response == ERROR_NO_MORE_ITEMS)
                {
                    // Update lpcValues, lpcbMaxValueNameLen, lpcbMaxValueLen
                    if (lpcValues != nullptr)
                        *lpcValues = CountValues;
                    if (lpcbMaxValueNameLen != nullptr)
                        *lpcbMaxValueNameLen = MaxValueNameLen;
                    if (lpcbMaxValueLen != nullptr)
                        *lpcbMaxValueLen = MaxDataLen;
#if _DEBUG
                    Log("[%d] RegQueryInfoKey: lpcValues=  %d, lpcbMaxValueNameLen= %d, lpcbMaxValueLen= %d\n", RegLocalInstance, CountValues, dValueName, dData);
#endif
                    break;
                }
                else
                {
#if _DEBUG
                    Log("[%d] RegQueryInfoKey logging fixup failure.\n", RegLocalInstance);
#endif
                    return response;
                }
                Index++;
            }
        }
        catch (...)
        {
#if _DEBUG
            Log("[%d] RegQueryInfoKey logging failure in RegEnumValue.\n", RegLocalInstance);
#endif
        }

        // Modify lpcSubKeys, lpcbMaxSubKeyLen
        // Since Deletion Markers for values may be present
        try
        {
            DWORD Index = 0;
            DWORD CountSubKeys = 0;
            DWORD MaxSubKeyLen = 0;
            auto response = ERROR_SUCCESS;

            while (response == ERROR_SUCCESS)
            {
                CharT SubKey[INT16_MAX];
                DWORD NameLen = INT16_MAX;

                if constexpr (psf::is_ansi<CharT>)
                {
                    response = RegEnumKeyExA(hKey, Index, SubKey, &NameLen, NULL, NULL, NULL, NULL);
                }
                else
                {
                    response = RegEnumKeyExW(hKey, Index, SubKey, &NameLen, NULL, NULL, NULL, NULL);
                }

                if (response == ERROR_SUCCESS)
                {
                    CountSubKeys++;
                    MaxSubKeyLen = max(MaxSubKeyLen, NameLen);
                }
                else if (response == ERROR_NO_MORE_ITEMS)
                {
                    // Update lpcSubKeys & lpcbMaxSubKeyLen
                    if (lpcSubKeys != nullptr)
                        *lpcSubKeys = CountSubKeys;
                    if (lpcbMaxSubKeyLen != nullptr)
                        *lpcbMaxSubKeyLen = MaxSubKeyLen;
#if _DEBUG
                    Log("[%d] RegQueryInfoKey: lpcSubKeys=  %d, lpcbMaxSubKeyLen= %d\n", RegLocalInstance, CountSubKeys, MaxSubKeyLen);
#endif
                    break;
                }
                else
                {
#if _DEBUG
                    Log("[%d] RegQueryInfoKey logging fixup failure.\n", RegLocalInstance);
#endif
                    return response;
                }
                Index++;
            }
        }
        catch (...)
        {
#if _DEBUG
            Log("[%d] RegQueryInfoKey logging failure in RegEnumKeyEx.\n", RegLocalInstance);
#endif
        }
    }
    else
    {
#if _DEBUG
        Log("[%d] RegQueryInfoKey: Error %d\n", RegLocalInstance, result);
#endif
    }

    QueryPerformanceCounter(&TickEnd);
#if _DEBUG
    Log("[%d] RegQueryInfoKey: returns %d\n", RegLocalInstance, result);
#endif
    return result;
}
DECLARE_STRING_FIXUP(RegQueryInfoKeyImpl, RegQueryInfoKeyFixup);

/// <summary>
/// detour RegGetValueA & RegGetValueW
/// for deletion marker fixup
/// </summary>
auto RegGetValueImpl = psf::detoured_string_function(&::RegGetValueA, &::RegGetValueW);
template <typename CharT>
LSTATUS __stdcall  RegGetValueFixup(
    _In_ HKEY key,
    _In_opt_ const CharT* SubKey,
    _In_opt_ const CharT* Value,
    _In_ DWORD dwFlags,
    _Out_opt_ LPDWORD pdwType,
    _When_((dwFlags & 0x7F) == RRF_RT_REG_SZ ||
        (dwFlags & 0x7F) == RRF_RT_REG_EXPAND_SZ ||
        (dwFlags & 0x7F) == (RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ) ||
        *pdwType == REG_SZ ||
        *pdwType == REG_EXPAND_SZ, _Post_z_)
    _When_((dwFlags & 0x7F) == RRF_RT_REG_MULTI_SZ ||
        *pdwType == REG_MULTI_SZ, _Post_ _NullNull_terminated_)
    _Out_writes_bytes_to_opt_(*pcbData, *pcbData) PVOID pvData,
    _Inout_opt_ LPDWORD pcbData
)
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    DWORD RegLocalInstance = ++g_RegIntceptInstance;
    auto entry = LogFunctionEntry();
    auto result = ERROR_SUCCESS;

    try
    {
#ifdef _DEBUG
        Log("[%d] RegGetValue:\n", RegLocalInstance);
#endif

        // If subkey is in redirected path, use it to get Registry Value
		const char* updatedSubKey = ReplaceRegistryQueryPath(&key, SubKey);
		if (updatedSubKey)
		{
			if constexpr (psf::is_ansi<CharT>) 
			{
				return RegGetValueImpl(key, updatedSubKey, Value, dwFlags, pdwType, pvData, pcbData);
			}
			else
			{
				auto wideSubKey = widen(updatedSubKey);
				return RegGetValueImpl(key, wideSubKey.c_str(), Value, dwFlags, pdwType, pvData, pcbData);
			}
		}

        //Get Registry Path from hkey
        std::string keypath = InterpretKeyPath(key) + "\\" + InterpretStringA(SubKey);
        keypath = ReplaceRegistrySyntax(keypath);

#ifdef _DEBUG
        Log("[%d] RegGetValue: path=%s", RegLocalInstance, keypath.c_str());
        bool isDeletionMarker = RegFixupDeletionMarker(keypath, Value, RegLocalInstance);
#else
        bool isDeletionMarker = RegFixupDeletionMarker(keypath, Value);
#endif
        if (isDeletionMarker == true)
        {
#ifdef _DEBUG
            Log("[%d] RegGetValue:Deletion Marker Success\n", RegLocalInstance);
#endif 
            result = ERROR_FILE_NOT_FOUND;
            LogCallingModule();
        }
        else
        {
            result = RegGetValueImpl(key, SubKey, Value, dwFlags, pdwType, pvData, pcbData);
        }
    }
    catch (...)
    {
        Log("[%d] RegGetValue logging failure.\n", RegLocalInstance);
    }

    QueryPerformanceCounter(&TickEnd);
#if _DEBUG
    Log("[%d] RegGetValue: returns %d\n", RegLocalInstance, result);
#endif
    return result;
}
DECLARE_STRING_FIXUP(RegGetValueImpl, RegGetValueFixup);


/// <summary>
/// detour RegQueryMultipleValuesA & RegQueryMultipleValuesW
/// for deletion marker fixup
/// </summary>
auto RegQueryMultipleValuesImpl = psf::detoured_string_function(&::RegQueryMultipleValuesA, &::RegQueryMultipleValuesW);
template <typename CharT, typename VALENT>
LSTATUS __stdcall RegQueryMultipleValuesFixup(
    _In_ HKEY key,
    _Out_writes_(num_vals) VALENT* val_list,
    _In_ DWORD num_vals,
    _Out_writes_bytes_to_opt_(*ldwTotsize, *ldwTotsize) __out_data_source(REGISTRY) CharT* lpValueBuf,
    _Inout_opt_ LPDWORD ldwTotsize
)
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    DWORD RegLocalInstance = ++g_RegIntceptInstance;
    auto entry = LogFunctionEntry();
    auto result = ERROR_SUCCESS;

    try
    {
#ifdef _DEBUG
        Log("[%d] RegQueryMultipleValues:\n", RegLocalInstance);
#endif
        //Get Registry Path from hkey
        std::string keypath = InterpretKeyPath(key);
        keypath = ReplaceRegistrySyntax(keypath);

#ifdef _DEBUG
        Log("[%d] RegQueryMultipleValues: path=%s", RegLocalInstance, keypath.c_str());
#endif
        bool isDeletionMarker = false;

        for (DWORD itr = 0; itr < num_vals; itr++)
        {
            CharT* value = val_list[itr].ve_valuename;
#ifdef _DEBUG
            Log("[%d] RegQueryMultipleValues: value=%s", RegLocalInstance, value);
            if (RegFixupDeletionMarker(keypath, value, RegLocalInstance) == true)
#else
            if (RegFixupDeletionMarker(keypath, value) == true)
#endif
            {
                isDeletionMarker = true;
                break;
            }
        }

        if (isDeletionMarker == true)
        {
#ifdef _DEBUG
            Log("[%d] RegQueryMultipleValues:Deletion Marker Success\n", RegLocalInstance);
#endif 
            result = ERROR_FILE_NOT_FOUND;
            LogCallingModule();
        }
        else
        {
            result = RegQueryMultipleValuesImpl(key, val_list, num_vals, lpValueBuf, ldwTotsize);
        }
    }
    catch (...)
    {
        Log("[%d] RegQueryMultipleValues logging failure.\n", RegLocalInstance);
    }

    QueryPerformanceCounter(&TickEnd);
#if _DEBUG
    Log("[%d] RegQueryMultipleValues: returns %d\n", RegLocalInstance, result);
#endif
    return result;
}
DECLARE_STRING_FIXUP(RegQueryMultipleValuesImpl, RegQueryMultipleValuesFixup);



/// <summary>
/// detour RegQueryValueExA & RegQueryValueExW
/// for deletion marker fixup
/// </summary>
auto RegQueryValueExImpl = psf::detoured_string_function(&RegQueryValueExA, &::RegQueryValueExW);
template <typename CharT>
LSTATUS __stdcall RegQueryValueExFixup(
    _In_ HKEY key,
    _In_opt_ const CharT* lpValueName,
    _Reserved_ LPDWORD lpReserved,
    _Out_opt_ LPDWORD lpType,
    _Out_writes_bytes_to_opt_(*lpcbData, *lpcbData) __out_data_source(REGISTRY) LPBYTE lpData,
    _When_(lpData == NULL, _Out_opt_) _When_(lpData != NULL, _Inout_opt_) LPDWORD lpcbData
)
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    DWORD RegLocalInstance = ++g_RegIntceptInstance;
    auto entry = LogFunctionEntry();
    auto result = ERROR_SUCCESS;

    try
    {
#ifdef _DEBUG
        Log("[%d] RegQueryValueEx:\n", RegLocalInstance);
#endif
        //Get Registry Path from hkey
        std::string keypath = InterpretKeyPath(key);
        keypath = ReplaceRegistrySyntax(keypath);

#ifdef _DEBUG
        Log("[%d] RegQueryValueEx: path=%s", RegLocalInstance, keypath.c_str());
        bool isDeletionMarker = RegFixupDeletionMarker(keypath, lpValueName, RegLocalInstance);
#else
        bool isDeletionMarker = RegFixupDeletionMarker(keypath, lpValueName);
#endif
        if (isDeletionMarker == true)
        {
#ifdef _DEBUG
            Log("[%d] RegQueryValueEx:Deletion Marker Success\n", RegLocalInstance);
#endif 
            result = ERROR_FILE_NOT_FOUND;
            LogCallingModule();
        }
        else
        {
            result = RegQueryValueExImpl(key, lpValueName, lpReserved, lpType, lpData, lpcbData);
        }
    }
    catch (...)
    {
        Log("[%d] RegQueryValueEx logging failure.\n", RegLocalInstance);
    }

    QueryPerformanceCounter(&TickEnd);
#if _DEBUG
    Log("[%d] RegQueryValueEx: returns %d\n", RegLocalInstance, result);
#endif
    return result;
}
DECLARE_STRING_FIXUP(RegQueryValueExImpl, RegQueryValueExFixup);


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
                std::string keypath = InterpretKeyPath(key) + "\\" + InterpretStringA(subKey);

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
                std::string keypath = InterpretKeyPath(key) + "\\" + InterpretStringA(subKey);
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
                std::string keypath = InterpretKeyPath(key) + "\\" + InterpretStringA(subKey);
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
                std::string keypath = InterpretKeyPath(key) + "\\" + InterpretStringA(subValueName);
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

