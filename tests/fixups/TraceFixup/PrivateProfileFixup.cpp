//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan, TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <psf_framework.h>

#include "Config.h"
#include "Logging.h"
#include "PreserveError.h"

#pragma comment(lib, "Lz32.lib")

using namespace std::literals;


auto GetPrivateProfileIntImpl = psf::detoured_string_function(&::GetPrivateProfileIntA, &::GetPrivateProfileIntW);
template <typename CharT>
UINT __stdcall GetPrivateProfileIntFixup(
    _In_opt_ const CharT* sectionName,
    _In_opt_ const CharT* key,
    _In_opt_ const INT nDefault,
    _In_opt_ const CharT* fileName) noexcept
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();
    auto result = GetPrivateProfileIntImpl(sectionName, key, nDefault, fileName);
    QueryPerformanceCounter(&TickEnd);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(true);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
    {
        if (output_method == trace_method::eventlog)
        {
            std::string inputs = "";
            std::string outputs = "";
            std::string results = "";
            try
            {
                inputs = "Path=" + InterpretStringA(fileName);
                inputs += "\nSection=" + InterpretStringA(sectionName);
                inputs += "\nKey=" + InterpretStringA(key);
                inputs += "\n" + InterpretAsHex("Default", nDefault);

                results = InterpretAsHex("Return", result);

                std::ostringstream sout;
                InterpretCallingModulePart1()
                    sout << InterpretCallingModulePart2()
                    InterpretCallingModulePart3()
                    std::string cm = sout.str();

                Log_ETW_PostMsgOperationA("GetPrivateProfileInt", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);
            }
            catch (...)
            {
                Log("GetPrivateProfileInt event logging failure");
            }
        }
        else
        {
            try
            {
                Log("GetPrivateProfileInt:\n");
                LogString("Path", fileName);
                LogString("Section", sectionName);
                LogString("Key", key);
                LogString("Returned",InterpretAsHex("Result", result).c_str());                
                LogCallingModule();
            }
            catch (...)
            {
                Log("GetPrivateProfileInt logging failure");
            }
        }
    }
    return result;
}    
DECLARE_STRING_FIXUP(GetPrivateProfileIntImpl, GetPrivateProfileIntFixup);



auto GetPrivateProfileSectionImpl = psf::detoured_string_function(&::GetPrivateProfileSectionA, &::GetPrivateProfileSectionW);
template <typename CharT>
DWORD __stdcall GetPrivateProfileSectionFixup(
    _In_opt_ const CharT* appName,
    _Out_writes_to_opt_(stringSize, return +1) CharT* string,
    _In_ DWORD stringLength,
    _In_opt_ const CharT* fileName) noexcept
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();
    auto result = GetPrivateProfileSectionImpl(appName, string, stringLength, fileName);
    QueryPerformanceCounter(&TickEnd);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(true);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
    {
        if (output_method == trace_method::eventlog)
        {
            std::string inputs = "";
            std::string outputs = "";
            std::string results = "";
            try
            {
                inputs = "Path=" + InterpretStringA(fileName);
                inputs = "Section=" + InterpretStringA(appName);

                outputs += "returnString=" + InterpretCountedString("ReturnString", string, stringLength);

                results = InterpretAsHex("Return", result);

                std::ostringstream sout;
                InterpretCallingModulePart1()
                    sout << InterpretCallingModulePart2()
                    InterpretCallingModulePart3()
                    std::string cm = sout.str();

                Log_ETW_PostMsgOperationA("GetPrivateProfileSection", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);
            }
            catch (...)
            {
                Log("GetPrivateProfileSection event logging failure");
            }
        }
        else
        {
            try
            {
                Log("GetPrivateProfileSection:\n");
                LogString("Path", fileName);
                LogString("Section", appName);
                LogString("Returned",InterpretAsHex("Result", result).c_str());
                LogCountedString("ReturnedString", string, stringLength);
                LogCallingModule();
            }
            catch (...)
            {
                Log("GetPrivateProfileSection logging failure");
            }
        }
    }
    return result;
}
DECLARE_STRING_FIXUP(GetPrivateProfileSectionImpl, GetPrivateProfileSectionFixup);



