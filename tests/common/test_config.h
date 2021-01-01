//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <map>
#include <string>
#include <string_view>

#include <utilities.h>

#include "error_logging.h"
#include "test_runner.h"

inline unique_handle g_testRunnerPipe;
inline std::int32_t g_testCount = 0;
inline std::int32_t g_successCount = 0;
inline std::int32_t g_failureCount = 0;

inline int parse_args(int argc, const wchar_t** argv, std::map<std::wstring_view, std::wstring>& allowedArgs)
{
    using namespace std::literals;

    for (int i = 1; i < argc; ++i)
    {
        const auto str = argv[i];
        std::size_t colonPos = 0;
        for (; str[colonPos] && (str[colonPos] != L':'); ++colonPos);

        bool handled = false;
        if (str[colonPos])
        {
            std::wstring_view arg(str, colonPos);
            std::wstring_view value(str + colonPos + 1);
            if (arg == L"/mode"sv)
            {
                if (value == L"test")
                {
                    g_testRunnerPipe = test_client_connect();
                    if (!g_testRunnerPipe)
                    {
                        return print_last_error("Failed to open pipe");
                    }
                    handled = true;
                }
            }
            else if (auto itr = allowedArgs.find(arg); itr != allowedArgs.end())
            {
                itr->second = value;
                handled = true;
            }
        }

        if (!handled)
        {
            std::wcout << error_text() << "ERROR: Unknown argument: " << error_info_text() << argv[i] << "\n";
            return ERROR_INVALID_PARAMETER;
        }
    }

    return ERROR_SUCCESS;
}

inline int parse_args(int argc, const wchar_t** argv)
{
    std::map<std::wstring_view, std::wstring> allowedArgs;
    return parse_args(argc, argv, allowedArgs);
}

// There's no convenient way to preserve compile-time knowledge of null-terminated string lengths. string_view comes
// close, but lacks the null-terminated guarantee :(
template <typename CharT>
struct basic_null_terminated_string_view
{
    const CharT* ptr;
    std::size_t length;

    template <
        typename String,
        std::enable_if_t<std::is_pointer_v<String>, int> = 0,
        std::enable_if_t<std::is_same_v<CharT, std::remove_cv_t<std::remove_pointer_t<String>>>, int> = 0>
    basic_null_terminated_string_view(String str) :
        ptr(str),
        length(std::char_traits<CharT>::length(str))
    {
    }

    template <std::size_t Size>
    basic_null_terminated_string_view(CharT (&str)[Size]) :
        basic_null_terminated_string_view(static_cast<const CharT*>(str))
    {
    }

    template <std::size_t Size>
    basic_null_terminated_string_view(const CharT (&str)[Size]) :
        ptr(str),
        length(Size - 1) // Because of the null character
    {
        assert(str[Size - 1] == '\0');
    }

    basic_null_terminated_string_view(const std::basic_string<CharT>& str) :
        ptr(str.c_str()),
        length(str.length())
    {
    }
};

using null_terminated_string_view = basic_null_terminated_string_view<char>;
using wnull_terminated_string_view = basic_null_terminated_string_view<wchar_t>;

inline int test_initialize(null_terminated_string_view name, std::int32_t testCount)
{
    if (g_testRunnerPipe)
    {
        // NOTE: Adding one to string length for null character
        init_test_message msg = {};
        msg.header.size = static_cast<std::int32_t>(sizeof(msg) + name.length + 1);
        msg.count = testCount;
        msg.name.reset(reinterpret_cast<char*>(&msg + 1));

        if (!::WriteFile(g_testRunnerPipe.get(), &msg, sizeof(msg), nullptr, nullptr))
        {
            return print_last_error("Failed to send init message to test server");
        }

        if (!::WriteFile(g_testRunnerPipe.get(), name.ptr, static_cast<DWORD>(name.length + 1), nullptr, nullptr))
        {
            return print_last_error("Failed to send init message to test server");
        }
    }
    else
    {
        g_testCount = testCount;
        std::wcout << console::change_foreground(console::color::cyan) <<
            "================================================================================\n" <<
            "Test Initialize\n" <<
            "Test Name:  " << console::change_foreground(console::color::dark_cyan) << name.ptr << console::revert_foreground() << "\n" <<
            "Test Count: " << console::change_foreground(console::color::dark_cyan) << testCount << console::revert_foreground() << "\n" <<
            "================================================================================\n";
    }

    return ERROR_SUCCESS;
}

inline int test_cleanup()
{
    if (g_testRunnerPipe)
    {
        cleanup_test_message msg = {};
        msg.header.size = sizeof(msg);

        if (!::WriteFile(g_testRunnerPipe.get(), &msg, sizeof(msg), nullptr, nullptr))
        {
            return print_last_error("Failed to send cleanup message to test server");
        }
    }
    else
    {
        std::wcout << console::change_foreground(console::color::cyan) <<
            "================================================================================\n" <<
            "Test Cleanup\n" <<
            "Test Count: " << console::change_foreground(console::color::dark_cyan) << g_testCount << console::revert_foreground() << "\n"
            "Succeeded:  " << console::change_foreground(console::color::dark_cyan) << g_successCount << console::revert_foreground() << "\n"
            "Failed:     " << console::change_foreground(console::color::dark_cyan) << g_failureCount << console::revert_foreground() << "\n"
            "================================================================================\n";
    }

    return ERROR_SUCCESS;
}

