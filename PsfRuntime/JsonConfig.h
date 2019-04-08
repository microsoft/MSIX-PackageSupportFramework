//------------------------------------------------------------------------------------------------------- 
// Copyright (C) Microsoft Corporation. All rights reserved. 
// Licensed under the MIT license. See LICENSE file in the project root for full license information. 
//------------------------------------------------------------------------------------------------------- 
#pragma once 
 
#include <unordered_map> 
#include <variant> 
#include <vector> 
 
#include <psf_config.h> 
 
struct json_null_impl : psf::json_null 
{ 
}; 
 
struct json_string_impl : psf::json_string 
{ 
    json_string_impl(std::string_view value) : narrow_string(value), wide_string(widen(value)) {} 
 
    virtual const char* narrow(_Out_opt_ unsigned* length) const noexcept override 
    { 
        if (length) 
        { 
            *length = static_cast<unsigned>(narrow_string.length()); 
        } 
 
        return narrow_string.c_str(); 
    } 
 
    virtual const wchar_t* wide(_Out_opt_ unsigned* length) const noexcept override 
    { 
        if (length) 
        { 
            *length = static_cast<unsigned>(wide_string.length()); 
        } 
 
        return wide_string.c_str(); 
    } 
 
    std::string narrow_string; 
    std::wstring wide_string; 
}; 
 
struct json_number_impl : psf::json_number 
{ 
    template <typename T> 
    json_number_impl(T value) : value(value) {} // NOTE: T _must_ be one of the three types used below 
 
    virtual std::uint64_t get_unsigned() const noexcept override 
    { 
        return value_as<std::uint64_t>(); 
    } 
 
    virtual std::int64_t get_signed() const noexcept override 
    { 
        return value_as<std::int64_t>(); 
    } 
 
    virtual double get_float() const noexcept override 
    { 
        return value_as<double>(); 
    } 
 
    template <typename T> 
    inline T value_as() const noexcept 
    { 
        switch (value.index()) 
        { 
        case 0: return static_cast<T>(std::get<std::int64_t>(value)); 
        case 1: return static_cast<T>(std::get<std::uint64_t>(value)); 
        case 2: return static_cast<T>(std::get<double>(value)); 
        } 
 
        assert(false); // valueless_by_exception, which shouldn't happen since construction has no business throwing 
        return T{}; 
    } 
 
    std::variant<std::int64_t, std::uint64_t, double> value; 
}; 
 
struct json_boolean_impl : psf::json_boolean 
{ 
    json_boolean_impl(bool value) : value(value) {} 
 
    virtual bool get() const noexcept override 
    { 
        return value; 
    } 
 
    bool value; 
}; 
 
struct json_object_impl : psf::json_object 
{ 
    using map_type = std::map<std::string, std::unique_ptr<psf::json_value>, std::less<>>; 
    using iterator_type = map_type::const_iterator; 
 
    virtual json_value* try_get(_In_ const char* key) const noexcept override 
    { 
        if (auto itr = values.find(key); itr != values.end()) 
        { 
            return itr->second.get(); 
        } 
 
        return nullptr; 
    } 
 
    // We can elide allocations if the iterator fits within the size of a pointer (which should always be the case when 
    // building release) 
    static constexpr bool handle_requires_allocation = sizeof(iterator_type) > sizeof(enumeration_handle*); 
 
    virtual enumeration_handle* begin_enumeration(_Out_ enumeration_data* data) const noexcept override 
    { 
        auto itr = values.begin(); 
        if (itr == values.end()) 
        { 
            *data = {}; 
            return nullptr; 
        } 
 
        data->key = itr->first.c_str(); 
        data->key_length = static_cast<unsigned>(itr->first.length()); 
        data->value = itr->second.get(); 
 
        if constexpr (handle_requires_allocation) 
        { 
            return reinterpret_cast<enumeration_handle*>(new iterator_type(itr)); 
        } 
        else 
        { 
            return handle_from_iterator(itr); 
        } 
    } 
 
    virtual enumeration_handle* advance(_In_ enumeration_handle* handle, _Inout_ enumeration_data* data) const noexcept override 
    { 
        auto& itr = iterator_from_handle(handle); 
        assert(itr != values.end()); 
 
        ++itr; 
        if (itr == values.end()) 
        { 
            if constexpr (handle_requires_allocation) 
            { 
                delete &itr; 
            } 
 
            *data = {}; 
            return nullptr; 
        } 
 
        data->key = itr->first.c_str(); 
        data->key_length = static_cast<unsigned>(itr->first.length()); 
        data->value = itr->second.get(); 
 
        return handle_from_iterator(itr); 
    } 
 
    virtual void cancel_enumeration(_In_ enumeration_handle* handle) const noexcept override 
    { 
        if constexpr (handle_requires_allocation) 
        { 
            delete &iterator_from_handle(handle); 
        } 
    } 
 
    iterator_type& iterator_from_handle(enumeration_handle*& handle) const noexcept 
    { 
        assert(handle); 
        if constexpr (handle_requires_allocation) 
        { 
            // Allocation means that the handle pointer is actually a pointer to a map iterator 
            return *reinterpret_cast<iterator_type*>(handle); 
        } 
        else 
        { 
            // No allocation means that the handle pointer _is_ the map iterator, e.g. as if it were a union 
            return reinterpret_cast<iterator_type&>(handle); 
        } 
    } 
 
    enumeration_handle* handle_from_iterator(iterator_type& itr) const noexcept 
    { 
        assert(itr != values.end()); 
        if constexpr (handle_requires_allocation) 
        { 
            // Allocation means that the handle pointer is actually a pointer to a map iterator 
            return reinterpret_cast<enumeration_handle*>(&itr); 
        } 
        else 
        { 
            // No allocation means that the handle pointer _is_ the map iterator, e.g. as if it were a union 
            // NOTE: We can reinterpret_cast a pointer to a reference, but not the other way around... 
            return *reinterpret_cast<enumeration_handle**>(&itr); 
        } 
    } 
 
    map_type values; 
}; 
 
struct json_array_impl : psf::json_array 
{ 
    virtual unsigned size() const noexcept override 
    { 
        return static_cast<unsigned>(values.size()); 
    } 
 
    virtual json_value* try_get_at(unsigned index) const noexcept override 
    { 
        if (index >= values.size()) 
        { 
            return nullptr; 
        } 
 
        return values[index].get(); 
    } 
 
    std::vector<std::unique_ptr<psf::json_value>> values; 
};
