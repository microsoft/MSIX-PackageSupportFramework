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
#include <psf_logging.h>
#include <utilities.h>

#include <filesystem>
using namespace std::literals;

#include "FunctionImplementations.h"
#include "Reg_Remediation_spec.h"


std::vector<Reg_Remediation_Spec>  g_regRemediationSpecs;




void InitializeFixups()
{
#if _DEBUG
    Log(L"Initializing RegLegacyFixups\n");
#endif
}


void InitializeConfiguration()
{
#if _DEBUG
    Log(L"RegLegacyFixups Start InitializeConfiguration()\n");
#endif
    if (auto rootConfig = ::PSFQueryCurrentDllConfig())
    {
        if (rootConfig != NULL)
        {
#if _DEBUG
            Log(L"RegLegacyFixups process config\n");
#endif
            const psf::json_array& rootConfigArray = rootConfig->as_array();
            for (auto& spec : rootConfigArray)
            {
#if _DEBUG
                Log(L"RegLegacyFixups: process spec\n");
#endif
                Reg_Remediation_Spec specItem;
                auto& specObject = spec.as_object();
                if (auto regItems = specObject.try_get("remediation"))
                {
#if _DEBUG
                    Log("RegLegacyFixups:  remediation array:\n");
#endif
                    const psf::json_array& remediationArray = regItems->as_array();
                    for (auto& regItem : remediationArray)
                    {
#if _DEBUG
                        Log(L"RegLegacyFixups:    remediation:\n");
#endif
                        auto& regItemObject = regItem.as_object();
                        Reg_Remediation_Record recordItem;
                        auto type = regItemObject.get("type").as_string().wstring();
#if _DEBUG
                        Log(L"RegLegacyFixups: have type");
                        Log(L"Type: %Ls\n", type.data());
#endif
                        //Reg_Remediation_Spec specItem;
                        if (type.compare(L"ModifyKeyAccess") == 0)
                        {
#if _DEBUG
                            Log(L"RegLegacyFixups:      is ModifyKeyAccess\n");
#endif
                            recordItem.remeditaionType = Reg_Remediation_Type_ModifyKeyAccess;

                            auto hiveType = regItemObject.try_get("hive")->as_string().wstring();
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
#if _DEBUG
                            Log(L"RegLegacyFixups:      have hive\n");
#endif
                            for (auto& pattern : regItemObject.get("patterns").as_array())
                            {
                                auto patternString = pattern.as_string().wstring();
#if _DEBUG
                                Log(L"Pattern:      %Ls\n", patternString.data());
#endif
                                recordItem.modifyKeyAccess.patterns.push_back(patternString.data());

                            }
#if _DEBUG
                            Log(L"RegLegacyFixups:      have patterns\n");
#endif
                            auto accessType = regItemObject.try_get("access")->as_string().wstring();
#if _DEBUG
                            Log(L"Access: %Ls\n", accessType.data());
#endif
                            if (accessType.compare(L"Full2RW") == 0)
                            {
                                recordItem.modifyKeyAccess.access = Modify_Key_Access_Type_Full2RW;
                            }
                            else if (accessType.compare(L"Full2MaxAllowed") == 0)
                            {
                                recordItem.modifyKeyAccess.access = Modify_Key_Access_Type_Full2MaxAllowed;
                            }
                            else if (accessType.compare(L"Full2R") == 0)
                            {
                                recordItem.modifyKeyAccess.access = Modify_Key_Access_Type_Full2R;
                            }
                            else if (accessType.compare(L"RW2R") == 0)
                            {
                                recordItem.modifyKeyAccess.access = Modify_Key_Access_Type_RW2R;
                            }
                            else if (accessType.compare(L"RW2MaxAllowed") == 0)
                            {
                                recordItem.modifyKeyAccess.access = Modify_Key_Access_Type_RW2MaxAllowed;
                            }
                            else
                            {
                                recordItem.modifyKeyAccess.access = Modify_Key_Access_Type_Unknown;
                            }
#if _DEBUG
                            Log(L"RegLegacyFixups:      have access\n");
#endif
                            specItem.remediationRecords.push_back(recordItem);
                        }
                        else if (type.compare(L"FakeDelete") == 0)
                        {
#if _DEBUG
                            Log(L"RegLegacyFixups:      is FakeDelete\n");
#endif
                            recordItem.remeditaionType = Reg_Remediation_type_FakeDelete;

                            auto hiveType = regItemObject.try_get("hive")->as_string().wstring();
#if _DEBUG
                            Log(L"Hive:      %Ls\n", hiveType.data());
#endif
                            if (hiveType.compare(L"HKCU") == 0)
                            {
                                recordItem.fakeDeleteKey.hive = Modify_Key_Hive_Type_HKCU;
                            }
                            else if (hiveType.compare(L"HKLM") == 0)
                            {
                                recordItem.fakeDeleteKey.hive = Modify_Key_Hive_Type_HKLM;
                            }
                            else
                            {
                                recordItem.fakeDeleteKey.hive = Modify_Key_Hive_Type_Unknown;
                            }
#if _DEBUG
                            Log(L"RegLegacyFixups:      have hive\n");
#endif

                            for (auto& pattern : regItemObject.get("patterns").as_array())
                            {
                                auto patternString = pattern.as_string().wstring();
#if _DEBUG
                                Log(L"Pattern:        %Ls\n", patternString.data());
#endif
                                recordItem.fakeDeleteKey.patterns.push_back(patternString.data());
                            }
#if _DEBUG
                            Log(L"RegLegacyFixups:      have patterns\n");
#endif


                            specItem.remediationRecords.push_back(recordItem);
                        }
                        else
                        {
                        }
                        g_regRemediationSpecs.push_back(specItem);
                    }
                }
            }
        }
        else
        {
#if _DEBUG
            Log(L"RegLegacyFixups: Fixup not found in json config.\n");
#endif
        }
#if _DEBUG
        Log(L"RegLegacyFixups End InitializeConfiguration()\n");
#endif
    }
}