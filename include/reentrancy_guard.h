//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// Type that's useful to detect reentrancy. This can be useful at either global for function-static scope. Note however,
// that either should be declared 'thread_local'. E.g. Use might look like:
//      void FooShim()
//      {
//          thread_local shims::reentrancy_guard reentrancyGuard;
//          auto guard = reentrancyGuard.enter();
//          if (guard) { /*shim code here*/ }
//          return FooImpl();
//      }
#pragma once

#include <utility>

namespace shims
{
    namespace details
    {
        struct restore_on_exit
        {
            restore_on_exit(bool& target, bool restoreValue) :
                m_target(&target),
                m_restoreValue(restoreValue)
            {
            }

            restore_on_exit(const restore_on_exit&) = delete;
            restore_on_exit& operator=(const restore_on_exit&) = delete;

            restore_on_exit(restore_on_exit&& other) :
                m_target(other.m_target),
                m_restoreValue(other.m_restoreValue)
            {
                other.m_target = nullptr;
            }

            ~restore_on_exit()
            {
                if (m_target)
                {
                    *m_target = m_restoreValue;
                }
            }

            explicit operator bool()
            {
                return !m_restoreValue;
            }

        private:

            bool* m_target;
            bool m_restoreValue;
        };
    }

    class reentrancy_guard
    {
    public:

        details::restore_on_exit enter()
        {
            auto restoreValue = std::exchange(m_isReentrant, true);
            return details::restore_on_exit{ m_isReentrant, restoreValue };
        }

    private:

        bool m_isReentrant = false;
    };
}
