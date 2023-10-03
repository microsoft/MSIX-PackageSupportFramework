//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <cassert>
#include <cstdio>
#include <cwchar>
#include <mutex>
#include <string>

#include <intrin.h>
#include <windows.h>
#include <lzexpand.h>
#include <winternl.h>

#include <psf_utils.h>

#include "Config.h"

// Conditionally define flags introduced after RS1 (14393) SDK
#ifndef FILE_ATTRIBUTE_PINNED
#define FILE_ATTRIBUTE_PINNED               0x00080000
#endif
#ifndef FILE_ATTRIBUTE_UNPINNED
#define FILE_ATTRIBUTE_UNPINNED             0x00100000
#endif
#ifndef FILE_ATTRIBUTE_RECALL_ON_OPEN
#define FILE_ATTRIBUTE_RECALL_ON_OPEN       0x00040000
#endif
#ifndef FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS
#define FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS 0x00400000
#endif
#ifndef FILE_ATTRIBUTE_STRICTLY_SEQUENTIAL
#define FILE_ATTRIBUTE_STRICTLY_SEQUENTIAL  0x20000000
#endif
#ifndef SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
#define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE    0x2
#endif
#ifndef REG_OPTION_DONT_VIRTUALIZE
#define REG_OPTION_DONT_VIRTUALIZE  0x00000010L
#endif
#ifndef LOAD_LIBRARY_OS_INTEGRITY_CONTINUITY
#define LOAD_LIBRARY_OS_INTEGRITY_CONTINUITY   0x00008000
#endif


// Since consumers typically call 'Log' multiple times, make sure that we synchronize that output
inline std::recursive_mutex g_outputMutex;

// Logs output using a printf-like format
inline void Log(const char* fmt, ...)
{
    if (output_method == trace_method::printf)
    {
        va_list args;
        va_start(args, fmt);
        std::vprintf(fmt, args);
        va_end(args);
    }
    else // trace_method::output_debug_string
    {
        std::string str;
        str.resize(256);

        va_list args;
        va_start(args, fmt);
        std::size_t count = std::vsnprintf(str.data(), str.size() + 1, fmt, args);
        assert(count >= 0);
        va_end(args);

        if (count > str.size())
        {
            count = 1024;       // vswprintf actually returns a negative number, let's just go with something big enough for our long strings; it is resized shortly.
            str.resize(count);

            va_list args2;
            va_start(args2, fmt);
            count = std::vsnprintf(str.data(), str.size() + 1, fmt, args2);
            assert(count >= 0);
            va_end(args2);
        }

        str.resize(count);

        ::OutputDebugStringA(str.c_str());
    }
}

inline void Log(const wchar_t* fmt, ...)
{
    if (output_method == trace_method::printf)
    {
        va_list args;
        va_start(args, fmt);
        std::vwprintf(fmt, args);
        va_end(args);
    }
    else // trace_method::output_debug_string
    {
        try
        {
            va_list args;
            va_start(args, fmt);

            std::wstring wstr;
            wstr.resize(256);
            std::size_t count = std::vswprintf(wstr.data(), wstr.size() + 1, fmt, args);
            va_end(args);
            assert(count >= 0);

            if (count > wstr.size())
            {
                count = 1024;       // vswprintf actually returns a negative number, let's just go with something big enough for our long strings; it is resized shortly.
                wstr.resize(count);
                va_list args2;
                va_start(args2, fmt);
                count = std::vswprintf(wstr.data(), wstr.size() + 1, fmt, args2);
                va_end(args2);
                assert(count >= 0);
            }
            wstr.resize(count);
            ::OutputDebugStringW(wstr.c_str());
        }
        catch (...)
        {
            ::OutputDebugStringA("Exception in wide Log()");
        }
    }

}

inline void LogString(const char* name, const char* value)
{
    Log("\t%s=%s\n", name, value);
}

inline void LogString(const char* name, const wchar_t* value)
{
    Log("\t%s=%ls\n", name, value);
}

inline std::string InterpretStringA(const char* value)
{
    if (value != NULL)
    {
        return value;
    }
    return "";
}

inline std::string InterpretStringA(const wchar_t* value)
{
    if (value != NULL)
    {
        return narrow(value);
    }
    return "";
}

inline void LogCountedString(const char* name, const char* value, std::size_t length)
{
    Log("\t%s=%.*s\n", name, length, value);
}

inline std::string InterpretCountedString(const char* name, const char* value, std::size_t length)
{
    std::ostringstream sout;
    if (value != NULL)
    {
        sout << name << "=" << std::setw(length) << value;
    }
    else
    {
        sout << name << "=NULL";
    }
    return sout.str();
}

inline void LogCountedString(const char* name, const wchar_t* value, std::size_t length)
{
    if (value != NULL)
    {
        Log("\t%s=%.*ls\n", name, length, value);
    }
    else
    {
        Log("\t%s=NULL", name);
    }
}

inline std::string InterpretCountedString(const char* name, const wchar_t* value, std::size_t length)
{
    std::ostringstream sout;
    if (value != NULL)
    {
        sout << name << "=" << std::setw(length) << narrow(value).c_str();
    }
    else
    {
        sout << name << "=NULL";
    }
    return sout.str();
}

inline std::string InterpretAsHex(const char* name, WORD value)
{
    std::ostringstream sout2;
    sout2 << name << "=0x" << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << value;
    return sout2.str();
}

inline std::string InterpretAsHex(const char* name, DWORD value)
{
    std::ostringstream sout;
    if (strlen(name) > 0)
    {
        sout << name << "=";
    }
    sout << "0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << value;
    return sout.str();
}

inline std::string InterpretAsHex(const char* name, UINT value)
{
    std::ostringstream sout;
    if (strlen(name) > 0)
    {
        sout << name << "=";
    }
    sout << "0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << value;
    return sout.str();
}

inline std::string InterpretAsHex(const char* name, INT value)
{
    std::ostringstream sout;
    if (strlen(name) > 0)
    {
        sout << name << "=";
    }
    sout << "0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << value;
    return sout.str();
}

inline std::string InterpretAsHex(const char* name, ULONGLONG value)
{
    std::ostringstream sout;
    if (strlen(name) > 0)
    {
        sout << name << "=";
    }
    sout << "0x" << std::uppercase << std::setfill('0') << std::setw(16) << std::hex << value;
    return sout.str();
}

inline std::string InterpretAsHex(const char* name, PHANDLE phandle)
{
    std::ostringstream sout2;
    if (phandle != nullptr && *phandle != nullptr)
    {
        sout2 << name << "= 0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << static_cast<const UINT*>(*phandle);
    }
    else
    {
        sout2 << name << "= 0x0";
    }
    return sout2.str();
}

inline std::string InterpretAsHex(const char* name, HANDLE handle)
{
    std::ostringstream sout;
    if (strlen(name) > 0)
    {
        sout << name << "=";
    }
    if (handle != nullptr)
    {
        //sout << "0x" << std::uppercase << std::setfill('0') << std::setw(16) << std::hex << *reinterpret_cast<const std::int64_t*>(handle);
        sout << "0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << static_cast<const UINT*>(handle);
    }
    else
    {
        sout << "0x0";
    }
    return sout.str();
}

inline constexpr const char* bool_to_string(BOOL value) noexcept
{
    return value ? "true" : "false";
}

inline constexpr const wchar_t* bool_to_wstring(BOOL value) noexcept
{
    return value ? L"true" : L"false";
}

inline void LogBool(const char* msg, BOOL value)
{
    Log("\t%s=%s\n", msg, bool_to_string(value));
}

inline std::string InterpretBool(const char* msg, BOOL value)
{
    std::ostringstream sout;
    sout << msg << bool_to_string(value);
    return sout.str();
}

inline const char * InterperetFunctionResult(function_result result)
{
    const char* resultMsg = "Unknown";
    switch (result)
    {
    case function_result::success:
        resultMsg = "Success";
        break;

    case function_result::indeterminate:
        resultMsg = "Indeterminate";
        break;

    case function_result::expected_failure:
        resultMsg = "Expected Failure";
        break;

    case function_result::failure:
        resultMsg = "Failure";
        break;
    }
    return resultMsg;
}

inline void LogFunctionResult(function_result result, const char* msg = "Result")
{
    const char* resultMsg = InterperetFunctionResult(result);

    Log("\t%s=%s\n", msg, resultMsg);
}

inline std::string InterpretReturn(function_result functionResult, DWORD resultcode)
{
    std::string results = InterperetFunctionResult(functionResult);
    
    results += "\n(";
    results += InterpretFrom_win32(resultcode);
    results += ")";
    results += "-" + std::to_string(resultcode);
    return results;
}

inline std::string InterpretReturn(function_result functionResult, BOOL bresultcode)
{
    std::string results = InterperetFunctionResult(functionResult);

    results += "\n(";	
    results += bresultcode ? "Success" : InterpretFrom_win32(GetLastError());
    results += ")";
    results += "-" + std::to_string(bresultcode);
    return results;
}

inline std::string InterpretReturn(function_result functionResult, HRESULT hr)
{
    std::string results = InterperetFunctionResult(functionResult);

    results += "\n(";
    if (SUCCEEDED(hr))
    {
        results += "Success";
    }
    else if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
    {
        results += InterpretFrom_win32(HRESULT_CODE(hr));
    }
    results += ")";
    results += "-" + std::to_string(hr);
    return results;
}

inline std::string InterpretReturn(function_result functionResult, HANDLE hand)
{
    std::string results = InterperetFunctionResult(functionResult);

    results += "\n(";
    if (hand == INVALID_HANDLE_VALUE)
    {
        results += "INVALID_HANDLE_VALUE";
    }
    else
    {
        results += InterpretFrom_win32(GetLastError());
    }
    results += ")";
    results += "-" + std::to_string(GetLastError());
    return results;
}

// Error logging
inline std::string win32_error_description(DWORD error)
{
    // std::system_category represents Win32 error values, so leverage it for textual descriptions. Unfortunately, it
    // perpetuates the issue where the last character is always a new line character, so remove it
    auto str = std::system_category().message(error);
    auto remove_last_if = [&](char ch) { if (!str.empty() && (str.back() == ch)) str.pop_back(); };
    remove_last_if('\n');
    remove_last_if('\r');
    remove_last_if('.');

    return str;
}

inline void LogWin32Error(DWORD error, const char* msg = "Error")
{
    auto str = win32_error_description(error);
    Log("\t%s=%d (%s)\n", msg, error, str.c_str());
}

inline std::string InterpretWin32Error(DWORD error, const char* msg = "Error")
{
    return InterpretAsHex(msg, error);
}

inline void LogLastError(const char* msg = "Last Error")
{
    LogWin32Error(::GetLastError(), msg);
}

inline std::string InterpretLastError(const char* msg = "Last Error")
{
    DWORD err = ::GetLastError();
    return InterpretFrom_win32(err)  + "\n" + InterpretWin32Error(err, msg);
}

inline void LogHResult(HRESULT hr)
{
    // NOTE: HRESULT and Win32 errors have non-ambiguous values and the API to translate from error code to description
    //       is the same for both, hence why this works out okay
    auto str = win32_error_description(hr);
    Log("\tHResult=%08X (%s)\n", hr, str.c_str());
}


inline void LogNTStatus(NTSTATUS status)
{
    static auto ntdllModule = ::LoadLibraryW(L"ntdll.dll");

    PWSTR messageBuffer;
    [[maybe_unused]] auto result = ::FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE,
        ntdllModule,
        status,
        0,
        reinterpret_cast<PWSTR>(&messageBuffer),
        0,
        nullptr);
    assert(result);

    // FormatMessage sometimes has a very annoying format... Just take what's between the {...}, if present
    std::wstring_view msg = messageBuffer;
    if (auto pos = msg.find('{'); pos != std::wstring_view::npos)
    {
        msg.remove_prefix(pos + 1);
        pos = msg.find('}');
        msg = msg.substr(0, pos);
    }

    // Remove any trailing new lines or periods since they look bad
    while (!msg.empty() && ((msg.back() == L'\r') || (msg.back() == L'\n') || (msg.back() == L'.')))
    {
        msg.remove_suffix(1);
    }

    Log("\tStatus=%08X (%.*ls)\n", status, msg.length(), msg.data());
    ::LocalFree(messageBuffer);
}

inline std::string InterpretNTStatus(NTSTATUS status)
{
    static auto ntdllModule = ::LoadLibraryW(L"ntdll.dll");

    PWSTR messageBuffer;
    [[maybe_unused]] auto result = ::FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE,
        ntdllModule,
        status,
        0,
        reinterpret_cast<PWSTR>(&messageBuffer),
        0,
        nullptr);
    assert(result);

    // FormatMessage sometimes has a very annoying format... Just take what's between the {...}, if present
    std::wstring_view msg = messageBuffer;
    if (auto pos = msg.find('{'); pos != std::wstring_view::npos)
    {
        msg.remove_prefix(pos + 1);
        pos = msg.find('}');
        if (pos != std::wstring_view::npos)
        {
            msg = msg.substr(0, pos - 1);
        }
    }
    if (auto pos2 = msg.find('}'); pos2 != std::wstring_view::npos)  // sometimes there is only the trailer }????
    {
        msg = msg.substr(0, pos2 - 1);
    }

    // Remove any trailing new lines or periods since they look bad
    while (!msg.empty() && ((msg.back() == L'\r') || (msg.back() == L'\n') || (msg.back() == L'.')))
    {
        msg.remove_suffix(1);
    }

    std::ostringstream sout;
    sout << InterpretCountedString("Status", msg.data(), msg.length());
    ::LocalFree(messageBuffer);
    return sout.str();
}

inline std::string InterpretReturnNT(function_result functionResult, NTSTATUS ntresultcode)
{
    std::string results = InterperetFunctionResult(functionResult);

    results += "\n";
    results += InterpretNTStatus(ntresultcode);
    return results;
}


