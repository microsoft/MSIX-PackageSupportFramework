//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
#include <string_view>
#include <vector>


using namespace std::literals;

enum  Reg_Remediation_Types
{
    Reg_Remediation_Type_Unknown = 0,
    Reg_Remediation_Type_ModifyKeyAccess
};

enum Modify_Key_Access_Types
{
    Modify_Key_Access_Type_Unknown = 0,
    Modify_Key_Access_Type_Full2RW,
    Modify_Key_Access_Type_Full2R,
    Modify_Key_Access_Type_RW2R
};

enum Modify_Key_Hive_Types
{
    Modify_Key_Hive_Type_Unknown = 0,
    Modify_Key_Hive_Type_HKCU = 1,
    Modify_Key_Hive_Type_HKLM = 2,
};
struct Modify_Key_Access
{
    Modify_Key_Hive_Types hive;
    std::vector<std::wstring> patterns;
    Modify_Key_Access_Types access;
};

struct Reg_Remediation_Record
{
        Modify_Key_Access modifyKeyAccess;
        // for future types
};

struct Reg_Remediation_Spec
{
    Reg_Remediation_Types remeditaionType;
    std::vector<Reg_Remediation_Record> remediationRecords;   
}; 

extern std::vector<Reg_Remediation_Spec>  g_regRemediationSpecs;