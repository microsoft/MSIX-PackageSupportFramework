//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <iostream>
#include <string>
#include <system_error>

#include "console_output.h"

constexpr auto error_color = console::color::red;
inline auto error_text()
{
    return console::change_foreground(error_color);
}

constexpr auto error_info_color = console::color::dark_red;
inline auto error_info_text()
{
    return console::change_foreground(error_info_color);
}

constexpr auto warning_color = console::color::yellow;
inline auto warning_text()
{
    return console::change_foreground(warning_color);
}

constexpr auto warning_info_color = console::color::dark_yellow;
inline auto warning_info_text()
{
    return console::change_foreground(warning_info_color);
}

constexpr auto info_color = console::color::dark_gray;
inline auto info_text()
{
    return console::change_foreground(info_color);
}

constexpr auto success_color = console::color::green;
inline auto success_text()
{
    return console::change_foreground(success_color);
}

inline std::string error_message(int err)
{
    // Remove the ".\r\n" that gets added to all messages
    auto msg = std::system_category().message(err);
    msg.resize(msg.length() - 3);
    return msg;
}

inline int print_error(int err, const char* msg = nullptr)
{
    if (msg)
    {
        std::wcout << error_text() << "ERROR: " << msg << std::endl;
    }

    std::wcout << error_text() << "ERROR: " << error_message(err).c_str() << " (" << err << ")\n";
    return err;
}

inline int print_last_error(const char* msg = nullptr)
{
    return print_error(::GetLastError(), msg);
}
