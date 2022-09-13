//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <typeinfo>

#include "utilities.h"

namespace psf
{
    struct json_null;
    struct json_string;
    struct json_number;
    struct json_boolean;
    struct json_object;
    struct json_array;

#define JSON_TYPES \
    JSON_TYPE(null), \
    JSON_TYPE(string), \
    JSON_TYPE(number), \
    JSON_TYPE(boolean), \
    JSON_TYPE(object), \
    JSON_TYPE(array)

#define JSON_TYPE(t) t
    enum class json_type { JSON_TYPES };

#undef JSON_TYPE
#define JSON_TYPE(t) #t
    static constexpr std::string_view json_type_names[] = { JSON_TYPES };

    namespace details
    {
        template <typename Interface>
        struct json_type_from_interface;

        template <> struct json_type_from_interface<json_null> { static constexpr json_type value = json_type::null; };
        template <> struct json_type_from_interface<json_string> { static constexpr json_type value = json_type::string; };
        template <> struct json_type_from_interface<json_number> { static constexpr json_type value = json_type::number; };
        template <> struct json_type_from_interface<json_boolean> { static constexpr json_type value = json_type::boolean; };
        template <> struct json_type_from_interface<json_object> { static constexpr json_type value = json_type::object; };
        template <> struct json_type_from_interface<json_array> { static constexpr json_type value = json_type::array; };

        template <typename Interface>
        constexpr json_type json_type_v = json_type_from_interface<Interface>::value;
    }

    struct json_value
    {
        virtual ~json_value() {}
        virtual const json_type type() const noexcept = 0;

        virtual const json_null* try_as_null() const noexcept = 0;
        virtual const json_string* try_as_string() const noexcept = 0;
        virtual const json_number* try_as_number() const noexcept = 0;
        virtual const json_boolean* try_as_boolean() const noexcept = 0;
        virtual const json_object* try_as_object() const noexcept = 0;
        virtual const json_array* try_as_array() const noexcept = 0;

        // Convenience functions
        template <typename T>
        const T* try_as() const noexcept
        {
            using type = std::remove_const_t<T>;
            if constexpr (std::is_same_v<type, json_null>)
            {
                return try_as_null();
            }
            else if constexpr (std::is_same_v<type, json_string>)
            {
                return try_as_string();
            }
            else if constexpr (std::is_same_v<type, json_number>)
            {
                return try_as_number();
            }
            else if constexpr (std::is_same_v<type, json_boolean>)
            {
                return try_as_boolean();
            }
            else if constexpr (std::is_same_v<type, json_object>)
            {
                return try_as_object();
            }
            else if constexpr (std::is_same_v<type, json_array>)
            {
                return try_as_array();
            }
            else
            {
                static_assert(false, "Invalid JSON type");
            }
        }

        const json_null& as_null() const
        {
            return as<json_null>();
        }

        const json_string& as_string() const
        {
            return as<json_string>();
        }

        const json_number& as_number() const
        {
            return as<json_number>();
        }

        const json_boolean& as_boolean() const
        {
            return as<json_boolean>();
        }

        const json_object& as_object() const
        {
            return as<json_object>();
        }

        const json_array& as_array() const
        {
            return as<json_array>();
        }

        template <typename T>
        const T& as() const
        {
            using namespace std::literals;

            auto result = try_as<T>();
            if (!result)
            {
                std::string source{ json_type_names[static_cast<std::underlying_type_t<json_type>>(json_type())] };
                std::string target{ json_type_names[static_cast<std::underlying_type_t<json_type>>(details::json_type_v<T>)] };
                auto error{ "Invalid JSON cast: cannot convert from "s + source + " to "s + target };
                throw std::runtime_error(error);
            }

            return *result;
        }
    };

    namespace details
    {
        template <typename Interface>
        struct json_value_base : json_value
        {
            virtual const json_type type() const noexcept override final
            {
                return json_type_v<Interface>;
            }

            virtual const json_null* try_as_null() const noexcept override final
            {
                return try_as_base<json_null>();
            }

            virtual const json_string* try_as_string() const noexcept override final
            {
                return try_as_base<json_string>();
            }

            virtual const json_number* try_as_number() const noexcept override final
            {
                return try_as_base<json_number>();
            }

            virtual const json_boolean* try_as_boolean() const noexcept override final
            {
                return try_as_base<json_boolean>();
            }

            virtual const json_object* try_as_object() const noexcept override final
            {
                return try_as_base<json_object>();
            }

            virtual const json_array* try_as_array() const noexcept override final
            {
                return try_as_base<json_array>();
            }

        private:

            template <typename T>
            const T* try_as_base() const noexcept
            {
                if constexpr (std::is_same_v<T, Interface>)
                {
                    return static_cast<const Interface*>(this);
                }
                else
                {
                    static_assert(json_type_v<T> != json_type_v<Interface>);
                    return nullptr;
                }
            }
        };
    }

