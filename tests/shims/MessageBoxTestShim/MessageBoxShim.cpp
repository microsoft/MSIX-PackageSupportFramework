
#include <shim_framework.h>

auto MessageBoxAImpl = &::MessageBoxA;
auto MessageBoxWImpl = &::MessageBoxW;

int WINAPI MessageBoxAShim(
    _In_opt_ HWND hwnd,
    _In_opt_ LPCSTR,
    _In_opt_ LPCSTR caption,
    _In_ UINT type)
{
    return MessageBoxAImpl(hwnd, "SUCCESS: You've been shimmed!", caption, type);
}

int WINAPI MessageBoxWShim(
    _In_opt_ HWND hwnd,
    _In_opt_ LPCWSTR,
    _In_opt_ LPCWSTR caption,
    _In_ UINT type)
{
    return MessageBoxWImpl(hwnd, L"SUCCESS: You've been shimmed!", caption, type);
}

DECLARE_SHIM(MessageBoxAImpl, MessageBoxAShim);
DECLARE_SHIM(MessageBoxWImpl, MessageBoxWShim);