inline const char* GetLZError(INT err)
{
    const char* msg = "Unknown";
    switch (err)
    {
    case LZERROR_BADINHANDLE:
        msg = "Invalid input handle";
        break;

    case LZERROR_BADOUTHANDLE:
        msg = "Invalid output handle";
        break;

    case LZERROR_READ:
        msg = "Corrupt compressed file format";
        break;

    case LZERROR_WRITE:
        msg = "Out of space for output file";
        break;

    case LZERROR_GLOBALLOC:
        msg = "Insufficient memory for LZFile struct";
        break;

    case LZERROR_GLOBLOCK:
        msg = "Bad global handle";
        break;

    case LZERROR_BADVALUE:
        msg = "Input parameter out of acceptable rang";
        break;

    case LZERROR_UNKNOWNALG:
        msg = "Compression algorithm not recognized";
        break;
    }
    return msg;
}

inline void LogLZError(INT err)
{
    Log("\tLZError=%d (%s)\n", err, GetLZError(err));
}

inline std::string InterpretLZError(INT err)
{
    return InterpretAsHex("Error", (DWORD)err) + GetLZError(err);
}

#define LogCallingModule() \
    if (trace_calling_module) \
    { \
        HMODULE moduleHandle; \
        if (::GetModuleHandleExW( \
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, \
            reinterpret_cast<const wchar_t*>(_ReturnAddress()), \
            &moduleHandle)) \
        { \
            Log("\tCalling Module=%ls\n", psf::get_module_path(moduleHandle).c_str()); \
        } \
    }

// The replacement for LogCallingModule for the ETW case is currently this three part call like this:
    //std::ostringstream sout;
    //InterpretCallingModulePart1()
    //sout << "Calling Module=" << InterpretCallingModulePart2()
    //InterpretCallingModulePart3()
    //std::string cm = sout.str();
// Maybe someone can figure out how to modify "InterpertCallingModule" to eliminate this.
#define InterpretCallingModulePart1() \
    if (trace_calling_module) \
    { \
        HMODULE moduleHandle; \
        if (::GetModuleHandleExW( \
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, \
            reinterpret_cast<const wchar_t*>(_ReturnAddress()), \
            &moduleHandle)) \
        { 

#define InterpretCallingModulePart2() \
     psf::get_module_path(moduleHandle).generic_string();

#define InterpretCallingModulePart3() \
        } \
    }

// As written, this would return the shim as the calling module.  So we can't use a function call,
// unless someone has a better idea.
//inline std::string InterpretCallingModule(const char *msg = "Calling Module=")
//{
//    std::ostringstream sout;
//    if (trace_calling_module)
//    {
//        HMODULE moduleHandle;
//        if (::GetModuleHandleExW(
//            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
//            reinterpret_cast<const wchar_t*>(_ReturnAddress()),
//            &moduleHandle))
//        {
//            sout << msg << psf::get_module_path(moduleHandle).generic_string();
//        }
//    }
//    return sout.str();
//}

// RAII type to acquire/release the output lock that also tracks/exposes whether or not the function result should be logged
struct output_lock
{
    // Don't let function calls made while processing output cause more output. This is effectively a "were we the first
    // to acquire the lock" check
    static inline bool processing_output = false;

    output_lock(function_type type, function_result result)
    {
        g_outputMutex.lock();
        m_inhibitOutput = std::exchange(processing_output, true);

        auto [shouldLog, shouldBreak] = configured_result(type, result);
        m_shouldLog = !m_inhibitOutput && shouldLog;
        if (shouldBreak)
        {
            ::DebugBreak();
        }
    }

    ~output_lock()
    {
        processing_output = m_inhibitOutput;
        g_outputMutex.unlock();
    }

    explicit operator bool() const noexcept
    {
        return m_shouldLog;
    }

private:

    bool m_inhibitOutput;
    bool m_shouldLog;
};

inline output_lock acquire_output_lock(function_type type, function_result result)
{
    return output_lock(type, result);
}

// RAII helper for handling the 'traceFunctionEntry' configuration
struct function_entry_tracker
{
    // Used for printing function entry/exit separators
    static inline thread_local std::size_t function_call_depth = 0;

    function_entry_tracker(const char* functionName)
    {
        if (trace_function_entry)
        {
            std::lock_guard<std::recursive_mutex> lock(g_outputMutex);
            if (!output_lock::processing_output)
            {
                if (++function_call_depth == 1)
                {
                    Log("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n");
                }

                // Most functions are named "SomeFunctionFixup" where the target API is "SomeFunction". Logging the API
                // is much more helpful than the Fixup function name, so remove the "Fixup" suffix, if present
                using namespace std::literals;
                constexpr auto fixupSuffix = "Fixup"sv;
                std::string name = functionName;
                if ((name.length() >= fixupSuffix.length()) && (name.substr(name.length() - fixupSuffix.length()) == fixupSuffix))
                {
                    name.resize(name.length() - fixupSuffix.length());
                }

                Log("Function Entry: %s\n", name.c_str());
            }
        }
    }

