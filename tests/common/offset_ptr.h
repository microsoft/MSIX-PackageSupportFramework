//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <cassert>
#include <cstdint>
#include <iterator>

template <typename T, typename OffsetT = std::int32_t>
class offset_ptr
{
public:
    // NOTE: We satisfy everything for RandomAccessIterator _except_ being CopyConstructible
    using value_type = T;
    using reference = T&;
    using pointer = T*;
    using difference_type = OffsetT;
    using iterator_category = std::random_access_iterator_tag;

    offset_ptr() = default;

    // NOTE: Since we're dealing with offsets, Copying becomes very awkward, so explicitly prohibit it. The equivalent
    //       functionality can be achieved by converting to pointer and back
    offset_ptr(const offset_ptr&) = delete;
    offset_ptr& operator=(const offset_ptr&) = delete;

    void reset(T* ptr) noexcept
    {
        m_offset = static_cast<OffsetT>(reinterpret_cast<std::uint8_t*>(ptr) - reinterpret_cast<std::uint8_t*>(this));
        assert(get() == ptr);
    }

    T* get() noexcept
    {
        if (!m_offset)
        {
            return nullptr;
        }

        return reinterpret_cast<T*>(reinterpret_cast<char*>(this) + m_offset);
    }

    const T* get() const noexcept
    {
        if (!m_offset)
        {
            return nullptr;
        }

        return reinterpret_cast<const T*>(reinterpret_cast<const char*>(this) + m_offset);
    }

    operator T*() noexcept
    {
        return get();
    }

    operator const T*() const noexcept
    {
        return get();
    }

    T* operator->() noexcept
    {
        return get();
    }

    const T* operator->() const noexcept
    {
        return get();
    }

    // NOTE: The index shouldn't be bound by the limitations of our internal offset type
    T& operator[](std::ptrdiff_t index) noexcept
    {
        return get()[index];
    }

    const T& operator[](std::ptrdiff_t index) const noexcept
    {
        return get()[index];
    }

    // NOTE: In the effort to avoid copies, we return pointers for operations that expect something somewhat equivalent
    offset_ptr& operator++() noexcept
    {
        m_offset += sizeof(T);
        return *this;
    }

    pointer operator++(int) noexcept
    {
        auto result = get();
        ++(*this);
        return result;
    }

    offset_ptr& operator+=(difference_type distance) noexcept
    {
        m_offset += distance * sizeof(T);
        return *this;
    }

    offset_ptr& operator--() noexcept
    {
        m_offset -= sizeof(T);
        return *this;
    }

    pointer operator--(int) noexcept
    {
        auto result = get();
        --(*this);
        return result;
    }

    offset_ptr& operator-=(difference_type distance) noexcept
    {
        m_offset -= distance * sizeof(T);
        return *this;
    }

    friend pointer operator+(const offset_ptr& lhs, std::ptrdiff_t rhs) noexcept
    {
        return lhs.get() + rhs;
    }

    friend pointer operator+(std::ptrdiff_t lhs, const offset_ptr& rhs) noexcept
    {
        return rhs.get() + lhs;
    }

    friend pointer operator-(const offset_ptr& lhs, std::ptrdiff_t rhs) noexcept
    {
        return lhs.get() - rhs;
    }

    friend std::ptrdiff_t operator-(const offset_ptr& lhs, const offset_ptr& rhs) noexcept
    {
        return lhs.get() - rhs.get();
    }

    friend bool operator==(const offset_ptr& lhs, const offset_ptr& rhs) noexcept
    {
        return lhs.get() == rhs.get();
    }

    friend bool operator!=(const offset_ptr& lhs, const offset_ptr& rhs) noexcept
    {
        return lhs.get() != rhs.get();
    }

    friend bool operator<(const offset_ptr& lhs, const offset_ptr& rhs) noexcept
    {
        return lhs.get() < rhs.get();
    }

    friend bool operator<=(const offset_ptr& lhs, const offset_ptr& rhs) noexcept
    {
        return lhs.get() <= rhs.get();
    }

    friend bool operator>(const offset_ptr& lhs, const offset_ptr& rhs) noexcept
    {
        return lhs.get() > rhs.get();
    }

    friend bool operator>=(const offset_ptr& lhs, const offset_ptr& rhs) noexcept
    {
        return lhs.get() >= rhs.get();
    }

private:

    OffsetT m_offset = 0;
};
