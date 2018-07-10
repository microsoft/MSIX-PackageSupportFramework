//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string_view>

#include <fancy_handle.h>

#include "console_output.h"
#include "offset_ptr.h"

static constexpr wchar_t test_runner_pipe_name[] = LR"(\\.\pipe\CentennialShimsTests)";

using unique_handle = std::unique_ptr<void, shims::handle_deleter<::CloseHandle>>;

inline unique_handle test_client_connect()
{
    using namespace std::literals;
    constexpr auto waitTimeout = std::chrono::duration_cast<std::chrono::milliseconds>(20s);

    unique_handle result;
    while (!result)
    {
        if (!::WaitNamedPipeW(test_runner_pipe_name, static_cast<DWORD>(waitTimeout.count())))
        {
            print_last_error("Failed waiting for named pipe to become available");
            break;
        }
        else
        {
            // Pipe is available. Try and connect to it. Note that we could fail if someone else beats us, but that's
            // okay. We shouldn't get starved since there's a finite number of tests
            result.reset(::CreateFileW(
                test_runner_pipe_name,
                GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr,
                OPEN_EXISTING,
                0,
                nullptr));
        }
    }

    return result;
}

enum class test_message_type : std::int16_t
{
    init,
    cleanup,

    begin,
    end,

    trace,
};

struct test_message
{
    test_message_type type;
    std::int32_t size; // Size, in bytes, of the complete message
};

inline const test_message* advance(const test_message* msg) noexcept
{
    return reinterpret_cast<const test_message*>(reinterpret_cast<const std::uint8_t*>(msg) + msg->size);
}

struct init_test_message
{
    test_message header = { test_message_type::init };
    offset_ptr<char> name;
    std::int32_t count = 0; // Number of tests
};

struct cleanup_test_message
{
    test_message header = { test_message_type::cleanup };
};

struct test_begin_message
{
    test_message header = { test_message_type::begin };
    offset_ptr<char> name;
};

struct test_end_message
{
    test_message header = { test_message_type::end };
    std::int32_t result = 0;
};

struct test_trace_message
{
    test_message header = { test_message_type::trace };
    offset_ptr<wchar_t> text;
    console::color color = console::color::gray;
    std::int8_t print_new_line = false; // NOTE: Interpreted as a boolean value
};