    ~function_entry_tracker()
    {
        if (trace_function_entry)
        {
            std::lock_guard<std::recursive_mutex> lock(g_outputMutex);
            if (!output_lock::processing_output)
            {
                if (--function_call_depth == 0)
                {
                    Log("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
                }
            }
        }
    }
};
#define LogFunctionEntry() function_entry_tracker{ __FUNCTION__ }

// Logging functions for enums, flags, and other defines
template <typename T, typename U>
inline constexpr bool IsFlagSet(T value, U flag)
{
    return static_cast<U>(value & flag) == flag;
}

#define LogIfFlagSetMsg(value, flag, msg) \
    if (IsFlagSet(value, flag)) \
    { \
        Log("%s%s", prefix, msg); \
        prefix = " | "; \
    }

#define LogIfFlagSet(value, flag) \
    LogIfFlagSetMsg(value, flag, #flag)

#define LogIfEqual(value, expected) \
    if (value == expected) \
    { \
        Log(#expected); \
    }

template <typename T, typename U>
inline constexpr bool IsFlagEqual(T value, U flag)
{
    return static_cast<T>(flag) == value;
}

inline void LogCommonAccess(ACCESS_MASK access, const char*& prefix)
{
    // Standard Rights (bits 16-23)
    LogIfFlagSet(access, DELETE);
    LogIfFlagSet(access, READ_CONTROL);
    LogIfFlagSet(access, WRITE_DAC);
    LogIfFlagSet(access, WRITE_OWNER);
    LogIfFlagSet(access, SYNCHRONIZE);

    // Access System Security (bit 24)
    LogIfFlagSet(access, ACCESS_SYSTEM_SECURITY);

    // Maximum Allowed (bit 25)
    LogIfFlagSet(access, MAXIMUM_ALLOWED);

    // NOTE: Bits 26-27 are reserved

    // Generic Rights (bits 28-31)
    LogIfFlagSet(access, GENERIC_ALL);
    LogIfFlagSet(access, GENERIC_EXECUTE);
    LogIfFlagSet(access, GENERIC_READ);
    LogIfFlagSet(access, GENERIC_WRITE);
}

inline std::string InterpretCommonAccess(ACCESS_MASK access, const char*& prefix)
{
    std::ostringstream sout;
    // Standard Rights (bits 16-23)
    if (IsFlagSet(access, DELETE))
    {
        sout << prefix << "DELETE";
        prefix = " | ";
    }
    if (IsFlagSet(access, READ_CONTROL))
    {
        sout << prefix << "READ_CONTROL";
        prefix = " | ";
    }
    if (IsFlagSet(access, WRITE_DAC))
    {
        sout << prefix << "WRITE_DAC";
        prefix = " | ";
    }
    if (IsFlagSet(access, WRITE_OWNER))
    {
        sout << prefix << "WRITE_OWNER";
        prefix = " | ";
    }
    if (IsFlagSet(access, SYNCHRONIZE))
    {
        sout << prefix << "SYNCHRONIZE";
        prefix = " | ";
    }

    // Access System Security (bit 24)
    if (IsFlagSet(access, ACCESS_SYSTEM_SECURITY))
    {
        sout << prefix << "ACCESS_SYSTEM_SECURITY";
        prefix = " | ";
    }

    // Maximum Allowed (bit 25)
    if (IsFlagSet(access, MAXIMUM_ALLOWED))
    {
        sout << prefix << "MAXIMUM_ALLOWED";
        prefix = " | ";
    }

    // NOTE: Bits 26-27 are reserved

    // Generic Rights (bits 28-31)
    if (IsFlagSet(access, GENERIC_ALL))
    {
        sout << prefix << "GENERIC_ALL";
        prefix = " | ";
    }
    if (IsFlagSet(access, GENERIC_EXECUTE))
    {
        sout << prefix << "GENERIC_EXECUTE";
        prefix = " | ";
    }
    if (IsFlagSet(access, GENERIC_READ))
    {
        sout << prefix << "GENERIC_READ";
        prefix = " | ";
    }
    if (IsFlagSet(access, GENERIC_WRITE))
    {
        sout << prefix << "GENERIC_WRITE";
        prefix = " | ";
    }
    return sout.str();
}

inline void LogFileAccess(DWORD access, const char* msg = "Access")
{
    Log("\t%s=%08X", msg, access);
    if (access)
    {
        const char* prefix = "";
        Log(" (");

        // Specific Rights (bits 0-15)
        LogIfFlagSet(access, FILE_READ_DATA);            // 0x00000001 - file & pipe
        LogIfFlagSet(access, FILE_WRITE_DATA);           // 0x00000002 - file & pipe
        LogIfFlagSet(access, FILE_APPEND_DATA);          // 0x00000004 - file
        LogIfFlagSet(access, FILE_READ_EA);              // 0x00000008 - file & directory
        LogIfFlagSet(access, FILE_WRITE_EA);             // 0x00000010 - file & directory
        LogIfFlagSet(access, FILE_EXECUTE);              // 0x00000020 - file
        LogIfFlagSet(access, FILE_READ_ATTRIBUTES);      // 0x00000080 - all
        LogIfFlagSet(access, FILE_WRITE_ATTRIBUTES);     // 0x00000100 - all

        LogCommonAccess(access, prefix);

        Log(")");
    }

    Log("\n");
}

inline std::string InterpretFileAccess(DWORD access, const char* msg = "Access")
{
    std::ostringstream sout;
    sout << InterpretAsHex(msg, access);
    if (access)
    {
        sout << " (";
        const char* prefix = "";

        // Specific Rights (bits 0-15)
        if (IsFlagSet(access, FILE_READ_DATA))
        {
            sout << prefix << "FILE_READ_DATA";
            prefix = " | ";
        }
        if (IsFlagSet(access, FILE_WRITE_DATA))
        {
            sout << prefix << "FILE_WRITE_DATA";
            prefix = " | ";
        }
        if (IsFlagSet(access, FILE_APPEND_DATA))
        {
            sout << prefix << "FILE_APPEND_DATA";
            prefix = " | ";
        }
        if (IsFlagSet(access, FILE_READ_EA))
        {
            sout << prefix << "FILE_READ_EA";
            prefix = " | ";
        }
        if (IsFlagSet(access, FILE_WRITE_EA))
        {
            sout << prefix << "FILE_WRITE_EA";
            prefix = " | ";
        }
        if (IsFlagSet(access, FILE_EXECUTE))
        {
            sout << prefix << "FILE_EXECUTE";
            prefix = " | ";
        }
        if (IsFlagSet(access, FILE_READ_ATTRIBUTES))
        {
            sout << prefix << "FILE_READ_ATTRIBUTES";
            prefix = " | ";
        }
        if (IsFlagSet(access, FILE_WRITE_ATTRIBUTES))
        {
            sout << prefix << "FILE_WRITE_ATTRIBUTES";
            prefix = " | ";
        }

        sout <<  InterpretCommonAccess(access, prefix);
        sout << ")";

    }

    return sout.str();
}

inline void LogDirectoryAccess(DWORD access, const char* msg = "Access")
{
    Log("\t%s=%08X", msg, access);
    if (access)
    {
        const char* prefix = "";
        Log(" (");

        // Specific Rights (bits 0-15)
        LogIfFlagSet(access, FILE_LIST_DIRECTORY);       // 0x00000001 - directory
        LogIfFlagSet(access, FILE_ADD_FILE);             // 0x00000002 - directory
        LogIfFlagSet(access, FILE_ADD_SUBDIRECTORY);     // 0x00000004 - directory
        LogIfFlagSet(access, FILE_READ_EA);              // 0x00000008 - file & directory
        LogIfFlagSet(access, FILE_WRITE_EA);             // 0x00000010 - file & directory
        LogIfFlagSet(access, FILE_TRAVERSE);             // 0x00000020 - directory
        LogIfFlagSet(access, FILE_DELETE_CHILD);         // 0x00000040 - directory
        LogIfFlagSet(access, FILE_READ_ATTRIBUTES);      // 0x00000080 - all
        LogIfFlagSet(access, FILE_WRITE_ATTRIBUTES);     // 0x00000100 - all

        LogCommonAccess(access, prefix);

        Log(")");
    }

    Log("\n");
}

inline std::string InterpretDirectoryAccess(DWORD access, const char* msg = "Access")
{
    std::ostringstream sout;
    sout << InterpretAsHex(msg, access);
    if (access)
    {
        sout << "(";
        const char* prefix = "";

        // Specific Rights (bits 0-15)
        if (IsFlagSet(access, FILE_LIST_DIRECTORY))
        {
            sout << prefix << "FILE_LIST_DIRECTORY";
            prefix = " | ";
        }
        if (IsFlagSet(access, FILE_ADD_FILE))
        {
            sout << prefix << "FILE_ADD_FILE";
            prefix = " | ";
        }
        if (IsFlagSet(access, FILE_ADD_SUBDIRECTORY))
        {
            sout << prefix << "FILE_ADD_SUBDIRECTORY";
            prefix = " | ";
        }
        if (IsFlagSet(access, FILE_READ_EA))
        {
            sout << prefix << "FILE_READ_EA";
            prefix = " | ";
        }
        if (IsFlagSet(access, FILE_WRITE_EA))
        {
            sout << prefix << "FILE_WRITE_EA";
            prefix = " | ";
        }
        if (IsFlagSet(access, FILE_TRAVERSE))
        {
            sout << prefix << "FILE_TRAVERSE";
            prefix = " | ";
        }
        if (IsFlagSet(access, FILE_DELETE_CHILD))
        {
            sout << prefix << "FILE_DELETE_CHILD";
            prefix = " | ";
        }
        if (IsFlagSet(access, FILE_READ_ATTRIBUTES))
        {
            sout << prefix << "FILE_READ_ATTRIBUTES";
            prefix = " | ";
        }
        if (IsFlagSet(access, FILE_WRITE_ATTRIBUTES))
        {
            sout << prefix << "FILE_WRITE_ATTRIBUTES";
            prefix = " | ";
        }

        sout <<  InterpretCommonAccess(access, prefix);
        sout << ")";
        
    }

    return sout.str();
}

inline void LogPipeAccess(DWORD access, const char* msg = "Access")
{
    Log("\t%s=%08X", msg, access);
    if (access)
    {
        const char* prefix = "";
        Log(" (");

        // Specific Rights (bits 0-15)
        LogIfFlagSet(access, FILE_READ_DATA);            // 0x00000001 - file & pipe
        LogIfFlagSet(access, FILE_WRITE_DATA);           // 0x00000002 - file & pipe
        LogIfFlagSet(access, FILE_CREATE_PIPE_INSTANCE); // 0x00000004 - named pipe
        LogIfFlagSet(access, FILE_READ_ATTRIBUTES);      // 0x00000080 - all
        LogIfFlagSet(access, FILE_WRITE_ATTRIBUTES);     // 0x00000100 - all

        LogCommonAccess(access, prefix);

        Log(")");
    }

    Log("\n");
}

inline std::string InterpretPipeAccess(DWORD access, const char* msg = "Access")
{
    std::ostringstream sout;
    sout << InterpretAsHex(msg,access);

    if (access)
    {
        sout << "(";
        const char* prefix = "";

        // Specific Rights (bits 0-15)
        if (IsFlagSet(access, FILE_READ_DATA))
        {
            sout << prefix << "FILE_READ_DATA";
            prefix = " | ";
        }
        if (IsFlagSet(access, FILE_WRITE_DATA))
        {
            sout << prefix << "FILE_WRITE_DATA";
            prefix = " | ";
        }
        if (IsFlagSet(access, FILE_CREATE_PIPE_INSTANCE))
        {
            sout << prefix << "FILE_CREATE_PIPE_INSTANCE";
            prefix = " | ";
        }
        if (IsFlagSet(access, FILE_READ_ATTRIBUTES))
        {
            sout << prefix << "FILE_READ_ATTRIBUTES";
            prefix = " | ";
        }
        if (IsFlagSet(access, FILE_WRITE_ATTRIBUTES))
        {
            sout << prefix << "FILE_WRITE_ATTRIBUTES";
            prefix = " | ";
        }

        sout << prefix << InterpretCommonAccess(access, prefix);
        sout << ")";

    }

    return sout.str();
}

inline void LogRegKeyAccess(DWORD access, const char* msg = "Access")
{
    Log("\t%s=%08X", msg, access);
    if (access)
    {
        const char* prefix = "";
        Log(" (");

        // Specific Rights (bits 0-15)
        LogIfFlagSet(access, KEY_QUERY_VALUE);          // 0x0001
        LogIfFlagSet(access, KEY_SET_VALUE);            // 0x0002
        LogIfFlagSet(access, KEY_CREATE_SUB_KEY);       // 0x0004
        LogIfFlagSet(access, KEY_ENUMERATE_SUB_KEYS);   // 0x0008
        LogIfFlagSet(access, KEY_NOTIFY);               // 0x0010
        LogIfFlagSet(access, KEY_CREATE_LINK);          // 0x0020

        LogCommonAccess(access, prefix);

        Log(")");
    }

    Log("\n");
}

inline std::string InterpretRegKeyAccess(DWORD access, const char* msg = "Access")
{
    std::ostringstream sout;
    sout << InterpretAsHex(msg,access);

    if (access)
    {
        sout << " (";
        const char* prefix = "";

        // Specific Rights (bits 0-15)
        if (IsFlagSet(access, KEY_QUERY_VALUE))
        {
            sout << prefix << "KEY_QUERY_VALUE";
            prefix = " | ";
        }
        if (IsFlagSet(access, KEY_SET_VALUE))
        {
            sout << prefix << "KEY_SET_VALUE";
            prefix = " | ";
        }
        if (IsFlagSet(access, KEY_CREATE_SUB_KEY))
        {
            sout << prefix << "KEY_CREATE_SUB_KEY";
            prefix = " | ";
        }
        if (IsFlagSet(access, KEY_ENUMERATE_SUB_KEYS))
        {
            sout << prefix << "KEY_ENUMERATE_SUB_KEYS";
            prefix = " | ";
        }
        if (IsFlagSet(access, KEY_NOTIFY))
        {
            sout << prefix << "KEY_NOTIFY";
            prefix = " | ";
        }
        if (IsFlagSet(access, KEY_CREATE_LINK))
        {
            sout << prefix << "KEY_CREATE_LINK";
            prefix = " | ";
        }

        sout << InterpretCommonAccess(access, prefix).c_str();

        sout << ")";
    }

    return sout.str();
}

inline void LogGenericAccess(DWORD access, const char* msg = "Access")
{
    Log("\t%s=%08X", msg, access);
    if (access)
    {
        const char* prefix = "";
        Log(" (");

        // Specific Rights (bits 0-15)

        // 0x00000001:
        //      FILE_READ_DATA - file & pipe
        //      FILE_LIST_DIRECTORY - directory
        //      KEY_QUERY_VALUE - reg key
        LogIfFlagSetMsg(access, FILE_READ_DATA, "(FILE_READ_DATA / FILE_LIST_DIRECTORY / KEY_QUERY_VALUE)");

        // 0x00000002:
        //      FILE_WRITE_DATA - file & pipe
        //      FILE_ADD_FILE - directory
        //      KEY_SET_VALUE - reg key
        LogIfFlagSetMsg(access, FILE_WRITE_DATA, "(FILE_WRITE_DATA / FILE_ADD_FILE / KEY_SET_VALUE)");

        // 0x00000004:
        //      FILE_APPEND_DATA - file
        //      FILE_ADD_SUBDIRECTORY - directory
        //      FILE_CREATE_PIPE_INSTANCE - pipe
        //      KEY_CREATE_SUB_KEY - reg key
        LogIfFlagSetMsg(access, FILE_APPEND_DATA, "(FILE_APPEND_DATA / FILE_ADD_SUBDIRECTORY / FILE_CREATE_PIPE_INSTANCE / KEY_CREATE_SUB_KEY)");

        // 0x00000008:
        //      FILE_READ_EA - file & directory
        //      KEY_ENUMERATE_SUB_KEYS - reg key
        LogIfFlagSetMsg(access, FILE_READ_EA, "(FILE_READ_EA / KEY_ENUMERATE_SUB_KEYS)");

        // 0x00000010:
        //      FILE_WRITE_EA - file & directory
        //      KEY_NOTIFY - reg key
        LogIfFlagSetMsg(access, FILE_WRITE_EA, "(FILE_WRITE_EA / KEY_NOTIFY)");

        // 0x00000020:
        //      FILE_EXECUTE - file
        //      FILE_TRAVERSE - directory
        //      KEY_CREATE_LINK - reg key
        LogIfFlagSetMsg(access, FILE_EXECUTE, "(FILE_EXECUTE / FILE_TRAVERSE / KEY_CREATE_LINK)");

        // 0x00000040:
        //      FILE_DELETE_CHILD - directory
        LogIfFlagSet(access, FILE_DELETE_CHILD);

        // 0x00000080:
        //      FILE_READ_ATTRIBUTES - all
        LogIfFlagSet(access, FILE_READ_ATTRIBUTES);

        // 0x00000100:
        //      FILE_WRITE_ATTRIBUTES - all
        LogIfFlagSet(access, FILE_WRITE_ATTRIBUTES);

        LogCommonAccess(access, prefix);

        Log(")");
    }

    Log("\n");
}

inline std::string InterpretGenericAccess(DWORD access, const char* msg = "Access")
{
    std::ostringstream sout;
    sout << InterpretAsHex(msg,access);

    if (access)
    { 
        sout << " (";
        const char* prefix = "";

        // Specific Rights (bits 0-15)

        // 0x00000001:
        //      FILE_READ_DATA - file & pipe
        //      FILE_LIST_DIRECTORY - directory
        //      KEY_QUERY_VALUE - reg key
        if (IsFlagSet(access, FILE_READ_DATA))
        {
            sout << prefix <<"(FILE_READ_DATA/FILE_LIST_DIRECTORY/KEY_QUERY_VALUE)";
            prefix = " | ";
        }

        // 0x00000002:
        //      FILE_WRITE_DATA - file & pipe
        //      FILE_ADD_FILE - directory
        //      KEY_SET_VALUE - reg key
        if (IsFlagSet(access, FILE_WRITE_DATA))
        {
            sout << prefix << "(FILE_WRITE_DATA/FILE_ADD_FILE/KEY_SET_VALUE)";
            prefix = " | ";
        }

        // 0x00000004:
        //      FILE_APPEND_DATA - file
        //      FILE_ADD_SUBDIRECTORY - directory
        //      FILE_CREATE_PIPE_INSTANCE - pipe
        //      KEY_CREATE_SUB_KEY - reg key
        if (IsFlagSet(access, FILE_APPEND_DATA))
        {
            sout << prefix << "(FILE_APPEND_DATA/FILE_ADD_SUBDIRECTORY/FILE_CREATE_PIPE_INSTANCE/KEY_CREATE_SUB_KEY)";
            prefix = " | ";
        }

        // 0x00000008:
        //      FILE_READ_EA - file & directory
        //      KEY_ENUMERATE_SUB_KEYS - reg key
        if (IsFlagSet(access, FILE_READ_EA))
        {
            sout << prefix << "(FILE_READ_EA/KEY_ENUMERATE_SUB_KEYS)";
            prefix = " | ";
        }

        // 0x00000010:
        //      FILE_WRITE_EA - file & directory
        //      KEY_NOTIFY - reg key
        if (IsFlagSet(access, FILE_WRITE_EA))
        {
            sout << prefix << "(FILE_WRITE_EA/KEY_NOTIFY)";
            prefix = " | ";
        }

        // 0x00000020:
        //      FILE_EXECUTE - file
        //      FILE_TRAVERSE - directory
        //      KEY_CREATE_LINK - reg key
        if (IsFlagSet(access, FILE_EXECUTE))
        {
            sout << prefix << "(FILE_EXECUTE/FILE_TRAVERSE/KEY_CREATE_LINK)";
            prefix = " | ";
        }

        // 0x00000040:
        //      FILE_DELETE_CHILD - directory
        if (IsFlagSet(access, FILE_DELETE_CHILD))
        {
            sout << prefix << "FILE_DELETE_CHILD";
            prefix = " | ";
        }

        // 0x00000080:
        //      FILE_READ_ATTRIBUTES - all
        if (IsFlagSet(access, FILE_READ_ATTRIBUTES))
        {
            sout << prefix << "FILE_READ_ATTRIBUTES";
            prefix = " | ";
        }

        // 0x00000100:
        //      FILE_WRITE_ATTRIBUTES - all
        if (IsFlagSet(access, FILE_WRITE_ATTRIBUTES))
        {
            sout << prefix << "FILE_WRITE_ATTRIBUTES";
            prefix = " | ";
        }

        // CommonAccess
        if (IsFlagSet(access, DELETE))
        {
            sout << prefix << "DELETE";
            prefix = " | ";
        }
        if (IsFlagSet(access, READ_CONTROL))
        {
            sout << prefix << "READ_CONTROL";
            prefix = " | ";
        }
        if (IsFlagSet(access, WRITE_DAC))
        {
            sout << prefix << "WRITE_DAC";
            prefix = " | ";
        }
        if (IsFlagSet(access, WRITE_OWNER))
        {
            sout << prefix << "WRITE_OWNER";
            prefix = " | ";
        }
        if (IsFlagSet(access, SYNCHRONIZE))
        {
            sout << prefix << "SYNCHRONIZE";
            prefix = " | ";
        }
        

        // Access System Security (bit 24)
        if (IsFlagSet(access, ACCESS_SYSTEM_SECURITY))
        {
            sout << prefix << "ACCESS_SYSTEM_SECURITY";
            prefix = " | ";
        }

        // Maximum Allowed (bit 25)
        if (IsFlagSet(access, MAXIMUM_ALLOWED))
        {
            sout << prefix << "MAXIMUM_ALLOWED";
            prefix = " | ";
        }

        // NOTE: Bits 26-27 are reserved

        // Generic Rights (bits 28-31)
        if (IsFlagSet(access, GENERIC_ALL))
        {
            sout << prefix << "GENERIC_ALL";
            prefix = " | ";
        }
        if (IsFlagSet(access, GENERIC_EXECUTE))
        {
            sout << prefix << "GENERIC_EXECUTE";
            prefix = " | ";
        }
        if (IsFlagSet(access, GENERIC_READ))
        {
            sout << prefix << "GENERIC_READ";
            prefix = " | ";
        }
        if (IsFlagSet(access, GENERIC_WRITE))
        {
            sout << prefix << "GENERIC_WRITE";
            prefix = " | ";
        }

        sout << ")";
    }

    return sout.str();
}

inline void LogShareMode(DWORD shareMode, const char* msg = "Share")
{
    Log("\t%s=%08X", msg, shareMode);
    if (shareMode)
    {
        const char* prefix = "";
        Log(" (");
        LogIfFlagSet(shareMode, FILE_SHARE_DELETE);
        LogIfFlagSet(shareMode, FILE_SHARE_READ);
        LogIfFlagSet(shareMode, FILE_SHARE_WRITE);
        Log(")");
    }

    Log("\n");
}

inline std::string InterpretShareMode(DWORD shareMode, const char* msg = "Share")
{
    std::ostringstream sout;
    sout << InterpretAsHex(msg, shareMode);
    if (shareMode)
    {
        sout << " (";
        const char* prefix = "";
        if (IsFlagSet(shareMode, FILE_SHARE_DELETE))
        {
            sout << prefix << "(FILE_SHARE_DELETE)";
            prefix = " | ";
        }
        if (IsFlagSet(shareMode, FILE_SHARE_READ))
        {
            sout << prefix << "(FILE_SHARE_READ)";
            prefix = " | ";
        }
        if (IsFlagSet(shareMode, FILE_SHARE_WRITE))
        {
            sout << prefix << "(FILE_SHARE_WRITE)";
            prefix = " | ";
        }

        sout << ")";
    }

    return sout.str();
}

inline void LogCreationDisposition(DWORD creationDisposition, const char* msg = "Disposition")
{
    Log("\t%s=%d (", msg, creationDisposition);
    LogIfEqual(creationDisposition, CREATE_ALWAYS)
    else LogIfEqual(creationDisposition, CREATE_NEW)
    else LogIfEqual(creationDisposition, OPEN_ALWAYS)
    else LogIfEqual(creationDisposition, OPEN_EXISTING)
    else LogIfEqual(creationDisposition, TRUNCATE_EXISTING)
    else Log("UNKNOWN");
    Log(")\n");
}

inline std::string InterpretCreationDisposition(DWORD creationDisposition, const char* msg = "Disposition")
{
    std::ostringstream sout;
    sout << InterpretAsHex(msg, creationDisposition);
    if (IsFlagEqual(creationDisposition, CREATE_ALWAYS))
    {
        sout << " (CREATE_ALWAYS)";
    }
    else if (IsFlagEqual(creationDisposition, CREATE_NEW))
    {
        sout << " (CREATE_NEW)";
    }
    else if (IsFlagEqual(creationDisposition, OPEN_ALWAYS))
    {
        sout << " (OPEN_ALWAYS)";
    }
    else if (IsFlagEqual(creationDisposition, OPEN_EXISTING))
    {
        sout << " (OPEN_EXISTING)";
    }
    else if (IsFlagEqual(creationDisposition, TRUNCATE_EXISTING))
    {
        sout << " (TRUNCATE_EXISTING)";
    }
    else
    {
        sout << " (UNKNOWN)";
    }
    return sout.str();
}

inline void LogInfoLevelId(GET_FILEEX_INFO_LEVELS infoLevelId, const char* msg = "Level")
{
    Log("\t%s=%d (", msg, infoLevelId);
    LogIfEqual(infoLevelId, GetFileExInfoStandard)
    else Log("UNKNOWN");
    Log(")\n");
}

inline std::string InterpretInfoLevelId(GET_FILEEX_INFO_LEVELS infoLevelId, const char* msg = "Level")
{
    std::ostringstream sout;
    sout << InterpretAsHex(msg, (DWORD)infoLevelId);
    if (IsFlagSet(infoLevelId, GetFileExInfoStandard))
    {
        sout << " (GetFileExInfoStandard)";
    }
    else
    {
        sout << "UNKNOWN";
    }
    return sout.str();
}

inline void LogFileAttributes(DWORD attributes, const char* msg = "Attributes")
{
    Log("\t%s=%08X", msg, attributes);
    if (attributes)
    {
        const char* prefix = "";
        Log(" (");
        LogIfFlagSet(attributes, FILE_ATTRIBUTE_READONLY);
        LogIfFlagSet(attributes, FILE_ATTRIBUTE_HIDDEN);
        LogIfFlagSet(attributes, FILE_ATTRIBUTE_SYSTEM);
        LogIfFlagSet(attributes, FILE_ATTRIBUTE_DIRECTORY);
        LogIfFlagSet(attributes, FILE_ATTRIBUTE_ARCHIVE);
        LogIfFlagSet(attributes, FILE_ATTRIBUTE_DEVICE);
        LogIfFlagSet(attributes, FILE_ATTRIBUTE_NORMAL);
        LogIfFlagSet(attributes, FILE_ATTRIBUTE_TEMPORARY);
        LogIfFlagSet(attributes, FILE_ATTRIBUTE_SPARSE_FILE);
        LogIfFlagSet(attributes, FILE_ATTRIBUTE_REPARSE_POINT);
        LogIfFlagSet(attributes, FILE_ATTRIBUTE_COMPRESSED);
        LogIfFlagSet(attributes, FILE_ATTRIBUTE_OFFLINE);
        LogIfFlagSet(attributes, FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
        LogIfFlagSet(attributes, FILE_ATTRIBUTE_ENCRYPTED);
        LogIfFlagSet(attributes, FILE_ATTRIBUTE_INTEGRITY_STREAM);
        LogIfFlagSet(attributes, FILE_ATTRIBUTE_VIRTUAL);
        LogIfFlagSet(attributes, FILE_ATTRIBUTE_NO_SCRUB_DATA);
        LogIfFlagSet(attributes, FILE_ATTRIBUTE_EA);
        LogIfFlagSet(attributes, FILE_ATTRIBUTE_PINNED);
        LogIfFlagSet(attributes, FILE_ATTRIBUTE_UNPINNED);
        LogIfFlagSet(attributes, FILE_ATTRIBUTE_RECALL_ON_OPEN);
        LogIfFlagSet(attributes, FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS);
        LogIfFlagSet(attributes, FILE_ATTRIBUTE_STRICTLY_SEQUENTIAL);
        Log(")");
    }

    Log("\n");
}

inline std::string InterpretFileAttributes(DWORD attributes, const char* msg = "Attributes")
{
    std::ostringstream sout;
    sout << InterpretAsHex(msg,attributes);
    if (attributes)
    {
        sout << " (";
        const char* prefix = "";
        
        if (IsFlagSet(attributes, FILE_ATTRIBUTE_READONLY))
        {
            sout << prefix << "FILE_ATTRIBUTE_READONLY";
            prefix = " | ";
        }
        if (IsFlagSet(attributes, FILE_ATTRIBUTE_HIDDEN))
        {
            sout << prefix << "FILE_ATTRIBUTE_HIDDEN";
            prefix = " | ";
        }
        if (IsFlagSet(attributes, FILE_ATTRIBUTE_SYSTEM))
        {
            sout << prefix << "FILE_ATTRIBUTE_SYSTEM";
            prefix = " | ";
        }
        if (IsFlagSet(attributes, FILE_ATTRIBUTE_DIRECTORY))
        {
            sout << prefix << "FILE_ATTRIBUTE_DIRECTORY";
            prefix = " | ";
        }
        if (IsFlagSet(attributes, FILE_ATTRIBUTE_ARCHIVE))
        {
            sout << prefix << "FILE_ATTRIBUTE_ARCHIVE";
            prefix = " | ";
        }
        if (IsFlagSet(attributes, FILE_ATTRIBUTE_DEVICE))
        {
            sout << prefix << "FILE_ATTRIBUTE_DEVICE";
            prefix = " | ";
        }
        if (IsFlagSet(attributes, FILE_ATTRIBUTE_NORMAL))
        {
            sout << prefix << "FILE_ATTRIBUTE_NORMAL";
            prefix = " | ";
        }
        if (IsFlagSet(attributes, FILE_ATTRIBUTE_TEMPORARY))
        {
            sout << prefix << "FILE_ATTRIBUTE_TEMPORARY";
            prefix = " | ";
        }
        if (IsFlagSet(attributes, FILE_ATTRIBUTE_SPARSE_FILE))
        {
            sout << prefix << "FILE_ATTRIBUTE_SPARSE_FILE";
            prefix = " | ";
        }
        if (IsFlagSet(attributes, FILE_ATTRIBUTE_REPARSE_POINT))
        {
            sout << prefix << "FILE_ATTRIBUTE_REPARSE_POINT";
            prefix = " | ";
        }
        if (IsFlagSet(attributes, FILE_ATTRIBUTE_COMPRESSED))
        {
            sout << prefix << "FILE_ATTRIBUTE_COMPRESSED";
            prefix = " | ";
        }
        if (IsFlagSet(attributes, FILE_ATTRIBUTE_OFFLINE))
        {
            sout << prefix << "FILE_ATTRIBUTE_OFFLINE";
            prefix = " | ";
        }
        if (IsFlagSet(attributes, FILE_ATTRIBUTE_NOT_CONTENT_INDEXED))
        {
            sout << prefix << "FILE_ATTRIBUTE_NOT_CONTENT_INDEXED";
            prefix = " | ";
        }
        if (IsFlagSet(attributes, FILE_ATTRIBUTE_ENCRYPTED))
        {
            sout << prefix << "FILE_ATTRIBUTE_ENCRYPTED";
            prefix = " | ";
        }
        if (IsFlagSet(attributes, FILE_ATTRIBUTE_INTEGRITY_STREAM))
        {
            sout << prefix << "FILE_ATTRIBUTE_INTEGRITY_STREAM";
            prefix = " | ";
        }
        if (IsFlagSet(attributes, FILE_ATTRIBUTE_VIRTUAL))
        {
            sout << prefix << "FILE_ATTRIBUTE_VIRTUAL";
            prefix = " | ";
        }
        if (IsFlagSet(attributes, FILE_ATTRIBUTE_NO_SCRUB_DATA))
        {
            sout << prefix << "FILE_ATTRIBUTE_NO_SCRUB_DATA";
            prefix = " | ";
        }
        if (IsFlagSet(attributes, FILE_ATTRIBUTE_EA))
        {
            sout << prefix << "FILE_ATTRIBUTE_EA";
            prefix = " | ";
        }
        if (IsFlagSet(attributes, FILE_ATTRIBUTE_PINNED))
        {
            sout << prefix << "FILE_ATTRIBUTE_PINNED";
            prefix = " | ";
        }
        if (IsFlagSet(attributes, FILE_ATTRIBUTE_UNPINNED))
        {
            sout << prefix << "FILE_ATTRIBUTE_UNPINNED";
            prefix = " | ";
        }
        if (IsFlagSet(attributes, FILE_ATTRIBUTE_RECALL_ON_OPEN))
        {
            sout << prefix << "FILE_ATTRIBUTE_RECALL_ON_OPEN";
            prefix = " | ";
        }
        if (IsFlagSet(attributes, FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS))
        {
            sout << prefix << "FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS";
            prefix = " | ";
        }
        if (IsFlagSet(attributes, FILE_ATTRIBUTE_STRICTLY_SEQUENTIAL))
        {
            sout << prefix << "FILE_ATTRIBUTE_STRICTLY_SEQUENTIAL";
            prefix = " | ";
        }
        sout << ")";
    }

    return sout.str();
}

inline void LogFileFlags(DWORD flags, const char* msg = "Flags")
{
    Log("\t%s=%08X", msg, flags);
    if (flags)
    {
        const char* prefix = "";
        Log(" (");
        LogIfFlagSet(flags, FILE_FLAG_WRITE_THROUGH);
        LogIfFlagSet(flags, FILE_FLAG_OVERLAPPED);
        LogIfFlagSet(flags, FILE_FLAG_NO_BUFFERING);
        LogIfFlagSet(flags, FILE_FLAG_RANDOM_ACCESS);
        LogIfFlagSet(flags, FILE_FLAG_SEQUENTIAL_SCAN);
        LogIfFlagSet(flags, FILE_FLAG_DELETE_ON_CLOSE);
        LogIfFlagSet(flags, FILE_FLAG_BACKUP_SEMANTICS);
        LogIfFlagSet(flags, FILE_FLAG_POSIX_SEMANTICS);
        LogIfFlagSet(flags, FILE_FLAG_SESSION_AWARE);
        LogIfFlagSet(flags, FILE_FLAG_OPEN_REPARSE_POINT);
        LogIfFlagSet(flags, FILE_FLAG_OPEN_NO_RECALL);
        LogIfFlagSet(flags, FILE_FLAG_FIRST_PIPE_INSTANCE);

        LogIfFlagSet(flags, FILE_FLAG_OPEN_REQUIRING_OPLOCK); // NOTE: CreateFile2 only!
        Log(")");
    }

    Log("\n");
}

inline std::string InterpretFileFlags(DWORD flags, const char* msg = "Flags")
{
    std::ostringstream sout;
    sout << InterpretAsHex(msg, flags);
    if (flags)
    {
        sout << " (";
        const char* prefix = "";

        if (IsFlagSet(flags, FILE_FLAG_WRITE_THROUGH))
        {
            sout << prefix << "FILE_FLAG_WRITE_THROUGH";
            prefix = " | ";
        }
        if (IsFlagSet(flags, FILE_FLAG_OVERLAPPED))
        {
            sout << prefix << "FILE_FLAG_OVERLAPPED";
            prefix = " | ";
        }
        if (IsFlagSet(flags, FILE_FLAG_NO_BUFFERING))
        {
            sout << prefix << "FILE_FLAG_NO_BUFFERING";
            prefix = " | ";
        }
        if (IsFlagSet(flags, FILE_FLAG_RANDOM_ACCESS))
        {
            sout << prefix << "FILE_FLAG_RANDOM_ACCESS";
            prefix = " | ";
        }
        if (IsFlagSet(flags, FILE_FLAG_SEQUENTIAL_SCAN))
        {
            sout << prefix << "FILE_FLAG_SEQUENTIAL_SCAN";
            prefix = " | ";
        }
        if (IsFlagSet(flags, FILE_FLAG_DELETE_ON_CLOSE))
        {
            sout << prefix << "FILE_FLAG_DELETE_ON_CLOSE";
            prefix = " | ";
        }
        if (IsFlagSet(flags, FILE_FLAG_BACKUP_SEMANTICS))
        {
            sout << prefix << "FILE_FLAG_BACKUP_SEMANTICS";
            prefix = " | ";
        }
        if (IsFlagSet(flags, FILE_FLAG_POSIX_SEMANTICS))
        {
            sout << prefix << "FILE_FLAG_POSIX_SEMANTICS";
            prefix = " | ";
        }
        if (IsFlagSet(flags, FILE_FLAG_SESSION_AWARE))
        {
            sout << prefix << "FILE_FLAG_SESSION_AWARE";
            prefix = " | ";
        }
        if (IsFlagSet(flags, FILE_FLAG_OPEN_REPARSE_POINT))
        {
            sout << prefix << "FILE_FLAG_OPEN_REPARSE_POINT";
            prefix = " | ";
        }
        if (IsFlagSet(flags, FILE_FLAG_OPEN_NO_RECALL))
        {
            sout << prefix << "FILE_FLAG_OPEN_NO_RECALL";
            prefix = " | ";
        }
        if (IsFlagSet(flags, FILE_FLAG_FIRST_PIPE_INSTANCE))
        {
            sout << prefix << "FILE_FLAG_FIRST_PIPE_INSTANCE";
            prefix = " | ";
        }

        if (IsFlagSet(flags, FILE_FLAG_OPEN_REQUIRING_OPLOCK))
        {
            sout << prefix << "FILE_FLAG_OPEN_REQUIRING_OPLOCK";
            prefix = " | ";
        }					 // NOTE: CreateFile2 only!
        sout << ")";
    }
    return sout.str();

}

inline void LogSQOS(DWORD sqosValue, const char* msg = "Security Quality of Service")
{
    Log("\t%s=%d (", msg, sqosValue);
    LogIfEqual(sqosValue, SECURITY_ANONYMOUS)
    else LogIfEqual(sqosValue, SECURITY_CONTEXT_TRACKING)
    else LogIfEqual(sqosValue, SECURITY_DELEGATION)
    else LogIfEqual(sqosValue, SECURITY_EFFECTIVE_ONLY)
    else LogIfEqual(sqosValue, SECURITY_IDENTIFICATION)
    else LogIfEqual(sqosValue, SECURITY_IMPERSONATION)
    else Log("UNKNOWN");
    Log(")\n");
}

inline std::string InterpretSQOS(DWORD sqosValue, const char* msg = "Security Quality of Service")
{
    std::ostringstream sout;
    sout << InterpretAsHex(msg,sqosValue);

    if (IsFlagSet(sqosValue, SECURITY_ANONYMOUS))
    {
        sout << "SECURITY_ANONYMOUS";
    }
    else if (IsFlagSet(sqosValue, SECURITY_CONTEXT_TRACKING))
    {
        sout << "SECURITY_CONTEXT_TRACKING";
    }
    else if (IsFlagSet(sqosValue, SECURITY_DELEGATION))
    {
        sout << "SECURITY_DELEGATION";
    }
    else if (IsFlagSet(sqosValue, SECURITY_EFFECTIVE_ONLY))
    {
        sout << "SECURITY_EFFECTIVE_ONLY";
    }
    else if (IsFlagSet(sqosValue, SECURITY_IDENTIFICATION))
    {
        sout << "SECURITY_CONTEXT_TRACSECURITY_IDENTIFICATIONKING";
    }
    else if (IsFlagSet(sqosValue, SECURITY_IMPERSONATION))
    {
        sout << "SECURITY_IMPERSONATION";
    }
    else
    {
        sout << "UNKNOWN";
    }
    sout << ")";
    return sout.str();
}

inline void LogFileFlagsAndAttributes(DWORD flagsAndAttributes, const char* msg = "Flags and Attributes")
{
    // NOTE: Some flags and attributes overlap, hence the apparent semi-duplication here. This is intentional and only
    //       what is explicitly called out on MSDN appears here
    //       See: https://msdn.microsoft.com/en-us/library/windows/desktop/aa363858(v=vs.85).aspx
    Log("\t%s=%08X", msg, flagsAndAttributes);
    if (flagsAndAttributes)
    {
        const char* prefix = "";
        Log(" (");
        LogIfFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_ARCHIVE);
        LogIfFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_ENCRYPTED);
        LogIfFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_HIDDEN);
        LogIfFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_NORMAL);
        LogIfFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_OFFLINE);
        LogIfFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_READONLY);
        LogIfFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_SYSTEM);
        LogIfFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_TEMPORARY);

        LogIfFlagSet(flagsAndAttributes, FILE_FLAG_BACKUP_SEMANTICS);
        LogIfFlagSet(flagsAndAttributes, FILE_FLAG_DELETE_ON_CLOSE);
        LogIfFlagSet(flagsAndAttributes, FILE_FLAG_NO_BUFFERING);
        LogIfFlagSet(flagsAndAttributes, FILE_FLAG_OPEN_NO_RECALL);
        LogIfFlagSet(flagsAndAttributes, FILE_FLAG_OPEN_REPARSE_POINT);
        LogIfFlagSet(flagsAndAttributes, FILE_FLAG_OVERLAPPED);
        LogIfFlagSet(flagsAndAttributes, FILE_FLAG_POSIX_SEMANTICS);
        LogIfFlagSet(flagsAndAttributes, FILE_FLAG_RANDOM_ACCESS);
        LogIfFlagSet(flagsAndAttributes, FILE_FLAG_SESSION_AWARE);
        LogIfFlagSet(flagsAndAttributes, FILE_FLAG_SEQUENTIAL_SCAN);
        LogIfFlagSet(flagsAndAttributes, FILE_FLAG_WRITE_THROUGH);

        if (IsFlagSet(flagsAndAttributes, SECURITY_SQOS_PRESENT))
        {
            auto sqosValue = flagsAndAttributes & SECURITY_VALID_SQOS_FLAGS;
            LogIfEqual(sqosValue, SECURITY_ANONYMOUS)
            else LogIfEqual(sqosValue, SECURITY_CONTEXT_TRACKING)
            else LogIfEqual(sqosValue, SECURITY_DELEGATION)
            else LogIfEqual(sqosValue, SECURITY_EFFECTIVE_ONLY)
            else LogIfEqual(sqosValue, SECURITY_IDENTIFICATION)
            else LogIfEqual(sqosValue, SECURITY_IMPERSONATION)
        }
        Log(")");
    }

    Log("\n");
}

