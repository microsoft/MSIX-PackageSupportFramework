//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <cassert>

#include <windows.h>
#include <detours.h>

#include "win32_error.h"

namespace detours
{
    // RAII wrapper around DetourTransactionBegin/Abort/Commit that ensures either Abort or Commit gets called
    struct transaction
    {
        transaction()
        {
            check_win32(::DetourTransactionBegin());
        }

        transaction(const transaction&) = delete;
        transaction& operator=(const transaction&) = delete;

        transaction(transaction&& other) noexcept :
            m_abort(std::exchange(other.m_abort, false))
        {
        }

        ~transaction()
        {
            if (m_abort)
            {
                ::DetourTransactionAbort();
            }
        }

        void commit()
        {
            assert(m_abort);
            check_win32(::DetourTransactionCommit());
            m_abort = false;
        }

    private:

        // If not explicitly told to commit, assume an error occurred and abort
        bool m_abort = true;
    };
}
