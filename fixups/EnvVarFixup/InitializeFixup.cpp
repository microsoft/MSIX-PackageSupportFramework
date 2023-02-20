//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <regex>
#include <vector>

#include <known_folders.h>
#include <objbase.h>

#include <psf_framework.h>
#include <utilities.h>

#include <filesystem>


#include "FunctionImplementations.h"
#include "EnvVar_spec.h"
#include "psf_tracelogging.h"

using namespace std::literals;

std::filesystem::path g_envvar_packageRootPath;
std::filesystem::path g_envvar_packageVfsRootPath;
bool                  g_envvar_forcepackagedlluse = false;


std::vector<env_var_spec> g_envvar_envVarSpecs;

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
#if _DEBUG
        ::OutputDebugStringA(str.c_str());
#endif
    }
    catch (...)
    {
#if _DEBUG
        ::OutputDebugStringA("Exception in Log()");
        ::OutputDebugStringA(fmt);
#endif
    }
}
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
#if _DEBUG
        wstr.resize(count);
        ::OutputDebugStringW(wstr.c_str());
#endif
    }
    catch (...)
    {
#if _DEBUG
        ::OutputDebugStringA("Exception in wide Log()");
        ::OutputDebugStringW(fmt);
#endif
    }
}
void LogString(DWORD inst, const char* name, const char* value)
{
    Log("[%d] %s=%s\n", inst, name, value);
}
void LogString(DWORD inst, const char* name, const wchar_t* value)
{
    Log("[%d] %s=%ls\n", inst, name, value);
}
void LogString(DWORD inst, const wchar_t* name, const char* value)
{
    Log("[%d] %ls=%s\n", inst, name, value);
}
void LogString(DWORD inst, const wchar_t* name, const wchar_t* value)
{
    Log(L"[%d] %ls=%ls\n", inst, name, value);
}

void InitializeFixups()
{

    Log("Initializing EnvVarFixup");

    // For path comparison's sake - and the fact that std::filesystem::path doesn't handle (root-)local device paths all
    // that well - ensure that these paths are drive-absolute
    auto packageRootPath = ::PSFQueryPackageRootPath();
    if (auto pathType = psf::path_type(packageRootPath);
        (pathType == psf::dos_path_type::root_local_device) || (pathType == psf::dos_path_type::local_device))
    {
        packageRootPath += 4;
    }
    assert(psf::path_type(packageRootPath) == psf::dos_path_type::drive_absolute);
    g_envvar_packageRootPath = psf::remove_trailing_path_separators(packageRootPath);
    g_envvar_packageVfsRootPath = g_envvar_packageRootPath / L"VFS";

} // InitializePaths()


void InitializeConfiguration()
{
    std::wstringstream traceDataStream;
    Log("EnvVarFixup InitializeConfiguration()");
    if (auto rootConfig = ::PSFQueryCurrentDllConfig())
    {
        auto& rootObject = rootConfig->as_object();
        traceDataStream << " config:\n";

        if (auto EnvVarsValue = rootObject.try_get("envVars"))
        {
            if (EnvVarsValue)
            {
                traceDataStream << " envVars:\n";
                const psf::json_array& dllArray = EnvVarsValue->as_array();
                int count = 0;
                for (auto& spec : dllArray)
                {
                    auto& specObject = spec.as_object();

                    auto variablenamePattern = specObject.get("name").as_string().wstring();
                    traceDataStream << " name: " << variablenamePattern << " ;";

                    auto variablevalue = specObject.get("value").as_string().wstring();
                    traceDataStream << " value: " << variablevalue << " ;";

                    auto useregistry = specObject.get("useregistry").as_string().wstring();
                    traceDataStream << " useregistry: " << useregistry << " ;";
                    LogString(0, "GetEnvFixup Config: name", variablenamePattern.data());
                    LogString(0, "GetEnvFixup Config: value", variablevalue.data());
                    LogString(0, "GetEnvFixup Config: useregistry", useregistry.data());
                    g_envvar_envVarSpecs.emplace_back();
                    g_envvar_envVarSpecs.back().variablename.assign(variablenamePattern.data(), variablenamePattern.length());
                    g_envvar_envVarSpecs.back().variablevalue = variablevalue;
                    if (useregistry.compare(L"true") == 0 ||
                        useregistry.compare(L"True") == 0 ||
                        useregistry.compare(L"TRUE") == 0)
                    {
                        g_envvar_envVarSpecs.back().useregistry = true;
                    }
                    else
                    {
                        g_envvar_envVarSpecs.back().useregistry = false;
                    }
                    count++;
                };
                Log("EnvVarFixup: %d config items read.", count);
            }
        }
        if (g_envvar_envVarSpecs.size() == 0)
        {
            Log("EnvVarFixup: Zero config items read.");
        }

        psf::TraceLogFixupConfig("EnvVarFixup", traceDataStream.str().c_str());
    }
}
