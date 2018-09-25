//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <debug.h>

#define PSF_DEFINE_EXPORTS
#include <psf_framework.h>

#include "Config.h"

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

BOOL __stdcall DllMain(HINSTANCE, DWORD reason, LPVOID) noexcept try
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (auto config = PSFQueryCurrentDllConfig())
        {
            auto& configObj = config->as_object();

            if (auto method = configObj.try_get("traceMethod"))
            {
                auto methodStr = method->as_string().string();
                if (methodStr == "printf"sv)
                {
                    output_method = trace_method::printf;
                }
                else
                {
                    // Otherwise, use the default (OutputDebugString)
                }
            }

            if (auto levels = configObj.try_get("traceLevels"))
            {
                g_traceLevels = &levels->as_object();

                // Set default level immediately for fallback purposes
                if (auto defaultLevel = g_traceLevels->try_get("default"))
                {
                    g_defaultTraceLevel = trace_level_from_configuration(defaultLevel->as_string().string(), g_defaultTraceLevel);
                }
            }

            if (auto levels = configObj.try_get("breakOn"))
            {
                g_breakLevels = &levels->as_object();

                // Set default level immediately for fallback purposes
                if (auto defaultLevel = g_breakLevels->try_get("default"))
                {
                    g_defaultBreakLevel = trace_level_from_configuration(defaultLevel->as_string().string(), g_defaultBreakLevel);
                }
            }

            if (auto debuggerConfig = configObj.try_get("waitForDebugger"))
            {
                wait_for_debugger = static_cast<bool>(debuggerConfig->as_boolean());
            }

            if (auto traceEntryConfig = configObj.try_get("traceFunctionEntry"))
            {
                trace_function_entry = static_cast<bool>(traceEntryConfig->as_boolean());
            }

            if (auto callerConfig = configObj.try_get("traceCallingModule"))
            {
                trace_calling_module = static_cast<bool>(callerConfig->as_boolean());
            }

            if (auto ignoreDllConfig = configObj.try_get("ignoreDllLoad"))
            {
                ignore_dll_load = static_cast<bool>(ignoreDllConfig->as_boolean());
            }
        }

        if (wait_for_debugger)
        {
            psf::wait_for_debugger();
        }
    }

    return TRUE;
}
catch (...)
{
    ::SetLastError(win32_from_caught_exception());
    return FALSE;
}
