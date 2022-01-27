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
#include "dll_location_spec.h"

#if _DEBUG
#define MOREDEBUG 1
#endif

using namespace std::literals;

std::filesystem::path g_dynf_packageRootPath;
std::filesystem::path g_dynf_packageVfsRootPath;
bool                  g_dynf_forcepackagedlluse = false;


std::vector<dll_location_spec> g_dynf_dllSpecs;

void InitializeFixups()
{
#if _DEBUG
    Log(L"Initializing DynamicLibraryFixup");
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
    g_dynf_packageRootPath = psf::remove_trailing_path_separators(packageRootPath);
    g_dynf_packageVfsRootPath = g_dynf_packageRootPath / L"VFS";

} // InitializePaths()


void InitializeConfiguration()
{
#if _DEBUG
    Log(L"DynamicLibraryFixup InitializeConfiguration()");
#endif
    if (auto rootConfig = ::PSFQueryCurrentDllConfig())
    {
        auto& rootObject = rootConfig->as_object();

        if (auto forceValue = rootObject.try_get("forcePackageDllUse"))  
        {
            if (forceValue)
                g_dynf_forcepackagedlluse = forceValue->as_boolean().get();
            else
                g_dynf_forcepackagedlluse = false;
        }

        if (g_dynf_forcepackagedlluse == true)
        {
#if _DEBUG
            Log(L"DynamicLibraryFixup ForcePackageDllUse=true");
#endif
            if (auto relativeDllsValue = rootObject.try_get("relativeDllPaths"))
            {
                if (relativeDllsValue)
                {
                    const psf::json_array& dllArray = relativeDllsValue->as_array();
                    int count = 0;
                    for (auto& spec : dllArray)
                    {
                        auto& specObject = spec.as_object();

                        std::wstring_view filename = specObject.get("name").as_string().wstring();

                        auto relpath = specObject.get("filepath").as_string().wstring();
                        std::filesystem::path fullpath = g_dynf_packageRootPath / relpath;

                        dllBitness bitness = NotSpecified;
                        std::wstring wArch = L"";
                        if (auto arch = specObject.try_get("architecture"))
                        {
                            wArch = arch->as_string().wstring();
                            if (wArch.compare(L"x86"))
                            {
                                bitness = x86;
                            }
                            else if (wArch.compare(L"x86"))
                            {
                                bitness = x64;
                            }
                            else if (wArch.compare(L"anyCPU"))
                            {
                                bitness = AnyCPU;
                            }
                        }
                        g_dynf_dllSpecs.emplace_back();
                        g_dynf_dllSpecs.back().full_filepath = fullpath;
                        g_dynf_dllSpecs.back().filename = filename;
                        g_dynf_dllSpecs.back().architecture = bitness;
#if MOREDEBUG
                        Log(L"DynamicLibraryFixup: %s : (%s) : %s", filename.data(), wArch.c_str(), fullpath.c_str());
#endif
                        count++;
                    };
#if _DEBUG
                    Log(L"DynamicLibraryFixup: %d relative items read.", count);
#endif
                }
            }
            else
            {
#if _DEBUG
                Log(L"DynamicLibraryFixup ForcePacageDllUse=false");
#endif
            }
        }
    }
}
