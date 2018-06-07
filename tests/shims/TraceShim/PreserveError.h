#pragma once

#include <windows.h>

// Preserves the result of GetLastError, restoring it on destruction of the object. Useful when any post-processing may
// change the result of GetLastError that you wish to ignore (e.g. such as logging)
struct preserve_last_error
{
    DWORD value = ::GetLastError();

    ~preserve_last_error()
    {
        ::SetLastError(value);
    }
};
