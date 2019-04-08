//-------------------------------------------------------------------------------------------------------    
// Copyright (C) Microsoft Corporation. All rights reserved.    
// Licensed under the MIT license. See LICENSE file in the project root for full license information.    
//-------------------------------------------------------------------------------------------------------    
    
#include <psf_framework.h>    
    
#include "Config.h"    
#include "FunctionImplementations.h"    
#include "Logging.h"    
#include "PreserveError.h"    
    
void LogKeyPath(HKEY key, const char* msg = "Key")    
{    
    ULONG size;    
    if (auto status = impl::NtQueryKey(key, winternl::KeyNameInformation, nullptr, 0, &size);    
        (status == STATUS_BUFFER_TOO_SMALL) || (status == STATUS_BUFFER_OVERFLOW))    
    {    
        auto buffer = std::make_unique<std::uint8_t[]>(size);    
        if (NT_SUCCESS(impl::NtQueryKey(key, winternl::KeyNameInformation, buffer.get(), size, &size)))    
        {    
            auto info = reinterpret_cast<winternl::PKEY_NAME_INFORMATION>(buffer.get());    
            LogCountedString(msg, info->Name, info->NameLength / 2);    
        }    
    }    
}    
std::string InterpretKeyPath(HKEY key, const char* msg = "Key")    
{    
	std::string sret = "";    
	ULONG size;    
	auto status = impl::NtQueryKey(key, winternl::KeyNameInformation, nullptr, 0, &size);    
	if ((status == STATUS_BUFFER_TOO_SMALL) || (status == STATUS_BUFFER_OVERFLOW))    
	{    
		auto buffer = std::make_unique<std::uint8_t[]>(size);    
		if (NT_SUCCESS(impl::NtQueryKey(key, winternl::KeyNameInformation, buffer.get(), size, &size)))    
		{    
			auto info = reinterpret_cast<winternl::PKEY_NAME_INFORMATION>(buffer.get());    
			sret = InterpretCountedString(msg, info->Name, info->NameLength / 2);    
		}    
		else    
			sret = "InterpretKeyPath failure2";    
	}    
	else if (status == STATUS_INVALID_HANDLE)    
	{    
		if (key == HKEY_LOCAL_MACHINE)    
			sret += msg + InterpretStringA(" HKEY_LOCAL_MACHINE");    
		else if (key == HKEY_CURRENT_USER)    
			sret = msg + InterpretStringA(" HKEY_CURRENT_USER");    
		else if (key == HKEY_CLASSES_ROOT)    
			sret = msg + InterpretStringA(" HKEY_CLASSES_ROOT");    
	}    
	else    
		sret = "InterpretKeyPath failure1" + InterpretAsHex("status",(DWORD)status);    
	return sret;    
}    
    
auto RegCreateKeyImpl = psf::detoured_string_function(&::RegCreateKeyA, &::RegCreateKeyW);    
template <typename CharT>    
LSTATUS __stdcall RegCreateKeyFixup(_In_ HKEY key, _In_opt_ const CharT* subKey, _Out_ PHKEY resultKey)    
{    
	LARGE_INTEGER TickStart, TickEnd;    
	QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = RegCreateKeyImpl(key, subKey, resultKey);    
	QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32(result);    
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))    
    {    
		if (output_method == trace_method::eventlog)    
		{    
			std::string inputs = "";    
			std::string outputs = "";    
			std::string results = "";    
    
			inputs = InterpretKeyPath(key);    
			inputs += " (" + InterpretAsHex("", key) + ")";    
			if (subKey)    
				inputs += "\nSubkey=" + InterpretStringA(subKey);    
    
			results = InterpretReturn(functionResult, result).c_str();    
			if (function_failed(functionResult))    
			{    
				outputs +=  InterpretLastError();    
			}    
			else    
			{    
				outputs +=  InterpretAsHex("Result Key", resultKey);    
			}    
    
			std::ostringstream sout;    
			InterpretCallingModulePart1()    
				sout << InterpretCallingModulePart2()    
				InterpretCallingModulePart3()    
				std::string cm = sout.str();    
    
			Log_ETW_PostMsgOperationA("RegCreateKey", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(),TickStart,TickEnd);    
		}    
		else    
		{    
        Log("RegCreateKey:\n");    
        LogKeyPath(key);    
        if (subKey) LogString("Sub Key", subKey);    
        LogFunctionResult(functionResult);    
        if (function_failed(functionResult))    
        {    
            LogWin32Error(result);    
        }    
        LogCallingModule();    
	}    
    }    
    
    return result;    
}    
DECLARE_STRING_FIXUP(RegCreateKeyImpl, RegCreateKeyFixup);    
    
