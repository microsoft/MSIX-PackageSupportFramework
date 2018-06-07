#pragma once

#include <cassert>
#include <system_error>
#include <windows.h>

[[noreturn]]
inline void throw_win32(int errorCode)
{
    assert(errorCode != NO_ERROR);
    throw std::system_error(errorCode, std::system_category());
}

[[noreturn]]
inline void throw_last_error()
{
    throw_win32(::GetLastError());
}

inline void check_win32(int errorCode)
{
    if (errorCode != NO_ERROR)
    {
        throw_win32(errorCode);
    }
}

inline void check_win32_bool(BOOL result)
{
    if (!result)
    {
        throw_last_error();
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
