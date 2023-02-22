//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
#include <Windows.h>

#include <debug.h>

#define PSF_DEFINE_EXPORTS
#include <psf_framework.h>

#include "Config.h"

//////// Need to undefine Preprocessor definition for NONAMELESSUNION on this .cpp file only for this to work
#include "psf_tracelogging.h"

#include "Logging.h"

// This handles event logging via ETW
// NOTE: The provider name and GUID must be kept in sync with PsfShimMonitor/MainWindow.xaml.cs
//       The format of the provider name uses dots here and dashes in C#.
TRACELOGGING_DEFINE_PROVIDER(
    g_Log_ETW_ComponentProvider,
    "Microsoft.Windows.PSFRuntime",
    (0xf7f4e8c4, 0x9981, 0x5221, 0xe6, 0xfb, 0xff, 0x9d, 0xd1, 0xcd, 0xa4, 0xe1),
    TraceLoggingOptionMicrosoftTelemetry());

using namespace std::literals;

trace_method output_method = trace_method::output_debug_string;
bool wait_for_debugger = false;
bool trace_function_entry = false;
bool trace_calling_module = true;
bool ignore_dll_load = true;

static const psf::json_object* g_traceLevels = nullptr;
static trace_level g_defaultTraceLevel = trace_level::unexpected_failures;

static const psf::json_object* g_breakLevels = nullptr;
static trace_level g_defaultBreakLevel = trace_level::ignore;





static trace_level trace_level_from_configuration(std::string_view str, trace_level defaultLevel)
{
    if (str == "always"sv)
    {
        return trace_level::always;
    }
    else if (str == "ignoreSuccess"sv)
    {
        return trace_level::ignore_success;
    }
    else if (str == "allFailures"sv)
    {
        return trace_level::all_failures;
    }
    else if (str == "unexpectedFailures"sv)
    {
        return trace_level::unexpected_failures;
    }
    else if (str == "ignore"sv)
    {
        return trace_level::ignore;
    }

    // Unknown input; use the default
    return defaultLevel;
}

static trace_level configured_level(function_type type, const psf::json_object* configuredLevels, trace_level defaultLevel)
{
    if (!configuredLevels)
    {
        return defaultLevel;
    }

    auto impl = [&](const char* configKey)
    {
        if (auto config = configuredLevels->try_get(configKey))
        {
            return trace_level_from_configuration(config->as_string().string(), defaultLevel);
        }

        return defaultLevel;
    };

    switch (type)
    {
    case function_type::filesystem:
        return impl("filesystem");

    case function_type::registry:
        return impl("registry");

    case function_type::process_and_thread:
        return impl("processAndThread");
        break;

    case function_type::dynamic_link_library:
        return impl("dynamicLinkLibrary");
        break;
    }

    // Fallback to default
    return defaultLevel;
}

static trace_level configured_trace_level(function_type type)
{
    return configured_level(type, g_traceLevels, g_defaultTraceLevel);
}

static trace_level configured_break_level(function_type type)
{
    return configured_level(type, g_breakLevels, g_defaultBreakLevel);
}

result_configuration configured_result(function_type type, function_result result)
{
    auto impl = [&](trace_level level)
    {
        switch (level)
        {
        case trace_level::always:
            return result >= function_result::success;
            break;

        case trace_level::ignore_success:
            return result >= function_result::indeterminate;
            break;

        case trace_level::all_failures:
            return result >= function_result::expected_failure;
            break;

        case trace_level::unexpected_failures:
            return result >= function_result::failure;
            break;

        case trace_level::ignore:
        default:
            return false;
        }
    };

    return { impl(configured_trace_level(type)), impl(configured_break_level(type)) };
}


// Set up the ETW Provider
void Log_ETW_Register()
{
    try
    {
        TraceLoggingRegister(g_Log_ETW_ComponentProvider);
    }
    catch (...)
    {
        // Unable to log should not crash an app. 
        ::OutputDebugStringW(L"Unable to attach to trace logging.");
    }
}

void Log_ETW_PostMsgA(const char* s)
{
    try
    {
        TraceLoggingWrite(g_Log_ETW_ComponentProvider, // handle to my provider
            "TraceEvent",              // Event Name that should uniquely identify your event.
            TraceLoggingValue(s, "Message")); // Field for your event in the form of (value, field name).
    }
    catch (...)
    {
        // Unable to log should not crash an app. 
        ::OutputDebugStringA("Unable to write to trace.");
    }
}

void Log_ETW_PostMsgOperationA(const char* operation, const char* inputs, const char* result, const char* outputs, const char* callingmodule, LARGE_INTEGER TickStart, LARGE_INTEGER TickEnd)
{
    try
    {
        TraceLoggingWrite(0x00, // handle to my provider
            "TraceEvent",              // Event Name that should uniquely identify your event.
            TraceLoggingValue(operation, "Operation"),
            TraceLoggingValue(inputs, "Inputs"),
            TraceLoggingValue(result, "Result"),
            TraceLoggingValue(outputs, "Outputs"),
            TraceLoggingValue(callingmodule, "Caller"),
            TraceLoggingInt64(TickStart.QuadPart, "Start"),
            TraceLoggingInt64(TickEnd.QuadPart, "End"),
            TraceLoggingBoolean(TRUE, "UTCReplace_AppSessionGuid"),
            TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES)
        ); // Field for your event in the form of (value, field name).
    }
    catch (...)
    {
        // Unable to log should not crash an app. 
        ::OutputDebugStringA("Unable to write to trace.");
    }
} 

