
#include <shim_framework.h>

#include "Config.h"
#include "FunctionImplementations.h"
#include "Logging.h"
#include "PreserveError.h"

void LogKeyPath(HKEY key, const char* msg = "Key")
{
    ULONG size;
    if (auto status = impl::NtQueryKey(key, winternl::KeyNameInformation, nullptr, 0, &size);
        (status == STATUS_BUFFER_TOO_SMALL) || (status == STATUS_BUFFER_OVERFLOW))
    {
        auto buffer = std::make_unique<std::uint8_t[]>(size);
        if (NT_SUCCESS(impl::NtQueryKey(key, winternl::KeyNameInformation, buffer.get(), size, &size)))
        {
            auto info = reinterpret_cast<winternl::PKEY_NAME_INFORMATION>(buffer.get());
            LogCountedString(msg, info->Name, info->NameLength / 2);
        }
    }
}

auto RegCreateKeyImpl = shims::detoured_string_function(&::RegCreateKeyA, &::RegCreateKeyW);
template <typename CharT>
LSTATUS __stdcall RegCreateKeyShim(_In_ HKEY key, _In_opt_ const CharT* subKey, _Out_ PHKEY resultKey)
{
    auto entry = LogFunctionEntry();
    auto result = RegCreateKeyImpl(key, subKey, resultKey);

    auto functionResult = from_win32(result);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        Log("RegCreateKey:\n");
        LogKeyPath(key);
        if (subKey) LogString("Sub Key", subKey);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogWin32Error(result);
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_STRING_SHIM(RegCreateKeyImpl, RegCreateKeyShim);

auto RegCreateKeyExImpl = shims::detoured_string_function(&::RegCreateKeyExA, &::RegCreateKeyExW);
template <typename CharT>
LSTATUS __stdcall RegCreateKeyExShim(
    _In_ HKEY key,
    _In_ const CharT* subKey,
    _Reserved_ DWORD reserved,
    _In_opt_ CharT* classType,
    _In_ DWORD options,
    _In_ REGSAM samDesired,
    _In_opt_ CONST LPSECURITY_ATTRIBUTES securityAttributes,
    _Out_ PHKEY resultKey,
    _Out_opt_ LPDWORD disposition)
{
    auto entry = LogFunctionEntry();
    auto result = RegCreateKeyExImpl(key, subKey, reserved, classType, options, samDesired, securityAttributes, resultKey, disposition);

    auto functionResult = from_win32(result);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        Log("RegCreateKeyEx:\n");
        LogKeyPath(key);
        LogString("Sub Key", subKey);
        if (classType) LogString("Class", classType);
        LogRegKeyFlags(options);
        LogRegKeyAccess(samDesired);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogWin32Error(result);
        }
        else if (disposition)
        {
            LogRegKeyDisposition(*disposition);
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_STRING_SHIM(RegCreateKeyExImpl, RegCreateKeyExShim);

auto RegOpenKeyImpl = shims::detoured_string_function(&::RegOpenKeyA, &::RegOpenKeyW);
template <typename CharT>
LSTATUS __stdcall RegOpenKeyShim(_In_ HKEY key, _In_opt_ const CharT* subKey, _Out_ PHKEY resultKey)
{
    auto entry = LogFunctionEntry();
    auto result = RegOpenKeyImpl(key, subKey, resultKey);

    auto functionResult = from_win32(result);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        Log("RegOpenKey:\n");
        LogKeyPath(key);
        if (subKey) LogString("Sub Key", subKey);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogWin32Error(result);
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_STRING_SHIM(RegOpenKeyImpl, RegOpenKeyShim);

auto RegOpenKeyExImpl = shims::detoured_string_function(&::RegOpenKeyExA, &::RegOpenKeyExW);
template <typename CharT>
LSTATUS __stdcall RegOpenKeyExShim(
    _In_ HKEY key,
    _In_opt_ const CharT* subKey,
    _In_opt_ DWORD options,
    _In_ REGSAM samDesired,
    _Out_ PHKEY resultKey)
{
    auto entry = LogFunctionEntry();
    auto result = RegOpenKeyExImpl(key, subKey, options, samDesired, resultKey);

    auto functionResult = from_win32(result);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        Log("RegOpenKeyEx:\n");
        LogKeyPath(key);
        if (subKey) LogString("Sub Key", subKey);
        LogRegKeyFlags(options);
        LogRegKeyAccess(samDesired);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogWin32Error(result);
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_STRING_SHIM(RegOpenKeyExImpl, RegOpenKeyExShim);

auto RegGetValueImpl = shims::detoured_string_function(&::RegGetValueA, &::RegGetValueW);
template <typename CharT>
LSTATUS __stdcall RegGetValueShim(
    _In_ HKEY key,
    _In_opt_ const CharT* subKey,
    _In_opt_ const CharT* value,
    _In_ DWORD flags,
    _Out_opt_ LPDWORD type,
    _Out_writes_bytes_to_opt_(*dataSize, *data) PVOID data,
    _Inout_opt_ LPDWORD dataSize)
{
    auto entry = LogFunctionEntry();
    auto result = RegGetValueImpl(key, subKey, value, flags, type, data, dataSize);

    auto functionResult = from_win32(result);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        Log("RegGetValue:\n");
        LogKeyPath(key);
        if (subKey) LogString("Sub Key", subKey);
        if (value) LogString("Value", value);
        LogRegKeyQueryFlags(flags);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogWin32Error(result);
        }
        else if (type)
        {
            LogRegKeyType(*type);
            if (data && dataSize) LogRegValue<CharT>(*type, data, *dataSize);
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_STRING_SHIM(RegGetValueImpl, RegGetValueShim);

auto RegQueryValueImpl = shims::detoured_string_function(&::RegQueryValueA, &::RegQueryValueW);
template <typename CharT>
LSTATUS __stdcall RegQueryValueShim(
    _In_ HKEY key,
    _In_opt_ const CharT* subKey,
    _Out_writes_bytes_to_opt_(*dataSize, *dataSize) CharT* data,
    _Inout_opt_ PLONG dataSize)
{
    auto entry = LogFunctionEntry();
    auto result = RegQueryValueImpl(key, subKey, data, dataSize);

    auto functionResult = from_win32(result);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        Log("RegQueryValue:\n");
        LogKeyPath(key);
        if (subKey) LogString("Sub Key", subKey);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogWin32Error(result);
        }
        else if (data && dataSize)
        {
            LogCountedString("Data", data, *dataSize / sizeof(CharT));
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_STRING_SHIM(RegQueryValueImpl, RegQueryValueShim);

auto RegQueryValueExImpl = shims::detoured_string_function(&::RegQueryValueExA, &::RegQueryValueExW);
template <typename CharT>
LSTATUS __stdcall RegQueryValueExShim(
    _In_ HKEY key,
    _In_opt_ const CharT* valueName,
    _Reserved_ LPDWORD reserved,
    _Out_opt_ LPDWORD type,
    _Out_writes_bytes_to_opt_(*dataSize, *dataSize) LPBYTE data,
    _When_(data == NULL, _Out_opt_) _When_(data != NULL, _Inout_opt_) LPDWORD dataSize)
{
    auto entry = LogFunctionEntry();
    auto result = RegQueryValueExImpl(key, valueName, reserved, type, data, dataSize);

    auto functionResult = from_win32(result);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        Log("RegQueryValueEx:\n");
        LogKeyPath(key);
        if (valueName) LogString("Value Name", valueName);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogWin32Error(result);
        }
        else if (type)
        {
            LogRegKeyType(*type);
            if (data && dataSize) LogRegValue<CharT>(*type, data, *dataSize);
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_STRING_SHIM(RegQueryValueExImpl, RegQueryValueExShim);

auto RegSetKeyValueImpl = shims::detoured_string_function(&::RegSetKeyValueA, &::RegSetKeyValueW);
template <typename CharT>
LSTATUS __stdcall RegSetKeyValueShim(
    _In_ HKEY key,
    _In_opt_ const CharT* subKey,
    _In_opt_ const CharT* valueName,
    _In_ DWORD type,
    _In_reads_bytes_opt_(dataSize) LPCVOID data,
    _In_ DWORD dataSize)
{
    auto entry = LogFunctionEntry();
    auto result = RegSetKeyValueImpl(key, subKey, valueName, type, data, dataSize);

    auto functionResult = from_win32(result);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        Log("RegSetKeyValue:\n");
        LogKeyPath(key);
        if (subKey) LogString("Sub Key", subKey);
        if (valueName) LogString("Value Name", valueName);
        LogRegKeyType(type);
        if (data) LogRegValue<CharT>(type, data, dataSize);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogWin32Error(result);
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_STRING_SHIM(RegSetKeyValueImpl, RegSetKeyValueShim);

auto RegSetValueImpl = shims::detoured_string_function(&::RegSetValueA, &::RegSetValueW);
template <typename CharT>
LSTATUS __stdcall RegSetValueShim(
    _In_ HKEY key,
    _In_opt_ const CharT* subKey,
    _In_ DWORD type,
    _In_reads_bytes_opt_(dataSize) const CharT* data,
    _In_ DWORD dataSize)
{
    auto entry = LogFunctionEntry();
    auto result = RegSetValueImpl(key, subKey, type, data, dataSize);

    auto functionResult = from_win32(result);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        Log("RegSetValue:\n");
        LogKeyPath(key);
        if (subKey) LogString("Sub Key", subKey);
        LogRegKeyType(type); // NOTE: _Must_ be REG_SZ
        if (data) LogString("Data", data);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogWin32Error(result);
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_STRING_SHIM(RegSetValueImpl, RegSetValueShim);

auto RegSetValueExImpl = shims::detoured_string_function(&::RegSetValueExA, &::RegSetValueExW);
template <typename CharT>
LSTATUS __stdcall RegSetValueExShim(
    _In_ HKEY key,
    _In_opt_ const CharT* valueName,
    _Reserved_ DWORD reserved,
    _In_ DWORD type,
    _In_reads_bytes_opt_(dataSize) CONST BYTE* data,
    _In_ DWORD dataSize)
{
    auto entry = LogFunctionEntry();
    auto result = RegSetValueExImpl(key, valueName, reserved, type, data, dataSize);

    auto functionResult = from_win32(result);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        Log("RegSetValueEx:\n");
        LogKeyPath(key);
        if (valueName) LogString("Value Name", valueName);
        LogRegKeyType(type);
        if (data) LogRegValue<CharT>(type, data, dataSize);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogWin32Error(result);
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_STRING_SHIM(RegSetValueExImpl, RegSetValueExShim);

auto RegDeleteKeyImpl = shims::detoured_string_function(&::RegDeleteKeyA, &::RegDeleteKeyW);
template <typename CharT>
LSTATUS __stdcall RegDeleteKeyShim(_In_ HKEY key, _In_ const CharT* subKey)
{
    auto entry = LogFunctionEntry();
    auto result = RegDeleteKeyImpl(key, subKey);

    auto functionResult = from_win32(result);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        Log("RegDeleteKey:\n");
        LogKeyPath(key);
        LogString("Sub Key", subKey);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogWin32Error(result);
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_STRING_SHIM(RegDeleteKeyImpl, RegDeleteKeyShim);

auto RegDeleteKeyExImpl = shims::detoured_string_function(&::RegDeleteKeyExA, &::RegDeleteKeyExW);
template <typename CharT>
LSTATUS __stdcall RegDeleteKeyExShim(
    _In_ HKEY key,
    _In_ const CharT* subKey,
    _In_ REGSAM samDesired,
    _Reserved_ DWORD reserved)
{
    auto entry = LogFunctionEntry();
    auto result = RegDeleteKeyExImpl(key, subKey, samDesired, reserved);

    auto functionResult = from_win32(result);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        Log("RegDeleteKeyEx:\n");
        LogKeyPath(key);
        LogString("Sub Key", subKey);
        LogRegKeyAccess(samDesired);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogWin32Error(result);
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_STRING_SHIM(RegDeleteKeyExImpl, RegDeleteKeyExShim);

auto RegDeleteKeyValueImpl = shims::detoured_string_function(&::RegDeleteKeyValueA, &::RegDeleteKeyValueW);
template <typename CharT>
LSTATUS __stdcall RegDeleteKeyValueShim(_In_ HKEY key, _In_opt_ const CharT* subKey, _In_opt_ const CharT* valueName)
{
    auto entry = LogFunctionEntry();
    auto result = RegDeleteKeyValueImpl(key, subKey, valueName);

    auto functionResult = from_win32(result);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        Log("RegDeleteKeyValue:\n");
        LogKeyPath(key);
        if (subKey) LogString("Sub Key", subKey);
        if (valueName) LogString("Value Name", valueName);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogWin32Error(result);
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_STRING_SHIM(RegDeleteKeyValueImpl, RegDeleteKeyValueShim);

auto RegDeleteValueImpl = shims::detoured_string_function(&::RegDeleteValueA, &::RegDeleteValueW);
template <typename CharT>
LSTATUS __stdcall RegDeleteValueShim(_In_ HKEY key, _In_opt_ const CharT* valueName)
{
    auto entry = LogFunctionEntry();
    auto result = RegDeleteValueImpl(key, valueName);

    auto functionResult = from_win32(result);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        Log("RegDeleteValue:\n");
        LogKeyPath(key);
        if (valueName) LogString("Value Name", valueName);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogWin32Error(result);
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_STRING_SHIM(RegDeleteValueImpl, RegDeleteValueShim);

auto RegDeleteTreeImpl = shims::detoured_string_function(&::RegDeleteTreeA, &::RegDeleteTreeW);
template <typename CharT>
LSTATUS __stdcall RegDeleteTreeShim(_In_ HKEY key, _In_opt_ const CharT* subKey)
{
    auto entry = LogFunctionEntry();
    auto result = RegDeleteTreeImpl(key, subKey);

    auto functionResult = from_win32(result);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        Log("RegDeleteTree:\n");
        LogKeyPath(key);
        if (subKey) LogString("Sub Key", subKey);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogWin32Error(result);
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_STRING_SHIM(RegDeleteTreeImpl, RegDeleteTreeShim);

auto RegCopyTreeImpl = shims::detoured_string_function(&::RegCopyTreeA, &::RegCopyTreeW);
template <typename CharT>
LSTATUS __stdcall RegCopyTreeShim(_In_ HKEY keySrc, _In_opt_ const CharT* subKey, _In_ HKEY keyDest)
{
    auto entry = LogFunctionEntry();
    auto result = RegCopyTreeImpl(keySrc, subKey, keyDest);

    auto functionResult = from_win32(result);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        Log("RegCopyTree:\n");
        LogKeyPath(keySrc, "Source");
        if (subKey) LogString("Sub Key", subKey);
        LogKeyPath(keyDest, "Dest");
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogWin32Error(result);
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_STRING_SHIM(RegCopyTreeImpl, RegCopyTreeShim);

auto RegEnumKeyImpl = shims::detoured_string_function(&::RegEnumKeyA, &::RegEnumKeyW);
template <typename CharT>
LSTATUS __stdcall RegEnumKeyShim(
    _In_ HKEY key,
    _In_ DWORD index,
    _Out_writes_opt_(nameLength) CharT* name,
    _In_ DWORD nameLength)
{
    auto entry = LogFunctionEntry();
    auto result = RegEnumKeyImpl(key, index, name, nameLength);

    auto functionResult = from_win32(result);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        Log("RegEnumKey:\n");
        LogKeyPath(key);
        Log("\tIndex=%d\n", index);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogWin32Error(result);
        }
        else if (name)
        {
            LogString("Name", name);
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_STRING_SHIM(RegEnumKeyImpl, RegEnumKeyShim);

auto RegEnumKeyExImpl = shims::detoured_string_function(&::RegEnumKeyExA, &::RegEnumKeyExW);
template <typename CharT>
LSTATUS __stdcall RegEnumKeyExShim(
    _In_ HKEY key,
    _In_ DWORD index,
    _Out_writes_to_opt_(*nameLength, *nameLength + 1) CharT* name,
    _Inout_ LPDWORD nameLength,
    _Reserved_ LPDWORD reserved,
    _Out_writes_to_opt_(*classNameLength, *classNameLength + 1) CharT* className,
    _Inout_opt_ LPDWORD classNameLength,
    _Out_opt_ PFILETIME lastWriteTime)
{
    auto entry = LogFunctionEntry();
    auto result = RegEnumKeyExImpl(key, index, name, nameLength, reserved, className, classNameLength, lastWriteTime);

    auto functionResult = from_win32(result);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        Log("RegEnumKeyEx:\n");
        LogKeyPath(key);
        Log("\tIndex=%d\n", index);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogWin32Error(result);
        }
        else
        {
            if (name) LogCountedString("Name", name, *nameLength);
            if (className && classNameLength) LogCountedString("Class", className, *classNameLength);
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_STRING_SHIM(RegEnumKeyExImpl, RegEnumKeyExShim);

auto RegEnumValueImpl = shims::detoured_string_function(&::RegEnumValueA, &::RegEnumValueW);
template <typename CharT>
LSTATUS __stdcall RegEnumValueShim(
    _In_ HKEY key,
    _In_ DWORD index,
    _Out_writes_to_opt_(*valueNameLength, *valueNameLength + 1) CharT* valueName,
    _Inout_ LPDWORD valueNameLength,
    _Reserved_ LPDWORD reserved,
    _Out_opt_ LPDWORD type,
    _Out_writes_bytes_to_opt_(*dataSize, *dataSize) __out_data_source(REGISTRY) LPBYTE data,
    _Inout_opt_ LPDWORD dataSize)
{
    auto entry = LogFunctionEntry();
    auto result = RegEnumValueImpl(key, index, valueName, valueNameLength, reserved, type, data, dataSize);

    auto functionResult = from_win32(result);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        Log("RegEnumValue:\n");
        LogKeyPath(key);
        Log("\tIndex=%d\n", index);
        LogFunctionResult(functionResult);
        if (function_failed(functionResult))
        {
            LogWin32Error(result);
        }
        else
        {
            if (valueName) LogCountedString("Value Name", valueName, *valueNameLength);
            if (type) LogRegKeyType(*type);
            if (type && data && dataSize) LogRegValue<CharT>(*type, data, *dataSize);
        }
        LogCallingModule();
    }

    return result;
}
DECLARE_STRING_SHIM(RegEnumValueImpl, RegEnumValueShim);

// NOTE: The following is a list of functions taken from https://msdn.microsoft.com/en-us/library/windows/desktop/ms724875(v=vs.85).aspx
//       that are _not_ present above. This is just a convenient collection of what's missing; it is not a collection of
//       future work.
//
// The following deal with key/value strings:
//      RegCreateKeyTransacted
//      RegDeleteKeyTransacted
//      RegLoadKey                  -- NOTE: File path
//      RegLoadMUIString
//      RegOpenKeyTransacted
//      RegQueryMultipleValues
//      RegReplaceKey               -- NOTE: File paths
//      RegUnLoadKey
//
// The following exclusively deal with HKEYs
//      RegCloseKey
//      RegConnectRegistry
//      RegDisableReflectionKey
//      RegEnableReflectionKey
//      RegFlushKey
//      RegGetKeySecurity
//      RegLoadAppKey               -- NOTE: File path
//      RegNotifyChangeKeyValue
//      RegOpenCurrentUser
//      RegOpenUserClassesRoot
//      RegOverridePredefKey
//      RegQueryInfoKey
//      RegQueryReflectionKey
//      RegRestoreKey               -- NOTE: File path
//      RegSaveKey                  -- NOTE: File path
//      RegSaveKeyEx                -- NOTE: File path
//      RegSetKeySecurity
