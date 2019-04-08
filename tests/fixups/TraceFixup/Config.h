//-------------------------------------------------------------------------------------------------------    
// Copyright (C) Microsoft Corporation. All rights reserved.    
// Licensed under the MIT license. See LICENSE file in the project root for full license information.    
//-------------------------------------------------------------------------------------------------------    
#pragma once    
    
#include <ntstatus.h>    
#include <windows.h>    
#include <winternl.h>    
    
enum class trace_method    
{    
    printf,    
    output_debug_string,    
	eventlog,    
};    
    
enum class function_result    
{    
    // The function succeeded    
    success,    
    
    // Function does not report success/failure (e.g. void-returning function)    
    indeterminate,    
    
    // The function failed, but the failure is a common expected failure (e.g. file not found, insufficient buffer, etc.)    
    expected_failure,    
    
    // The function failed    
    failure,    
};    
    
inline constexpr bool function_succeeded(function_result result)    
{    
    // NOTE: Only true for explicit success    
    return result == function_result::success;    
}    
    
inline constexpr bool function_failed(function_result result)    
{    
    return result >= function_result::expected_failure;    
}    
    
inline function_result from_win32(DWORD code)    
{    
    switch (code)    
    {    
    case ERROR_SUCCESS:    
        return function_result::success;    
    
    case ERROR_FILE_NOT_FOUND:    
    case ERROR_PATH_NOT_FOUND:    
    case ERROR_INVALID_NAME:    
    case ERROR_ALREADY_EXISTS:    
    case ERROR_FILE_EXISTS:    
    case ERROR_INSUFFICIENT_BUFFER:    
    case ERROR_MORE_DATA:    
    case ERROR_NO_MORE_ITEMS:    
    case ERROR_NO_MORE_FILES:    
    case ERROR_MOD_NOT_FOUND:    
        return function_result::expected_failure;    
    
    default:    
        return function_result::failure;    
    }    
}    
inline std::string InterpretFrom_win32(DWORD code)    
{    
	switch (code)    
	{    
	case ERROR_SUCCESS:    
		return "Success";    
	case ERROR_FILE_NOT_FOUND:    
		return "File not found";    
	case ERROR_PATH_NOT_FOUND:    
		return "Path not found";    
	case ERROR_INVALID_NAME:    
		return "Invalid Name";    
	case ERROR_ALREADY_EXISTS:    
		return "Already exists";    
	case ERROR_FILE_EXISTS:    
		return "File exists";    
	case ERROR_INSUFFICIENT_BUFFER:    
		return "Buffer overflow";    
	case ERROR_MORE_DATA:    
		return "More data";    
	case ERROR_NO_MORE_ITEMS:    
		return "No more items";    
	case ERROR_NO_MORE_FILES:    
		return "No more files";    
	case ERROR_MOD_NOT_FOUND:    
		return "Module not found";    
	default:    
		return "Unknown failure";    
	}    
}    
    
inline function_result from_win32_bool(BOOL value)    
{    
    return value ? function_result::success : from_win32(::GetLastError());    
}    
    
inline function_result from_ntstatus(NTSTATUS status)    
{    
    if (NT_SUCCESS(status))    
    {    
        return function_result::success;    
    }    
    
    switch (status)    
    {    
    case STATUS_BUFFER_OVERFLOW:    
    case STATUS_BUFFER_TOO_SMALL:    
    case STATUS_OBJECT_NAME_NOT_FOUND:    
    case STATUS_OBJECT_PATH_NOT_FOUND:    
    case STATUS_OBJECT_NAME_COLLISION:    
    case STATUS_OBJECT_NAME_INVALID:    
        return function_result::expected_failure;    
    
    default:    
        return function_result::failure;    
    }    
}    
    
inline function_result from_hresult(HRESULT hr)    
{    
    if (SUCCEEDED(hr))    
    {    
        return function_result::success;    
    }    
    
    if (HRESULT_FACILITY(hr) == FACILITY_WIN32)    
    {    
        return from_win32(HRESULT_CODE(hr));    
    }    
    
    return function_result::failure;    
}    
    
inline function_result from_lzerror(INT err)    
{    
    if (err >= 0)    
    {    
        return function_result::success;    
    }    
    
    return function_result::failure;    
}    
    
enum class function_type    
{    
    filesystem,    
    
    registry,    
    
    process_and_thread,    
    
    dynamic_link_library,    
};    
    
// NOTE: Function entry tracing unaffected by these settings    
enum class trace_level    
{    
    // Always traces function invocation, even if the function succeeded    
    always,    
    
    // Ignores success    
    ignore_success,    
    
    // Trace all failures, even expected ones    
    all_failures,    
    
    // Only traces failures that aren't considered "common" (file not found, etc.)    
    unexpected_failures,    
    
    // Don't trace any output (except function entry, if enabled)    
    ignore,    
};    
    
    
    
extern trace_method output_method;    
extern bool wait_for_debugger;    
extern bool trace_function_entry;    
extern bool trace_calling_module;    
extern bool ignore_dll_load;    
    
extern void Log_ETW_PostMsgA(const char *);    
extern void Log_ETW_PostMsgW(const wchar_t *);    
extern void Log_ETW_PostMsgOperationA(const char *operation, const char *inputs, const char *result, const char *outputs, const char *caller, LARGE_INTEGER TickStart, LARGE_INTEGER TickEnd );    
    
struct result_configuration    
{    
    bool should_log;    
    bool should_break;    
};    
    
result_configuration configured_result(function_type type, function_result result);