auto RegCreateKeyExImpl = psf::detoured_string_function(&::RegCreateKeyExA, &::RegCreateKeyExW);    
template <typename CharT>    
LSTATUS __stdcall RegCreateKeyExFixup(    
    _In_ HKEY key,    
    _In_ const CharT* subKey,    
    _Reserved_ DWORD reserved,    
    _In_opt_ CharT* classType,    
    _In_ DWORD options,    
    _In_ REGSAM samDesired,    
    _In_opt_ CONST LPSECURITY_ATTRIBUTES securityAttributes,    
    _Out_ PHKEY resultKey,    
    _Out_opt_ LPDWORD disposition)    
{    
	LARGE_INTEGER TickStart, TickEnd;    
	QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = RegCreateKeyExImpl(key, subKey, reserved, classType, options, samDesired, securityAttributes, resultKey, disposition);    
	QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32(result);    
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))    
    {    
		if (output_method == trace_method::eventlog)    
		{    
			std::string inputs = "";    
			std::string outputs = "";    
			std::string results = "";    
    
			inputs = InterpretKeyPath(key);    
			inputs += " (" + InterpretAsHex("", key) + ")";    
			if (subKey)    
				inputs += "\nSubkey=" + InterpretStringA(subKey);    
			if (classType)    
				inputs += "\n" + InterpretStringA(classType);    
			inputs += "\n" + InterpretRegKeyFlags(options);    
			inputs += "\n" + InterpretRegKeyAccess(samDesired);    
    
			results = InterpretReturn(functionResult, result).c_str();    
			if (function_failed(functionResult))    
			{    
				outputs +=  InterpretLastError();    
			}    
			else    
			{    
				outputs +=  InterpretAsHex("Result Key", resultKey);    
				outputs += "\n" + InterpretRegKeyDisposition(*disposition);    
			}    
    
			std::ostringstream sout;    
			InterpretCallingModulePart1()    
				sout << InterpretCallingModulePart2()    
				InterpretCallingModulePart3()    
				std::string cm = sout.str();    
    
			Log_ETW_PostMsgOperationA("RegCreateKeyEx", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
		}    
		else    
		{    
        Log("RegCreateKeyEx:\n");    
        LogKeyPath(key);    
        LogString("Sub Key", subKey);    
        if (classType) LogString("Class", classType);    
        LogRegKeyFlags(options);    
        LogRegKeyAccess(samDesired);    
        LogFunctionResult(functionResult);    
        if (function_failed(functionResult))    
        {    
            LogWin32Error(result);    
        }    
        else if (disposition)    
        {    
            LogRegKeyDisposition(*disposition);    
        }    
        LogCallingModule();    
	}    
    }    
    
    return result;    
}    
DECLARE_STRING_FIXUP(RegCreateKeyExImpl, RegCreateKeyExFixup);    
    
auto RegOpenKeyImpl = psf::detoured_string_function(&::RegOpenKeyA, &::RegOpenKeyW);    
template <typename CharT>    
LSTATUS __stdcall RegOpenKeyFixup(_In_ HKEY key, _In_opt_ const CharT* subKey, _Out_ PHKEY resultKey)    
{    
	LARGE_INTEGER TickStart, TickEnd;    
	QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = RegOpenKeyImpl(key, subKey, resultKey);    
	QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32(result);    
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))    
    {    
		if (output_method == trace_method::eventlog)    
		{    
			std::string inputs = "";    
			std::string outputs = "";    
			std::string results = "";    
    
			inputs = InterpretKeyPath(key);    
			inputs += " (" + InterpretAsHex("", key) + ")";    
			if (subKey)    
				inputs += "\nSubkey=" + InterpretStringA(subKey);    
    
			results = InterpretReturn(functionResult, result).c_str();    
			if (function_failed(functionResult))    
			{    
				if (::GetLastError() == 0)    
					outputs += "Key Not Found";    
				else    
					outputs +=  InterpretLastError();    
			}    
			else    
			{    
				outputs +=  InterpretAsHex("Result Key", resultKey);    
			}    
    
			std::ostringstream sout;    
			InterpretCallingModulePart1()    
				sout << InterpretCallingModulePart2()    
				InterpretCallingModulePart3()    
				std::string cm = sout.str();    
			    
			Log_ETW_PostMsgOperationA("RegOpenKey", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
		}    
		else    
		{    
        Log("RegOpenKey:\n");    
        LogKeyPath(key);    
        if (subKey) LogString("Sub Key", subKey);    
        LogFunctionResult(functionResult);    
        if (function_failed(functionResult))    
        {    
            LogWin32Error(result);    
        }    
        LogCallingModule();    
	}    
    }    
    
    return result;    
}    
DECLARE_STRING_FIXUP(RegOpenKeyImpl, RegOpenKeyFixup);    
    
auto RegOpenKeyExImpl = psf::detoured_string_function(&::RegOpenKeyExA, &::RegOpenKeyExW);    
template <typename CharT>    
LSTATUS __stdcall RegOpenKeyExFixup(    
    _In_ HKEY key,    
    _In_opt_ const CharT* subKey,    
    _In_opt_ DWORD options,    
    _In_ REGSAM samDesired,    
    _Out_ PHKEY resultKey)    
{    
	LARGE_INTEGER TickStart, TickEnd;    
	QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = RegOpenKeyExImpl(key, subKey, options, samDesired, resultKey);    
	QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32(result);    
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))    
    {    
		if (output_method == trace_method::eventlog)    
		{    
			std::string inputs = "";     
			std::string outputs = "";    
			std::string results = "";    
    
			inputs = InterpretKeyPath(key);      
			inputs += " (" + InterpretAsHex("", key) + ")";    
			if (subKey)    
				inputs += "\nSubkey=" + InterpretStringA(subKey);    
			if (options)    
				inputs += InterpretRegKeyFlags(options);    
			inputs += "\n" + InterpretRegKeyAccess(samDesired);    
    
			results = InterpretReturn(functionResult,result).c_str();    
			if (function_failed(functionResult))    
			{    
				if (::GetLastError() == 0)    
					outputs += "Key Not Found";    
				else    
					outputs +=  InterpretLastError();    
			}    
			else    
			{    
				outputs +=  InterpretAsHex("Result Key", resultKey);    
			}    
    
			std::ostringstream sout;    
			InterpretCallingModulePart1()    
				sout <<  InterpretCallingModulePart2()    
				InterpretCallingModulePart3()    
				std::string cm = sout.str();    
    
			Log_ETW_PostMsgOperationA("RegOpenKeyEx", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
		}    
		else    
		{    
        Log("RegOpenKeyEx:\n");    
        LogKeyPath(key);    
        if (subKey) LogString("Sub Key", subKey);    
        LogRegKeyFlags(options);    
        LogRegKeyAccess(samDesired);    
        LogFunctionResult(functionResult);    
        if (function_failed(functionResult))    
        {    
            LogWin32Error(result);    
        }    
        LogCallingModule();    
	}    
    }    
    
    return result;    
}    
DECLARE_STRING_FIXUP(RegOpenKeyExImpl, RegOpenKeyExFixup);    
    
