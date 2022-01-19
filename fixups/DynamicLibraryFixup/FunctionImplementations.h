//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// Collection of function LoadLibrary will (presumably) call kernelbase!LoadLibraryA/W, even though we detour that call. Useful to
// reduce the risk of us fixing ourselves, which can easily lead to infinite recursion. Note that this isn't perfect.
// For example, CreateFileFixup could call kernelbase!CopyFileW, which could in turn call (the fixed) CreateFile again
#pragma once

#include <reentrancy_guard.h>
#include <psf_framework.h>
#include <psf_logging.h>

// A much bigger hammer to avoid reentrancy. Still, the impl::* functions are good to have around to prevent the
// unnecessary invocation of the fixup
inline thread_local psf::reentrancy_guard g_reentrancyGuard;

namespace impl
{

}


inline std::wstring InterpretStringW(const char* value)
{
    return widen(value);
}
inline std::wstring InterpretStringW(const wchar_t* value)
{
    return value;
}

inline std::wstring GetFilenameOnly(std::wstring path)
{
    size_t index = path.find_last_of(L"\\", std::wstring::npos);
    if (index == std::wstring::npos)
        return path;
    return path.substr(index + 1);
}

///void Log(const char* fmt, ...);
///void LogString(const char* name, const char* value);
///void LogString(const char* name, const wchar_t* value);