auto GetPrivateProfileSectionNamesImpl = psf::detoured_string_function(&::GetPrivateProfileSectionNamesA, &::GetPrivateProfileSectionNamesW);
template <typename CharT>
DWORD __stdcall GetPrivateProfileSectionNamesFixup(
    _Out_writes_to_opt_(stringSize, return +1) CharT* string,
    _In_ DWORD stringLength,
    _In_opt_ const CharT* fileName) noexcept
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();
    auto result = GetPrivateProfileSectionNamesImpl(string, stringLength, fileName);
    QueryPerformanceCounter(&TickEnd);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(true);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
    {
        if (output_method == trace_method::eventlog)
        {
            std::string inputs = "";
            std::string outputs = "";
            std::string results = "";
            try
            {
                inputs = "Path=" + InterpretStringA(fileName);

                outputs += "returnString=" + InterpretCountedString("ReturnString", string, stringLength);

                results = InterpretAsHex("Return", result);

                std::ostringstream sout;
                InterpretCallingModulePart1()
                    sout << InterpretCallingModulePart2()
                    InterpretCallingModulePart3()
                    std::string cm = sout.str();

                Log_ETW_PostMsgOperationA("GetPrivateProfileSectionNames", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);
            }
            catch (...)
            {
                Log("GetPrivateProfileSectionNames event logging failure");
            }
        }
        else
        {
            try
            {
                Log("GetPrivateProfileSectionNames:\n");
                LogString("Path", fileName);
                LogString("Returned",InterpretAsHex("Result", result).c_str());
                LogCountedString("ReturnedString", string, stringLength);
                LogCallingModule();
            }
            catch (...)
            {
                Log("GetPrivateProfileSectionNames logging failure");
            }
        }
    }
    return result;
}
DECLARE_STRING_FIXUP(GetPrivateProfileSectionNamesImpl, GetPrivateProfileSectionNamesFixup);



auto GetPrivateProfileStringImpl = psf::detoured_string_function(&::GetPrivateProfileStringA, &::GetPrivateProfileStringW);
template <typename CharT>
DWORD __stdcall GetPrivateProfileStringFixup(
    _In_opt_ const CharT* appName,
    _In_opt_ const CharT* keyName,
    _In_opt_ const CharT* defaultString,
    _Out_writes_to_opt_(returnStringSizeInChars, return +1) CharT* string,
    _In_ DWORD stringLength,
    _In_opt_ const CharT* fileName) noexcept
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();
    auto result = GetPrivateProfileStringImpl(appName, keyName, defaultString, string, stringLength, fileName);
    QueryPerformanceCounter(&TickEnd);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(true);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
    {
        if (output_method == trace_method::eventlog)
        {
            std::string inputs = "";
            std::string outputs = "";
            std::string results = "";
            try
            {
                inputs = "Path=" + InterpretStringA(fileName);
                inputs += "\nSection=" + InterpretStringA(appName);
                inputs += "\nKey=" + InterpretStringA(keyName);
                inputs += "\nDefault=" + InterpretStringA(defaultString);

                outputs += "returnString=" + InterpretCountedString("ReturnString", string, stringLength);

                results = InterpretAsHex("Return", result);

                std::ostringstream sout;
                InterpretCallingModulePart1()
                    sout << InterpretCallingModulePart2()
                    InterpretCallingModulePart3()
                    std::string cm = sout.str();

                Log_ETW_PostMsgOperationA("GetPrivateProfileString", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);
            }
            catch (...)
            {
                Log("GetPrivateProfileString event logging failure");
            }
        }
        else
        {
            try
            {
                Log("GetPrivateProfileString:\n");
                LogString("Path", fileName);
                LogString("Section", appName);
                LogString("Key", keyName);
                LogString("Default", defaultString);
                LogString("Returned", InterpretAsHex("Result", result).c_str());
                LogCountedString("Returned String", string, stringLength);
                LogCallingModule();
            }
            catch (...)
            {
                Log("GetPrivateProfileString logging failure");
            }
        }
    }
    return result;
}
DECLARE_STRING_FIXUP(GetPrivateProfileStringImpl, GetPrivateProfileStringFixup);



