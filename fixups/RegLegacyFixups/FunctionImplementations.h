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

#include <cassert>
//#include <ntstatus.h>
#include <windows.h>
#include <winternl.h>

// A much bigger hammer to avoid reentrancy. Still, the impl::* functions are good to have around to prevent the
// unnecessary invocation of the fixup
inline thread_local psf::reentrancy_guard g_reentrancyGuard;

namespace winternl
{
    typedef enum _KEY_INFORMATION_CLASS
    {
        KeyBasicInformation,
        KeyNodeInformation,
        KeyFullInformation,
        KeyNameInformation,
        KeyCachedInformation,
        KeyFlagsInformation,
        KeyVirtualizationInformation,
        KeyHandleTagsInformation,
        KeyTrustInformation,
        KeyLayerInformation,
        MaxKeyInfoClass
    } KEY_INFORMATION_CLASS;

    typedef enum _KEY_VALUE_INFORMATION_CLASS
    {
        KeyValueBasicInformation,
        KeyValueFullInformation,
        KeyValuePartialInformation,
        KeyValueFullInformationAlign64,
        KeyValuePartialInformationAlign64,
        KeyValueLayerInformation,
        MaxKeyValueInfoClass
    } KEY_VALUE_INFORMATION_CLASS;

    typedef struct _KEY_BASIC_INFORMATION
    {
        LARGE_INTEGER LastWriteTime;
        ULONG         TitleIndex;
        ULONG         NameLength;
        WCHAR         Name[1];
    } KEY_BASIC_INFORMATION, * PKEY_BASIC_INFORMATION;

    typedef struct _KEY_NODE_INFORMATION
    {
        LARGE_INTEGER LastWriteTime;
        ULONG         TitleIndex;
        ULONG         ClassOffset;
        ULONG         ClassLength;
        ULONG         NameLength;
        WCHAR         Name[1];
    } KEY_NODE_INFORMATION, * PKEY_NODE_INFORMATION;

    typedef struct _KEY_FULL_INFORMATION
    {
        LARGE_INTEGER LastWriteTime;
        ULONG         TitleIndex;
        ULONG         ClassOffset;
        ULONG         ClassLength;
        ULONG         SubKeys;
        ULONG         MaxNameLen;
        ULONG         MaxClassLen;
        ULONG         Values;
        ULONG         MaxValueNameLen;
        ULONG         MaxValueDataLen;
        WCHAR         Class[1];
    } KEY_FULL_INFORMATION, * PKEY_FULL_INFORMATION;

    typedef struct _KEY_NAME_INFORMATION
    {
        ULONG NameLength;
        WCHAR Name[1];
    } KEY_NAME_INFORMATION, * PKEY_NAME_INFORMATION;

    typedef struct _KEY_CACHED_INFORMATION
    {
        LARGE_INTEGER LastWriteTime;
        ULONG         TitleIndex;
        ULONG         SubKeys;
        ULONG         MaxNameLen;
        ULONG         Values;
        ULONG         MaxValueNameLen;
        ULONG         MaxValueDataLen;
        ULONG         NameLength;
    } KEY_CACHED_INFORMATION, * PKEY_CACHED_INFORMATION;

    typedef struct _KEY_VIRTUALIZATION_INFORMATION
    {
        ULONG VirtualizationCandidate;
        ULONG VirtualizationEnabled;
        ULONG VirtualTarget;
        ULONG VirtualStore;
        ULONG VirtualSource;
        ULONG Reserved;
    } KEY_VIRTUALIZATION_INFORMATION, * PKEY_VIRTUALIZATION_INFORMATION;

    typedef struct _KEY_VALUE_BASIC_INFORMATION
    {
        ULONG TitleIndex;
        ULONG Type;
        ULONG NameLength;
        WCHAR Name[1];
    } KEY_VALUE_BASIC_INFORMATION, PKEY_VALUE_BASIC_INFORMATION;

    typedef struct _KEY_VALUE_FULL_INFORMATION
    {
        ULONG TitleIndex;
        ULONG Type;
        ULONG DataOffset;
        ULONG DataLength;
        ULONG NameLength;
        WCHAR Name[1];
    } KEY_VALUE_FULL_INFORMATION, PKEY_VALUE_FULL_INFORMATION;

    typedef struct _KEY_VALUE_PARTIAL_INFORMATION
    {
        ULONG TitleIndex;
        ULONG Type;
        ULONG DataLength;
        UCHAR Data[1];
    } KEY_VALUE_PARTIAL_INFORMATION, PKEY_VALUE_PARTIAL_INFORMATION;

    // Function declarations not present in winternl.h
    NTSTATUS __stdcall NtQueryInformationFile(
        HANDLE FileHandle,
        PIO_STATUS_BLOCK IoStatusBlock,
        PVOID FileInformation,
        ULONG Length,
        FILE_INFORMATION_CLASS FileInformationClass);

    NTSTATUS __stdcall NtQueryKey(
        HANDLE KeyHandle,
        KEY_INFORMATION_CLASS KeyInformationClass,
        PVOID KeyInformation,
        ULONG Length,
        PULONG ResultLength);

    NTSTATUS __stdcall NtQueryValueKey(
        HANDLE KeyHandle,
        PUNICODE_STRING ValueName,
        KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
        PVOID KeyValueInformation,
        ULONG Length,
        PULONG ResultLength);

    NTSTATUS __stdcall RegDeleteKey(
        HANDLE hKey,
        PUNICODE_STRING lpSubKey
    );

    NTSTATUS __stdcall RegDeleteKeyEx(
        HANDLE hKey,
        PUNICODE_STRING lpSubKey,
        DWORD viewDesired, // 32/64
        DWORD Reserved
    );

    NTSTATUS __stdcall RegDeleteKeyTransacted(
        HANDLE hKey,
        PUNICODE_STRING lpSubKey,
        DWORD viewDesired, // 32/64
        DWORD Reserved,
        HANDLE hTransaction,
        PVOID  pExtendedParameter
    );

    NTSTATUS __stdcall RegDeleteValue(
        HANDLE hKey,
        PUNICODE_STRING lpValueName
    );
}



namespace winternl
{
    // NOTE: The functions in winternl.h are not included in any import lib and therefore must be manually loaded in
    template <typename Func>
    inline Func GetNtDllInternalFunction(const char* functionName)
    {
        static auto mod = ::LoadLibraryW(L"ntdll.dll");
        assert(mod);

        // Ignore namespaces
        for (auto ptr = functionName; *ptr; ++ptr)
        {
            if (*ptr == ':')
            {
                functionName = ptr + 1;
            }
        }

        auto result = reinterpret_cast<Func>(::GetProcAddress(mod, functionName));
        assert(result);
        return result;
    }
}


#define WINTERNL_FUNCTION(Name) (winternl::GetNtDllInternalFunction<decltype(&Name)>(#Name));

namespace impl
{

    inline auto NtQueryKey = WINTERNL_FUNCTION(winternl::NtQueryKey);

}






inline std::wstring InterpretStringW(const char* value)
{
    return widen(value);
}
inline std::wstring InterpretStringW(const wchar_t* value)
{
    return value;
}

