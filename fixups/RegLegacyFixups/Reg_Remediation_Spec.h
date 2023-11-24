//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
#include <unordered_map>
#include <unordered_set>
#include <string_view>
#include <vector>


using namespace std::literals;

enum  Reg_Remediation_Types
{
    Reg_Remediation_Type_Unknown = 0,
    Reg_Remediation_Type_ModifyKeyAccess,
    Reg_Remediation_Type_FakeDelete,
    Reg_Remediation_type_DeletionMarker
};

enum Modify_Key_Access_Types
{
    Modify_Key_Access_Type_Unknown = 0,
    Modify_Key_Access_Type_Full2RW,
    Modify_Key_Access_Type_Full2R,
    Modify_Key_Access_Type_Full2MaxAllowed,
    Modify_Key_Access_Type_RW2R,
    Modify_Key_Access_Type_RW2MaxAllowed
};

enum Modify_Key_Hive_Types
{
    Modify_Key_Hive_Type_Unknown = 0,
    Modify_Key_Hive_Type_HKCU = 1,
    Modify_Key_Hive_Type_HKLM = 2
};

struct Modify_Key_Access
{
    Modify_Key_Hive_Types hive;
    std::vector<std::wstring> patterns;
    Modify_Key_Access_Types access;
};

struct Fake_Delete_Key
{
    Modify_Key_Hive_Types hive;
    std::vector<std::wstring> patterns;
};

struct Deletion_Marker
{
    Modify_Key_Hive_Types hive;
    std::wstring key;
    std::vector<std::wstring> values;
};

struct Reg_Remediation_Record
{
    Reg_Remediation_Types remeditaionType;
    Modify_Key_Access modifyKeyAccess;
    Fake_Delete_Key fakeDeleteKey;
    Deletion_Marker deletionMarker;
};

struct Reg_Remediation_Spec
{
    std::vector<Reg_Remediation_Record> remediationRecords;
};

extern std::vector<Reg_Remediation_Spec>  g_regRemediationSpecs;
extern std::unordered_set<std::string> g_regRedirectedHivePaths;
extern std::vector<std::wstring> g_regCreatedKeysOrdered;