inline  std::string InterpretFileFlagsAndAttributes(DWORD flagsAndAttributes, const char* msg = "Flags and Attributes")
{
    // NOTE: Some flags and attributes overlap, hence the apparent semi-duplication here. This is intentional and only
    //       what is explicitly called out on MSDN appears here
    //       See: https://msdn.microsoft.com/en-us/library/windows/desktop/aa363858(v=vs.85).aspx
    std::ostringstream sout;
    sout << InterpretAsHex(msg, flagsAndAttributes);
    if (flagsAndAttributes)
    {
        const char* prefix = "";

        sout << "\n\tFlags (";
        prefix = "";
        if (IsFlagSet(flagsAndAttributes, FILE_FLAG_BACKUP_SEMANTICS))
        {
            sout << prefix << "BACKUP_SEMANTICS";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_FLAG_DELETE_ON_CLOSE))
        {
            sout << prefix << "DELETE_ON_CLOSE";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_FLAG_NO_BUFFERING))
        {
            sout << prefix << "NO_BUFFERING";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_FLAG_OPEN_NO_RECALL))
        {
            sout << prefix << "OPEN_NO_RECALL";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_FLAG_OPEN_REPARSE_POINT))
        {
            sout << prefix << "OPEN_REPARSE_POINT";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_FLAG_OVERLAPPED))
        {
            sout << prefix << "OVERLAPPED";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_FLAG_POSIX_SEMANTICS))
        {
            sout << prefix << "POSIX_SEMANTICS";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_FLAG_RANDOM_ACCESS))
        {
            sout << prefix << "RANDOM_ACCESS";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_FLAG_SESSION_AWARE))
        {
            sout << prefix << "SESSION_AWARE";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_FLAG_SEQUENTIAL_SCAN))
        {
            sout << prefix << "SEQUENTIAL_SCAN";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_FLAG_WRITE_THROUGH))
        {
            sout << prefix << "WRITE_THROUGH";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_FLAG_FIRST_PIPE_INSTANCE))
        {
            sout << prefix << "FIRST_PIPE_INSTANCE";
            prefix = " | ";
        }
        sout << ")";


        if (IsFlagSet(flagsAndAttributes, SECURITY_SQOS_PRESENT))
        {
            sout << "\nSecurity_SQOS Flags (";
            prefix = "";

            auto sqosValue = flagsAndAttributes & SECURITY_VALID_SQOS_FLAGS;
            if (IsFlagSet(sqosValue, SECURITY_ANONYMOUS))
            {
                sout << prefix << "ANONYMOUS";
                prefix = " | ";
            }
            else if (IsFlagSet(sqosValue, SECURITY_CONTEXT_TRACKING))
            {
                sout << prefix << "CONTEXT_TRACKING";
                prefix = " | ";
            }
            else if (IsFlagSet(sqosValue, SECURITY_DELEGATION))
            {
                sout << prefix << "DELEGATION";
                prefix = " | ";
            }
            else if (IsFlagSet(sqosValue, SECURITY_EFFECTIVE_ONLY))
            {
                sout << prefix << "EFFECTIVE_ONLY";
                prefix = " | ";
            }
            else if (IsFlagSet(sqosValue, SECURITY_IDENTIFICATION))
            {
                sout << prefix << "IDENTIFICATION";
                prefix = " | ";
            }
            else if (IsFlagSet(sqosValue, SECURITY_IMPERSONATION))
            {
                sout << prefix << "IMPERSONATION";
                prefix = " | ";
            }
            sout << ")";
        }

        sout << "\n\tAttributes (";
        prefix = "";

        if (IsFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_DIRECTORY))
        {
            sout << prefix << "DIRECTORY";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_DEVICE))
        {
            sout << prefix << "DEVICE";
            prefix = " | ";
        }if (IsFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_ARCHIVE))
        {
            sout << prefix << "ARCHIVE";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_HIDDEN))
        {
            sout << prefix << "HIDDEN";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_NORMAL))
        {
            sout << prefix << "NORMAL";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_OFFLINE))
        {
            sout << prefix << "OFFLINE";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_READONLY))
        {
            sout << prefix << "READONLY";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_COMPRESSED))
        {
            sout << prefix << "COMPRESSED";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_ENCRYPTED))
        {
            sout << prefix << "ENCRYPTED";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_SYSTEM))
        {
            sout << prefix << "SYSTEM";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_SPARSE_FILE))
        {
            sout << prefix << "SPARSE_FILE";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_REPARSE_POINT))
        {
            sout << prefix << "REPARSE_POINT";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_TEMPORARY))
        {
            sout << prefix << "TEMPORARY";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_OFFLINE))
        {
            sout << prefix << "OFFLINE";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_NOT_CONTENT_INDEXED))
        {
            sout << prefix << "NOT_CONTENT_INDEXED";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_INTEGRITY_STREAM))
        {
            sout << prefix << "INTEGRITY_STREAM";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_VIRTUAL))
        {
            sout << prefix << "VIRTUAL";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_NO_SCRUB_DATA))
        {
            sout << prefix << "NO_SCRUB_DATA";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_EA))
        {
            sout << prefix << "EA";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_PINNED))
        {
            sout << prefix << "PINNED";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_UNPINNED))
        {
            sout << prefix << "UNPINNED";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_RECALL_ON_OPEN))
        {
            sout << prefix << "RECALL_ON_OPEN";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS))
        {
            sout << prefix << "RECALL_ON_DATA_ACCESS";
            prefix = " | ";
        }
        if (IsFlagSet(flagsAndAttributes, FILE_ATTRIBUTE_STRICTLY_SEQUENTIAL))
        {
            sout << prefix << "STRICTLY_SEQUENTIAL";
            prefix = " | ";
        }
        sout << ")";
    }

    return sout.str();
}