auto RegGetValueImpl = psf::detoured_string_function(&::RegGetValueA, &::RegGetValueW);    
template <typename CharT>    
LSTATUS __stdcall RegGetValueFixup(    
    _In_ HKEY key,    
    _In_opt_ const CharT* subKey,    
    _In_opt_ const CharT* value,    
    _In_ DWORD flags,    
    _Out_opt_ LPDWORD type,    
    _Out_writes_bytes_to_opt_(*dataSize, *data) PVOID data,    
    _Inout_opt_ LPDWORD dataSize)    
{    
	DWORD lclType = 0;    
	LARGE_INTEGER TickStart, TickEnd;    
	QueryPerformanceCounter(&TickStart);    		auto entry = LogFunctionEntry();    
    
    auto result = RegGetValueImpl(key, subKey, value, flags, type, data, dataSize);    
	QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32(result);    
	if (type)    
		*type = lclType;    
    
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))    
    {    
		if (output_method == trace_method::eventlog)    
		{    
			std::string inputs = "";    
			std::string outputs = "";    
			std::string results = "";    
    
			inputs = InterpretKeyPath(key);    
			inputs += " (" + InterpretAsHex("", key) + ")";    
			if (subKey)    
				inputs += "\nSubkey=" + InterpretStringA(subKey);    
			if (value)    
				inputs += "\nValue=" + InterpretStringA(value);    
			inputs += "\n" + InterpretRegKeyQueryFlags(flags);    
    
			results = InterpretReturn(functionResult, result).c_str();    
			if (function_failed(functionResult))    
			{    
				outputs += InterpretLastError();    
			}    
			else     
			{    
				outputs += InterpretRegKeyType(lclType);    
				if (data && dataSize)    
				{    
					outputs += "\n" + InterpretRegValueA<CharT>(lclType, data, *dataSize);    
				}    
			}    
    
			std::ostringstream sout;    
			InterpretCallingModulePart1()    
				sout <<InterpretCallingModulePart2()    
				InterpretCallingModulePart3()    
				std::string cm = sout.str();    
    
			Log_ETW_PostMsgOperationA("RegGetValue", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
		}    
		else    
		{    
        Log("RegGetValue:\n");    
        LogKeyPath(key);    
        if (subKey) LogString("Sub Key", subKey);    
        if (value) LogString("Value", value);    
        LogRegKeyQueryFlags(flags);    
        LogFunctionResult(functionResult);    
        if (function_failed(functionResult))    
        {    
            LogWin32Error(result);    
        }    
        else if (type)    
        {    
            LogRegKeyType(*type);    
            if (data && dataSize) LogRegValue<CharT>(*type, data, *dataSize);    
        }    
        LogCallingModule();    
	}    
    }    
    
    return result;    
}    
DECLARE_STRING_FIXUP(RegGetValueImpl, RegGetValueFixup);    
    
auto RegQueryValueImpl = psf::detoured_string_function(&::RegQueryValueA, &::RegQueryValueW);    
template <typename CharT>    
LSTATUS __stdcall RegQueryValueFixup(    
    _In_ HKEY key,    
    _In_opt_ const CharT* subKey,    
    _Out_writes_bytes_to_opt_(*dataSize, *dataSize) CharT* data,    
    _Inout_opt_ PLONG dataSize)    
{    
	LARGE_INTEGER TickStart, TickEnd;    
	QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = RegQueryValueImpl(key, subKey, data, dataSize);    
	QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32(result);    
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))    
    {    
		if (output_method == trace_method::eventlog)    
		{    
			std::string inputs = "";    
			std::string outputs = "";    
			std::string results = "";    
    
			inputs = InterpretKeyPath(key);    
			inputs += " (" + InterpretAsHex("", key) + ")";    
			if (subKey)    
				inputs += "\nSubkey=" + InterpretStringA(subKey);    
    
			results = InterpretReturn(functionResult,result).c_str();    
			if (function_failed(functionResult))    
			{    
				outputs += InterpretLastError();    
			}    
			else if (data && dataSize)    
				outputs +=  InterpretCountedString("Data", data, *dataSize);    
    
			std::ostringstream sout;    
			InterpretCallingModulePart1()    
				sout <<  InterpretCallingModulePart2()    
				InterpretCallingModulePart3()    
				std::string cm = sout.str();    
    
			Log_ETW_PostMsgOperationA("RegQueryValue", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
		}    
		else    
		{    
        Log("RegQueryValue:\n");    
        LogKeyPath(key);    
        if (subKey) LogString("Sub Key", subKey);    
        LogFunctionResult(functionResult);    
        if (function_failed(functionResult))    
        {    
            LogWin32Error(result);    
        }    
        else if (data && dataSize)    
        {    
            LogCountedString("Data", data, *dataSize / sizeof(CharT));    
        }    
        LogCallingModule();    
	}    
    }    
    
    return result;    
}    
DECLARE_STRING_FIXUP(RegQueryValueImpl, RegQueryValueFixup);    
    