void Log_ETW_PostMsgW(const wchar_t* s)
{
    try
    {
        TraceLoggingWrite(g_Log_ETW_ComponentProvider, // handle to my provider
            "TraceEvent",              // Event Name that should uniquely identify your event.
            TraceLoggingValue(s, "Message")); // Field for your event in the form of (value, field name).
    }
    catch (...)
    {
        // Unable to log should not crash an app. 
        ::OutputDebugStringW(L"Unable to write to trace.");
    }
}


// Tear down the ETW Provider
void Log_ETW_UnRegister()
{
    try
    {
        TraceLoggingUnregister(g_Log_ETW_ComponentProvider);
    }
    catch (...)
    {
        // Unable to log should not crash an app. 
        ::OutputDebugStringW(L"Unable to close trace.");
    }
}

BOOL __stdcall DllMain(HINSTANCE, DWORD reason, LPVOID) noexcept try
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        Log_ETW_Register();

        std::wstringstream traceDataStream;

        if (auto config = PSFQueryCurrentDllConfig())
        {
            auto& configObj = config->as_object();

            traceDataStream << " config:\n";
            if (auto method = configObj.try_get("traceMethod"))
            {
                auto methodStr = method->as_string().string();
                traceDataStream << " traceMethod:" << method->as_string().wide() << " ;";

                if (methodStr == "printf"sv)
                {
                    output_method = trace_method::printf;
                    Log("config traceMethod is printf");
                }
                else if (methodStr == "eventlog"sv)
                {
                    output_method = trace_method::eventlog;
                    Log("config traceMethod is eventlog");
                }
                else {
                    // Otherwise, use the default (OutputDebugString)
                    Log("config traceMethod is default");
                }
            }

            if (auto levels = configObj.try_get("traceLevels"))
            {
                g_traceLevels = &levels->as_object();
                traceDataStream << " traceLevels:\n";

                // Set default level immediately for fallback purposes
                if (auto defaultLevel = g_traceLevels->try_get("default"))
                {
                    traceDataStream << " default:" << defaultLevel->as_string().wide() << " ;";
                    g_defaultTraceLevel = trace_level_from_configuration(defaultLevel->as_string().string(), g_defaultTraceLevel);
                }
            }

            if (auto levels = configObj.try_get("breakOn"))
            {
                g_breakLevels = &levels->as_object();
                traceDataStream << " breakOn:\n";

                // Set default level immediately for fallback purposes
                if (auto defaultLevel = g_breakLevels->try_get("default"))
                {
                    traceDataStream << " default:" << defaultLevel->as_string().wide() << " ;";
                    g_defaultBreakLevel = trace_level_from_configuration(defaultLevel->as_string().string(), g_defaultBreakLevel);
                }
            }

            if (auto debuggerConfig = configObj.try_get("waitForDebugger"))
            {
                traceDataStream << " waitForDebugger:" << static_cast<bool>(debuggerConfig->as_boolean()) << " ;";
                wait_for_debugger = static_cast<bool>(debuggerConfig->as_boolean());
            }

            if (auto traceEntryConfig = configObj.try_get("traceFunctionEntry"))
            {
                traceDataStream << " traceFunctionEntry:" << static_cast<bool>(traceEntryConfig->as_boolean()) << " ;";
                trace_function_entry = static_cast<bool>(traceEntryConfig->as_boolean());
            }

            if (auto callerConfig = configObj.try_get("traceCallingModule"))
            {
                traceDataStream << " traceCallingModule:" << static_cast<bool>(callerConfig->as_boolean()) << " ;";
                trace_calling_module = static_cast<bool>(callerConfig->as_boolean());
            }

            if (auto ignoreDllConfig = configObj.try_get("ignoreDllLoad"))
            {
                traceDataStream << " ignoreDllLoad:" << static_cast<bool>(ignoreDllConfig->as_boolean()) << " ;";
                ignore_dll_load = static_cast<bool>(ignoreDllConfig->as_boolean());
            }
            try
            {
                psf::TraceLogFixupConfig("TraceFixup", traceDataStream.str().c_str());
            }
            catch (...)
            {
                // Unable to log should not crash an app. 
                ::OutputDebugStringW(L"Unable to write to trace");
            }
        }

        if (wait_for_debugger)
        {
            psf::wait_for_debugger();
        }
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        Log_ETW_UnRegister();
    }

    return TRUE;
}
catch (...)
{
    psf::TraceLogExceptions("TraceFixupException", "TraceFixup attach ERROR");
    ::SetLastError(win32_from_caught_exception());
    return FALSE;
}
