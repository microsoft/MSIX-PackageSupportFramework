//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// Random functions/types that are broadly useful
#pragma once

#include <cassert>
#include <cctype>
#include <string_view>

#include "win32_error.h"
#include "known_folders.h"

template <typename CharT>
struct case_insensitive_char_traits : std::char_traits<CharT>
{
    using MyBase = std::char_traits<CharT>;

    using char_type = CharT;
    using int_type = typename MyBase::int_type;
    using off_type = typename MyBase::off_type;
    using pos_type = typename MyBase::pos_type;
    using state_type = typename MyBase::state_type;

    // We can use the char_traits definitions for:
    //      assign
    //      copy
    //      move
    //      length
    //      to_char_type
    //      to_int_type
    //      eof
    //      not_eof

    static constexpr bool eq(char_type lhs, char_type rhs) noexcept
    {
        return std::tolower(lhs) == std::tolower(rhs);
    }

    static constexpr bool lt(char_type lhs, char_type rhs) noexcept
    {
        return std::tolower(lhs) < std::tolower(rhs);
    }

    static constexpr int compare(const char_type* lhs, const char_type* rhs, std::size_t count) noexcept
    {
        // NOTE: There's currently no wmemicmp/_wmemicmp function
        char_type lc = 0, rc = 0;
        for (; count && (lc == rc); --count)
        {
            lc = static_cast<char_type>(std::tolower(*lhs++));
            rc = static_cast<char_type>(std::tolower(*rhs++));
        }

        return (lc < rc) ? -1 : (lc > rc) ? 1 : 0;
    }

    static constexpr const char_type* find(const char_type* str, std::size_t count, const char_type& ch) noexcept
    {
        // NOTE: There's currently no wmemichr/_wmemichr function
        auto c = std::tolower(ch);
        for (; count; --count)
        {
            if (std::tolower(*str) == c) return str;
            ++str;
        }

        return nullptr;
    }

    static constexpr bool eq_int_type(int_type lhs, int_type rhs) noexcept
    {
        return std::tolower(lhs) == std::tolower(rhs);
    }
};

template <typename CharT>
using basic_istring = std::basic_string<CharT, case_insensitive_char_traits<CharT>>;
using istring = basic_istring<char>;
using iwstring = basic_istring<wchar_t>;
using iu16string = basic_istring<char16_t>;
using iu32string = basic_istring<char32_t>;

template <typename CharT>
using basic_istring_view = std::basic_string_view<CharT, case_insensitive_char_traits<CharT>>;
using istring_view = basic_istring_view<char>;
using iwstring_view = basic_istring_view<wchar_t>;
using iu16string_view = basic_istring_view<char16_t>;
using iu32string_view = basic_istring_view<char32_t>;

inline istring operator""_is(const char* str, std::size_t length) { return istring(str, length); }
inline iwstring operator""_is(const wchar_t* str, std::size_t length) { return iwstring(str, length); }
inline iu16string operator""_is(const char16_t* str, std::size_t length) { return iu16string(str, length); }
inline iu32string operator""_is(const char32_t* str, std::size_t length) { return iu32string(str, length); }

inline constexpr istring_view operator""_isv(const char* str, std::size_t length) { return istring_view(str, length); }
inline constexpr iwstring_view operator""_isv(const wchar_t* str, std::size_t length) { return iwstring_view(str, length); }
inline constexpr iu16string_view operator""_isv(const char16_t* str, std::size_t length) { return iu16string_view(str, length); }
inline constexpr iu32string_view operator""_isv(const char32_t* str, std::size_t length) { return iu32string_view(str, length); }


inline std::wstring widen(std::string_view str, UINT codePage = CP_UTF8)
{
    std::wstring result;
    if (str.empty())
    {
        // MultiByteToWideChar fails when given a length of zero
        return result;
    }

    // UTF-16 should occupy at most as many characters as UTF-8
    result.resize(str.length());

    // NOTE: Since we call MultiByteToWideChar with a non-negative input string size, the resulting string is not null
    //       terminated, so we don't need to '+1' the size on input and '-1' the size on resize
    if (auto size = ::MultiByteToWideChar(
        codePage,
        MB_ERR_INVALID_CHARS,
        str.data(), static_cast<int>(str.length()),
        result.data(), static_cast<int>(result.length())))
    {
        assert(static_cast<std::size_t>(size) <= result.length());
        result.resize(size);
    }
    else
    {
        throw_last_error();
    }

    return result;
};