auto GetPrivateProfileStructImpl = psf::detoured_string_function(&::GetPrivateProfileStructA, &::GetPrivateProfileStructW);
template <typename CharT>
BOOL __stdcall GetPrivateProfileStructFixup(
    _In_opt_ const CharT* sectionName,
    _In_opt_ const CharT* key,
    _Out_writes_to_opt_(uSizeStruct, return) LPVOID structArea,
    _In_ UINT uSizeStruct,
    _In_opt_ const CharT* fileName) noexcept
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();
    auto result = GetPrivateProfileStructImpl(sectionName, key, structArea, uSizeStruct, fileName);
    QueryPerformanceCounter(&TickEnd);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(true);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
    {
        if (output_method == trace_method::eventlog)
        {
            std::string inputs = "";
            std::string outputs = "";
            std::string results = "";
            try
            {
                inputs = "Path=" + InterpretStringA(fileName);
                inputs += "\nSection=" + InterpretStringA(sectionName);
                inputs += "\nKey=" + InterpretStringA(key);

                outputs += "returnStruct=(not displayed)";

                results = InterpretBool("Return",result);

                std::ostringstream sout;
                InterpretCallingModulePart1()
                    sout << InterpretCallingModulePart2()
                    InterpretCallingModulePart3()
                    std::string cm = sout.str();

                Log_ETW_PostMsgOperationA("GetPrivateProfileStruct", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);
            }
            catch (...)
            {
                Log("GetPrivateProfileStruct event logging failure");
            }
        }
        else
        {
            try
            {
                Log("GetPrivateProfileStuct:\n");
                LogString("Path", fileName);
                LogString("Section", sectionName);
                LogString("Key", key);
                LogBool("Result", result);
                LogString("ReturnedStruct", "(not displayed)");
                LogCallingModule();
            }
            catch (...)
            {
                Log("GetPrivateProfileStruct logging failure");
            }
        }
    }
    return result;
}
DECLARE_STRING_FIXUP(GetPrivateProfileStructImpl, GetPrivateProfileStructFixup);






auto WritePrivateProfileSectionImpl = psf::detoured_string_function(&::WritePrivateProfileSectionA, &::WritePrivateProfileSectionW);
template <typename CharT>
BOOL __stdcall WritePrivateProfileSectionFixup(
    _In_opt_ const CharT* appName,
    _In_opt_ const CharT* string,
    _In_opt_ const CharT* fileName) noexcept
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();
    auto result = WritePrivateProfileSectionImpl(appName, string, fileName);
    QueryPerformanceCounter(&TickEnd);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(true);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
    {
        if (output_method == trace_method::eventlog)
        {
            std::string inputs = "";
            std::string outputs = "";
            std::string results = "";
            try
            {
                inputs = "Path=" + InterpretStringA(fileName);
                inputs = "Section=" + InterpretStringA(appName);
                inputs = "ValueString=" + InterpretStringA(string);


                results = InterpretBool("Return", result);

                std::ostringstream sout;
                InterpretCallingModulePart1()
                    sout << InterpretCallingModulePart2()
                    InterpretCallingModulePart3()
                    std::string cm = sout.str();

                Log_ETW_PostMsgOperationA("WritePrivateProfileSection", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);
            }
            catch (...)
            {
                Log("WritePrivateProfileSection event logging failure");
            }
        }
        else
        {
            try
            {
                Log("WritePrivateProfileSection:\n");
                LogString("Path", fileName);
                LogString("Section", appName);
                LogString("ValueString", string);
                LogBool("Return", result);
                LogCallingModule();
            }
            catch (...)
            {
                Log("WritePrivateProfileSection logging failure");
            }
        }
    }
    return result;
}
DECLARE_STRING_FIXUP(WritePrivateProfileSectionImpl, WritePrivateProfileSectionFixup);




