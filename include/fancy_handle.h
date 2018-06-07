#pragma once

#include <windows.h>

namespace shims
{
    struct fancy_handle
    {
        HANDLE value = INVALID_HANDLE_VALUE;

        fancy_handle() noexcept = default;

        fancy_handle(std::nullptr_t) noexcept
        {
        }

        fancy_handle(HANDLE value) noexcept :
            value(value)
        {
        }

        HANDLE get() const noexcept
        {
            return value;
        }

        operator HANDLE() const noexcept
        {
            return get();
        }

        HANDLE normalize() const noexcept
        {
            return (value == INVALID_HANDLE_VALUE) ? nullptr : value;
        }

        explicit operator bool() const noexcept
        {
            return normalize() != nullptr;
        }

        friend bool operator==(const fancy_handle& lhs, const fancy_handle& rhs)
        {
            return lhs.normalize() == rhs.normalize();
        }

        friend bool operator!=(const fancy_handle& lhs, const fancy_handle& rhs)
        {
            return lhs.normalize() != rhs.normalize();
        }

        friend bool operator<(const fancy_handle& lhs, const fancy_handle& rhs)
        {
            return lhs.normalize() < rhs.normalize();
        }

        friend bool operator<=(const fancy_handle& lhs, const fancy_handle& rhs)
        {
            return lhs.normalize() <= rhs.normalize();
        }

        friend bool operator>(const fancy_handle& lhs, const fancy_handle& rhs)
        {
            return lhs.normalize() > rhs.normalize();
        }

        friend bool operator>=(const fancy_handle& lhs, const fancy_handle& rhs)
        {
            return lhs.normalize() >= rhs.normalize();
        }
    };

    template <auto CloseFn>
    struct handle_deleter
    {
        using pointer = fancy_handle;

        void operator()(const pointer& handle) noexcept
        {
            if (handle)
            {
                CloseFn(handle);
            }
        }
    };
}
