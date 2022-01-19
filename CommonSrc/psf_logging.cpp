//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include <windows.h>
#include <stdarg.h>
#include <debugapi.h>
#include <string>
#include <assert.h>

#include <utilities.h>

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

void LogString(const char* name, const char* value)
{
    if ((value != NULL && value[1] != 0x0))
    {
        Log(L"%s=%s\n", name, value);
    }
    else
    {
        Log(L"%s=%ls", name, (wchar_t*)value);
    }
}

void LogString(const char* name, const wchar_t* value)
{
    if ((value != NULL && ((char*)value)[1] == 0x0))
    {
        Log(L"%s=%ls\n", name, value);
    }
    else
    {
        Log(L"%s=%s", name, (char*)value);
    }
}

void LogString(const wchar_t* name, const char* value)
{
    if ((value != NULL && value[1] != 0x0))
    {
        Log(L"%ls=%s\n", name, value);
    }
    else
    {
        Log(L"%ls=%ls", name, (wchar_t*)value);
    }
}

void LogString(const wchar_t* name, const wchar_t* value)
{
    if ((value != NULL && ((char*)value)[1] == 0x0))
    {
        Log(L"%ls=%ls\n", name, value);
    }
    else
    {
        Log(L"%ls=%s", name, (char*)value);
    }
}


void LogString(DWORD inst, const char* name, const char* value)
{
    if ((value != NULL && value[1] != 0x0))
    {
        Log(L"[%d]%s=%s\n", inst, name, value);
    }
    else
    {
        Log(L"[%d]%s=%ls", inst, name, (wchar_t*)value);
    }
}

void LogString(DWORD inst, const char* name, const wchar_t* value)
{
    if ((value != NULL && ((char*)value)[1] == 0x0))
    {
        Log(L"[%d]%s=%ls\n", inst, name, value);
    }
    else
    {
        Log(L"[%d]%s=%s", inst, name, (char*)value);
    }
}

void LogStringAA(DWORD inst, const char* name, const char* value)
{
    Log(L"[%d]%s=%s\n", inst, name, value);

}
void LogStringAW(DWORD inst, const char* name, const wchar_t* value)
{
    Log(L"[%d]%s=%ls", inst, name, value);
}

void LogString(DWORD inst, const wchar_t* name, const char* value)
{
    if ((value != NULL && value[1] != 0x0))
    {
        Log(L"[%d]%ls=%ls\n", inst, name, widen(value).c_str());
    }
    else
    {
        Log(L"[%d]%ls=%ls", inst, name, (wchar_t*)value);
    }
}

void LogString(DWORD inst, const wchar_t* name, const wchar_t* value)
{
    if ((value != NULL && ((char*)value)[1] == 0x0))
    {
        Log(L"[%d]%ls=%ls\n", inst, name, value);
    }
    else
    {
        Log(L"[%d]%ls=%ls", inst, name, widen((const char*)value).c_str());
    }
}

void LogStringWA(DWORD inst, const wchar_t* name, const char* value)
{
    Log(L"[%d]%ls=%ls\n", inst, name, widen(value).c_str());
}
void LogStringWW(DWORD inst, const wchar_t* name, const wchar_t* value)
{
    Log(L"[%d]%ls=%ls", inst, name, value);
}


void LogCountedStringW(const char* name, const wchar_t* value, std::size_t length)
{
    Log("\t%s=%.*ls\n", name, length, value);
}

void Loghexdump(void* pAddressIn, long  lSize)
{
    char szBuf[128];
    long lIndent = 1;
    long lOutLen, lIndex, lIndex2, lOutLen2;
    long lRelPos;
    struct { char* pData; unsigned long lSize; } buf;
    unsigned char* pTmp, ucTmp;
    unsigned char* rememberPtmp;
    unsigned char* pAddress = (unsigned char*)pAddressIn;

    buf.pData = (char*)pAddress;
    buf.lSize = lSize;

    while (buf.lSize > 0)
    {
        pTmp = (unsigned char*)buf.pData;
        lOutLen = (int)buf.lSize;
        if (lOutLen > 16)
            lOutLen = 16;

        // create a 64-character formatted output line:
        sprintf_s(szBuf, 100, " >                            "
            "                      "
            "         ");
        rememberPtmp = pTmp;
        lOutLen2 = lOutLen;

        for (lIndex = 1 + lIndent, lIndex2 = 53 - 15 + lIndent, lRelPos = 0;
            lOutLen2;
            lOutLen2--, lIndex += 2, lIndex2++
            )
        {
            ucTmp = *pTmp++;

            sprintf_s(szBuf + lIndex, 100 - lIndex, "%02X ", (unsigned short)ucTmp);
            if (!isprint(ucTmp))  ucTmp = '.'; // nonprintable char
            szBuf[lIndex2] = ucTmp;

            if (!(++lRelPos & 3))     // extra blank after 4 bytes
            {
                lIndex++; szBuf[lIndex + 2] = ' ';
            }
        }

        if (!(lRelPos & 3)) lIndex--;

        sprintf_s(szBuf + lIndex, 100 - lIndex, "<    %08lx  ", (unsigned long)(rememberPtmp - pAddress));
        szBuf[lIndex + 14] = 0x0;

        ::OutputDebugStringA(szBuf);

        buf.pData += lOutLen;
        buf.lSize -= lOutLen;
    }
}