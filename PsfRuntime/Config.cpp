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
#include <psf_constants.h>
#include <psf_runtime.h>
#include <psf_utils.h>
#include <stringapiset.h>
#include <utilities.h>
#include <wil\resource.h>

#include "Config.h"
#include "JsonConfig.h"
#include "psf_tracelogging.h"

using namespace std::literals;

static std::wstring g_PackageFullName;
static std::wstring g_PackageFamilyName;
static std::wstring g_ApplicationUserModelId;
static std::wstring g_ApplicationId;
static std::filesystem::path g_PackageRootPath;
static std::filesystem::path g_FinalPackageRootPath;
static std::filesystem::path g_CurrentExecutable;

// The object that constructs the JSON DOM and holds the root
static struct
{
    bool on_value(std::unique_ptr<psf::json_value> value)
    {
        if (!state_stack.empty())
        {
            assert(root);
            auto& current_value = state_stack.back();
            switch (current_value.index())
            {
            case 0:
            {
                auto& map = std::get<0>(current_value)->values;
                if (map.find(object_key) != map.end())
                {
                    error_message = "'" + object_key + "' already exists in map";
                    return false;
                }

                return std::get<0>(current_value)->values.emplace(std::move(object_key), std::move(value)).second;
            }   break;

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
            error_message = "Can't have more than one root";
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
        if (result)
        {
            state_stack.emplace_back(objPtr);
        }

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
        if (result)
        {
            state_stack.emplace_back(arrPtr);
        }

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
    std::unique_ptr<psf::json_value> root;

    // Since all we get are callbacks, we don't have the luxury of using stack memory to save state, so use the heap
    // NOTE: Since we're immediately done processing strings, numbers, booleans, and null, we only need to save state
    //       for objects and arrays
    std::vector<std::variant<json_object_impl*, json_array_impl*>> state_stack;
    std::string object_key;

    // When non-empty, provides a more useful error message displayed to the user for invalid config.json files
    std::string error_message;

    bool enableReportError{ true };
} g_JsonHandler;

void Log(const char* fmt, ...)
{
    std::string str;
    str.resize(256);

    va_list args;
    va_start(args, fmt);
    std::size_t count = std::vsnprintf(str.data(), str.size() + 1, fmt, args);
    assert(count >= 0);
    va_end(args);

    if (count > str.size())
    {
        count = 1024;       // vswprintf actually returns a negative number, let's just go with something big enough for our long strings; it is resized shortly.
        str.resize(count);

        va_list args2;
        va_start(args2, fmt);
        count = std::vsnprintf(str.data(), str.size() + 1, fmt, args2);
        assert(count >= 0);
        va_end(args2);
    }

    str.resize(count);
    ::OutputDebugStringA(str.c_str());
}
void LogString(const char* name, const char* value)
{
    Log("\t%s=%s\n", name, value);
}
void LogString(const char* name, const wchar_t* value)
{
    Log("\t%s=%ls\n", name, value);
}

void LogCountedStringW(const char* name, const wchar_t* value, std::size_t length)
{
    Log("\t%s=%.*ls\n", name, length, value);
}

static const psf::json_object* g_CurrentExeConfig = nullptr;


void load_json()
{
#pragma warning(suppress:4996) // Nonsense warning; _wfopen is perfectly safe
    auto file = _wfopen((g_PackageRootPath / L"config.json").c_str(), L"rb, ccs=UTF-8");
    if (!file)
    {
        Log("Config.json not found in root of package %ls, look elsewhere.", g_PackageRootPath.c_str());
        ///Check folder with application, then everyhwere in package if needed
#pragma warning(suppress:4996) // Nonsense warning; _wfopen is perfectly safe
        file = _wfopen((g_CurrentExecutable.parent_path() / L"config.json").c_str(), L"rb, ccs=UTF-8");
        if (file)
        {
            Log("Config.json found in executable folder of package %ls", g_PackageRootPath.c_str());
        }
        else
        {
            Log("Config.json not found in executable folder of package %ls, continue looking elsewhere.", g_PackageRootPath.c_str());
            // If not in those two locations, must check everywhere in package.
            for (auto& dentry : std::filesystem::recursive_directory_iterator(g_PackageRootPath))
            {
                try
                {
                    if (dentry.is_regular_file())
                    {
                        if (dentry.path().filename().compare(L"config.json") == 0)
                        {
                            Log("Found config at: %ls", dentry.path().c_str());
#pragma warning(suppress:4996) // Nonsense warning; _wfopen is perfectly safe
                            file = _wfopen(dentry.path().c_str(), L"rb, ccs=UTF-8");
                            break;
                        }
                    }
                }
                catch (...)
                {
                    Log("Non-fatal error enumerating directories while looking for config.json.");
                    psf::TraceLogExceptions("PSFConfigException", L"Non-fatal error enumerating directories while looking for config.json.");
                }
            }
        }

        if (!file)
        {
            psf::TraceLogExceptions("PSFConfigException", "config.json was not found");
            Log("config.json was not found");
            throw std::runtime_error("'Config.json' was not found. Kindly add it inside the package and re-install the application.");
        }
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
        if (g_JsonHandler.error_message.empty())
        {
            msgStream << "Error: " << rapidjson::GetParseError_En(result.Code()) << "\n";
        }
        else
        {
            msgStream << "Error: " << g_JsonHandler.error_message << "\n";
        }
        msgStream << "File Offest: " << result.Offset();
        psf::TraceLogExceptions("PSFConfigException", msgStream.str().c_str());
        throw std::runtime_error(msgStream.str());
    }
    else if (!g_JsonHandler.root)
    {
        psf::TraceLogExceptions("PSFConfigException", "config.json has no contents");
        throw std::runtime_error("config.json has no contents");
    }

    assert(g_JsonHandler.state_stack.empty());

    // Cache a pointer to the current executable's config, as we are most likely to reference that later
    auto currentExe = g_CurrentExecutable.stem();
    if (auto processes = g_JsonHandler.root->as_object().try_get("processes"))
    {
        if (processes)
        {
            for (auto& processConfig : processes->as_array())
            {
                auto& obj = processConfig.as_object();
                auto exe = obj.get("executable").as_string().wstring();
                if (!g_CurrentExeConfig && std::regex_match(currentExe.native(), std::wregex(exe.data(), exe.length())))
                {
                    g_CurrentExeConfig = &obj;
                    LogCountedStringW("Processes config match", exe.data(), exe.length());
                    break;
                }
                else if (!g_CurrentExeConfig)
                {
                    //LogCountedStringW("Processes config notmatched", exe.data(), exe.length());
                }
            }
        }
        else
        {
            Log("No processes to match; no fixups to load.");
        }
    }
    else
    {
        Log("No Processes to match; no fixups to load.");
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
    if (psf::is_packaged())
    {
        g_PackageFullName = psf::current_package_full_name();
        g_PackageFamilyName = psf::current_package_family_name();
        g_ApplicationUserModelId = psf::current_application_user_model_id();
        g_ApplicationId = psf::application_id_from_application_user_model_id(g_ApplicationUserModelId);
        g_PackageRootPath = psf::current_package_path();
        g_FinalPackageRootPath = psf::get_final_path_name(g_PackageRootPath);
        g_CurrentExecutable = psf::current_executable_path();

        LogCountedStringW("g_PackageFullName", g_PackageFullName.data(), g_PackageFullName.length());
        LogCountedStringW("g_PackageFamilyName", g_PackageFamilyName.data(), g_PackageFamilyName.length());
        LogCountedStringW("g_ApplicationUserModelId", g_ApplicationUserModelId.data(), g_ApplicationUserModelId.length());
        LogCountedStringW("g_ApplicationId", g_ApplicationId.data(), g_ApplicationId.length());
        LogString("g_PackageRootPath", g_PackageRootPath.c_str());
        LogString("g_FinalPackageRootPath", g_FinalPackageRootPath.c_str());
        LogString("g_CurrentExecutable", g_CurrentExecutable.c_str());
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

const std::wstring& PackageFamilyName() noexcept
{
    return g_PackageFamilyName;
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

const std::filesystem::path& FinalPackageRootPath() noexcept
{
    return g_FinalPackageRootPath;
}

// API definitions
PSFAPI DWORD __stdcall PSFRegister(_Inout_ void** implFn, _In_ void* fixupFn) noexcept
{
    return ::DetourAttach(implFn, fixupFn);
}

PSFAPI DWORD __stdcall PSFUnregister(_Inout_ void** implFn, _In_ void* fixupFn) noexcept
{
    return ::DetourDetach(implFn, fixupFn);
}

PSFAPI const wchar_t* __stdcall PSFQueryPackageFullName() noexcept
{
    return g_PackageFullName.c_str();
}

PSFAPI const wchar_t* __stdcall PSFQueryPackageFamilyName() noexcept
{
    return g_PackageFamilyName.c_str();
}

PSFAPI const wchar_t* __stdcall PSFQueryApplicationUserModelId() noexcept
{
    return g_ApplicationUserModelId.c_str();
}

PSFAPI const wchar_t* __stdcall PSFQueryApplicationId() noexcept
{
    return g_ApplicationId.c_str();
}

PSFAPI const wchar_t* __stdcall PSFQueryPackageRootPath() noexcept
{
    return g_PackageRootPath.c_str();
}

PSFAPI const wchar_t* __stdcall PSFQueryFinalPackageRootPath() noexcept
{
    return g_FinalPackageRootPath.c_str();
}

PSFAPI const psf::json_value* __stdcall PSFQueryConfigRoot() noexcept
{
    return g_JsonHandler.root.get();
}

PSFAPI const psf::json_object* __stdcall PSFQueryAppLaunchConfig(_In_ const wchar_t* applicationId, bool verbose) noexcept try
{
    for (auto& app : g_JsonHandler.root->as_object().get("applications").as_array())
    {
        auto& appObj = app.as_object();
        auto appId = appObj.get("id").as_string().wstring();

        if (iwstring_view(appId.data(), appId.length()) == applicationId)
        {
            if (verbose)
            {
                LogCountedStringW("Json Application match against id", appId.data(), appId.length());
            }
            return &appObj;
        }
    }

    if (verbose)
    {
        Log("\tNo Matches");
    }
    psf::TraceLogExceptions("PSFConfigException", (L"No matching app config found for appId " + std::wstring(applicationId)).c_str());
    return nullptr;
}
catch (...)
{
    psf::TraceLogExceptions("PSFConfigException", (L"PSFQueryAppLaunchConfig Exception " + std::wstring(applicationId)).c_str());
    return nullptr;
}

PSFAPI const psf::json_object* __stdcall PSFQueryCurrentAppLaunchConfig(bool verbose) noexcept
{
    return PSFQueryAppLaunchConfig(g_ApplicationId.c_str(), verbose);
}

PSFAPI const psf::json_object* __stdcall PSFQueryAppMonitorConfig() noexcept
{
    const psf::json_object* application = PSFQueryAppLaunchConfig(g_ApplicationId.c_str(), false);
    auto mon = application->try_get("monitor");

    if (mon)
    {
        auto& monObj = mon->as_object();
        return &monObj;
    }

    return nullptr;
}

PSFAPI const psf::json_object* __stdcall PSFQueryStartScriptInfo() noexcept
{
    auto application = PSFQueryAppLaunchConfig(g_ApplicationId.c_str(), false);

    auto mon = application->try_get("startScript");
    if (mon)
    {
        auto& monObj = mon->as_object();
        return &monObj;
    }

    return nullptr;
}

PSFAPI const psf::json_object* __stdcall PSFQueryEndScriptInfo() noexcept
{
    auto application = PSFQueryAppLaunchConfig(g_ApplicationId.c_str(), false);
    auto mon = application->try_get("endScript");

    if (mon)
    {
        auto& monObj = mon->as_object();
        return &monObj;
    }

    return nullptr;
}

static inline constexpr iwstring_view remove_suffix_if(iwstring_view str, iwstring_view suffix)
{
    if ((str.length() >= suffix.length()) && (str.substr(str.length() - suffix.length()) == suffix))
    {
        str.remove_suffix(suffix.length());
    }

    return str;
}

PSFAPI const psf::json_object* __stdcall PSFQueryExeConfig(const wchar_t* executable) noexcept try
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
    psf::TraceLogExceptions("PSFConfigException", (L"PSFQueryExeConfig Exception " + std::wstring(executable)).c_str());
    return nullptr;
}

PSFAPI const psf::json_object* __stdcall PSFQueryCurrentExeConfig() noexcept
{
    return g_CurrentExeConfig;
}

static inline const psf::json_value* find_config(const psf::json_object* exeConfig, const wchar_t* dll)
{
    if (!exeConfig)
    {
        return nullptr;
    }

    // We consider two dll names to match if:
    //  (1) They compare identical
    //  (2) One name is of the form AAAAABB.dll and the other is of the form AAAAA.dll for some architecture bitness
    //      'BB' (32 or 64)
    auto targetDll = remove_suffix_if(remove_suffix_if(dll, L".dll"_isv), psf::warch_string);

    if (auto fixups = exeConfig->try_get("fixups"))
    {
        for (auto& fixupConfig : fixups->as_array())
        {
            auto& fixupConfigObj = fixupConfig.as_object();

            auto dllStr = fixupConfigObj.get("dll").as_string().wstring();
            iwstring_view dllStrView(dllStr.data(), dllStr.length());
            if (targetDll == remove_suffix_if(remove_suffix_if(dllStrView, L".dll"_isv), psf::warch_string))
            {
                // NOTE: config is optional
                return fixupConfigObj.try_get("config");
            }
        }
    }

    return nullptr;
}

PSFAPI const psf::json_value* __stdcall PSFQueryConfig(const wchar_t* executable, const wchar_t* dll) noexcept try
{
    return find_config(PSFQueryExeConfig(executable), dll);
}
catch (...)
{
    psf::TraceLogExceptions("PSFConfigException", (L"PSFQueryConfig failed. executable: " + std::wstring(executable) + L"dll: " + std::wstring(dll)).c_str());
    return nullptr;
}

PSFAPI const psf::json_value* __stdcall PSFQueryDllConfig(const wchar_t* dll) noexcept try
{
    return find_config(g_CurrentExeConfig, dll);
}
catch (...)
{
    psf::TraceLogExceptions("PSFConfigException", (L"PSFQueryConfig failed. dll: " + std::wstring(dll)).c_str());
    return nullptr;
}

PSFAPI void __stdcall PSFReportError(const wchar_t* error) noexcept
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
