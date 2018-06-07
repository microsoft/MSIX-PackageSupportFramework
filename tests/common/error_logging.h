#pragma once

#include <iostream>
#include <string>
#include <system_error>

#include "console_output.h"

inline auto error_text()
{
    return console::change_foreground(console::color::red);
}

inline auto warning_text()
{
    return console::change_foreground(console::color::yellow);
}

inline auto success_text()
{
    return console::change_foreground(console::color::green);
}

inline std::string error_message(int err)
{
    // Remove the ".\r\n" that gets added to all messages
    auto msg = std::system_category().message(err);
    msg.resize(msg.length() - 3);
    return msg;
}

inline void print_error(int err)
{
    std::cout << error_text() << "ERROR: " << error_message(err) << " (" << err << ")\n";
}

inline int print_last_error(const char* msg = nullptr, int err = ::GetLastError())
{
    if (msg)
    {
        std::cout << error_text() << "ERROR: " << msg << std::endl;
    }

    print_error(err);
    return err;
}
