#pragma once
#include <windef.h>
#include <handleapi.h>
#include <winreg.h>

class UniqueHKEY
{
public:
    UniqueHKEY()
    {
    }

    ~UniqueHKEY()
    {
        if (key != INVALID_HANDLE_VALUE)
        {
            RegCloseKey((HKEY)key);
        }
    }

    PHKEY operator&()
    {
        return (PHKEY)&key;
    }

    void Set(HKEY value)
    {
        key = value;
    }

    HKEY Get()
    {
        return (HKEY)key;
    }



private:
    HANDLE key = INVALID_HANDLE_VALUE;
};