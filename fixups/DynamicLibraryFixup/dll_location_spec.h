//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

typedef enum dllBitness
{
    x86,
    x64,
    AnyCPU,
    NotSpecified
} DllBitness;
struct dll_location_spec
{
    std::filesystem::path full_filepath;
    std::wstring_view filename;
    dllBitness architecture;
};