auto RegQueryValueExImpl = psf::detoured_string_function(&::RegQueryValueExA, &::RegQueryValueExW);    
template <typename CharT>    
LSTATUS __stdcall RegQueryValueExFixup(    
    _In_ HKEY key,    
    _In_opt_ const CharT* valueName,    
    _Reserved_ LPDWORD reserved,    
    _Out_opt_ LPDWORD type,    
    _Out_writes_bytes_to_opt_(*dataSize, *dataSize) LPBYTE data,    
    _When_(data == NULL, _Out_opt_) _When_(data != NULL, _Inout_opt_) LPDWORD dataSize)    
{    
	LARGE_INTEGER TickStart, TickEnd;    
	QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = RegQueryValueExImpl(key, valueName, reserved, type, data, dataSize);    
	QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32(result);    
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))    
    {    
		if (output_method == trace_method::eventlog)    
		{    
			std::string inputs = "";    
			std::string outputs = "";    
			std::string results = "";    
    
			inputs = InterpretKeyPath(key);    
			inputs += " (" + InterpretAsHex("", key) + ")";    
			if (valueName)    
				inputs += "\nValue Name=" + InterpretStringA(valueName);    
    
			results = InterpretReturn(functionResult, result).c_str();    
			if (function_failed(functionResult))    
			{    
				outputs +=  InterpretLastError();    
			}    
			else if (type)    
			{    
				outputs +=  InterpretRegKeyType(*type);    
				if (data && dataSize)    
					outputs += "\n" + InterpretRegValueA<CharT>(*type, data, *dataSize);    
			}    
    
			std::ostringstream sout;    
			InterpretCallingModulePart1()    
				sout << InterpretCallingModulePart2()    
				InterpretCallingModulePart3()    
				std::string cm = sout.str();    
    
			Log_ETW_PostMsgOperationA("RegQueryValueEx", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
		}    
		else    
		{    
        Log("RegQueryValueEx:\n");    
        LogKeyPath(key);    
        if (valueName) LogString("Value Name", valueName);    
        LogFunctionResult(functionResult);    
        if (function_failed(functionResult))    
        {    
            LogWin32Error(result);    
        }    
        else if (type)    
        {    
            LogRegKeyType(*type);    
            if (data && dataSize) LogRegValue<CharT>(*type, data, *dataSize);    
        }    
        LogCallingModule();    
	}    
    }    
    
    return result;    
}    
DECLARE_STRING_FIXUP(RegQueryValueExImpl, RegQueryValueExFixup);    
    
auto RegSetKeyValueImpl = psf::detoured_string_function(&::RegSetKeyValueA, &::RegSetKeyValueW);    
template <typename CharT>    
LSTATUS __stdcall RegSetKeyValueFixup(    
    _In_ HKEY key,    
    _In_opt_ const CharT* subKey,    
    _In_opt_ const CharT* valueName,    
    _In_ DWORD type,    
    _In_reads_bytes_opt_(dataSize) LPCVOID data,    
    _In_ DWORD dataSize)    
{    
	LARGE_INTEGER TickStart, TickEnd;    
	QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = RegSetKeyValueImpl(key, subKey, valueName, type, data, dataSize);    
	QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32(result);    
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))    
    {    
		if (output_method == trace_method::eventlog)    
		{    
			std::string inputs = "";    
			std::string outputs = "";    
			std::string results = "";    
    
			inputs = InterpretKeyPath(key);    
			inputs += " (" + InterpretAsHex("", key) + ")";    
			if (subKey)    
				inputs += "\nSubkey=" + InterpretStringA(subKey);    
			inputs += "\n" + InterpretRegKeyType(type);    
			if (valueName)    
				inputs += "\nValue Name" + InterpretStringA(valueName);    
			inputs += "\n" + InterpretRegKeyType(type);    
			if (data )    
				inputs += "\n" + InterpretRegValueA<CharT>(type, data, dataSize);    
    
			results = InterpretReturn(functionResult, result).c_str();    
			if (function_failed(functionResult))    
			{    
				outputs +=  InterpretLastError();    
			}    
    
			std::ostringstream sout;    
			InterpretCallingModulePart1()    
				sout << InterpretCallingModulePart2()    
				InterpretCallingModulePart3()    
				std::string cm = sout.str();    
    
			Log_ETW_PostMsgOperationA("RegSetKeyValue", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
		}    
		else    
		{    
        Log("RegSetKeyValue:\n");    
        LogKeyPath(key);    
        if (subKey) LogString("Sub Key", subKey);    
        if (valueName) LogString("Value Name", valueName);    
        LogRegKeyType(type);    
        if (data) LogRegValue<CharT>(type, data, dataSize);    
        LogFunctionResult(functionResult);    
        if (function_failed(functionResult))    
        {    
            LogWin32Error(result);    
        }    
        LogCallingModule();    
	}    
    }    
    
    return result;    
}    
DECLARE_STRING_FIXUP(RegSetKeyValueImpl, RegSetKeyValueFixup);    
    
auto RegSetValueImpl = psf::detoured_string_function(&::RegSetValueA, &::RegSetValueW);    
template <typename CharT>    
LSTATUS __stdcall RegSetValueFixup(    
    _In_ HKEY key,    
    _In_opt_ const CharT* subKey,    
    _In_ DWORD type,    
    _In_reads_bytes_opt_(dataSize) const CharT* data,    
    _In_ DWORD dataSize)    
{    
	LARGE_INTEGER TickStart, TickEnd;    
	QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = RegSetValueImpl(key, subKey, type, data, dataSize);    
	QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32(result);    
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))    
    {    
		if (output_method == trace_method::eventlog)    
		{    
			std::string inputs = "";    
			std::string outputs = "";    
			std::string results = "";    
    
			inputs = InterpretKeyPath(key);    
			inputs += " (" + InterpretAsHex("", key) + ")";    
			if (subKey)    
				inputs += "\nSubkey=" + InterpretStringA(subKey);    
			inputs += "\n" + InterpretRegKeyType(type);    
			if (data)    
				inputs += "\n" + InterpretRegValueA<CharT>(type, data, dataSize);    
    
			results = InterpretReturn(functionResult, result).c_str();    
			if (function_failed(functionResult))    
			{    
				outputs +=  InterpretLastError();    
			}    
    
			std::ostringstream sout;    
			InterpretCallingModulePart1()    
				sout << InterpretCallingModulePart2()    
				InterpretCallingModulePart3()    
				std::string cm = sout.str();    
    
			Log_ETW_PostMsgOperationA("RegSetValue", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
		}    
		else    
		{    
        Log("RegSetValue:\n");    
        LogKeyPath(key);    
        if (subKey) LogString("Sub Key", subKey);    
        LogRegKeyType(type); // NOTE: _Must_ be REG_SZ    
        if (data) LogString("Data", data);    
        LogFunctionResult(functionResult);    
        if (function_failed(functionResult))    
        {    
            LogWin32Error(result);    
        }    
        LogCallingModule();    
	}    
    }    
    
    return result;    
}    
DECLARE_STRING_FIXUP(RegSetValueImpl, RegSetValueFixup);    
    