inline void LogCopyFlags(DWORD flags, const char* msg = "Flags")
{

    Log("\t%s=%08X", msg, flags);
    if (flags)
    {
        const char* prefix = "";
        Log(" (");
        LogIfFlagSet(flags, COPY_FILE_FAIL_IF_EXISTS);
        LogIfFlagSet(flags, COPY_FILE_RESTARTABLE);
        LogIfFlagSet(flags, COPY_FILE_OPEN_SOURCE_FOR_WRITE);
        LogIfFlagSet(flags, COPY_FILE_ALLOW_DECRYPTED_DESTINATION);
        LogIfFlagSet(flags, COPY_FILE_COPY_SYMLINK);
        LogIfFlagSet(flags, COPY_FILE_NO_BUFFERING);
        LogIfFlagSet(flags, COPY_FILE_REQUEST_SECURITY_PRIVILEGES);
        LogIfFlagSet(flags, COPY_FILE_RESUME_FROM_PAUSE);
        LogIfFlagSet(flags, COPY_FILE_IGNORE_EDP_BLOCK);
        LogIfFlagSet(flags, COPY_FILE_IGNORE_SOURCE_ENCRYPTION);
        LogIfFlagSet(flags, COPY_FILE_NO_OFFLOAD);
        Log(")");
    }

    Log("\n");
}

inline std::string InterpretCopyFlags(DWORD flags, const char* msg = "Flags")
{
    std::ostringstream sout;
    sout << InterpretAsHex(msg, flags);
    if (flags)
    {
        sout << " (";
        const char* prefix = "";

        if (IsFlagSet(flags, COPY_FILE_FAIL_IF_EXISTS))
        {
            sout << prefix << "COPY_FILE_FAIL_IF_EXISTS";
            prefix = " | ";
        }
        if (IsFlagSet(flags, COPY_FILE_RESTARTABLE))
        {
            sout << prefix << "COPY_FILE_RESTARTABLE";
            prefix = " | ";
        }
        if (IsFlagSet(flags, COPY_FILE_OPEN_SOURCE_FOR_WRITE))
        {
            sout << prefix << "COPY_FILE_OPEN_SOURCE_FOR_WRITE";
            prefix = " | ";
        }
        if (IsFlagSet(flags, COPY_FILE_ALLOW_DECRYPTED_DESTINATION))
        {
            sout << prefix << "COPY_FILE_ALLOW_DECRYPTED_DESTINATION";
            prefix = " | ";
        }
        if (IsFlagSet(flags, COPY_FILE_COPY_SYMLINK))
        {
            sout << prefix << "COPY_FILE_COPY_SYMLINK";
            prefix = " | ";
        }
        if (IsFlagSet(flags, COPY_FILE_NO_BUFFERING))
        {
            sout << prefix << "COPY_FILE_NO_BUFFERING";
            prefix = " | ";
        }
        if (IsFlagSet(flags, COPY_FILE_REQUEST_SECURITY_PRIVILEGES))
        {
            sout << prefix << "COPY_FILE_REQUEST_SECURITY_PRIVILEGES";
            prefix = " | ";
        }
        if (IsFlagSet(flags, COPY_FILE_RESUME_FROM_PAUSE))
        {
            sout << prefix << "COPY_FILE_RESUME_FROM_PAUSE";
            prefix = " | ";
        }
        if (IsFlagSet(flags, COPY_FILE_IGNORE_EDP_BLOCK))
        {
            sout << prefix << "COPY_FILE_IGNORE_EDP_BLOCK";
            prefix = " | ";
        }
        if (IsFlagSet(flags, COPY_FILE_IGNORE_SOURCE_ENCRYPTION))
        {
            sout << prefix << "COPY_FILE_IGNORE_SOURCE_ENCRYPTION";
            prefix = " | ";
        }
        if (IsFlagSet(flags, COPY_FILE_NO_OFFLOAD))
        {
            sout << prefix << "COPY_FILE_NO_OFFLOAD";
            prefix = " | ";
        }
        sout << ")";
    }

    return sout.str();
}

