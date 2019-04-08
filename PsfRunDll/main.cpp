//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <algorithm>
#include <cstring>
#include <string>

#include <windows.h>
#include <shellapi.h>

using EntryPointProc = void (CALLBACK *)(HWND, HINSTANCE, LPSTR, int);

int __stdcall WinMain(HINSTANCE, HINSTANCE, PSTR cmdLine, int cmdShow)
{
    // RUNDLL.EXE <dllname>,<entrypoint> <optional arguments>
    auto dllPath = cmdLine;

    // The dll path can be a quoted string, in which case we should ignore commas until we find the closing quote
    if (*cmdLine == '"')
    {
        ++dllPath;
        cmdLine = std::strchr(cmdLine + 1, '"');
        if (!cmdLine) return ERROR_INVALID_PARAMETER;
        *cmdLine++ = '\0';
        if (*cmdLine != ',') return ERROR_INVALID_PARAMETER;
    }
    else
    {
        cmdLine = std::strchr(cmdLine, ',');
        if (!cmdLine) return ERROR_INVALID_PARAMETER;
    }

    *cmdLine++ = '\0';
    auto entrypoint = cmdLine;

    char dummyString[] = "";
    cmdLine = std::strchr(cmdLine, ' ');
    if (cmdLine)
    {
        *cmdLine++ = '\0';
    }
    else
    {
        cmdLine = dummyString;
    }

    auto mod = ::LoadLibraryA(dllPath);
    if (!mod)
    {
        return ::GetLastError();
    }

    EntryPointProc proc = nullptr;
    if (*entrypoint == '#')
    {
        // Special case; load by ordinal
        proc = reinterpret_cast<EntryPointProc>(::GetProcAddress(mod, MAKEINTRESOURCEA(atoi(entrypoint + 1))));
    }
    else
    {
        proc = reinterpret_cast<EntryPointProc>(::GetProcAddress(mod, entrypoint));
    }

    if (!proc)
    {
        return ::GetLastError();
    }

    proc(nullptr, mod, cmdLine, cmdShow);

    return 0;
}
