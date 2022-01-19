//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

// Must add psf_logging.cpp from the CommonSrc folder into the project.

#include <windows.h>




void Log(const char* fmt, ...);


void Log(const wchar_t* fmt, ...);

void LogString(const char* name, const char* value);

void LogString(const char* name, const wchar_t* value);

void LogString(const wchar_t* name, const char* value);

void LogString(const wchar_t* name, const wchar_t* value);


void LogString(DWORD inst, const char* name, const char* value);

void LogString(DWORD inst, const char* name, const wchar_t* value);

void LogStringAA(DWORD inst, const char* name, const char* value);
void LogStringAW(DWORD inst, const char* name, const wchar_t* value);

void LogString(DWORD inst, const wchar_t* name, const char* value);

void LogString(DWORD inst, const wchar_t* name, const wchar_t* value);

void LogStringWA(DWORD inst, const wchar_t* name, const char* value);
void LogStringWW(DWORD inst, const wchar_t* name, const wchar_t* value);

void LogCountedStringW(const char* name, const wchar_t* value, std::size_t length);

void Loghexdump(void* pAddressIn, long  lSize);
