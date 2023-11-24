//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

enum  Env_Remediation_Types
{
    Env_Remediation_Type_Unknown = 0,
    Env_Remediation_Type_Registry,
    Env_Remediation_Type_Dependency,
};


struct env_var_spec
{
    Env_Remediation_Types remediationType;

    std::wregex variablename;
    std::wstring_view variablevalue;
    bool useregistry;
    std::wstring dependency;
};