    struct json_null : details::json_value_base<json_null>
    {
    };

    struct json_string : details::json_value_base<json_string>
    {
        virtual const char* narrow(_Out_opt_ unsigned* length = nullptr) const noexcept = 0;
        virtual const wchar_t* wide(_Out_opt_ unsigned* length = nullptr) const noexcept = 0;

        // Convenience functions
        std::string_view string() const noexcept
        {
            unsigned size;
            auto str = narrow(&size);
            return { str, size };
        }

        std::wstring_view wstring() const noexcept
        {
            unsigned size;
            auto str = wide(&size);
            return { str, size };
        }
    };

    struct json_number : details::json_value_base<json_number>
    {
        virtual std::uint64_t get_unsigned() const noexcept = 0;
        virtual std::int64_t get_signed() const noexcept = 0;
        virtual double get_float() const noexcept = 0;

        // Convenience functions
        template <
            typename T,
            std::enable_if_t<std::disjunction_v<std::is_floating_point<T>, std::is_arithmetic<T>>, int> = 0>
        auto get() const noexcept
        {
            if constexpr (std::is_floating_point_v<T>)
            {
                return static_cast<T>(get_float());
            }
            else if constexpr (std::is_signed_v<T>)
            {
                return static_cast<T>(get_signed());
            }
            else
            {
                return static_cast<T>(get_unsigned());
            }
        }
    };

    struct json_boolean : details::json_value_base<json_boolean>
    {
        virtual bool get() const noexcept = 0;

        // Convenience functions
        explicit operator bool() const noexcept
        {
            // E.g. `if (*jsonBool) { ... }`
            return get();
        }
    };

    struct json_object : details::json_value_base<json_object>
    {
        virtual const json_value* try_get(_In_ const char* key) const noexcept = 0;

        // Iteration support. All values have been exhausted when either begin_enumeration or advance return null. Early
        // termination of enumeration must call cancel_enumeration to free resources. Example:
        //      enumeration_data data;
        //      for (auto it = obj->begin_enumeration(&data); it; it = obj->advance(it, &data))
        //      {
        //          if (some_condition(data))
        //          {
        //              obj->cancel_enumeration(it);
        //              break;
        //          }
        //      }
        struct enumeration_handle;
        struct enumeration_data
        {
            const char* key;
            unsigned key_length;
            json_value* value;
        };
        virtual enumeration_handle* begin_enumeration(_Out_ enumeration_data* data) const noexcept = 0;
        virtual enumeration_handle* advance(_In_ enumeration_handle* handle, _Inout_ enumeration_data* data) const noexcept = 0;
        virtual void cancel_enumeration(_In_ enumeration_handle* handle) const noexcept = 0;

        // Convenience functions
        const json_value& get(_In_ const char* key) const
        {
            auto result = try_get(key);
            if (!result)
            {
                auto message = std::string{ "Key '" } + key + "' does not exist in the JSON object";
                throw std::out_of_range(message);
            }

            return *result;
        }

        struct iterator
        {
            using difference_type = int;
            using value_type = const std::pair<std::string_view, const json_value&>;
            using pointer = value_type*;
            using reference = value_type;
            using iterator_category = std::input_iterator_tag;

            iterator(const json_object* object) noexcept :
                object(object)
            {
            }

            iterator(const json_object* object, enumeration_handle* handle, const enumeration_data& data) noexcept :
                object(object),
                handle(handle),
                data(data)
            {
            }

            iterator(iterator&& other) noexcept :
                iterator(other.object, other.handle, other.data)
            {
                other.handle = nullptr;
                other.data = {};
            }

            iterator& operator=(iterator&& other) noexcept
            {
                std::swap(object, other.object);
                std::swap(handle, other.handle);
                std::swap(data, other.data);
            }

            // NOTE: enumeration_handles are not copy-able, so for the time being we must remain, at best, an
            //       InputIterator
            iterator(const iterator&) = delete;
            iterator& operator=(const iterator&) = delete;

            ~iterator()
            {
                if (handle)
                {
                    object->cancel_enumeration(handle);
                }
            }

            struct proxy_result
            {
                value_type value;

                reference operator*() const noexcept
                {
                    return value;
                }

                pointer operator->() const noexcept
                {
                    return &value;
                }
            };

            // InputIterator
            iterator& operator++() noexcept
            {
                handle = object->advance(handle, &data);
                return *this;
            }

            auto operator++(int) noexcept
            {
                proxy_result result{ **this };
                ++(*this);
                return result;
            }

            reference operator*() const noexcept
            {
                return { std::string_view(data.key, data.key_length), *data.value };
            }

            auto operator->() const noexcept
            {
                return proxy_result{ **this };
            }

