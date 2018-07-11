//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <windows.h>

static constexpr bool flag_set(DWORD value, DWORD flag)
{
    return (value & flag) == flag;
}

static std::wstring file_attributes(DWORD value)
{
    std::wstring result;
    const wchar_t* prefix = L"";

    auto test_flag = [&](DWORD flag, const wchar_t* name)
    {
        if (flag_set(value, flag))
        {
            result += prefix;
            result += name;
            prefix = L" | ";
        }
    };
    test_flag(FILE_ATTRIBUTE_READONLY, L"FILE_ATTRIBUTE_READONLY");
    test_flag(FILE_ATTRIBUTE_HIDDEN, L"FILE_ATTRIBUTE_HIDDEN");
    test_flag(FILE_ATTRIBUTE_SYSTEM, L"FILE_ATTRIBUTE_SYSTEM");
    test_flag(FILE_ATTRIBUTE_DIRECTORY, L"FILE_ATTRIBUTE_DIRECTORY");
    test_flag(FILE_ATTRIBUTE_ARCHIVE, L"FILE_ATTRIBUTE_ARCHIVE");
    test_flag(FILE_ATTRIBUTE_DEVICE, L"FILE_ATTRIBUTE_DEVICE");
    test_flag(FILE_ATTRIBUTE_NORMAL, L"FILE_ATTRIBUTE_NORMAL");
    test_flag(FILE_ATTRIBUTE_TEMPORARY, L"FILE_ATTRIBUTE_TEMPORARY");
    test_flag(FILE_ATTRIBUTE_SPARSE_FILE, L"FILE_ATTRIBUTE_SPARSE_FILE");
    test_flag(FILE_ATTRIBUTE_REPARSE_POINT, L"FILE_ATTRIBUTE_REPARSE_POINT");
    test_flag(FILE_ATTRIBUTE_COMPRESSED, L"FILE_ATTRIBUTE_COMPRESSED");
    test_flag(FILE_ATTRIBUTE_OFFLINE, L"FILE_ATTRIBUTE_OFFLINE");
    test_flag(FILE_ATTRIBUTE_NOT_CONTENT_INDEXED, L"FILE_ATTRIBUTE_NOT_CONTENT_INDEXED");
    test_flag(FILE_ATTRIBUTE_ENCRYPTED, L"FILE_ATTRIBUTE_ENCRYPTED");
    test_flag(FILE_ATTRIBUTE_INTEGRITY_STREAM, L"FILE_ATTRIBUTE_INTEGRITY_STREAM");
    test_flag(FILE_ATTRIBUTE_VIRTUAL, L"FILE_ATTRIBUTE_VIRTUAL");
    test_flag(FILE_ATTRIBUTE_NO_SCRUB_DATA, L"FILE_ATTRIBUTE_NO_SCRUB_DATA");
    test_flag(FILE_ATTRIBUTE_EA, L"FILE_ATTRIBUTE_EA");
    test_flag(FILE_ATTRIBUTE_PINNED, L"FILE_ATTRIBUTE_PINNED");
    test_flag(FILE_ATTRIBUTE_UNPINNED, L"FILE_ATTRIBUTE_UNPINNED");
    test_flag(FILE_ATTRIBUTE_RECALL_ON_OPEN, L"FILE_ATTRIBUTE_RECALL_ON_OPEN");
    test_flag(FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS, L"FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS");
#ifdef FILE_ATTRIBUTE_STRICTLY_SEQUENTIAL
    test_flag(FILE_ATTRIBUTE_STRICTLY_SEQUENTIAL, L"FILE_ATTRIBUTE_STRICTLY_SEQUENTIAL");
#endif

    return result;
}
