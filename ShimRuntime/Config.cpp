//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <regex>
#include <sstream>
#include <string_view>
#include <vector>

#include <windows.h>
#include <detours.h>
#include <rapidjson/reader.h>
#include <rapidjson/error/en.h>
#include <rapidjson/filereadstream.h>
#include <shim_constants.h>
#include <shim_runtime.h>
#include <shim_utils.h>
#include <stringapiset.h>
#include <utilities.h>

#include "Config.h"
#include "JsonConfig.h"

using namespace std::literals;

static std::wstring g_PackageFullName;
static std::wstring g_ApplicationUserModelId;
static std::wstring g_ApplicationId;
static std::filesystem::path g_PackageRootPath;
static std::filesystem::path g_CurrentExecutable;

// The object that constructs the JSON DOM and holds the root
static struct
{
    bool on_value(std::unique_ptr<shims::json_value> value)
    {
        if (!state_stack.empty())
        {
            assert(root);
            auto& current_value = state_stack.back();
            switch (current_value.index())
            {
            case 0:
                std::get<0>(current_value)->values.emplace(std::move(object_key), std::move(value));
                break;

            case 1:
                std::get<1>(current_value)->values.emplace_back(std::move(value));
                break;
            }
        }
        else if (!root)
        {
            root = std::move(value);
        }
        else
        {
            // Can't have more than one root; return error
            return false;
        }

        return true;
    }

    bool Null()
    {
        return on_value(std::make_unique<json_null_impl>());
    }

    bool Bool(bool b)
    {
        return on_value(std::make_unique<json_boolean_impl>(b));
    }

    bool Int(std::int64_t value)
    {
        return on_value(std::make_unique<json_number_impl>(value));
    }

    bool Uint(std::uint64_t value)
    {
        return on_value(std::make_unique<json_number_impl>(value));
    }

    bool Int64(std::int64_t value)
    {
        return on_value(std::make_unique<json_number_impl>(value));
    }

    bool Uint64(std::uint64_t value)
    {
        return on_value(std::make_unique<json_number_impl>(value));
    }

    bool Double(double value)
    {
        return on_value(std::make_unique<json_number_impl>(value));
    }

    bool RawNumber(const char* /*str*/, rapidjson::SizeType /*length*/, bool /*copy*/)
    {
        // Provided only to satisfy compilation, but never called since we never pass kParseNumbersAsStringsFlag
        // Consider: Update rapidjson to use if constexpr
        assert(false);
        return false;
    }

    bool String(const char* str, rapidjson::SizeType length, [[maybe_unused]] bool copy)
    {
        // Caller should always own the memory
        assert(copy);
        return on_value(std::make_unique<json_string_impl>(std::string_view(str, length)));
    }

    bool StartObject()
    {
        // NOTE: We must call 'on_value' before appending to 'state_stack', otherwise we'll try and add the object as a
        //       child of itself
        auto obj = std::make_unique<json_object_impl>();
        auto objPtr = obj.get();
        auto result = on_value(std::move(obj));
        state_stack.emplace_back(objPtr);
        return result;
    }

    bool Key(const char* str, rapidjson::SizeType length, [[maybe_unused]] bool copy)
    {
        // Caller should always own the memory
        assert(copy);
        assert(object_key.empty());
        object_key.assign(str, length);
        return true;
    }

    bool EndObject([[maybe_unused]] rapidjson::SizeType memberCount)
    {
#ifndef NDEBUG
        assert(!state_stack.empty());

        auto current = state_stack.back();
        assert(current.index() == 0);

        auto ptr = std::get<0>(current);
        assert(ptr->values.size() == memberCount);
#endif

        state_stack.pop_back();
        return true;
    }

    bool StartArray()
    {
        // NOTE: We must call 'on_value' before appending to 'state_stack', otherwise we'll try and add the array as a
        //       child of itself
        auto arr = std::make_unique<json_array_impl>();
        auto arrPtr = arr.get();
        auto result = on_value(std::move(arr));
        state_stack.emplace_back(arrPtr);
        return result;
    }

    bool EndArray([[maybe_unused]] rapidjson::SizeType elementCount)
    {
#ifndef NDEBUG
        assert(!state_stack.empty());

        auto current = state_stack.back();
        assert(current.index() == 1);

        auto ptr = std::get<1>(current);
        assert(ptr->values.size() == elementCount);
#endif

        state_stack.pop_back();
        return true;
    }