            friend bool operator==(const iterator& lhs, const iterator& rhs) noexcept
            {
                // NOTE: handles might not compare equal, but the memory they point to will
                assert(lhs.object == rhs.object);
                return lhs.data.key == rhs.data.key;
            }

            friend bool operator!=(const iterator& lhs, const iterator& rhs) noexcept
            {
                return !(lhs == rhs);
            }

            const json_object* object;
            enumeration_handle* handle = nullptr;
            enumeration_data data = {};
        };
        using const_iterator = iterator;

        iterator begin() const noexcept
        {
            enumeration_data data;
            auto handle = begin_enumeration(&data);
            return iterator{ this, handle, data };
        }

        const_iterator cbegin() const noexcept
        {
            return begin();
        }

        iterator end() const noexcept
        {
            return iterator{ this };
        }

        const_iterator cend() const noexcept
        {
            return end();
        }
    };

    struct json_array : details::json_value_base<json_array>
    {
        virtual unsigned size() const noexcept = 0;
        virtual const json_value* try_get_at(unsigned index) const noexcept = 0;

        // Convenience functions
        const json_value& get_at(unsigned index) const
        {
            auto result = try_get_at(index);
            if (!result)
            {
                throw std::out_of_range("Index out of range");
            }

            return *result;
        }

        const json_value& operator[](unsigned index) const
        {
            return get_at(index);
        }

        struct iterator
        {
            using difference_type = int;
            using value_type = const json_value;
            using pointer = value_type*;
            using reference = value_type&;
            using iterator_category = std::random_access_iterator_tag;

            // InputIterator
            iterator& operator++() noexcept
            {
                return (*this) += 1;
            }

            reference operator*() const noexcept
            {
                return (*this)[0];
            }

            pointer operator->() const noexcept
            {
                return &(**this);
            }

            friend bool operator==(const iterator& lhs, const iterator& rhs) noexcept
            {
                assert(lhs.array == rhs.array);
                return lhs.index == rhs.index;
            }

            friend bool operator!=(const iterator& lhs, const iterator& rhs) noexcept
            {
                return !(lhs == rhs);
            }

            // ForwardIterator
            iterator operator++(int) noexcept
            {
                auto copy = *this;
                ++(*this);
                return copy;
            }

            // BidirectionalIterator
            iterator& operator--() noexcept
            {
                return (*this) -= 1;
            }

            iterator operator--(int) noexcept
            {
                auto copy = *this;
                --(*this);
                return copy;
            }

            // RandomAccessIterator
            iterator& operator+=(difference_type diff) noexcept
            {
                index += diff;
                assert(index <= array->size());
                return *this;
            }

            iterator& operator-=(difference_type diff) noexcept
            {
                return (*this) += -diff;
            }

            friend iterator operator+(iterator lhs, difference_type rhs) noexcept
            {
                return lhs += rhs;
            }

            friend iterator operator-(iterator lhs, difference_type rhs) noexcept
            {
                return lhs -= rhs;
            }

            friend iterator operator+(difference_type lhs, iterator rhs) noexcept
            {
                return rhs += lhs;
            }

            friend difference_type operator-(const iterator& lhs, const iterator& rhs) noexcept
            {
                return static_cast<difference_type>(lhs.index) - static_cast<difference_type>(rhs.index);
            }

            reference operator[](difference_type offset) const noexcept
            {
                assert((index + offset) < array->size());
                return array->get_at(index + offset);
            }

            friend bool operator<(const iterator& lhs, const iterator& rhs) noexcept
            {
                assert(lhs.array == rhs.array);
                return lhs.index < rhs.index;
            }

            friend bool operator>(const iterator& lhs, const iterator& rhs) noexcept
            {
                assert(lhs.array == rhs.array);
                return lhs.index > rhs.index;
            }

            friend bool operator<=(const iterator& lhs, const iterator& rhs) noexcept
            {
                return !(lhs > rhs);
            }

            friend bool operator>=(const iterator& lhs, const iterator& rhs) noexcept
            {
                return !(lhs < rhs);
            }

            const json_array* array;
            unsigned index;
        };
        using const_iterator = iterator;

        iterator begin() const noexcept
        {
            return iterator{ this, 0 };
        }

        const_iterator cbegin() const noexcept
        {
            return begin();
        }

        iterator end() const noexcept
        {
            return iterator{ this, size() };
        }

        const_iterator cend() const noexcept
        {
            return end();
        }

        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = reverse_iterator;

        reverse_iterator rbegin() const noexcept
        {
            return reverse_iterator(end());
        }

        const_reverse_iterator crbegin() const noexcept
        {
            return rbegin();
        }

        reverse_iterator rend() const noexcept
        {
            return reverse_iterator(begin());
        }

        const_reverse_iterator crend() const noexcept
        {
            return rend();
        }
    };
}