inline std::wstring widen(std::wstring str, UINT = CP_UTF8)
{
    return str;
}

inline std::string narrow(std::wstring_view str, UINT codePage = CP_UTF8)
{
    std::string result;
    if (str.empty())
    {
        // WideCharToMultiByte fails when given a length of zero
        return result;
    }

    // UTF-8 can occupy more characters than an equivalent UTF-16 string. WideCharToMultiByte gives us the required
    // size, so leverage it before we resize the buffer on the first pass of the loop
    for (int size = 0; ; )
    {
        // NOTE: Since we call WideCharToMultiByte with a non-negative input string size, the resulting string is not
        //       null terminated, so we don't need to '+1' the size on input and '-1' the size on resize
        size = ::WideCharToMultiByte(
            codePage,
            (codePage == CP_UTF8) ? WC_ERR_INVALID_CHARS : 0,
            str.data(), static_cast<int>(str.length()),
            result.data(), static_cast<int>(result.length()),
            nullptr, nullptr);

        if (size > 0)
        {
            auto finished = (static_cast<std::size_t>(size) <= result.length());
            assert(finished ^ (result.length() == 0));
            result.resize(size);

            if (finished)
            {
                break;
            }
        }
        else
        {
            throw_last_error();
        }
    }

    return result;
}

inline std::string narrow(std::string str, UINT = CP_UTF8)
{
    return str;
}

// A convenience type to avoid a copy for already-wide function argument strings
struct wide_argument_string
{
    const wchar_t* value = nullptr;
    const wchar_t* c_str() const noexcept
    {
        return value;
    }
};

struct wide_argument_string_with_buffer : wide_argument_string
{
    std::wstring buffer;

    wide_argument_string_with_buffer() = default;
    wide_argument_string_with_buffer(std::wstring str) :
        buffer(std::move(str))
    {
        value = buffer.c_str();
    }
};

inline wide_argument_string_with_buffer widen_argument(const char* str)
{
    if (str)
    {
        return wide_argument_string_with_buffer{ widen(str) };
    }
    else
    {
        // Calling widen with a null pointer will crash. Default constructed wide_argument_string_with_buffer will
        // preserve the null argument
        return wide_argument_string_with_buffer{};
    }
}

inline wide_argument_string widen_argument(const wchar_t* str) noexcept
{
    return wide_argument_string{ str };
}

template <typename CharT>
errno_t strcatImpl(CharT* dest, rsize_t destBufSize, CharT const* src)
{
    if (std::is_same<CharT, char>::value)
    {
        return strcat_s(reinterpret_cast<char*>(dest), destBufSize, reinterpret_cast<const char*>(src));
    }
    else
    {
        return wcscat_s(reinterpret_cast<wchar_t*>(dest), destBufSize, reinterpret_cast<const wchar_t*>(src));
    }
}

template <typename CharT>
CharT* strtokImpl(CharT* inpStr, CharT const* delim, CharT** token)
{
    if (std::is_same<CharT, char>::value)
    {
        return reinterpret_cast<CharT*>(strtok_s(reinterpret_cast<char*>(inpStr), reinterpret_cast<const char*>(delim), reinterpret_cast<char**>(token)));
    }
    else
    {
        return reinterpret_cast<CharT*>(wcstok_s(reinterpret_cast<wchar_t*>(inpStr), reinterpret_cast<const wchar_t*>(delim), reinterpret_cast<wchar_t**>(token)));
    }
}

template <typename CharT>
size_t strlenImpl(CharT const* inpStr)
{
    if (std::is_same<CharT, char>::value)
    {
        return strlen(reinterpret_cast<const char*>(inpStr));
    }
    else
    {
        return wcslen(reinterpret_cast<const wchar_t*>(inpStr));
    }
}

template <typename CharT>
bool is_path_relative(const CharT* path, const std::filesystem::path& basePath)
{
    return std::equal(basePath.native().begin(), basePath.native().end(), path, psf::path_compare{});
}