    // Root of the tree, filled in by the first object/array/string, etc. encountered
    std::unique_ptr<shims::json_value> root;

    // Since all we get are callbacks, we don't have the luxury of using stack memory to save state, so use the heap
    // NOTE: Since we're immediately done processing strings, numbers, booleans, and null, we only need to save state
    //       for objects and arrays
    std::vector<std::variant<json_object_impl*, json_array_impl*>> state_stack;
    std::string object_key;

    bool enableReportError{ true };
} g_JsonHandler;

static const shims::json_object* g_CurrentExeConfig = nullptr;

void load_json()
{
#pragma warning(suppress:4996) // Nonsense warning; _wfopen is perfectly safe
    auto file = _wfopen((g_PackageRootPath / L"config.json").c_str(), L"rb, ccs=UTF-8");
    if (!file)
    {
        throw std::system_error(errno, std::generic_category(), "config.json could not be opened");
    }

    char buffer[2048];
    rapidjson::FileReadStream stream(file, buffer, std::size(buffer));
    rapidjson::AutoUTFInputStream<char32_t, rapidjson::FileReadStream> autoStream(stream);

    rapidjson::GenericReader<rapidjson::AutoUTF<char32_t>, rapidjson::UTF8<>> reader;
    auto result = reader.Parse(autoStream, g_JsonHandler);
    fclose(file);

    if (result.IsError())
    {
        std::stringstream msgStream;
        msgStream << "Error occurred when parsing config.json\n";
        msgStream << "Error: " << rapidjson::GetParseError_En(result.Code()) << "\n";
        msgStream << "File Offest: " << result.Offset();
        throw std::runtime_error(msgStream.str());
    }
    else if (!g_JsonHandler.root)
    {
        throw std::runtime_error("config.json has no contents");
    }

    assert(g_JsonHandler.state_stack.empty());

    // Cache a pointer to the current executable's config, as we are most likely to reference that later
    auto currentExe = g_CurrentExecutable.stem();
    if (auto processes = g_JsonHandler.root->as_object().try_get("processes"))
    {
        for (auto& processConfig : processes->as_array())
        {
            auto& obj = processConfig.as_object();
            auto exe = obj.get("executable").as_string().wstring();
            if (!g_CurrentExeConfig && std::regex_match(currentExe.native(), std::wregex(exe.data(), exe.length())))
            {
                g_CurrentExeConfig = &obj;
                break;
            }
        }
    }

    // Permit ReportError disabling iff basic config.json parse succeeded
    auto enableReportError = g_JsonHandler.root->as_object().try_get("enableReportError");
    if (enableReportError)
    {
        g_JsonHandler.enableReportError = enableReportError->as_boolean().get();
    }
}

void LoadConfig()
{
    if (shims::is_packaged())
    {
        g_PackageFullName = shims::current_package_full_name();
        g_ApplicationUserModelId = shims::current_application_user_model_id();
        g_ApplicationId = shims::application_id_from_application_user_model_id(g_ApplicationUserModelId);
        g_PackageRootPath = shims::current_package_path();
        g_CurrentExecutable = shims::current_executable_path();
    }
    else
    {
        // FUTURE: It may be useful to enable testing outside of a packaged environment. For now, finding ourselves in
        //         this situation almost certainly indicates a bug, so bail out early
        std::terminate();
    }

    load_json();
}

const std::wstring& PackageFullName() noexcept
{
    return g_PackageFullName;
}

const std::wstring& ApplicationUserModelId() noexcept
{
    return g_ApplicationUserModelId;
}

const std::wstring& ApplicationId() noexcept
{
    return g_ApplicationId;
}

const std::filesystem::path& PackageRootPath() noexcept
{
    return g_PackageRootPath;
}

// API definitions
SHIMAPI DWORD __stdcall ShimRegister(_Inout_ void** implFn, _In_ void* shimFn) noexcept
{
    return ::DetourAttach(implFn, shimFn);
}

SHIMAPI DWORD __stdcall ShimUnregister(_Inout_ void** implFn, _In_ void* shimFn) noexcept
{
    return ::DetourDetach(implFn, shimFn);
}

SHIMAPI const wchar_t* __stdcall ShimQueryPackageFullName() noexcept
{
    return g_PackageFullName.c_str();
}

