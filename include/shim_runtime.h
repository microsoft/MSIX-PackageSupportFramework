//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <windows.h>

#include "shim_config.h"
#include "shim_utils.h"

#ifndef SHIMAPI
#ifdef SHIMRUNTIME_EXPORTS
#define SHIMAPI __declspec(dllexport)
#else
#define SHIMAPI __declspec(dllimport)
#endif
#endif

using ShimInitializeProc = int (__stdcall *)() noexcept;
using ShimUninitializeProc = int (__stdcall *)() noexcept;

// ShimRuntime exports
// NOTE: Unless stated otherwise, all memory returned is allocated by the ShimRuntime and remains valid so long as the
//       dll is loaded.
extern "C" {

// Abstractions around the DetourAttach/Detach calls so that shims don't need to link against the detours lib
SHIMAPI DWORD __stdcall ShimRegister(_Inout_ void** implFn, _In_ void* shimFn) noexcept;
SHIMAPI DWORD __stdcall ShimUnregister(_Inout_ void** implFn, _In_ void* shimFn) noexcept;

// Simplifications around the package query API from appmodel.h
// NOTE: These functions are guaranteed to succeed as ShimRuntime will fail to load if they can't be set (e.g. when
//       running outside of a package)
SHIMAPI const wchar_t* __stdcall ShimQueryPackageFullName() noexcept;
SHIMAPI const wchar_t* __stdcall ShimQueryApplicationUserModelId() noexcept;
SHIMAPI const wchar_t* __stdcall ShimQueryApplicationId() noexcept;
SHIMAPI const wchar_t* __stdcall ShimQueryPackageRootPath() noexcept;

SHIMAPI const shims::json_value* __stdcall ShimQueryConfigRoot() noexcept;

SHIMAPI const shims::json_object* __stdcall ShimQueryAppLaunchConfig(_In_ const wchar_t* applicationId) noexcept;
SHIMAPI const shims::json_object* __stdcall ShimQueryCurrentAppLaunchConfig() noexcept;

// NOTE: Returns null when no configuration is set for the executable, or if an error occurred
SHIMAPI const shims::json_value* __stdcall ShimQueryConfig(const wchar_t* executable, const wchar_t* dll) noexcept;
SHIMAPI const shims::json_object* __stdcall ShimQueryExeConfig(const wchar_t* executable) noexcept;
SHIMAPI const shims::json_object* __stdcall ShimQueryCurrentExeConfig() noexcept;
SHIMAPI const shims::json_value* __stdcall ShimQueryDllConfig(const wchar_t* dll) noexcept;

inline const shims::json_value* ShimQueryCurrentDllConfig()
{
    return ShimQueryDllConfig(shims::current_module_path().filename().c_str());
}

SHIMAPI void __stdcall ShimReportError(const wchar_t* error) noexcept;

}
