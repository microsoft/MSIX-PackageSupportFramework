//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <windows.h>

#include "psf_config.h"
#include "psf_utils.h"

#ifndef PSFAPI
#ifdef PSFRUNTIME_EXPORTS
#define PSFAPI __declspec(dllexport)
#else
#define PSFAPI __declspec(dllimport)
#endif
#endif

using PSFInitializeProc = int (__stdcall *)() noexcept;
using PSFUninitializeProc = int (__stdcall *)() noexcept;

// PsfRuntime exports
// NOTE: Unless stated otherwise, all memory returned is allocated by the PsfRuntime and remains valid so long as the
//       dll is loaded.
extern "C" {

// Abstractions around the DetourAttach/Detach calls so that fixups don't need to link against the detours lib
PSFAPI DWORD __stdcall PSFRegister(_Inout_ void** implFn, _In_ void* fixupFn) noexcept;
PSFAPI DWORD __stdcall PSFUnregister(_Inout_ void** implFn, _In_ void* fixupFn) noexcept;

// Simplifications around the package query API from appmodel.h
// NOTE: These functions are guaranteed to succeed as PsfRuntime will fail to load if they can't be set (e.g. when
//       running outside of a package)
PSFAPI const wchar_t* __stdcall PSFQueryPackageFullName() noexcept;
PSFAPI const wchar_t* __stdcall PSFQueryApplicationUserModelId() noexcept;
PSFAPI const wchar_t* __stdcall PSFQueryApplicationId() noexcept;
PSFAPI const wchar_t* __stdcall PSFQueryPackageRootPath() noexcept;
PSFAPI const wchar_t* __stdcall PSFQueryFinalPackageRootPath() noexcept;

PSFAPI const psf::json_value* __stdcall PSFQueryConfigRoot() noexcept;

PSFAPI const psf::json_object* __stdcall PSFQueryAppLaunchConfig(_In_ const wchar_t* applicationId, bool verbose) noexcept;
PSFAPI const psf::json_object* __stdcall PSFQueryCurrentAppLaunchConfig(bool verbose) noexcept;
PSFAPI const psf::json_object* __stdcall PSFQueryAppMonitorConfig() noexcept;
PSFAPI const psf::json_object* __stdcall PSFQueryStartScriptInfo() noexcept;
PSFAPI const psf::json_object* __stdcall PSFQueryEndScriptInfo() noexcept;

// NOTE: Returns null when no configuration is set for the executable, or if an error occurred
PSFAPI const psf::json_value* __stdcall PSFQueryConfig(const wchar_t* executable, const wchar_t* dll) noexcept;
PSFAPI const psf::json_object* __stdcall PSFQueryExeConfig(const wchar_t* executable) noexcept;
PSFAPI const psf::json_object* __stdcall PSFQueryCurrentExeConfig() noexcept;
PSFAPI const psf::json_value* __stdcall PSFQueryDllConfig(const wchar_t* dll) noexcept;

inline const psf::json_value* PSFQueryCurrentDllConfig()
{
    return PSFQueryDllConfig(psf::current_module_path().filename().c_str());
}

PSFAPI void __stdcall PSFReportError(const wchar_t* error) noexcept;

}
