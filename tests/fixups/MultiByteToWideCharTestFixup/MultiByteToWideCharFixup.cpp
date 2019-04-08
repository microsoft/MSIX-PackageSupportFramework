//-------------------------------------------------------------------------------------------------------    
// Copyright (C) Microsoft Corporation. All rights reserved.    
// Licensed under the MIT license. See LICENSE file in the project root for full license information.    
//-------------------------------------------------------------------------------------------------------    
    
#include <psf_framework.h>    
    
auto MultiByteToWideCharImpl = &::MultiByteToWideChar;    
    
int __stdcall MultiByteToWideCharFixup(    
    _In_ UINT codePage,    
    _In_ DWORD flags,    
    _In_NLS_string_(cbMultiByte) LPCCH multiByteStr,    
    _In_ int multiByteLength,    
    _Out_writes_to_opt_(cchWideChar,return) LPWSTR wideCharStr,    
    _In_ int wideCharLength)    
{    
    // Only fixup the scenarios that we actually want to fixup    
    constexpr char expectedMessage[] = "This message should have been fixed";    
    if ((multiByteLength == static_cast<int>(std::size(expectedMessage) - 1)) &&    
        (std::strncmp(multiByteStr, expectedMessage, multiByteLength) == 0))    
    {    
        constexpr wchar_t fixedMessage[] = L"You've been fixed!";    
        constexpr int fixedLength = static_cast<int>(std::size(fixedMessage) - 1);    
        if (wideCharLength >= fixedLength)    
        {    
            std::wcsncpy(wideCharStr, fixedMessage, fixedLength);    
            return fixedLength;    
        }    
    }    
    
    return MultiByteToWideCharImpl(codePage, flags, multiByteStr, multiByteLength, wideCharStr, wideCharLength);    
}    
DECLARE_FIXUP(MultiByteToWideCharImpl, MultiByteToWideCharFixup);
