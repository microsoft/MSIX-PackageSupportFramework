
#include <windows.h>

int main()
{
    ::MessageBoxW(nullptr, L"FAILED: This message should have been shimmed", L"Composition Test", MB_OK);
}
