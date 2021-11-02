//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <thread>
#include <windows.h>

namespace psf
{
    inline void LogWFD(const wchar_t* msg)
    {
        ::OutputDebugStringW(msg);
    }
    inline void wait_for_debugger()
    {
        LogWFD(L"Start WFD");
        // If a debugger is already attached, ignore as they have likely already set all breakpoints, etc. they need
        if (!::IsDebuggerPresent())
        {
            LogWFD(L"WFD: not yet.");
            while (!::IsDebuggerPresent())
            {
                LogWFD(L"WFD: still not yet.");
                ::Sleep(1000);
            }
            LogWFD(L"WFD: Yes.");
            // NOTE: When a debugger attaches (invasively), it will inject a DebugBreak in a new thread. Unfortunately,
            //       that does not synchronize with, and may occur _after_ IsDebuggerPresent returns true, allowing
            //       execution to continue for a short period of time. In order to get around this, we'll insert our own
            //       DebugBreak call here. We also add a short(-ish) sleep so that this is likely to be the second break
            //       seen, so that the injected DebugBreak doesn't preempt us in the middle of debugging. This is of
            //       course best effort
            ::Sleep(5000);
            std::this_thread::yield();
            ::DebugBreak();
        }
        LogWFD(L"WFD: Done.");
    }
}
