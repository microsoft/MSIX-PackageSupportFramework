//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <optional>
#include <type_traits>

#include <windows.h>
#include <win32_error.h>

namespace console
{
    enum class color : WORD
    {
        dark_red        = FOREGROUND_RED,
        red             = FOREGROUND_RED | FOREGROUND_INTENSITY,
        dark_green      = FOREGROUND_GREEN,
        green           = FOREGROUND_GREEN | FOREGROUND_INTENSITY,
        dark_blue       = FOREGROUND_BLUE,
        blue            = FOREGROUND_BLUE | FOREGROUND_INTENSITY,

        dark_magenta    = FOREGROUND_RED | FOREGROUND_BLUE,
        magenta         = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
        dark_cyan       = FOREGROUND_BLUE | FOREGROUND_GREEN,
        cyan            = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
        dark_yellow     = FOREGROUND_RED | FOREGROUND_GREEN,
        yellow          = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,

        black           = 0,
        dark_gray       = FOREGROUND_INTENSITY,
        gray            = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
        white           = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
    };

    namespace details
    {
        struct set_foreground { color value; };
        struct revert_foreground {};
    }

    template <typename StreamT>
    struct change_foreground_t
    {
        change_foreground_t(StreamT& stream, color foreground) :
            m_outputStream(stream)
        {
            auto out = ::GetStdHandle(STD_OUTPUT_HANDLE);
            if (out == INVALID_HANDLE_VALUE)
            {
                throw_last_error();
            }

            // NOTE: GetConsoleScreenBufferInfo can fail if we're re-directing output to a file, so ignore failures
            CONSOLE_SCREEN_BUFFER_INFO info;
            if (::GetConsoleScreenBufferInfo(out, &info))
            {
                m_previousAttributes = info.wAttributes;
                m_outputStream.flush();

                WORD attrib = (info.wAttributes & static_cast<WORD>(0xFF00)) | static_cast<WORD>(foreground);
                check_win32_bool(::SetConsoleTextAttribute(out, attrib));
            }
        }

        ~change_foreground_t()
        {
            reset();
        }

        void flush()
        {
            m_outputStream.flush();
        }

        StreamT& reset()
        {
            if (m_previousAttributes)
            {
                if (auto out = ::GetStdHandle(STD_OUTPUT_HANDLE); out != INVALID_HANDLE_VALUE)
                {
                    m_outputStream.flush();
                    ::SetConsoleTextAttribute(out, m_previousAttributes.value());
                }
            }

            return m_outputStream;
        }

        change_foreground_t& operator<<(StreamT& (*fn)(StreamT&))
        {
            m_outputStream << fn;
            return *this;
        }

        template <
            typename T,
            std::enable_if_t<!std::disjunction_v<
                std::is_same<std::decay_t<T>, details::set_foreground>,
                std::is_same<std::decay_t<T>, details::revert_foreground>
            >, int> = 0>
        change_foreground_t& operator<<(T&& arg)
        {
            m_outputStream << std::forward<T>(arg);
            return *this;
        }

    private:

        StreamT& m_outputStream;
        std::optional<WORD> m_previousAttributes;
    };

    inline details::set_foreground change_foreground(color foreground)
    {
        return details::set_foreground{ foreground };
    }

    inline details::revert_foreground revert_foreground()
    {
        return details::revert_foreground{};
    }

    namespace details
    {
        template <typename StreamT>
        inline change_foreground_t<StreamT> operator<<(StreamT& stream, details::set_foreground foreground)
        {
            return change_foreground_t<StreamT>(stream, foreground.value);
        }

        template <typename StreamT>
        StreamT& operator<<(change_foreground_t<StreamT>& stream, details::revert_foreground)
        {
            return stream.reset();
        }
    }
}
