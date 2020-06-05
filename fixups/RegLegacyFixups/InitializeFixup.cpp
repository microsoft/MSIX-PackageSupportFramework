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

    Log("Initializing RegLegacyFixups");
    
}


void InitializeConfiguration()
{
    Log("RegLegacyFixups Start InitializeConfiguration()");
    
    if (auto rootConfig = ::PSFQueryCurrentDllConfig())
    {
        const psf::json_array& rootConfigArray = rootConfig->as_array();
        for (auto& spec : rootConfigArray)
        {
            Reg_Remediation_Spec specItem;

            auto& specObject = spec.as_object();
            auto type = specObject.get("type").as_string().wstring();
#if _DEBUG
            Log(L"Type: %Ls", type.data());
#endif
            if (type.compare(L"ModifyKeyAccess") == 0)
            {
                specItem.remeditaionType = Reg_Remediation_Type_ModifyKeyAccess;
                if (auto remValue = specObject.try_get("remediation"))
                {
                    const psf::json_array& remediationsArray = remValue->as_array();
                    for (auto& remediationsItem : remediationsArray)
                    {
                        Reg_Remediation_Record recordItem;
                        auto& remediationsItemObject = remediationsItem.as_object();
                        
                        auto hiveType = remediationsItemObject.try_get("hive")->as_string().wstring(); 
#if _DEBUG
                        Log(L"Hive: %Ls", hiveType.data());
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

                        if (auto patterns = remediationsItemObject.try_get("patterns"))
                        {
                            const psf::json_array& patternsArray = patterns->as_array();
                            for (auto& pattern : patternsArray)
                            {
#if _DEBUG
                                Log(L"Pattern: %Ls", pattern.as_string().wide());
#endif
                                recordItem.modifyKeyAccess.patterns.push_back(pattern.as_string().wide());
                            }
                        }

                        auto accessType = remediationsItemObject.try_get("access")->as_string().wstring();
#if _DEBUG
                        Log(L"Access: %Ls", accessType.data());
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

        Log("RegLegacyFixups End InitializeConfiguration()");
    }
}