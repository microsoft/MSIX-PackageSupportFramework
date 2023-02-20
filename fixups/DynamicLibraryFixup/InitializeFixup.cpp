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
#include "psf_tracelogging.h"

using namespace std::literals;

std::filesystem::path g_dynf_packageRootPath;
std::filesystem::path g_dynf_packageVfsRootPath;
bool                  g_dynf_forcepackagedlluse = false;


std::vector<dll_location_spec> g_dynf_dllSpecs;

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
        ::OutputDebugStringA("Exception in Log()");
        ::OutputDebugStringA(fmt);
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
        wstr.resize(count);
#if _DEBUG
        ::OutputDebugStringW(wstr.c_str());
#endif
    }
    catch (...)
    {
        ::OutputDebugStringA("Exception in wide Log()");
        ::OutputDebugStringW(fmt);
    }
}
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

void InitializeFixups()
{

    Log("Initializing DynamicLibraryFixup");

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
    std::wstringstream traceDataStream;
    Log("DynamicLibraryFixup InitializeConfiguration()");
    if (auto rootConfig = ::PSFQueryCurrentDllConfig())
    {
        auto& rootObject = rootConfig->as_object();
        traceDataStream << " config:\n";

        if (auto forceValue = rootObject.try_get("forcePackageDllUse"))  
        {
            if (forceValue)
                g_dynf_forcepackagedlluse = forceValue->as_boolean().get();
            else
                g_dynf_forcepackagedlluse = false;
            traceDataStream << " forcePackageDllUse:" << (g_dynf_forcepackagedlluse ? L"true" : L"false") << " ;";
        }

        if (g_dynf_forcepackagedlluse == true)
        {
            Log("DynamicLibraryFixup ForcePackageDllUse=true");
            if (auto relativeDllsValue = rootObject.try_get("relativeDllPaths"))
            {
                if (relativeDllsValue)
                {
                    traceDataStream << " relativeDllPaths:\n" ;
                    const psf::json_array& dllArray = relativeDllsValue->as_array();
                    int count = 0;
                    for (auto& spec : dllArray)
                    {
                        auto& specObject = spec.as_object();

                        auto filename = specObject.get("name").as_string().wstring();
                        traceDataStream << " name: " << filename << " ;";

                        auto relpath = specObject.get("filepath").as_string().wstring();
                        traceDataStream << " filepath: " << relpath << " ;";
                        std::filesystem::path fullpath = g_dynf_packageRootPath / relpath;

                        g_dynf_dllSpecs.emplace_back();
                        g_dynf_dllSpecs.back().full_filepath = fullpath;
                        g_dynf_dllSpecs.back().filename = filename;
                        count++;
                    };
                    Log("DynamicLibraryFixup: %d relative items read.", count);
                }
            }
            else
                Log("DynamicLibraryFixup ForcePacageDllUse=false");
        }

        psf::TraceLogFixupConfig("DynamicLibraryFixup", traceDataStream.str().c_str());
    }
}