auto RegSetValueExImpl = psf::detoured_string_function(&::RegSetValueExA, &::RegSetValueExW);    
template <typename CharT>    
LSTATUS __stdcall RegSetValueExFixup(    
    _In_ HKEY key,    
    _In_opt_ const CharT* valueName,    
    _Reserved_ DWORD reserved,    
    _In_ DWORD type,    
    _In_reads_bytes_opt_(dataSize) CONST BYTE* data,    
    _In_ DWORD dataSize)    
{    
	LARGE_INTEGER TickStart, TickEnd;    
	QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = RegSetValueExImpl(key, valueName, reserved, type, data, dataSize);    
	QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32(result);    
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))    
    {    
		if (output_method == trace_method::eventlog)    
		{    
			std::string inputs = "";    
			std::string outputs = "";    
			std::string results = "";    
    
			inputs = InterpretKeyPath(key);    
			inputs += " (" + InterpretAsHex("", key) + ")";    
			if (valueName)    
				inputs += "\nValue Name=" + InterpretStringA(valueName);    
			inputs += "\n" + InterpretRegKeyType(type);    
			if (data)    
				inputs += "\n" + InterpretRegValueA<CharT>(type, data, dataSize);    
    
			results = InterpretReturn(functionResult, result).c_str();    
			if (function_failed(functionResult))    
			{    
				outputs +=  InterpretLastError();    
			}    
    
			std::ostringstream sout;    
			InterpretCallingModulePart1()    
				sout << InterpretCallingModulePart2()    
				InterpretCallingModulePart3()    
				std::string cm = sout.str();    
    
			Log_ETW_PostMsgOperationA("RegSetValueEx", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
		}    
		else    
		{    
        Log("RegSetValueEx:\n");    
        LogKeyPath(key);    
        if (valueName) LogString("Value Name", valueName);    
        LogRegKeyType(type);    
        if (data) LogRegValue<CharT>(type, data, dataSize);    
        LogFunctionResult(functionResult);    
        if (function_failed(functionResult))    
        {    
            LogWin32Error(result);    
        }    
        LogCallingModule();    
	}    
    }    
    
    return result;    
}    
DECLARE_STRING_FIXUP(RegSetValueExImpl, RegSetValueExFixup);    
    
auto RegDeleteKeyImpl = psf::detoured_string_function(&::RegDeleteKeyA, &::RegDeleteKeyW);    
template <typename CharT>    
LSTATUS __stdcall RegDeleteKeyFixup(_In_ HKEY key, _In_ const CharT* subKey)    
{    
	LARGE_INTEGER TickStart, TickEnd;    
	QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = RegDeleteKeyImpl(key, subKey);    
	QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32(result);    
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))    
    {    
		if (output_method == trace_method::eventlog)    
		{    
			std::string inputs = "";    
			std::string outputs = "";    
			std::string results = "";    
    
			inputs = InterpretKeyPath(key);    
			inputs += " (" + InterpretAsHex("", key) + ")";    
			if (subKey)    
				inputs += "\nSubkey=" + InterpretStringA(subKey);    
    
			results = InterpretReturn(functionResult, result).c_str();    
			if (function_failed(functionResult))    
			{    
				outputs +=  InterpretLastError();    
			}    
    
			std::ostringstream sout;    
			InterpretCallingModulePart1()    
				sout << InterpretCallingModulePart2()    
				InterpretCallingModulePart3()    
				std::string cm = sout.str();    
    
			Log_ETW_PostMsgOperationA("RegDeleteKey", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
		}    
		else    
		{    
        Log("RegDeleteKey:\n");    
        LogKeyPath(key);    
        LogString("Sub Key", subKey);    
        LogFunctionResult(functionResult);    
        if (function_failed(functionResult))    
        {    
            LogWin32Error(result);    
        }    
        LogCallingModule();    
	}    
    }    
    
    return result;    
}    
DECLARE_STRING_FIXUP(RegDeleteKeyImpl, RegDeleteKeyFixup);    
    
auto RegDeleteKeyExImpl = psf::detoured_string_function(&::RegDeleteKeyExA, &::RegDeleteKeyExW);    
template <typename CharT>    
LSTATUS __stdcall RegDeleteKeyExFixup(    
    _In_ HKEY key,    
    _In_ const CharT* subKey,    
    _In_ REGSAM samDesired,    
    _Reserved_ DWORD reserved)    
{    
	LARGE_INTEGER TickStart, TickEnd;    
	QueryPerformanceCounter(&TickStart);    
	auto entry = LogFunctionEntry();    
    auto result = RegDeleteKeyExImpl(key, subKey, samDesired, reserved);    
	QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32(result);    
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))    
    {    
		if (output_method == trace_method::eventlog)    
		{    
			std::string inputs = "";    
			std::string outputs = "";    
			std::string results = "";    
    
			inputs = InterpretKeyPath(key);    
			inputs += " (" + InterpretAsHex("", key) + ")";    
			if (subKey)    
				inputs += "\nSubkey=" + InterpretStringA(subKey);    
			inputs += "\n" + InterpretRegKeyAccess(samDesired);    
    
			results = InterpretReturn(functionResult, result).c_str();    
			if (function_failed(functionResult))    
			{    
				outputs += InterpretLastError();    
			}    
    
			std::ostringstream sout;    
			InterpretCallingModulePart1()    
				sout << InterpretCallingModulePart2()    
				InterpretCallingModulePart3()    
				std::string cm = sout.str();    
    
			Log_ETW_PostMsgOperationA("RegDeleteKeyEx", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
		}    
		else    
		{    
        Log("RegDeleteKeyEx:\n");    
        LogKeyPath(key);    
        LogString("Sub Key", subKey);    
        LogRegKeyAccess(samDesired);    
        LogFunctionResult(functionResult);    
        if (function_failed(functionResult))    
        {    
            LogWin32Error(result);    
        }    
        LogCallingModule();    
	}    
    }    
    
    return result;    
}    
DECLARE_STRING_FIXUP(RegDeleteKeyExImpl, RegDeleteKeyExFixup);    
    
