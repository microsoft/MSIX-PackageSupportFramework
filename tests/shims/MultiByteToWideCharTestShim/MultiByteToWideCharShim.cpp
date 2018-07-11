//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <shim_framework.h>

auto MultiByteToWideCharImpl = &::MultiByteToWideChar;

int __stdcall MultiByteToWideCharShim(
    _In_ UINT codePage,
    _In_ DWORD flags,
    _In_NLS_string_(cbMultiByte) LPCCH multiByteStr,
    _In_ int multiByteLength,
    _Out_writes_to_opt_(cchWideChar,return) LPWSTR wideCharStr,
    _In_ int wideCharLength)
{
    // Only shim the scenarios that we actually want to shim
    constexpr char expectedMessage[] = "This message should have been shimmed";
    if ((multiByteLength == static_cast<int>(std::size(expectedMessage) - 1)) &&
        (std::strncmp(multiByteStr, expectedMessage, multiByteLength) == 0))
    {
        constexpr wchar_t shimmedMessage[] = L"You've been shimmed!";
        constexpr int shimmedLength = static_cast<int>(std::size(shimmedMessage) - 1);
        if (wideCharLength >= shimmedLength)
        {
            std::wcsncpy(wideCharStr, shimmedMessage, shimmedLength);
            return shimmedLength;
        }
    }

    return MultiByteToWideCharImpl(codePage, flags, multiByteStr, multiByteLength, wideCharStr, wideCharLength);
}
DECLARE_SHIM(MultiByteToWideCharImpl, MultiByteToWideCharShim);