SHIMAPI const wchar_t* __stdcall ShimQueryApplicationUserModelId() noexcept
{
    return g_ApplicationUserModelId.c_str();
}

SHIMAPI const wchar_t* __stdcall ShimQueryApplicationId() noexcept
{
    return g_ApplicationId.c_str();
}

SHIMAPI const wchar_t* __stdcall ShimQueryPackageRootPath() noexcept
{
    return g_PackageRootPath.c_str();
}

SHIMAPI const shims::json_value* __stdcall ShimQueryConfigRoot() noexcept
{
    return g_JsonHandler.root.get();
}

SHIMAPI const shims::json_object* __stdcall ShimQueryAppLaunchConfig(_In_ const wchar_t* applicationId) noexcept try
{
    for (auto& app : g_JsonHandler.root->as_object().get("applications").as_array())
    {
        auto& appObj = app.as_object();
        auto appId = appObj.get("id").as_string().wstring();
        if (iwstring_view(appId.data(), appId.length()) == applicationId)
        {
            return &appObj;
        }
    }

    return nullptr;
}
catch (...)
{
    return nullptr;
}

SHIMAPI const shims::json_object* __stdcall ShimQueryCurrentAppLaunchConfig() noexcept
{
    return ShimQueryAppLaunchConfig(g_ApplicationId.c_str());
}

static inline constexpr iwstring_view remove_suffix_if(iwstring_view str, iwstring_view suffix)
{
    if ((str.length() >= suffix.length()) && (str.substr(str.length() - suffix.length()) == suffix))
    {
        str.remove_suffix(suffix.length());
    }

    return str;
}

SHIMAPI const shims::json_object* __stdcall ShimQueryExeConfig(const wchar_t* executable) noexcept try
{
    const auto exeName = remove_suffix_if(executable, L".exe"_isv);
    if (auto processes = g_JsonHandler.root->as_object().try_get("processes"))
    {
        for (auto& processConfig : processes->as_array())
        {
            auto& obj = processConfig.as_object();
            auto exe = obj.get("executable").as_string().wstring();

            if (std::regex_match(exeName.begin(), exeName.end(), std::wregex(exe.data(), exe.length())))
            {
                return &obj;
            }
        }
    }

    return nullptr;
}
catch (...)
{
    return nullptr;
}

SHIMAPI const shims::json_object* __stdcall ShimQueryCurrentExeConfig() noexcept
{
    return g_CurrentExeConfig;
}

static inline const shims::json_value* find_config(const shims::json_object* exeConfig, const wchar_t* dll)
{
    if (!exeConfig)
    {
        return nullptr;
    }

    // We consider two dll names to match if:
    //  (1) They compare identical
    //  (2) One name is of the form AAAAABB.dll and the other is of the form AAAAA.dll for some architecture bitness
    //      'BB' (32 or 64)
    auto targetDll = remove_suffix_if(remove_suffix_if(dll, L".dll"_isv), shims::warch_string);

    if (auto shims = exeConfig->try_get("shims"))
    {
        for (auto& shimConfig : shims->as_array())
        {
            auto& shimConfigObj = shimConfig.as_object();

            auto dllStr = shimConfigObj.get("dll").as_string().wstring();
            iwstring_view dllStrView(dllStr.data(), dllStr.length());
            if (targetDll == remove_suffix_if(remove_suffix_if(dllStrView, L".dll"_isv), shims::warch_string))
            {
                // NOTE: config is optional
                return shimConfigObj.try_get("config");
            }
        }
    }

    return nullptr;
}

SHIMAPI const shims::json_value* __stdcall ShimQueryConfig(const wchar_t* executable, const wchar_t* dll) noexcept try
{
    return find_config(ShimQueryExeConfig(executable), dll);
}
catch (...)
{
    return nullptr;
}

SHIMAPI const shims::json_value* __stdcall ShimQueryDllConfig(const wchar_t* dll) noexcept try
{
    return find_config(g_CurrentExeConfig, dll);
}
catch (...)
{
    return nullptr;
}

SHIMAPI void __stdcall ShimReportError(const wchar_t* error) noexcept
{
    if (!g_JsonHandler.enableReportError)
    {
        return;
    }
    if (IsDebuggerPresent())
    {
        OutputDebugStringW(error);
        OutputDebugStringW(L"\n");
        DebugBreak();
    }
    else
    {
        MessageBoxW(NULL, error, L"Package Support Framework", MB_OK);
    }
}