auto RegDeleteKeyValueImpl = psf::detoured_string_function(&::RegDeleteKeyValueA, &::RegDeleteKeyValueW);    
template <typename CharT>    
LSTATUS __stdcall RegDeleteKeyValueFixup(_In_ HKEY key, _In_opt_ const CharT* subKey, _In_opt_ const CharT* valueName)    
{    
	LARGE_INTEGER TickStart, TickEnd;    
	QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = RegDeleteKeyValueImpl(key, subKey, valueName);    
	QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32(result);    
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))    
    {    
		if (output_method == trace_method::eventlog)    
		{    
			std::string inputs = "";    
			std::string outputs = "";    
			std::string results = "";    
    
			inputs = InterpretKeyPath(key);    
			inputs += " (" + InterpretAsHex("", key) + ")";    
			if (subKey)    
				inputs += "\nSubkey=" + InterpretStringA(subKey);    
			if (valueName)    
				inputs += "\nValue Name=" + InterpretStringA(valueName);    
    
			results = InterpretReturn(functionResult, result).c_str();    
			if (function_failed(functionResult))    
			{    
				outputs += InterpretLastError();    
			}    
    
			std::ostringstream sout;    
			InterpretCallingModulePart1()    
				sout << InterpretCallingModulePart2()    
				InterpretCallingModulePart3()    
				std::string cm = sout.str();    
    
			Log_ETW_PostMsgOperationA("RegDeleteKeyValue", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
		}    
		else    
		{    
        Log("RegDeleteKeyValue:\n");    
        LogKeyPath(key);    
        if (subKey) LogString("Sub Key", subKey);    
        if (valueName) LogString("Value Name", valueName);    
        LogFunctionResult(functionResult);    
        if (function_failed(functionResult))    
        {    
            LogWin32Error(result);    
        }    
        LogCallingModule();    
	}    
    }    
    
    return result;    
}    
DECLARE_STRING_FIXUP(RegDeleteKeyValueImpl, RegDeleteKeyValueFixup);    
    
auto RegDeleteValueImpl = psf::detoured_string_function(&::RegDeleteValueA, &::RegDeleteValueW);    
template <typename CharT>    
LSTATUS __stdcall RegDeleteValueFixup(_In_ HKEY key, _In_opt_ const CharT* valueName)    
{    
	LARGE_INTEGER TickStart, TickEnd;    
	QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = RegDeleteValueImpl(key, valueName);    
	QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32(result);    
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))    
    {    
		if (output_method == trace_method::eventlog)    
		{    
			std::string inputs = "";    
			std::string outputs = "";    
			std::string results = "";    
    
			inputs = InterpretKeyPath(key);    
			inputs += " (" + InterpretAsHex("", key) + ")";    
			if (valueName)    
				inputs += "\nValue Name=" + InterpretStringA(valueName);    
    
			results = InterpretReturn(functionResult, result).c_str();    
			if (function_failed(functionResult))    
			{    
				outputs +=  InterpretLastError();    
			}    
    
			std::ostringstream sout;    
			InterpretCallingModulePart1()    
				sout << InterpretCallingModulePart2()    
				InterpretCallingModulePart3()    
				std::string cm = sout.str();    
    
			Log_ETW_PostMsgOperationA("RegDeleteValue", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
		}    
		else    
		{    
        Log("RegDeleteValue:\n");    
        LogKeyPath(key);    
        if (valueName) LogString("Value Name", valueName);    
        LogFunctionResult(functionResult);    
        if (function_failed(functionResult))    
        {    
            LogWin32Error(result);    
        }    
        LogCallingModule();    
	}    
    }    
    
    return result;    
}    
DECLARE_STRING_FIXUP(RegDeleteValueImpl, RegDeleteValueFixup);    
    
auto RegDeleteTreeImpl = psf::detoured_string_function(&::RegDeleteTreeA, &::RegDeleteTreeW);    
template <typename CharT>    
LSTATUS __stdcall RegDeleteTreeFixup(_In_ HKEY key, _In_opt_ const CharT* subKey)    
{    
	LARGE_INTEGER TickStart, TickEnd;    
	QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = RegDeleteTreeImpl(key, subKey);    
	QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32(result);    
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))    
    {    
		if (output_method == trace_method::eventlog)    
		{    
			std::string inputs = "";    
			std::string outputs = "";    
			std::string results = "";    
    
			inputs = InterpretKeyPath(key);    
			inputs += " (" + InterpretAsHex("", key) + ")";    
			if (subKey)    
				inputs += "\nSubkey=" + InterpretStringA(subKey);    
    
			results = InterpretReturn(functionResult, result).c_str();    
			if (function_failed(functionResult))    
			{    
				outputs += InterpretLastError();    
			}    
    
			std::ostringstream sout;    
			InterpretCallingModulePart1()    
				sout << InterpretCallingModulePart2()    
				InterpretCallingModulePart3()    
				std::string cm = sout.str();    
    
			Log_ETW_PostMsgOperationA("RegDeleteTree", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
		}    
		else    
		{    
        Log("RegDeleteTree:\n");    
        LogKeyPath(key);    
        if (subKey) LogString("Sub Key", subKey);    
        LogFunctionResult(functionResult);    
        if (function_failed(functionResult))    
        {    
            LogWin32Error(result);    
        }    
        LogCallingModule();    
	}    
    }    
    
    return result;    
}    
DECLARE_STRING_FIXUP(RegDeleteTreeImpl, RegDeleteTreeFixup);    
    
