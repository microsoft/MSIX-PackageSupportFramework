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
#include <psf_logging.h>
#include <utilities.h>

#include <filesystem>


#include "FunctionImplementations.h"
#include "EnvVar_spec.h"

using namespace std::literals;

std::filesystem::path g_envvar_packageRootPath;
std::filesystem::path g_envvar_packageVfsRootPath;
bool                  g_envvar_forcepackagedlluse = false;


std::vector<env_var_spec> g_envvar_envVarSpecs;

void InitializeFixups()
{


#if _DEBUG
    Log(L"Initializing EnvVarFixup");
#endif

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
#if _DEBUG
    Log(L"EnvVarFixup InitializeConfiguration()");
#endif
    if (auto rootConfig = ::PSFQueryCurrentDllConfig())
    {
        auto& rootObject = rootConfig->as_object();


        if (auto EnvVarsValue = rootObject.try_get("envVars"))
        {
            if (EnvVarsValue)
            {
                const psf::json_array& dllArray = EnvVarsValue->as_array();
                int count = 0;
                for (auto& spec : dllArray)
                {
                    auto& specObject = spec.as_object();

                    auto variablenamePattern = specObject.get("name").as_string().wstring();

                    auto variablevalue = specObject.get("value").as_string().wstring();

                    auto useregistry = specObject.get("useregistry").as_string().wstring();
#if _DEBUG
                    LogString(0, L"GetEnvFixup Config: name", variablenamePattern.data());
                    LogString(0, L"GetEnvFixup Config: value", variablevalue.data());
                    LogString(0, L"GetEnvFixup Config: useregistry", useregistry.data());
#endif
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
#if _DEBUG
                Log(L"EnvVarFixup: %d config items read.", count);
#endif
            }
        }
        if (g_envvar_envVarSpecs.size() == 0)
        {
#if _DEBUG
            Log(L"EnvVarFixup: Zero config items read.");
#endif
        }
    }
}
