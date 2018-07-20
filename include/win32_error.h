//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <cassert>
#include <sstream>
#include <system_error>
#include <windows.h>

[[noreturn]]
inline void throw_win32(int errorCode, const char* message = "")
{
    assert(errorCode != NO_ERROR);
    throw std::system_error(errorCode, std::system_category(), message);
}

[[noreturn]]
inline void throw_last_error(const char* message = "")
{
    throw_win32(::GetLastError(), message);
}

inline void check_win32(int errorCode, const char* message = "")
{
    if (errorCode != NO_ERROR)
    {
        throw_win32(errorCode, message);
    }
}

inline void check_win32_bool(BOOL result, const char* message = "")
{
    if (!result)
    {
        throw_last_error(message);
    }
}

inline int win32_from_error_code(const std::error_code& err)
{
    if (err.category() == std::system_category())
    {
        return err.value();
    }

    // Currently unnecessary to try and map other values
    return ERROR_UNHANDLED_EXCEPTION;
}

inline int win32_from_caught_exception()
{
    try
    {
        throw;
    }
    catch (std::system_error& e)
    {
        return win32_from_error_code(e.code());
    }
    catch (std::bad_alloc&)
    {
        return ERROR_OUTOFMEMORY;
    }
    catch (std::length_error&)
    {
        return ERROR_OUTOFMEMORY;
    }
    catch (std::out_of_range&)
    {
        return ERROR_BUFFER_OVERFLOW;
    }
    catch (std::range_error&)
    {
        return ERROR_ARITHMETIC_OVERFLOW;
    }
    catch (std::overflow_error&)
    {
        return ERROR_ARITHMETIC_OVERFLOW;
    }
    catch (std::underflow_error&)
    {
        return ERROR_ARITHMETIC_OVERFLOW;
    }
    catch (std::invalid_argument&)
    {
        return ERROR_INVALID_PARAMETER;
    }
    catch (...)
    {
        // Swallow unknown/un-handled exceptions; use default return below
    }

    return ERROR_UNHANDLED_EXCEPTION;
}

inline std::string message_from_caught_exception()
{
    try
    {
        throw;
    }
    catch (std::system_error& e)
    {
        std::stringstream ss;
        ss << e.what() << " (" << win32_from_error_code(e.code()) << ")";
        return ss.str();
    }
    catch (std::exception& e)
    {
        return e.what();
    }
    catch (...)
    {
        // fall through to unknown error message
    }
    return "Unknown Error";
}