auto RegCopyTreeImpl = psf::detoured_string_function(&::RegCopyTreeA, &::RegCopyTreeW);    
template <typename CharT>    
LSTATUS __stdcall RegCopyTreeFixup(_In_ HKEY keySrc, _In_opt_ const CharT* subKey, _In_ HKEY keyDest)    
{    
	LARGE_INTEGER TickStart, TickEnd;    
	QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = RegCopyTreeImpl(keySrc, subKey, keyDest);    
	QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32(result);    
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))    
    {    
		if (output_method == trace_method::eventlog)    
		{    
			std::string inputs = "";    
			std::string outputs = "";    
			std::string results = "";    
    
			inputs = InterpretKeyPath(keySrc);    
			inputs += " (" + InterpretAsHex("", keySrc) + ")";    
			if (subKey)    
				inputs += "\nSubkey=" + InterpretStringA(subKey);    
			inputs += "\n" + InterpretKeyPath(keyDest, "Destination Key");  InterpretAsHex("Destination Key", keyDest);    
			inputs += "\n" + InterpretAsHex("Destination Key", keyDest);    
    
			results = InterpretReturn(functionResult, result).c_str();    
			if (function_failed(functionResult))    
			{    
				outputs += InterpretLastError();    
			}    
    
			std::ostringstream sout;    
			InterpretCallingModulePart1()    
				sout << InterpretCallingModulePart2()    
				InterpretCallingModulePart3()    
				std::string cm = sout.str();    
    
			Log_ETW_PostMsgOperationA("RegCopyTree", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
		}    
		else    
		{    
        Log("RegCopyTree:\n");    
        LogKeyPath(keySrc, "Source");    
        if (subKey) LogString("Sub Key", subKey);    
        LogKeyPath(keyDest, "Dest");    
        LogFunctionResult(functionResult);    
        if (function_failed(functionResult))    
        {    
            LogWin32Error(result);    
        }    
        LogCallingModule();    
	}    
    }    
    
    return result;    
}    
DECLARE_STRING_FIXUP(RegCopyTreeImpl, RegCopyTreeFixup);    
    
auto RegEnumKeyImpl = psf::detoured_string_function(&::RegEnumKeyA, &::RegEnumKeyW);    
template <typename CharT>    
LSTATUS __stdcall RegEnumKeyFixup(    
    _In_ HKEY key,    
    _In_ DWORD index,    
    _Out_writes_opt_(nameLength) CharT* name,    
    _In_ DWORD nameLength)    
{    
	LARGE_INTEGER TickStart, TickEnd;    
	QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = RegEnumKeyImpl(key, index, name, nameLength);    
	QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32(result);    
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))    
    {    
		if (output_method == trace_method::eventlog)    
		{    
			std::string inputs = "";    
			std::string outputs = "";    
			std::string results = "";    
    
			inputs = InterpretKeyPath(key);    
			inputs += " (" + InterpretAsHex("", key) + ")";    
			inputs += "\n" + InterpretAsHex("Index", index);    
    
			results = InterpretReturn(functionResult, result).c_str();    
			if (function_failed(functionResult))    
			{    
				outputs +=  InterpretLastError();    
			}    
			else if (name)    
			{    
				outputs += "\n Name=" + InterpretStringA(name);    
			}    
    
			std::ostringstream sout;    
			InterpretCallingModulePart1()    
				sout << InterpretCallingModulePart2()    
				InterpretCallingModulePart3()    
				std::string cm = sout.str();    
    
			Log_ETW_PostMsgOperationA("RegEnumKey", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
		}    
		else    
		{    
        Log("RegEnumKey:\n");    
        LogKeyPath(key);    
        Log("\tIndex=%d\n", index);    
        LogFunctionResult(functionResult);    
        if (function_failed(functionResult))    
        {    
            LogWin32Error(result);    
        }    
        else if (name)    
        {    
            LogString("Name", name);    
        }    
        LogCallingModule();    
	}    
    }    
    
    return result;    
}    
DECLARE_STRING_FIXUP(RegEnumKeyImpl, RegEnumKeyFixup);    
    