inline int test_begin(null_terminated_string_view name)
{
    if (g_testRunnerPipe)
    {
        test_begin_message msg = {};
        msg.header.size = static_cast<std::int32_t>(sizeof(msg) + name.length + 1);
        msg.name.reset(reinterpret_cast<char*>(&msg + 1));

        if (!::WriteFile(g_testRunnerPipe.get(), &msg, sizeof(msg), nullptr, nullptr))
        {
            return print_last_error("Failed to send test begin message to test server");
        }

        if (!::WriteFile(g_testRunnerPipe.get(), name.ptr, static_cast<DWORD>(name.length + 1), nullptr, nullptr))
        {
            return print_last_error("Failed to send test begin message to test server");
        }
    }
    else
    {
        std::wcout << console::change_foreground(console::color::magenta) << "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n";
        std::wcout << "Test Begin: " << info_text() << name.ptr << "\n";
        std::wcout << "--------------------------------------------------------------------------------\n";
    }

    return ERROR_SUCCESS;
}

inline int test_end(std::int32_t result)
{
    if (g_testRunnerPipe)
    {
        test_end_message msg = {};
        msg.header.size = static_cast<std::int32_t>(sizeof(msg));
        msg.result = result;

        if (!::WriteFile(g_testRunnerPipe.get(), &msg, sizeof(msg), nullptr, nullptr))
        {
            return print_last_error("Failed to send test begin message to test server");
        }
    }
    else
    {
        std::wcout << "--------------------------------------------------------------------------------\n";
        std::wcout << "Test End\n" << "Result: ";
        if (result)
        {
            ++g_failureCount;
            std::wcout << error_text() << "FAILED\n";
            std::wcout << "Error Code: " << error_text() << result;
            std::wcout << " (" << error_info_text() << error_message(result).c_str() << console::revert_foreground() << ")\n";
        }
        else
        {
            ++g_successCount;
            std::wcout << success_text() << "SUCCESS\n";
        }

        std::wcout << console::change_foreground(console::color::magenta) << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n";
    }

    return ERROR_SUCCESS;
}

inline void trace_message(
    wnull_terminated_string_view message,
    console::color color = console::color::gray,
    bool newLine = false)
{
    if (g_testRunnerPipe)
    {
        test_trace_message msg = {};
        msg.header.size = static_cast<std::int32_t>(sizeof(msg) + (message.length + 1) * 2);
        msg.color = color;
        msg.print_new_line = newLine;
        msg.text.reset(reinterpret_cast<wchar_t*>(&msg + 1));

        if (!::WriteFile(g_testRunnerPipe.get(), &msg, sizeof(msg), nullptr, nullptr))
        {
            throw_win32(print_last_error("Failed to send test begin message to test server"));
        }

        if (!::WriteFile(g_testRunnerPipe.get(), message.ptr, static_cast<DWORD>((message.length + 1) * 2), nullptr, nullptr))
        {
            throw_win32(print_last_error("Failed to send test begin message to test server"));
        }
    }
    else
    {
        std::wcout << console::change_foreground(color) << message.ptr;
        if (newLine)
        {
            std::wcout << L"\n";
        }
    }
}

inline void trace_message(
    null_terminated_string_view message,
    console::color color = console::color::gray,
    bool newLine = false)
{
    trace_message(widen(std::string_view{ message.ptr, static_cast<UINT>(message.length) }), color, newLine);
}

struct new_line_t {};
constexpr new_line_t new_line = {};

// NOTE: Deduction guides don't work for constructing function arguments :(. Fortunately, our function overloads don't
//       collide when using a generic 'StringT' template
template <typename StringT, typename... Args>
inline void trace_messages(console::color color, StringT&& message, console::color nextColor, Args&&... args);

template <typename StringT, typename... Args>
inline void trace_messages(console::color color, StringT&& message, Args&&... args)
{
    trace_message(std::forward<StringT>(message), color);

    if constexpr (sizeof...(Args) > 0)
    {
        trace_messages(color, std::forward<Args>(args)...);
    }
}

template <typename StringT, typename... Args>
inline void trace_messages(console::color color, StringT&& message, console::color nextColor, Args&&... args)
{
    trace_message(std::forward<StringT>(message), color);
    trace_messages(nextColor, std::forward<Args>(args)...);
}

template <typename StringT, typename... Args>
inline void trace_messages(console::color color, StringT&& message, new_line_t, Args&&... args)
{
    trace_message(std::forward<StringT>(message), color, true);

    if constexpr (sizeof...(Args) > 0)
    {
        // NOTE: Working around MSVC bug using 'tuple_element' in 'if constexpr'. Please see:
        // https://developercommunity.visualstudio.com/content/problem/279415/using-tuple-element-in-if-constexpr-yields-erroneo.html
        //using first_t = std::tuple_element_t<0, std::tuple<Args...>>;
        using first_t = typename std::conditional_t<(sizeof...(Args) > 0),
            std::tuple_element<0, std::tuple<Args...>>,
            std::add_pointer<void>>::type;
        if constexpr (std::is_same_v<std::decay_t<first_t>, console::color>)
        {
            trace_messages(std::forward<Args>(args)...);
        }
        else
        {
            trace_messages(color, std::forward<Args>(args)...);
        }
    }
}

template <typename StringT, typename... Args>
inline void trace_messages(StringT&& message, Args&&... args)
{
    trace_messages(console::color::gray, std::forward<StringT>(message), std::forward<Args>(args)...);
}

inline int trace_error(int error, const wchar_t* msg = nullptr)
{
    if (msg)
    {
        trace_messages(error_color, L"ERROR: ", msg, new_line);
    }

    trace_messages(error_color, L"ERROR: ", error_message(error), L" (", error_info_color, std::to_wstring(error), error_color, L")\n");
    return error;
}

inline int trace_last_error(const wchar_t* msg = nullptr)
{
    return trace_error(::GetLastError(), msg);
}
