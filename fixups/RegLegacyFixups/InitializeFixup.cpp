//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "pch.h"

#include <regex>
#include <vector>

#include <known_folders.h>
#include <objbase.h>

#include <psf_framework.h>
#include <utilities.h>

#include <filesystem>
using namespace std::literals;

#include "FunctionImplementations.h"
#include "Reg_Remediation_spec.h"


std::vector<Reg_Remediation_Spec>  g_regRemediationSpecs;


void Log(const char* fmt, ...)
{
    try
    {
        va_list args;
        va_start(args, fmt);
        std::string str;
        str.resize(256);
        std::size_t count = std::vsnprintf(str.data(), str.size() + 1, fmt, args);
        assert(count >= 0);
        va_end(args);

        if (count > str.size())
        {
            count = 1024;       // vswprintf actually returns a negative number, let's just go with something big enough for our long strings; it is resized shortly.
            str.resize(count);

            va_list args2;
            va_start(args2, fmt);
            count = std::vsnprintf(str.data(), str.size() + 1, fmt, args2);
            assert(count >= 0);
            va_end(args2);
        }

        str.resize(count);
#if _DEBUG
        ::OutputDebugStringA(str.c_str());
#endif
    }
    catch (...)
    {
        ::OutputDebugStringA("Exception in Log()");
        ::OutputDebugStringA(fmt);
    }
}
void Log(const wchar_t* fmt, ...)
{
    try
    {
        va_list args;
        va_start(args, fmt);

        std::wstring wstr;
        wstr.resize(256);
        std::size_t count = std::vswprintf(wstr.data(), wstr.size() + 1, fmt, args);
        va_end(args);

        if (count > wstr.size())
        {
            count = 1024;       // vswprintf actually returns a negative number, let's just go with something big enough for our long strings; it is resized shortly.
            wstr.resize(count);
            va_list args2;
            va_start(args2, fmt);
            count = std::vswprintf(wstr.data(), wstr.size() + 1, fmt, args2);
            va_end(args2);
        }
        wstr.resize(count);
#if _DEBUG
        ::OutputDebugStringW(wstr.c_str());
#endif
    }
    catch (...)
    {
        ::OutputDebugStringA("Exception in wide Log()");
        ::OutputDebugStringW(fmt);
    }
}
void LogString(const char* name, const char* value)
{
    Log("%s=%s\n", name, value);
}
void LogString(const char* name, const wchar_t* value)
{
    Log("%s=%ls\n", name, value);
}
void LogString(const wchar_t* name, const char* value)
{
    Log("%ls=%s\n", name, value);
}
void LogString(const wchar_t* name, const wchar_t* value)
{
    Log(L"%ls=%ls\n", name, value);
}


void InitializeFixups()
{

    Log("Initializing RegLegacyFixups\n");
    
}


void InitializeConfiguration()
{
    Log("RegLegacyFixups Start InitializeConfiguration()\n");
    
    if (auto rootConfig = ::PSFQueryCurrentDllConfig())
    {
        if (rootConfig != NULL)
        {
            Log("RegLegacyFixups process config\n");
            const psf::json_array& rootConfigArray = rootConfig->as_array();
            for (auto& spec : rootConfigArray)
            {
                Log("RegLegacyFixups: process spec\n");

                Reg_Remediation_Spec specItem;

                auto& specObject = spec.as_object();
                Log("RegLegacyFixups: Have specObject\n");

                auto type = specObject.get("type").as_string().wstring();
                Log("RegLegacyFixups: have type");
#if _DEBUG
                Log(L"Type: %Ls\n", type.data());
#endif
                if (type.compare(L"ModifyKeyAccess") == 0)
                {
                    Log("RegLegacyFixups: is ModifyKeyAccess\n");
                    specItem.remeditaionType = Reg_Remediation_Type_ModifyKeyAccess;
                    if (auto remValue = specObject.try_get("remediation"))
                    {
                        Log("RegLegacyFixups: have rediation(s)\n");
                        const psf::json_array& remediationsArray = remValue->as_array();
                        for (auto& remediationsItem : remediationsArray)
                        {
                            Log("RegLegacyFixups: process remediation\n");
                            Reg_Remediation_Record recordItem;
                            auto& remediationsItemObject = remediationsItem.as_object();

                            auto hiveType = remediationsItemObject.try_get("hive")->as_string().wstring();
#if _DEBUG
                            Log(L"Hive: %Ls\n", hiveType.data());
#endif
                            if (hiveType.compare(L"HKCU") == 0)
                            {
                                recordItem.modifyKeyAccess.hive = Modify_Key_Hive_Type_HKCU;
                            }
                            else if (hiveType.compare(L"HKLM") == 0)
                            {
                                recordItem.modifyKeyAccess.hive = Modify_Key_Hive_Type_HKLM;
                            }
                            else
                            {
                                recordItem.modifyKeyAccess.hive = Modify_Key_Hive_Type_Unknown;
                            }
                            Log("RegLegacyFixups: have hive\n");

                            for (auto& pattern : remediationsItemObject.get("patterns").as_array())
                            {
                                auto patternString = pattern.as_string().wstring();
                                
#if _DEBUG
                                Log(L"Pattern: %Ls\n", patternString.data());
#endif
                                recordItem.modifyKeyAccess.patterns.push_back(patternString.data());
                               
                            }
                            Log("RegLegacyFixups: have patterns\n");

                            auto accessType = remediationsItemObject.try_get("access")->as_string().wstring();
#if _DEBUG
                            Log(L"Access: %Ls\n", accessType.data());
#endif
                            if (accessType.compare(L"Full2RW") == 0)
                            {
                                recordItem.modifyKeyAccess.access = Modify_Key_Access_Type_Full2RW;
                            }
                            else if (accessType.compare(L"Full2R") == 0)
                            {
                                recordItem.modifyKeyAccess.access = Modify_Key_Access_Type_Full2R;
                            }
                            else if (accessType.compare(L"RW2R") == 0)
                            {
                                recordItem.modifyKeyAccess.access = Modify_Key_Access_Type_RW2R;
                            }
                            else
                            {
                                recordItem.modifyKeyAccess.access = Modify_Key_Access_Type_Unknown;
                            }
                            Log("RegLegacyFixups: have access\n");
                            specItem.remediationRecords.push_back(recordItem);
                        }
                    }

                }
                else
                {
                    specItem.remeditaionType = Reg_Remediation_Type_Unknown;
                }
                g_regRemediationSpecs.push_back(specItem);

            }
        }
        else
        {
            Log("RegLegacyFixups: Fixup not found in json config.\n");
        }
        Log("RegLegacyFixups End InitializeConfiguration()\n");
    }
}