
#include <shim_framework.h>

auto MessageBoxAImpl = &::MessageBoxA;
auto MessageBoxWImpl = &::MessageBoxW;

int WINAPI MessageBoxAShim(
    _In_opt_ HWND hwnd,
    _In_opt_ LPCSTR text,
    _In_opt_ LPCSTR caption,
    _In_ UINT type)
{
    std::string textStr = text;
    textStr += " And you've been shimmed again!";
    return MessageBoxAImpl(hwnd, textStr.c_str(), caption, type);
}

int WINAPI MessageBoxWShim(
    _In_opt_ HWND hwnd,
    _In_opt_ LPCWSTR text,
    _In_opt_ LPCWSTR caption,
    _In_ UINT type)
{
    std::wstring textStr = text;
    textStr += L" And you've been shimmed again!";
    return MessageBoxWImpl(hwnd, textStr.c_str(), caption, type);
}

DECLARE_SHIM(MessageBoxAImpl, MessageBoxAShim);
DECLARE_SHIM(MessageBoxWImpl, MessageBoxWShim);