inline void LogSymlinkFlags(DWORD flags, const char* msg = "Flags")
{
    Log("\t%s=%08X", msg, flags);
    if (flags)
    {
        const char* prefix = "";
        Log(" (");
        LogIfFlagSet(flags, SYMBOLIC_LINK_FLAG_DIRECTORY);
        LogIfFlagSet(flags, SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE);
        Log(")");
    }

    Log("\n");
}

inline std::string InterpretSymlinkFlags(DWORD flags, const char* msg = "Flags")
{
    std::ostringstream sout;
    sout << InterpretAsHex(msg, flags);
    if (flags)
    {
        sout << " (";
        const char* prefix = "";

        if (IsFlagSet(flags, SYMBOLIC_LINK_FLAG_DIRECTORY))
        {
            sout << prefix << "SYMBOLIC_LINK_FLAG_DIRECTORY";
            prefix = " | ";
        }
        if (IsFlagSet(flags, SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE))
        {
            sout << prefix << "SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE";
            prefix = " | ";
        }

        sout << ")";
    }

    return sout.str();
}

inline void LogMoveFileFlags(DWORD flags, const char* msg = "Flags")
{
    Log("\t%s=%08X", msg, flags);
    if (flags)
    {
        const char* prefix = "";
        Log(" (");
        LogIfFlagSet(flags, MOVEFILE_REPLACE_EXISTING);
        LogIfFlagSet(flags, MOVEFILE_COPY_ALLOWED);
        LogIfFlagSet(flags, MOVEFILE_DELAY_UNTIL_REBOOT);
        LogIfFlagSet(flags, MOVEFILE_WRITE_THROUGH);
        LogIfFlagSet(flags, MOVEFILE_CREATE_HARDLINK);
        LogIfFlagSet(flags, MOVEFILE_FAIL_IF_NOT_TRACKABLE);
        Log(")");
    }

    Log("\n");
}

inline std::string InterpretMoveFileFlags(DWORD flags, const char* msg = "Flags")
{
    std::ostringstream sout;
    sout << InterpretAsHex(msg, flags);
    if (flags)
    {
        sout << " (";	
        const char* prefix = "";

        if (IsFlagSet(flags, MOVEFILE_REPLACE_EXISTING))
        {
            sout << prefix << "MOVEFILE_REPLACE_EXISTING";
            prefix = " | ";
        }
        if (IsFlagSet(flags, MOVEFILE_COPY_ALLOWED))
        {
            sout << prefix << "MOVEFILE_COPY_ALLOWED";
            prefix = " | ";
        }
        if (IsFlagSet(flags, MOVEFILE_DELAY_UNTIL_REBOOT))
        {
            sout << prefix << "MOVEFILE_DELAY_UNTIL_REBOOT";
            prefix = " | ";
        }
        if (IsFlagSet(flags, MOVEFILE_WRITE_THROUGH))
        {
            sout << prefix << "MOVEFILE_WRITE_THROUGH";
            prefix = " | ";
        }
        if (IsFlagSet(flags, MOVEFILE_CREATE_HARDLINK))
        {
            sout << prefix << "MOVEFILE_CREATE_HARDLINK";
            prefix = " | ";
        }
        if (IsFlagSet(flags, MOVEFILE_FAIL_IF_NOT_TRACKABLE))
        {
            sout << prefix << "MOVEFILE_FAIL_IF_NOT_TRACKABLE";
            prefix = " | ";
        }
        sout << ")";
    }

    return sout.str();
}

inline void LogReplaceFileFlags(DWORD flags, const char* msg = "Flags")
{
    Log("\t%s=%08X", msg, flags);
    if (flags)
    {
        const char* prefix = "";
        Log(" (");
        LogIfFlagSet(flags, REPLACEFILE_WRITE_THROUGH);
        LogIfFlagSet(flags, REPLACEFILE_IGNORE_MERGE_ERRORS);
        LogIfFlagSet(flags, REPLACEFILE_IGNORE_ACL_ERRORS);
        Log(")");
    }

    Log("\n");
}

inline std::string InterpretReplaceFileFlags(DWORD flags, const char* msg = "Flags")
{
    std::ostringstream sout;
    sout << InterpretAsHex(msg, flags);
    if (flags)
    {
        sout << " (";
        const char* prefix = "";

        if (IsFlagSet(flags, REPLACEFILE_WRITE_THROUGH))
        {
            sout << prefix << "REPLACEFILE_WRITE_THROUGH";
            prefix = " | ";
        }
        if (IsFlagSet(flags, REPLACEFILE_IGNORE_MERGE_ERRORS))
        {
            sout << prefix << "REPLACEFILE_IGNORE_MERGE_ERRORS";
            prefix = " | ";
        }
        if (IsFlagSet(flags, REPLACEFILE_IGNORE_ACL_ERRORS))
        {
            sout << prefix << "REPLACEFILE_IGNORE_ACL_ERRORS";
            prefix = " | ";
        }
        sout << ")";
    }

    return sout.str();
}

inline void LogInfoLevelId(FINDEX_INFO_LEVELS infoLevelId, const char* msg = "Info Level Id")
{
    Log("\t%s=%d (", msg, infoLevelId);
    LogIfEqual(infoLevelId, FindExInfoStandard)
    else LogIfEqual(infoLevelId, FindExInfoBasic)
    else Log("UNKNOWN");
    Log(")\n");
}

inline std::string InterpretInfoLevelId(FINDEX_INFO_LEVELS infoLevelId, const char* msg = "Info Level Id")
{
    std::ostringstream sout;
    sout << InterpretAsHex(msg, (DWORD)infoLevelId);
    
    if (IsFlagSet(infoLevelId, FindExInfoStandard))
    {
        sout <<  "FindExInfoStandard";
    }
    else if (IsFlagSet(infoLevelId, FindExInfoBasic))
    {
        sout << "FindExInfoBasic";
    }
    else 
    {
        sout << "UNKNOWN";
    }
    return sout.str();

}

inline void LogSearchOp(FINDEX_SEARCH_OPS searchOp, const char* msg = "Search Op")
{
    Log("\t%s=%d (", msg, searchOp);
    LogIfEqual(searchOp, FindExSearchNameMatch)
    else LogIfEqual(searchOp, FindExSearchLimitToDirectories)
    else LogIfEqual(searchOp, FindExSearchLimitToDevices)
    else Log("UNKNOWN");
    Log(")\n");
}

inline std::string InterpretSearchOp(FINDEX_SEARCH_OPS searchOp, const char* msg = "Search Op")
{
    std::ostringstream sout;
    sout << InterpretAsHex(msg, (DWORD)searchOp);

    if (IsFlagSet(searchOp, FindExSearchNameMatch))
    {
        sout << "FindExSearchNameMatch";
    }
    else if (IsFlagSet(searchOp, FindExSearchLimitToDirectories))
    {
        sout << "FindExSearchLimitToDirectories";
    }
    else if (IsFlagSet(searchOp, FindExSearchLimitToDevices))
    {
        sout << "FindExSearchLimitToDevices";
    }
    else
    {
        sout << "UNKNOWN";
    }
    return sout.str();

}

inline void LogFindFirstFileExFlags(DWORD flags, const char* msg = "Flags")
{
    Log("\t%s=%08X", msg, flags);
    if (flags)
    {
        const char* prefix = "";
        Log(" (");
        LogIfFlagSet(flags, FIND_FIRST_EX_CASE_SENSITIVE);
        LogIfFlagSet(flags, FIND_FIRST_EX_LARGE_FETCH);
        Log(")");
    }

    Log("\n");
}

inline std::string InterpretFindFirstFileExFlags(DWORD flags, const char* msg = "Flags")
{
    std::ostringstream sout;
    sout << InterpretAsHex(msg, flags);
    if (flags)
    {
        sout << " (";
        const char* prefix = "";

        if (IsFlagSet(flags, FIND_FIRST_EX_CASE_SENSITIVE))
        {
            sout << prefix << "FIND_FIRST_EX_CASE_SENSITIVE";
            prefix = " | ";
        }
        if (IsFlagSet(flags, FIND_FIRST_EX_LARGE_FETCH))
        {
            sout << prefix << "FIND_FIRST_EX_LARGE_FETCH";
            prefix = " | ";
        }

        sout << ")";
    }

    return sout.str();
}

inline void LogProcessCreationFlags(DWORD flags, const char* msg = "Flags")
{
    Log("\t%s=%08X", msg, flags);
    if (flags)
    {
        const char* prefix = "";
        Log(" (");
        LogIfFlagSet(flags, DEBUG_PROCESS);                     // 0x00000001
        LogIfFlagSet(flags, DEBUG_ONLY_THIS_PROCESS);           // 0x00000002
        LogIfFlagSet(flags, CREATE_SUSPENDED);                  // 0x00000004
        LogIfFlagSet(flags, DETACHED_PROCESS);                  // 0x00000008
        LogIfFlagSet(flags, CREATE_NEW_CONSOLE);                // 0x00000010
        LogIfFlagSet(flags, NORMAL_PRIORITY_CLASS);             // 0x00000020
        LogIfFlagSet(flags, IDLE_PRIORITY_CLASS);               // 0x00000040
        LogIfFlagSet(flags, HIGH_PRIORITY_CLASS);               // 0x00000080
        LogIfFlagSet(flags, REALTIME_PRIORITY_CLASS);           // 0x00000100
        LogIfFlagSet(flags, CREATE_NEW_PROCESS_GROUP);          // 0x00000200
        LogIfFlagSet(flags, CREATE_UNICODE_ENVIRONMENT);        // 0x00000400
        LogIfFlagSet(flags, CREATE_SEPARATE_WOW_VDM);           // 0x00000800
        LogIfFlagSet(flags, CREATE_SHARED_WOW_VDM);             // 0x00001000
        LogIfFlagSet(flags, CREATE_FORCEDOS);                   // 0x00002000
        LogIfFlagSet(flags, BELOW_NORMAL_PRIORITY_CLASS);       // 0x00004000
        LogIfFlagSet(flags, ABOVE_NORMAL_PRIORITY_CLASS);       // 0x00008000
        LogIfFlagSet(flags, INHERIT_PARENT_AFFINITY);           // 0x00010000
        LogIfFlagSet(flags, INHERIT_CALLER_PRIORITY);           // 0x00020000
        LogIfFlagSet(flags, CREATE_PROTECTED_PROCESS);          // 0x00040000
        LogIfFlagSet(flags, EXTENDED_STARTUPINFO_PRESENT);      // 0x00080000
        LogIfFlagSet(flags, PROCESS_MODE_BACKGROUND_BEGIN);     // 0x00100000
        LogIfFlagSet(flags, PROCESS_MODE_BACKGROUND_END);       // 0x00200000
        LogIfFlagSet(flags, CREATE_BREAKAWAY_FROM_JOB);         // 0x01000000
        LogIfFlagSet(flags, CREATE_PRESERVE_CODE_AUTHZ_LEVEL);  // 0x02000000
        LogIfFlagSet(flags, CREATE_DEFAULT_ERROR_MODE);         // 0x04000000
        LogIfFlagSet(flags, CREATE_NO_WINDOW);                  // 0x08000000
        LogIfFlagSet(flags, PROFILE_USER);                      // 0x10000000
        LogIfFlagSet(flags, PROFILE_KERNEL);                    // 0x20000000
        LogIfFlagSet(flags, PROFILE_SERVER);                    // 0x40000000
        LogIfFlagSet(flags, CREATE_IGNORE_SYSTEM_DEFAULT);      // 0x80000000
        Log(")");
    }

    Log("\n");
}

