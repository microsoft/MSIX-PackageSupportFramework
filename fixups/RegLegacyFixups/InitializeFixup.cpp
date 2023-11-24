//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "pch.h"

#include <regex>
#include <vector>

#include <known_folders.h>
#include <objbase.h>

#include <psf_framework.h>
#include <utilities.h>

#include <filesystem>
using namespace std::literals;

#include "FunctionImplementations.h"
#include "Reg_Remediation_spec.h"
#include "psf_tracelogging.h"

using namespace winrt::Windows::Management::Deployment;
using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::Foundation::Collections;

std::vector<Reg_Remediation_Spec>  g_regRemediationSpecs;

#if _DEBUG 
void Log(const char* fmt, ...)
{
   
    try
    {
        va_list args;
        va_start(args, fmt);
        std::string str;
        str.resize(256);
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
    catch (...)
    {
        ::OutputDebugStringA("Exception in Log()");
        ::OutputDebugStringA(fmt);
    }
}
#else
void Log(const char* , ...)
{
}
#endif
#if _DEBUG
void Log(const wchar_t* fmt, ...)
{
    try
    {
        va_list args;
        va_start(args, fmt);

        std::wstring wstr;
        wstr.resize(256);
        std::size_t count = std::vswprintf(wstr.data(), wstr.size() + 1, fmt, args);
        va_end(args);

        if (count > wstr.size())
        {
            count = 1024;       // vswprintf actually returns a negative number, let's just go with something big enough for our long strings; it is resized shortly.
            wstr.resize(count);
            va_list args2;
            va_start(args2, fmt);
            count = std::vswprintf(wstr.data(), wstr.size() + 1, fmt, args2);
            va_end(args2);
        }
        wstr.resize(count);
        ::OutputDebugStringW(wstr.c_str());
    }
    catch (...)
    {
        ::OutputDebugStringA("Exception in wide Log()");
        ::OutputDebugStringW(fmt);
    }
}
#else
void Log(const wchar_t* , ...)
{
}
#endif

void LogString(const char* name, const char* value)
{
    Log("%s=%s\n", name, value);
}
void LogString(const char* name, const wchar_t* value)
{
    Log("%s=%ls\n", name, value);
}
void LogString(const wchar_t* name, const char* value)
{
    Log("%ls=%s\n", name, value);
}
void LogString(const wchar_t* name, const wchar_t* value)
{
    Log(L"%ls=%ls\n", name, value);
}

struct EntryToCreate 
{
    std::wstring path;
    std::unordered_map<std::wstring, std::wstring> values;
};


void CreateRegistryRedirectEntries(std::wstring_view dependency_name, std::vector<EntryToCreate> entries)
{
    Log("RegLegacyFixups Create redirected registry values\n");

    Package pkg = Package::Current();
    IVectorView<Package> dependencies = pkg.Dependencies();

    std::wregex dependency_version_regex = std::wregex(L"%dependency_version%");
    std::wregex dependency_path_regex = std::wregex(L"%dependency_root_path%");

    Package dependency = nullptr;

#ifdef _DEBUG
    Log("Filtering required dependency\n");
#endif
    for (auto&& dep : dependencies) {
        std::wstring_view name = dep.Id().Name();
#ifdef _DEBUG
        Log("Checking package dependency %.*LS\n", name.length(), name.data());
#endif

        if (name.find(dependency_name) != std::wstring::npos) {
#ifdef _DEBUG
            Log("Found required dependency\n");
#endif
            dependency = dep;
            break;
        }
    }
    if (dependency == nullptr) 
    {
        Log("No package with name %.*LS found\n", static_cast<int>(dependency_name.length()), dependency_name.data());
        return;
    }

    std::wstring dependency_path(dependency.EffectivePath());
    auto version = dependency.Id().Version();
    std::wstring dependency_version = std::to_wstring(version.Major) + L"." + std::to_wstring(version.Minor) + L"." + std::to_wstring(version.Build) + L"." + std::to_wstring(version.Revision);
    Log("Dependency path: %LS, version: %LS\n", dependency_path.c_str(), dependency_version.c_str());

    for (const auto& reg_entry : entries)
    {
        HKEY res;
        std::wstring reg_path = std::regex_replace(reg_entry.path, dependency_version_regex, dependency_version);

        LSTATUS st = ::RegCreateKeyExW(HKEY_CURRENT_USER, reg_path.c_str(), 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &res, NULL);
        if (st != ERROR_SUCCESS)
        {
            Log("Create key %LS failed\n", reg_entry.path.c_str());
            return;
        }

        g_regRedirectedHivePaths.insert(narrow(reg_path));
        g_regCreatedKeysOrdered.push_back(reg_path);

        for (const auto& it : reg_entry.values)
        {
            std::wstring value_data = std::regex_replace(it.second, dependency_path_regex, dependency_path);
            value_data = std::regex_replace(value_data, dependency_version_regex, dependency_version);

            st = ::RegSetValueExW(res, it.first.c_str(), 0, REG_SZ, (LPBYTE)value_data.c_str(),
                (DWORD)(value_data.size() + 1 /* for null termination char */) * sizeof(wchar_t));

            if (st != ERROR_SUCCESS)
            {
#ifdef _DEBUG
                Log("set value %s failed\n", it.first.c_str());
#endif
            }
        }
    }
}

void CleanupFixups()
{
    Log("RegLegacyFixups CleanupFixups()\n");

    // Deleted the created keys in reverse order
    std::vector<std::wstring>::reverse_iterator redirected_path = g_regCreatedKeysOrdered.rbegin();

    while (redirected_path != g_regCreatedKeysOrdered.rend())
    {
        LSTATUS st = ::RegDeleteTreeW(HKEY_CURRENT_USER, redirected_path->c_str());
        if (st != ERROR_SUCCESS)
        {
            Log("Delete key %LS failed\n", redirected_path->c_str());
        }
        redirected_path++;
    }
}


void InitializeConfiguration()
{
    std::wstringstream traceDataStream;
    Log("RegLegacyFixups Start InitializeConfiguration()\n");
    if (auto rootConfig = ::PSFQueryCurrentDllConfig())
    {
        if (rootConfig != NULL)
        {
            traceDataStream << " config:\n";
            Log("RegLegacyFixups process config\n");
            const psf::json_array& rootConfigArray = rootConfig->as_array();

            for (auto& spec : rootConfigArray)
            {
                Log("RegLegacyFixups: process spec\n");
                Reg_Remediation_Spec specItem;
                auto& specObject = spec.as_object();
                if (auto regItems = specObject.try_get("remediation"))
                {
                    traceDataStream << " remediation:\n";
                    Log("RegLegacyFixups:  remediation array:\n");
                    const psf::json_array& remediationArray = regItems->as_array();
                    for (auto& regItem : remediationArray)
                    {
                        Log("RegLegacyFixups:    remediation:\n");
                        auto& regItemObject = regItem.as_object();
                        Reg_Remediation_Record recordItem;
                        auto type = regItemObject.get("type").as_string().wstring();
                        traceDataStream << " type: " << type << " ;";
                        Log("RegLegacyFixups: have type");
                        Log(L"Type: %Ls\n", type.data());
                        //Reg_Remediation_Spec specItem;
                        if (type.compare(L"ModifyKeyAccess") == 0)
                        {
                            Log("RegLegacyFixups:      is ModifyKeyAccess\n");
                            recordItem.remeditaionType = Reg_Remediation_Type_ModifyKeyAccess;

                            auto hiveType = regItemObject.try_get("hive")->as_string().wstring();
                            traceDataStream << " hive: " << hiveType << " ;";
                            Log(L"Hive: %Ls\n", hiveType.data());
                            if (hiveType.compare(L"HKCU") == 0)
                            {
                                recordItem.modifyKeyAccess.hive = Modify_Key_Hive_Type_HKCU;
                            }
                            else if (hiveType.compare(L"HKLM") == 0)
                            {
                                recordItem.modifyKeyAccess.hive = Modify_Key_Hive_Type_HKLM;
                            }
                            else
                            {
                                recordItem.modifyKeyAccess.hive = Modify_Key_Hive_Type_Unknown;
                            }
                            Log("RegLegacyFixups:      have hive\n");
                            traceDataStream << " patterns:\n";
                            for (auto& pattern : regItemObject.get("patterns").as_array())
                            {
                                
                                auto patternString = pattern.as_string().wstring();
                                traceDataStream << patternString << " ;";
                                Log(L"Pattern:      %Ls\n", patternString.data());
                                recordItem.modifyKeyAccess.patterns.push_back(patternString.data());

                            }
                            Log("RegLegacyFixups:      have patterns\n");

                            auto accessType = regItemObject.try_get("access")->as_string().wstring();
                            traceDataStream << " access: " << accessType << " ;";
                            Log(L"Access: %Ls\n", accessType.data());
                            if (accessType.compare(L"Full2RW") == 0)
                            {
                                recordItem.modifyKeyAccess.access = Modify_Key_Access_Type_Full2RW;
                            }
                            else if (accessType.compare(L"Full2MaxAllowed") == 0)
                            {
                                recordItem.modifyKeyAccess.access = Modify_Key_Access_Type_Full2MaxAllowed;
                            }
                            else if (accessType.compare(L"Full2R") == 0)
                            {
                                recordItem.modifyKeyAccess.access = Modify_Key_Access_Type_Full2R;
                            }
                            else if (accessType.compare(L"RW2R") == 0)
                            {
                                recordItem.modifyKeyAccess.access = Modify_Key_Access_Type_RW2R;
                            }
                            else if (accessType.compare(L"RW2MaxAllowed") == 0)
                            {
                                recordItem.modifyKeyAccess.access = Modify_Key_Access_Type_RW2MaxAllowed;
                            }
                            else
                            {
                                recordItem.modifyKeyAccess.access = Modify_Key_Access_Type_Unknown;
                            }
                            Log("RegLegacyFixups:      have access\n");
                            specItem.remediationRecords.push_back(recordItem);
                        }
                        else if (type.compare(L"FakeDelete") == 0)
                        {
                            Log("RegLegacyFixups:      is FakeDelete\n");
                            recordItem.remeditaionType = Reg_Remediation_Type_FakeDelete;

                            auto hiveType = regItemObject.try_get("hive")->as_string().wstring();
                            traceDataStream << " hive: " << hiveType << " ;";
                            Log(L"Hive:      %Ls\n", hiveType.data());
                            if (hiveType.compare(L"HKCU") == 0)
                            {
                                recordItem.fakeDeleteKey.hive = Modify_Key_Hive_Type_HKCU;
                            }
                            else if (hiveType.compare(L"HKLM") == 0)
                            {
                                recordItem.fakeDeleteKey.hive = Modify_Key_Hive_Type_HKLM;
                            }
                            else
                            {
                                recordItem.fakeDeleteKey.hive = Modify_Key_Hive_Type_Unknown;
                            }
                            Log("RegLegacyFixups:      have hive\n");

                            traceDataStream << " patterns:\n";
                            for (auto& pattern : regItemObject.get("patterns").as_array())
                            {
                                auto patternString = pattern.as_string().wstring();
                                traceDataStream << patternString << " ;";
                                Log(L"Pattern:        %Ls\n", patternString.data());
                                recordItem.fakeDeleteKey.patterns.push_back(patternString.data());
                            }
                            Log("RegLegacyFixups:      have patterns\n");

                            specItem.remediationRecords.push_back(recordItem);
                        }
                        else if (type.compare(L"Redirect") == 0)
                        {
                            std::wstring_view dependency = regItemObject.get("dependency").as_string().wstring();
                            const psf::json_array& data = regItemObject.get("data").as_array();

                            std::vector<EntryToCreate>entriesTtoCreate;

                            for (auto& entry : data)
                            {
                                auto& obj = entry.as_object();

                                EntryToCreate toCreate;
                                std::wstring_view key = obj.get("key").as_string().wstring();
                                toCreate.path = key;

                                for (const auto& it : obj.get("values").as_object())
                                {
                                    // RegSetValueExW expected null terminated strings. string_view cannot guarantee a null terminated string. Need to copy to a std::wstring
                                    std::wstring value_name(it.first.begin(), it.first.end());
                                    std::wstring value_data(it.second.as_string().wstring());

                                    toCreate.values.insert({ value_name, value_data });
                                }

                                entriesTtoCreate.push_back(toCreate);
                            }

                            CreateRegistryRedirectEntries(dependency, entriesTtoCreate);
                        }
                        // Look for DeletionMarker remediation
                        else if (type.compare(L"DeletionMarker") == 0)
                        {
                            Log("RegLegacyFixups: \t is DeletionMarker\n");
                            recordItem.remeditaionType = Reg_Remediation_type_DeletionMarker;

                            // Look for hive - data
                            auto hiveType = regItemObject.try_get("hive")->as_string().wstring();
                            traceDataStream << " hive: " << hiveType << " ;";
                            Log(L"Hive:      %Ls\n", hiveType.data());

                            if (hiveType.compare(L"HKCU") == 0)
                            {
                                recordItem.deletionMarker.hive = Modify_Key_Hive_Type_HKCU;
                            }
                            else if (hiveType.compare(L"HKLM") == 0)
                            {
                                recordItem.deletionMarker.hive = Modify_Key_Hive_Type_HKLM;
                            }
                            else
                            {
                                recordItem.deletionMarker.hive = Modify_Key_Hive_Type_Unknown;
                            }
                            Log("RegLegacyFixups: \t have hive\n");

                            // Look for key - data
                            auto keyString = regItemObject.get("key").as_string().wstring();
                            traceDataStream << keyString << " ;";
                            Log(L"key:        %Ls\n", keyString.data());
                            recordItem.deletionMarker.key = keyString.data();
                            
                            Log("RegLegacyFixups: \t have key\n");

                            // Look for value - data
                            traceDataStream << " values:\n";

                            auto values = regItemObject.try_get("values");
                            if (values != nullptr)
                            {
                                for (auto& value : values->as_array())
                                {
                                    auto valueString = value.as_string().wstring();
                                    traceDataStream << valueString << " ;";
                                    Log(L"Value: \t %Ls\n", valueString.data());
                                    recordItem.deletionMarker.values.push_back(valueString.data());
                                }

                                Log("RegLegacyFixups: \t have value\n");
                            }
                            else
                            {
                                Log("RegLegacyFixups: \t have no value\n");
                            }
                            
                            specItem.remediationRecords.push_back(recordItem);
                        }
                        else
                        {
                        }
                        g_regRemediationSpecs.push_back(specItem);
                    }
                }
            }
            psf::TraceLogFixupConfig("RegLegacyFixup", traceDataStream.str().c_str());
        }
        else
        {
            Log("RegLegacyFixups: Fixup not found in json config.\n");
        }
        Log("RegLegacyFixups End InitializeConfiguration()\n");
    }
}