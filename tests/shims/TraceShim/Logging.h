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

#include <shim_utils.h>

#include "Config.h"

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
        std::wstring str;
        str.resize(256);
        while (true)
        {
            va_list args;
            va_start(args, fmt);
            auto count = std::vswprintf(str.data(), str.size() + 1, fmt, args);
            va_end(args);

            if (count >= 0)
            {
                str.resize(count);
                break;
            }

            str.resize(str.size() * 2);
        }

        ::OutputDebugStringW(str.c_str());
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

inline void LogCountedString(const char* name, const char* value, std::size_t length)
{
    Log("\t%s=%.*s\n", name, length, value);
}

inline void LogCountedString(const char* name, const wchar_t* value, std::size_t length)
{
    Log("\t%s=%.*ls\n", name, length, value);
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

inline void LogFunctionResult(function_result result, const char* msg = "Result")
{
    const char* resultMsg = "Unknown";
    switch (result)
    {
    case function_result::success:
        resultMsg = "Success";
        break;

    case function_result::indeterminate:
        resultMsg = "N/A";
        break;

    case function_result::expected_failure:
        resultMsg = "Expected Failure";
        break;

    case function_result::failure:
        resultMsg = "Failure";
        break;
    }

    Log("\t%s=%s\n", msg, resultMsg);
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

inline void LogLastError(const char* msg = "Last Error")
{
    LogWin32Error(::GetLastError(), msg);
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

inline void LogLZError(INT err)
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

    Log("\tLZError=%d (%s)\n", err, msg);
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
            Log(L"\tCalling Module=%ls\n", shims::get_module_path(moduleHandle).c_str()); \
        } \
    }

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

// RAII helper for handling the 'trace_function_entry' configuration
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

                // Most functions are named "SomeFunctionShim" where the target API is "SomeFunction". Logging the API
                // is much more helpful than the Shimmed function name, so remove the "Shim" suffix, if present
                using namespace std::literals;
                constexpr auto shimSuffix = "Shim"sv;
                std::string name = functionName;
                if ((name.length() >= shimSuffix.length()) && (name.substr(name.length() - shimSuffix.length()) == shimSuffix))
                {
                    name.resize(name.length() - shimSuffix.length());
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

inline void LogInfoLevelId(GET_FILEEX_INFO_LEVELS infoLevelId, const char* msg = "Level")
{
    Log("\t%s=%d (", msg, infoLevelId);
    LogIfEqual(infoLevelId, GetFileExInfoStandard)
    else Log("UNKNOWN");
    Log(")\n");
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
#ifdef FILE_ATTRIBUTE_STRICTLY_SEQUENTIAL
        LogIfFlagSet(attributes, FILE_ATTRIBUTE_STRICTLY_SEQUENTIAL);
#endif
        Log(")");
    }

    Log("\n");
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

inline void LogInfoLevelId(FINDEX_INFO_LEVELS infoLevelId, const char* msg = "Info Level Id")
{
    Log("\t%s=%d (", msg, infoLevelId);
    LogIfEqual(infoLevelId, FindExInfoStandard)
    else LogIfEqual(infoLevelId, FindExInfoBasic)
    else Log("UNKNOWN");
    Log(")\n");
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

inline void LogRegKeyDisposition(DWORD disposition, const char* msg = "Disposition")
{
    Log("\t%s=%d (", msg, disposition);
    LogIfEqual(disposition, REG_CREATED_NEW_KEY)
    else LogIfEqual(disposition, REG_OPENED_EXISTING_KEY)
    else Log("UNKNOWN");
    Log(")\n");
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

inline void LogRegKeyType(DWORD type, const char* msg = "Type")
{
    Log("\t%s=%d (", msg, type);
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

template <typename CharT>
inline void LogRegValue(DWORD type, const void* data, std::size_t dataSize, const char* msg = "Data")
{
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
            if constexpr (shims::is_ansi<CharT>)
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
            Log("%02X", *ptr);
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