inline std::string InterpretProcessCreationFlags(DWORD flags, const char* msg = "Flags")
{
    std::ostringstream sout;
    sout << InterpretAsHex(msg,flags);
    if (flags)
    {
        sout << " (";
        const char* prefix = "";

        if (IsFlagSet(flags, DEBUG_PROCESS))
        {
            sout << prefix << "DEBUG_PROCESS";
            prefix = " | ";
        }
        if (IsFlagSet(flags, DEBUG_ONLY_THIS_PROCESS))
        {
            sout << prefix << "DEBUG_ONLY_THIS_PROCESS";
            prefix = " | ";
        }
        if (IsFlagSet(flags, CREATE_SUSPENDED))
        {
            sout << prefix << "CREATE_SUSPENDED";
            prefix = " | ";
        }
        if (IsFlagSet(flags, DETACHED_PROCESS))
        {
            sout << prefix << "DETACHED_PROCESS";
            prefix = " | ";
        }
        if (IsFlagSet(flags, CREATE_NEW_CONSOLE))
        {
            sout << prefix << "CREATE_NEW_CONSOLE";
            prefix = " | ";
        }
        if (IsFlagSet(flags, NORMAL_PRIORITY_CLASS))
        {
            sout << prefix << "NORMAL_PRIORITY_CLASS";
            prefix = " | ";
        }
        if (IsFlagSet(flags, IDLE_PRIORITY_CLASS))
        {
            sout << prefix << "IDLE_PRIORITY_CLASS";
            prefix = " | ";
        }
        if (IsFlagSet(flags, HIGH_PRIORITY_CLASS))
        {
            sout << prefix << "HIGH_PRIORITY_CLASS";
            prefix = " | ";
        }
        if (IsFlagSet(flags, REALTIME_PRIORITY_CLASS))
        {
            sout << prefix << "REALTIME_PRIORITY_CLASS";
            prefix = " | ";
        }
        if (IsFlagSet(flags, CREATE_NEW_PROCESS_GROUP))
        {
            sout << prefix << "CREATE_NEW_PROCESS_GROUP";
            prefix = " | ";
        }
        if (IsFlagSet(flags, CREATE_UNICODE_ENVIRONMENT))
        {
            sout << prefix << "CREATE_UNICODE_ENVIRONMENT";
            prefix = " | ";
        }
        if (IsFlagSet(flags, CREATE_SEPARATE_WOW_VDM))
        {
            sout << prefix << "CREATE_SEPARATE_WOW_VDM";
            prefix = " | ";
        }
        if (IsFlagSet(flags, CREATE_SHARED_WOW_VDM))
        {
            sout << prefix << "CREATE_SHARED_WOW_VDM";
            prefix = " | ";
        }
        if (IsFlagSet(flags, CREATE_FORCEDOS))
        {
            sout << prefix << "CREATE_FORCEDOS";
            prefix = " | ";
        }
        if (IsFlagSet(flags, BELOW_NORMAL_PRIORITY_CLASS))
        {
            sout << prefix << "BELOW_NORMAL_PRIORITY_CLASS";
            prefix = " | ";
        }
        if (IsFlagSet(flags, ABOVE_NORMAL_PRIORITY_CLASS))
        {
            sout << prefix << "ABOVE_NORMAL_PRIORITY_CLASS";
            prefix = " | ";
        }
        if (IsFlagSet(flags, INHERIT_PARENT_AFFINITY))
        {
            sout << prefix << "INHERIT_PARENT_AFFINITY";
            prefix = " | ";
        }
        if (IsFlagSet(flags, INHERIT_CALLER_PRIORITY))
        {
            sout << prefix << "INHERIT_CALLER_PRIORITY";
            prefix = " | ";
        }
        if (IsFlagSet(flags, CREATE_PROTECTED_PROCESS))
        {
            sout << prefix << "CREATE_PROTECTED_PROCESS";
            prefix = " | ";
        }
        if (IsFlagSet(flags, EXTENDED_STARTUPINFO_PRESENT))
        {
            sout << prefix << "EXTENDED_STARTUPINFO_PRESENT";
            prefix = " | ";
        }
        if (IsFlagSet(flags, PROCESS_MODE_BACKGROUND_BEGIN))
        {
            sout << prefix << "PROCESS_MODE_BACKGROUND_BEGIN";
            prefix = " | ";
        }
        if (IsFlagSet(flags, PROCESS_MODE_BACKGROUND_END))
        {
            sout << prefix << "PROCESS_MODE_BACKGROUND_END";
            prefix = " | ";
        }
        if (IsFlagSet(flags, CREATE_BREAKAWAY_FROM_JOB))
        {
            sout << prefix << "CREATE_BREAKAWAY_FROM_JOB";
            prefix = " | ";
        }
        if (IsFlagSet(flags, CREATE_PRESERVE_CODE_AUTHZ_LEVEL))
        {
            sout << prefix << "CREATE_PRESERVE_CODE_AUTHZ_LEVEL";
            prefix = " | ";
        }
        if (IsFlagSet(flags, CREATE_DEFAULT_ERROR_MODE))
        {
            sout << prefix << "CREATE_DEFAULT_ERROR_MODE";
            prefix = " | ";
        }
        if (IsFlagSet(flags, CREATE_NO_WINDOW))
        {
            sout << prefix << "CREATE_NO_WINDOW";
            prefix = " | ";
        }
        if (IsFlagSet(flags, PROFILE_USER))
        {
            sout << prefix << "PROFILE_USER";
            prefix = " | ";
        }
        if (IsFlagSet(flags, PROFILE_KERNEL))
        {
            sout << prefix << "PROFILE_KERNEL";
            prefix = " | ";
        }
        if (IsFlagSet(flags, PROFILE_SERVER))
        {
            sout << prefix << "PROFILE_SERVER";
            prefix = " | ";
        }
        if (IsFlagSet(flags, CREATE_IGNORE_SYSTEM_DEFAULT))
        {
            sout << prefix << "CREATE_IGNORE_SYSTEM_DEFAULT";
            prefix = " | ";
        }
        sout << ")";
    }

    return sout.str();
}

inline void LogRegKeyFlags(DWORD flags, const char* msg = "Options")
{
    Log("\t%s=%08X", msg, flags);
    if (flags)
    {
        const char* prefix = "";
        Log(" (");
        LogIfFlagSet(flags, REG_OPTION_VOLATILE);           // 0x0001
        LogIfFlagSet(flags, REG_OPTION_CREATE_LINK);        // 0x0002
        LogIfFlagSet(flags, REG_OPTION_BACKUP_RESTORE);     // 0x0004
        LogIfFlagSet(flags, REG_OPTION_OPEN_LINK);          // 0x0008
        LogIfFlagSet(flags, REG_OPTION_DONT_VIRTUALIZE);    // 0x0010
        Log(")");
    }
    else
    {
        Log(" (REG_OPTION_NON_VOLATILE)"); // 0x0000
    }

    Log("\n");
}

inline std::string InterpretRegKeyFlags(DWORD flags, const char* msg = "Options")
{
    std::ostringstream sout;
    sout << InterpretAsHex(msg, flags);
    if (flags)
    {
        sout << " (";
        const char* prefix = "";
        
        if (IsFlagSet(flags, REG_OPTION_VOLATILE))
        {
            sout << prefix << "REG_OPTION_VOLATILE";
            prefix = " | ";
        }
        if (IsFlagSet(flags, REG_OPTION_CREATE_LINK))
        {
            sout << prefix << "REG_OPTION_CREATE_LINK";
            prefix = " | ";
        }
        if (IsFlagSet(flags, REG_OPTION_BACKUP_RESTORE))
        {
            sout << prefix << "REG_OPTION_BACKUP_RESTORE";
            prefix = " | ";
        }
        if (IsFlagSet(flags, REG_OPTION_OPEN_LINK))
        {
            sout << prefix << "REG_OPTION_OPEN_LINK";
            prefix = " | ";
        }
        if (IsFlagSet(flags, REG_OPTION_DONT_VIRTUALIZE))
        {
            sout << prefix << "REG_OPTION_DONT_VIRTUALIZE";
            prefix = " | ";
        }
        sout << ")";
    }
    else
    {
        sout << " (REG_OPTION_NON_VOLATILE)"; // 0x0000
    }

    return sout.str();
}

inline void LogRegKeyDisposition(DWORD disposition, const char* msg = "Disposition")
{
    Log("\t%s=%d (", msg, disposition);
    LogIfEqual(disposition, REG_CREATED_NEW_KEY)
    else LogIfEqual(disposition, REG_OPENED_EXISTING_KEY)
    else Log("UNKNOWN");
    Log(")\n");
}

inline std::string InterpretRegKeyDisposition(DWORD disposition, const char* msg = "Disposition")
{
    std::ostringstream sout;
    sout << InterpretAsHex(msg,disposition);
    if (IsFlagEqual(disposition, REG_OPTION_OPEN_LINK))
    {
        sout << "REG_OPTION_OPEN_LINK";
    }
    else if (IsFlagEqual(disposition, REG_OPENED_EXISTING_KEY))
    {
        sout << "REG_OPENED_EXISTING_KEY";
    }
    else
    {
        sout << "UNKNOWN";
    }
    return sout.str();
}

inline void LogRegKeyQueryFlags(DWORD flags, const char* msg = "Flags")
{
    Log("\t%s=%08X", msg, flags);
    if (flags)
    {
        const char* prefix = "";
        Log(" (");
        LogIfFlagSet(flags, RRF_RT_REG_NONE);       // 0x00000001
        LogIfFlagSet(flags, RRF_RT_REG_SZ);         // 0x00000002
        LogIfFlagSet(flags, RRF_RT_REG_EXPAND_SZ);  // 0x00000004
        LogIfFlagSet(flags, RRF_RT_REG_BINARY);     // 0x00000008
        LogIfFlagSet(flags, RRF_RT_REG_DWORD);      // 0x00000010
        LogIfFlagSet(flags, RRF_RT_REG_MULTI_SZ);   // 0x00000020
        LogIfFlagSet(flags, RRF_RT_REG_QWORD);      // 0x00000040

        LogIfFlagSet(flags, RRF_SUBKEY_WOW6464KEY); // 0x00010000
        LogIfFlagSet(flags, RRF_SUBKEY_WOW6432KEY); // 0x00020000
        LogIfFlagSet(flags, RRF_NOEXPAND);          // 0x10000000
        LogIfFlagSet(flags, RRF_ZEROONFAILURE);     // 0x20000000
        Log(")");
    }

    Log("\n");
}

inline std::string InterpretRegKeyQueryFlags(DWORD flags, const char* msg = "Flags")
{
    std::ostringstream sout;
    sout << InterpretAsHex(msg, flags);
    if (flags)
    {
        const char* prefix = "";
        sout << " (";

        if (IsFlagSet(flags, RRF_RT_REG_NONE))
        {
            sout << prefix << "REG_NONE";
            prefix = " | ";
        }
        if (IsFlagSet(flags, RRF_RT_REG_SZ))
        {
            sout << prefix << "REG_SZ";
            prefix = " | ";
        }
        if (IsFlagSet(flags, RRF_RT_REG_EXPAND_SZ))
        {
            sout << prefix << "REG_EXPAND_SZ";
            prefix = " | ";
        }
        if (IsFlagSet(flags, RRF_RT_REG_BINARY))
        {
            sout << prefix << "REG_BINARY";
            prefix = " | ";
        }
        if (IsFlagSet(flags, RRF_RT_REG_DWORD))
        {
            sout << prefix << "REG_DWORD";
            prefix = " | ";
        }
        if (IsFlagSet(flags, RRF_RT_REG_MULTI_SZ))
        {
            sout << prefix << "REG_MULTI_SZ";
            prefix = " | ";
        }
        if (IsFlagSet(flags, RRF_RT_REG_QWORD))
        {
            sout << prefix << "REG_QWORD";
            prefix = " | ";
        }

        if (IsFlagSet(flags, RRF_SUBKEY_WOW6464KEY))
        {
            sout << prefix << "WOW6464KEY";
            prefix = " | ";
        }
        if (IsFlagSet(flags, RRF_SUBKEY_WOW6432KEY))
        {
            sout << prefix << "WOW6432KEY";
            prefix = " | ";
        }
        if (IsFlagSet(flags, RRF_NOEXPAND))
        {
            sout << prefix << "NOEXPAND";
            prefix = " | ";
        }
        if (IsFlagSet(flags, RRF_ZEROONFAILURE))
        {
            sout << prefix << "ZEROONFAILURE";
            prefix = " | ";
        }
        sout << ")";
    }

    return sout.str();
}

inline void LogRegKeyType(DWORD type, const char* msg = "Type")
{
    Log("\t%s=%08X (", msg, type);
    LogIfEqual(type, REG_NONE)
    else LogIfEqual(type, REG_SZ)
    else LogIfEqual(type, REG_EXPAND_SZ)
    else LogIfEqual(type, REG_BINARY)
    else LogIfEqual(type, REG_DWORD)
    else LogIfEqual(type, REG_DWORD_LITTLE_ENDIAN)
    else LogIfEqual(type, REG_DWORD_BIG_ENDIAN)
    else LogIfEqual(type, REG_LINK)
    else LogIfEqual(type, REG_MULTI_SZ)
    else LogIfEqual(type, REG_RESOURCE_LIST)
    else LogIfEqual(type, REG_FULL_RESOURCE_DESCRIPTOR)
    else LogIfEqual(type, REG_RESOURCE_REQUIREMENTS_LIST)
    else LogIfEqual(type, REG_QWORD)
    else LogIfEqual(type, REG_QWORD_LITTLE_ENDIAN)
    else Log("UNKNOWN");
    Log(")\n");
}

inline std::string InterpretRegKeyType(DWORD type, const char* msg = "Type")
{
    std::ostringstream sout;
    sout << InterpretAsHex(msg,type);

    if (IsFlagEqual(type, REG_NONE))
    {
        sout << " (REG_NONE)";
    }
    else if (IsFlagEqual(type, REG_SZ))
    {
        sout << " (REG_SZ)";
    }
    else if (IsFlagEqual(type, REG_EXPAND_SZ))
    {
        sout << " (REG_EXPAND_SZ)";
    }
    else if (IsFlagEqual(type, REG_BINARY))
    {
        sout << " (REG_BINARY)";
    }
    else if (IsFlagEqual(type, REG_DWORD))
    {
        sout << " (REG_DWORD)";
    }
    else if (IsFlagEqual(type, REG_DWORD_LITTLE_ENDIAN))
    {
        sout << " (REG_DWORD_LITTLE_ENDIAN)";
    }
    else if (IsFlagEqual(type, REG_DWORD_BIG_ENDIAN))
    {
        sout << " (REG_DWORD_BIG_ENDIAN)";
    }
    else if (IsFlagEqual(type, REG_LINK))
    {
        sout << " (REG_LINK)";
    }
    else if (IsFlagEqual(type, REG_MULTI_SZ))
    {
        sout << " (REG_MULTI_SZ)";
    }
    else if (IsFlagEqual(type, REG_RESOURCE_LIST))
    {
        sout << "REG_RESOURCE_LIST";
    }
    else if (IsFlagEqual(type, REG_FULL_RESOURCE_DESCRIPTOR))
    {
        sout << " (REG_FULL_RESOURCE_DESCRIPTOR)";
    }
    else if (IsFlagEqual(type, REG_RESOURCE_REQUIREMENTS_LIST))
    {
        sout << " (REG_RESOURCE_REQUIREMENTS_LIST)";
    }
    else if (IsFlagEqual(type, REG_QWORD))
    {
        sout << " (REG_QWORD)";
    }
    else if (IsFlagEqual(type, REG_QWORD_LITTLE_ENDIAN))
    {
        sout << " (REG_QWORD_LITTLE_ENDIAN)";
    }
    else
    {
        sout << " (UNKNOWN)";
    }
    return sout.str();
}