auto WritePrivateProfileStringImpl = psf::detoured_string_function(&::WritePrivateProfileStringA, &::WritePrivateProfileStringW);
template <typename CharT>
BOOL __stdcall WritePrivateProfileStringFixup(
    _In_opt_ const CharT* appName,
    _In_opt_ const CharT* keyName,
    _In_opt_ const CharT* string,
    _In_opt_ const CharT* fileName) noexcept
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();
    auto result = WritePrivateProfileStringImpl(appName, keyName, string, fileName);
    QueryPerformanceCounter(&TickEnd);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(true);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
    {
        if (output_method == trace_method::eventlog)
        {
            std::string inputs = "";
            std::string outputs = "";
            std::string results = "";
            try
            {
                inputs = "Path=" + InterpretStringA(fileName);
                inputs += "\nSection=" + InterpretStringA(appName);
                inputs += "\nKey=" + InterpretStringA(keyName);
                inputs += "\nValue=" + InterpretStringA(string);


                results = InterpretBool("Return", result);

                std::ostringstream sout;
                InterpretCallingModulePart1()
                    sout << InterpretCallingModulePart2()
                    InterpretCallingModulePart3()
                    std::string cm = sout.str();

                Log_ETW_PostMsgOperationA("WritePrivateProfileString", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);
            }
            catch (...)
            {
                Log("WritePrivateProfileString event logging failure");
            }
        }
        else
        {
            try
            {
                Log("WritePrivateProfileString:\n");
                LogString("Path", fileName);
                LogString("Section", appName);
                LogString("Key", keyName);
                LogString("Value", string);
                LogBool("Result", result);
                LogCallingModule();
            }
            catch (...)
            {
                Log("WritePrivateProfileString logging failure");
            }
        }
    }
    return result;
}
DECLARE_STRING_FIXUP(WritePrivateProfileStringImpl, WritePrivateProfileStringFixup);



auto WritePrivateProfileStructImpl = psf::detoured_string_function(&::WritePrivateProfileStructA, &::WritePrivateProfileStructW);
template <typename CharT>
BOOL __stdcall WritePrivateProfileStructFixup(
    _In_opt_ const CharT* appName,
    _In_opt_ const CharT* keyName,
    _In_opt_ const LPVOID structData,
    _In_opt_ const UINT   uSizeStruct,
    _In_opt_ const CharT* fileName) noexcept
{
    LARGE_INTEGER TickStart, TickEnd;
    QueryPerformanceCounter(&TickStart);
    auto entry = LogFunctionEntry();
    auto result = WritePrivateProfileStructImpl(appName, keyName, structData, uSizeStruct, fileName);
    QueryPerformanceCounter(&TickEnd);
    preserve_last_error preserveError;

    auto functionResult = from_win32_bool(true);
    if (auto lock = acquire_output_lock(function_type::filesystem, functionResult))
    {
        if (output_method == trace_method::eventlog)
        {
            std::string inputs = "";
            std::string outputs = "";
            std::string results = "";
            try
            {
                inputs = "Path=" + InterpretStringA(fileName);
                inputs += "\nSection=" + InterpretStringA(appName);
                inputs += "\nKey=" + InterpretStringA(keyName);
                inputs += "\nStruct=(not displayed)";

                results = InterpretBool("Return", result);

                std::ostringstream sout;
                InterpretCallingModulePart1()
                    sout << InterpretCallingModulePart2()
                    InterpretCallingModulePart3()
                    std::string cm = sout.str();

                Log_ETW_PostMsgOperationA("WritePrivateProfileStruct", inputs.c_str(), results.c_str(), outputs.c_str(), cm.c_str(), TickStart, TickEnd);
            }
            catch (...)
            {
                Log("WritePrivateProfileStruct event logging failure");
            }
        }
        else
        {
            try
            {
                Log("WritePrivateProfileStuct:\n");
                LogString("Path", fileName);
                LogString("Section", appName);
                LogString("Key", keyName);
                LogString("Struct", "(not displayed)");
                LogBool("Result", result);
                LogCallingModule();
            }
            catch (...)
            {
                Log("WritePrivateProfileStruct logging failure");
            }
        }
    }
    return result;
}
DECLARE_STRING_FIXUP(WritePrivateProfileStructImpl, WritePrivateProfileStructFixup);