auto RegEnumKeyExImpl = psf::detoured_string_function(&::RegEnumKeyExA, &::RegEnumKeyExW);    
template <typename CharT>    
LSTATUS __stdcall RegEnumKeyExFixup(    
    _In_ HKEY key,    
    _In_ DWORD index,    
    _Out_writes_to_opt_(*nameLength, *nameLength + 1) CharT* name,    
    _Inout_ LPDWORD nameLength,    
    _Reserved_ LPDWORD reserved,    
    _Out_writes_to_opt_(*classNameLength, *classNameLength + 1) CharT* className,    
    _Inout_opt_ LPDWORD classNameLength,    
    _Out_opt_ PFILETIME lastWriteTime)    
{    
	LARGE_INTEGER TickStart, TickEnd;    
	QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = RegEnumKeyExImpl(key, index, name, nameLength, reserved, className, classNameLength, lastWriteTime);    
	QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32(result);    
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))    
    {    
		if (output_method == trace_method::eventlog)    
		{    
			std::string inputs = "";    
			std::string outputs = "";    
			std::string results = "";    
    
			inputs = InterpretKeyPath(key);    
			inputs += " (" + InterpretAsHex("", key) + ")";    
			inputs += "\n" + InterpretAsHex("Index", index);    
    
			results = InterpretReturn(functionResult, result).c_str();    
			if (function_failed(functionResult))    
			{    
				outputs += InterpretLastError();    
			}    
			else     
			{    
				if (name)    
					outputs += "\n Name=" + InterpretStringA(name);    
				if (className && classNameLength)    
					outputs += "\n" + InterpretCountedString("Class", className, *classNameLength);    
			}    
    
			std::ostringstream sout;    
			InterpretCallingModulePart1()    
				sout << InterpretCallingModulePart2()    
				InterpretCallingModulePart3()    
				std::string cm = sout.str();    
    
			Log_ETW_PostMsgOperationA("RegEnumKeyEx", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
		}    
		else    
		{    
        Log("RegEnumKeyEx:\n");    
        LogKeyPath(key);    
        Log("\tIndex=%d\n", index);    
        LogFunctionResult(functionResult);    
        if (function_failed(functionResult))    
        {    
            LogWin32Error(result);    
        }    
        else    
        {    
            if (name) LogCountedString("Name", name, *nameLength);    
            if (className && classNameLength) LogCountedString("Class", className, *classNameLength);    
        }    
        LogCallingModule();    
	}    
    }    
    
    return result;    
}    
DECLARE_STRING_FIXUP(RegEnumKeyExImpl, RegEnumKeyExFixup);    
    
auto RegEnumValueImpl = psf::detoured_string_function(&::RegEnumValueA, &::RegEnumValueW);    
template <typename CharT>    
LSTATUS __stdcall RegEnumValueFixup(    
    _In_ HKEY key,    
    _In_ DWORD index,    
    _Out_writes_to_opt_(*valueNameLength, *valueNameLength + 1) CharT* valueName,    
    _Inout_ LPDWORD valueNameLength,    
    _Reserved_ LPDWORD reserved,    
    _Out_opt_ LPDWORD type,    
    _Out_writes_bytes_to_opt_(*dataSize, *dataSize) __out_data_source(REGISTRY) LPBYTE data,    
    _Inout_opt_ LPDWORD dataSize)    
{    
	LARGE_INTEGER TickStart, TickEnd;    
	QueryPerformanceCounter(&TickStart);    
    auto entry = LogFunctionEntry();    
    auto result = RegEnumValueImpl(key, index, valueName, valueNameLength, reserved, type, data, dataSize);    
	QueryPerformanceCounter(&TickEnd);    
    
    auto functionResult = from_win32(result);    
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))    
    {    
		if (output_method == trace_method::eventlog)    
		{    
			std::string inputs = "";    
			std::string outputs = "";    
			std::string results = "";    
    
			inputs = InterpretKeyPath(key);    
			inputs += " (" + InterpretAsHex("", key) + ")";    
    
			std::ostringstream sout1;    
			sout1 << "\nIndex 0x" << std::uppercase << std::hex;    
			inputs += sout1.str();    
    
			results = InterpretReturn(functionResult, result).c_str();    
			if (function_failed(functionResult))    
			{    
				outputs +=  InterpretLastError();    
			}    
			else    
			{    
				if (valueName)    
					outputs += InterpretCountedString("Value Name", valueName, *valueNameLength) + "\n";    
				if (type)    
					outputs +=  InterpretRegKeyType(*type) + "\n";    
				if (type && data && dataSize)    
					outputs += "Value=" + InterpretRegValueA<CharT>(*type, data, *dataSize);    
			}    
    
			std::ostringstream sout;    
			InterpretCallingModulePart1()    
				sout << InterpretCallingModulePart2()    
				InterpretCallingModulePart3()    
				std::string cm = sout.str();    
    
			Log_ETW_PostMsgOperationA("RegEnumValue", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);    
		}    
		else    
		{    
        Log("RegEnumValue:\n");    
        LogKeyPath(key);    
        Log("\tIndex=%d\n", index);    
        LogFunctionResult(functionResult);    
        if (function_failed(functionResult))    
        {    
            LogWin32Error(result);    
        }    
        else    
        {    
            if (valueName) LogCountedString("Value Name", valueName, *valueNameLength);    
            if (type) LogRegKeyType(*type);    
            if (type && data && dataSize) LogRegValue<CharT>(*type, data, *dataSize);    
        }    
        LogCallingModule();    
	}    
    }    
    
    return result;    
}    
DECLARE_STRING_FIXUP(RegEnumValueImpl, RegEnumValueFixup);    
    
// NOTE: The following is a list of functions taken from https://msdn.microsoft.com/en-us/library/windows/desktop/ms724875(v=vs.85).aspx    
//       that are _not_ present above. This is just a convenient collection of what's missing; it is not a collection of    
//       future work.    
//    
// The following deal with key/value strings:    
//      RegCreateKeyTransacted    
//      RegDeleteKeyTransacted    
//      RegLoadKey                  -- NOTE: File path    
//      RegLoadMUIString    
//      RegOpenKeyTransacted    
//      RegQueryMultipleValues    
//      RegReplaceKey               -- NOTE: File paths    
//      RegUnLoadKey    
//    
// The following exclusively deal with HKEYs    
//      RegCloseKey    
//      RegConnectRegistry    
//      RegDisableReflectionKey    
//      RegEnableReflectionKey    
//      RegFlushKey    
//      RegGetKeySecurity    
//      RegLoadAppKey               -- NOTE: File path    
//      RegNotifyChangeKeyValue    
//      RegOpenCurrentUser    
//      RegOpenUserClassesRoot    
//      RegOverridePredefKey    
//      RegQueryInfoKey    
//      RegQueryReflectionKey    
//      RegRestoreKey               -- NOTE: File path    
//      RegSaveKey                  -- NOTE: File path    
//      RegSaveKeyEx                -- NOTE: File path    
//      RegSetKeySecurity