template <typename CharT>
inline void LogRegValue(DWORD type, const void* data, std::size_t dataSize, const char* msg = "Data")
{
    int counter = 0;
    switch (type)
    {
    case REG_SZ:
    case REG_EXPAND_SZ:
    case REG_LINK:
        LogCountedString(msg, reinterpret_cast<const CharT*>(data), dataSize / sizeof(CharT));
        break;

    case REG_MULTI_SZ:
    {
        int index = 0;
        for (auto str = reinterpret_cast<const CharT*>(data); *str; ++index)
        {
            std::size_t strLen;
            if constexpr (psf::is_ansi<CharT>)
            {
                Log("\t%s[%d]=%s\n", msg, index, str);
                strLen = std::strlen(str);
            }
            else
            {
                Log("\t%s[%d]=%ls\n", msg, index, str);
                strLen = std::wcslen(str);
            }

            // +1 for null character
            ++strLen;

            // There's at least another terminating null character, hence the +1 on the comparison
            assert(dataSize >= ((strLen + 1) * sizeof(CharT)));
            dataSize -= strLen * sizeof(CharT);
            str += strLen;
        }
    }   break;

    case REG_BINARY:
        Log("\t%s=", msg);
        for (auto ptr = reinterpret_cast<const unsigned char*>(data); dataSize--; ++ptr)
        {
            if (++counter < 16)
            {
                Log("%02X", *ptr);
            }
            else if (counter == 16)
            {
                Log("...");
            }                
        }
        Log("\n");
        break;

    case REG_DWORD_LITTLE_ENDIAN: // NOTE: Same as REG_DWORD
        Log("\t%s=%u\n", msg, *reinterpret_cast<const DWORD*>(data));
        break;

    case REG_DWORD_BIG_ENDIAN:
    {
        auto value = *reinterpret_cast<const DWORD*>(data);
        value = (value & 0xFF000000 >> 24) | (value & 0x00FF0000 >> 8) | (value & 0x0000FF00 << 8) | (value & 0x000000FF << 24);
        Log("\t%s=%u\n", msg, value);
    }   break;

    case REG_QWORD_LITTLE_ENDIAN: // NOTE: Same as REG_QWORD
        Log("\t%s=%llu\n", msg, *reinterpret_cast<const std::int64_t*>(data));
        break;

        // Ignore as they likely aren't important to applications
    case REG_RESOURCE_LIST:
    case REG_FULL_RESOURCE_DESCRIPTOR:
    case REG_RESOURCE_REQUIREMENTS_LIST:
        break;
    }
}

template <typename CharT>
inline std::string InterpretRegValueA(DWORD type, const void* data, std::size_t dataSize, const char* msg = "Data")
{
    std::ostringstream sout;
    int index = 0;
    
    switch (type)
    {
    case REG_SZ:
    case REG_EXPAND_SZ:
    case REG_LINK:
        sout << InterpretCountedString(msg, reinterpret_cast<const CharT*>(data), dataSize / sizeof(CharT));
        break;

    case REG_MULTI_SZ:
        sout << msg << "=";
        for (auto str = reinterpret_cast<const CharT*>(data); *str; ++index)
        {
            sout << "[";
            sout << std::dec << index;
            sout << "] ";
            std::size_t strLen;
            if constexpr (psf::is_ansi<CharT>)
            {
                sout << InterpretStringA(str) << "\n";
                strLen = std::strlen(str);
            }
            else
            {
                sout << InterpretStringA(str) << "\n";
                strLen = std::wcslen(str);
            }

            // +1 for null character
            ++strLen;

            // There's at least another terminating null character, hence the +1 on the comparison
            assert(dataSize >= ((strLen + 1) * sizeof(CharT)));
            dataSize -= strLen * sizeof(CharT);
            str += strLen;
        }
        break;

    case REG_BINARY:
        sout << msg << "=";
        for (auto ptr = reinterpret_cast<const unsigned char*>(data); dataSize--; ++ptr)
        {
            sout << " 0x" << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << *ptr;
        }
        break;

    case REG_DWORD_LITTLE_ENDIAN: // NOTE: Same as REG_DWORD
        sout << msg << "=";
        sout << " 0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << *reinterpret_cast<const DWORD*>(data);
        break;

    case REG_DWORD_BIG_ENDIAN:
        sout << msg << "="; 
        {
            auto value = *reinterpret_cast<const DWORD*>(data);
            value = (value & 0xFF000000 >> 24) | (value & 0x00FF0000 >> 8) | (value & 0x0000FF00 << 8) | (value & 0x000000FF << 24);
            sout << " 0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << value;
        }
        break;

    case REG_QWORD_LITTLE_ENDIAN: // NOTE: Same as REG_QWORD
        sout << msg << "=";
        sout << " 0x" << std::uppercase << std::setfill('0') << std::setw(16) << std::hex << *reinterpret_cast<const std::int64_t*>(data);
        break;

        // Ignore as they likely aren't important to applications
    case REG_RESOURCE_LIST:
    case REG_FULL_RESOURCE_DESCRIPTOR:
    case REG_RESOURCE_REQUIREMENTS_LIST:
        sout << msg << "=";
        sout << "[formatter not implemented]";
        break;
    }

    return sout.str();
}

inline void LogOpenFileStyle(WORD style, const char* msg = "Style")
{
    Log("\t%s=%08X", msg, style);
    if (style)
    {
        const char* prefix = "";
        Log(" (");
        LogIfFlagSet(style, OF_READ);               // 0x00000000
        LogIfFlagSet(style, OF_WRITE);              // 0x00000001
        LogIfFlagSet(style, OF_READWRITE);          // 0x00000002
        LogIfFlagSet(style, OF_SHARE_COMPAT);       // 0x00000000
        LogIfFlagSet(style, OF_SHARE_EXCLUSIVE);    // 0x00000010
        LogIfFlagSet(style, OF_SHARE_DENY_WRITE);   // 0x00000020
        LogIfFlagSet(style, OF_SHARE_DENY_READ);    // 0x00000030
        LogIfFlagSet(style, OF_SHARE_DENY_NONE);    // 0x00000040
        LogIfFlagSet(style, OF_PARSE);              // 0x00000100
        LogIfFlagSet(style, OF_DELETE);             // 0x00000200
        LogIfFlagSet(style, OF_VERIFY);             // 0x00000400
        LogIfFlagSet(style, OF_CANCEL);             // 0x00000800
        LogIfFlagSet(style, OF_CREATE);             // 0x00001000
        LogIfFlagSet(style, OF_PROMPT);             // 0x00002000
        LogIfFlagSet(style, OF_EXIST);              // 0x00004000
        LogIfFlagSet(style, OF_REOPEN);             // 0x00008000
        Log(")");
    }

    Log("\n");
}

inline std::string InterpretOpenFileStyle(WORD style, const char* msg = "Style")
{
    std::ostringstream sout;
    sout << InterpretAsHex(msg, style);
    if (style)
    {
        const char* prefix = "";
        sout << " (";

        if (IsFlagSet(style, OF_READ))
        {
            sout << prefix << "OF_READ";
            prefix = " | ";
        }
        if (IsFlagSet(style, OF_WRITE))
        {
            sout << prefix << "OF_WRITE";
            prefix = " | ";
        }
        if (IsFlagSet(style, OF_READWRITE))
        {
            sout << prefix << "OF_READWRITE";
            prefix = " | ";
        }
        if (IsFlagSet(style, OF_SHARE_COMPAT))
        {
            sout << prefix << "OF_SHARE_COMPAT";
            prefix = " | ";
        }
        if (IsFlagSet(style, OF_SHARE_EXCLUSIVE))
        {
            sout << prefix << "OF_SHARE_EXCLUSIVE";
            prefix = " | ";
        }
        if (IsFlagSet(style, OF_SHARE_DENY_WRITE))
        {
            sout << prefix << "OF_SHARE_DENY_WRITE";
            prefix = " | ";
        }
        if (IsFlagSet(style, OF_SHARE_DENY_READ))
        {
            sout << prefix << "OF_SHARE_DENY_READ";
            prefix = " | ";
        }
        if (IsFlagSet(style, OF_SHARE_DENY_NONE))
        {
            sout << prefix << "OF_SHARE_DENY_NONE";
            prefix = " | ";
        }
        if (IsFlagSet(style, OF_PARSE))
        {
            sout << prefix << "OF_PARSE";
            prefix = " | ";
        }
        if (IsFlagSet(style, OF_DELETE))
        {
            sout << prefix << "OF_DELETE";
            prefix = " | ";
        }
        if (IsFlagSet(style, OF_VERIFY))
        {
            sout << prefix << "OF_VERIFY";
            prefix = " | ";
        }
        if (IsFlagSet(style, OF_CANCEL))
        {
            sout << prefix << "OF_CANCEL";
            prefix = " | ";
        }
        if (IsFlagSet(style, OF_CREATE))
        {
            sout << prefix << "OF_CREATE";
            prefix = " | ";
        }
        if (IsFlagSet(style, OF_PROMPT))
        {
            sout << prefix << "OF_PROMPT";
            prefix = " | ";
        }
        if (IsFlagSet(style, OF_EXIST))
        {
            sout << prefix << "OF_EXIST";
            prefix = " | ";
        }
        if (IsFlagSet(style, OF_REOPEN))
        {
            sout << prefix << "OF_REOPEN";
            prefix = " | ";
        }
        sout << ")";
    }

    return sout.str();
}

inline void LogLoadLibraryFlags(DWORD flags, const char* msg = "Flags")
{
    Log("\t%s=%08X", msg, flags);
    if (flags)
    {
        constexpr DWORD LOAD_PACKAGED_LIBRARY = 0x00000004; // Documented, but not defined

        const char* prefix = "";
        Log(" (");
        LogIfFlagSet(flags, DONT_RESOLVE_DLL_REFERENCES);               // 0x00000001
        LogIfFlagSet(flags, LOAD_LIBRARY_AS_DATAFILE);                  // 0x00000002
        LogIfFlagSet(flags, LOAD_PACKAGED_LIBRARY);                     // 0x00000004
        LogIfFlagSet(flags, LOAD_WITH_ALTERED_SEARCH_PATH);             // 0x00000008
        LogIfFlagSet(flags, LOAD_IGNORE_CODE_AUTHZ_LEVEL);              // 0x00000010
        LogIfFlagSet(flags, LOAD_LIBRARY_AS_IMAGE_RESOURCE);            // 0x00000020
        LogIfFlagSet(flags, LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE);        // 0x00000040
        LogIfFlagSet(flags, LOAD_LIBRARY_REQUIRE_SIGNED_TARGET);        // 0x00000080
        LogIfFlagSet(flags, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);          // 0x00000100
        LogIfFlagSet(flags, LOAD_LIBRARY_SEARCH_APPLICATION_DIR);       // 0x00000200
        LogIfFlagSet(flags, LOAD_LIBRARY_SEARCH_USER_DIRS);             // 0x00000400
        LogIfFlagSet(flags, LOAD_LIBRARY_SEARCH_SYSTEM32);              // 0x00000800
        LogIfFlagSet(flags, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);          // 0x00001000
        LogIfFlagSet(flags, LOAD_LIBRARY_SAFE_CURRENT_DIRS);            // 0x00002000
        LogIfFlagSet(flags, LOAD_LIBRARY_SEARCH_SYSTEM32_NO_FORWARDER); // 0x00004000
        LogIfFlagSet(flags, LOAD_LIBRARY_OS_INTEGRITY_CONTINUITY);      // 0x00008000
        Log(")");
    }

    Log("\n");
}

inline std::string InterpretLoadLibraryFlags(DWORD flags, const char* msg = "Flags")
{
    std::ostringstream sout;
    sout << InterpretAsHex(msg, flags);
    if (flags)
    {
        constexpr DWORD LOAD_PACKAGED_LIBRARY = 0x00000004; // Documented, but not defined

        sout << " (";
        const char* prefix = "";

        if (IsFlagSet(flags, DONT_RESOLVE_DLL_REFERENCES))
        {
            sout << prefix << "DONT_RESOLVE_DLL_REFERENCES";
            prefix = " | ";
        }
        if (IsFlagSet(flags, LOAD_LIBRARY_AS_DATAFILE))
        {
            sout << prefix << "LOAD_LIBRARY_AS_DATAFILE";
            prefix = " | ";
        }
        if (IsFlagSet(flags, LOAD_PACKAGED_LIBRARY))
        {
            sout << prefix << "LOAD_PACKAGED_LIBRARY";
            prefix = " | ";
        }
        if (IsFlagSet(flags, LOAD_WITH_ALTERED_SEARCH_PATH))
        {
            sout << prefix << "LOAD_WITH_ALTERED_SEARCH_PATH";
            prefix = " | ";
        }
        if (IsFlagSet(flags, LOAD_IGNORE_CODE_AUTHZ_LEVEL))
        {
            sout << prefix << "LOAD_IGNORE_CODE_AUTHZ_LEVEL";
            prefix = " | ";
        }
        if (IsFlagSet(flags, LOAD_LIBRARY_AS_IMAGE_RESOURCE))
        {
            sout << prefix << "LOAD_LIBRARY_AS_IMAGE_RESOURCE";
            prefix = " | ";
        }
        if (IsFlagSet(flags, LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE))
        {
            sout << prefix << "LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE";
            prefix = " | ";
        }
        if (IsFlagSet(flags, LOAD_LIBRARY_REQUIRE_SIGNED_TARGET))
        {
            sout << prefix << "LOAD_LIBRARY_REQUIRE_SIGNED_TARGET";
            prefix = " | ";
        }
        if (IsFlagSet(flags, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR))
        {
            sout << prefix << "LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR";
            prefix = " | ";
        }
        if (IsFlagSet(flags, LOAD_LIBRARY_SEARCH_APPLICATION_DIR))
        {
            sout << prefix << "LOAD_LIBRARY_SEARCH_APPLICATION_DIR";
            prefix = " | ";
        }
        if (IsFlagSet(flags, LOAD_LIBRARY_SEARCH_USER_DIRS))
        {
            sout << prefix << "LOAD_LIBRARY_SEARCH_USER_DIRS";
            prefix = " | ";
        }
        if (IsFlagSet(flags, LOAD_LIBRARY_SEARCH_SYSTEM32))
        {
            sout << prefix << "LOAD_LIBRARY_SEARCH_SYSTEM32";
            prefix = " | ";
        }
        if (IsFlagSet(flags, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS))
        {
            sout << prefix << "LOAD_LIBRARY_SEARCH_DEFAULT_DIRS";
            prefix = " | ";
        }
        if (IsFlagSet(flags, LOAD_LIBRARY_SAFE_CURRENT_DIRS))
        {
            sout << prefix << "LOAD_LIBRARY_SAFE_CURRENT_DIRS";
            prefix = " | ";
        }
        if (IsFlagSet(flags, LOAD_LIBRARY_SEARCH_SYSTEM32_NO_FORWARDER))
        {
            sout << prefix << "LOAD_LIBRARY_SEARCH_SYSTEM32_NO_FORWARDER";
            prefix = " | ";
        }
        if (IsFlagSet(flags, LOAD_LIBRARY_OS_INTEGRITY_CONTINUITY))
        {
            sout << prefix << "LOAD_LIBRARY_OS_INTEGRITY_CONTINUITY";
            prefix = " | ";
        }
        sout << ")";
    }

    return sout.str();
}
