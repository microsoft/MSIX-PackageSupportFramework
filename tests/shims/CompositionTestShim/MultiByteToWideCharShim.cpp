//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <shim_framework.h>

auto MultiByteToWideCharImpl = &::MultiByteToWideChar;

int __stdcall MultiByteToWideCharShim(
    _In_ UINT codePage,
    _In_ DWORD flags,
    _In_NLS_string_(cbMultiByte) LPCCH multiByteStr,
    _In_ int multiByteLength,
    _Out_writes_to_opt_(cchWideChar, return) LPWSTR wideCharStr,
    _In_ int wideCharLength)
{
    auto result = MultiByteToWideCharImpl(codePage, flags, multiByteStr, multiByteLength, wideCharStr, wideCharLength);

    // Only shim the scenarios that we actually want to shim
    constexpr char expectedMessage[] = "This message should have been shimmed";
    if ((multiByteLength == static_cast<int>(std::size(expectedMessage) - 1)) &&
        (std::strncmp(multiByteStr, expectedMessage, multiByteLength) == 0) &&
        (wideCharLength >= result))
    {
        constexpr wchar_t appendMessage[] = L" And you've been shimmed again!";
        constexpr int appendLength = static_cast<int>(std::size(appendMessage)) - 1;
        if ((result + appendLength) <= wideCharLength)
        {
            std::wcsncpy(wideCharStr + result, appendMessage, appendLength);
            result += appendLength;
        }
    }

    return result;
}
DECLARE_SHIM(MultiByteToWideCharImpl, MultiByteToWideCharShim);
