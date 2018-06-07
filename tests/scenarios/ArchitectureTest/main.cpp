
#include <conio.h>
#include <iostream>

#include <Windows.h>

#include <error_logging.h>

int main(int argc, const char**)
{
#ifdef _M_IX86
        const auto targetExe = L"ArchitectureTest64.exe";
        const auto title = L"Hello, from x86!";
#else
        const auto targetExe = L"ArchitectureTest32.exe";
        const auto title = L"Hello, from x64!";
#endif

    // When properly shimmed, the message should be modified to say "SUCCESS: You've been shimmed!"
    ::MessageBoxW(nullptr, L"FAILED: This message should have been shimmed", title, MB_OK);

    // Use the presence of additional arguments to determine if we've already done the cross-architecture launch or not
    if (argc <= 1)
    {
        STARTUPINFO si{};
        PROCESS_INFORMATION pi;

        wchar_t launchString[1024];
        ::swprintf(launchString, std::size(launchString), L"%ls foo", targetExe);

        std::wcout << L"Launching \"" << targetExe << L"\"\n";
        if (::CreateProcessW(nullptr, launchString, nullptr, nullptr, false, 0, nullptr, nullptr, &si, &pi))
        {
            ::WaitForSingleObject(pi.hProcess, INFINITE);
        }
        else
        {
            std::cout << error_text() << "ERROR: Failed to launch target executable\n";
        }

        std::cout << "Press any key to continue . . . ";
        _getch();
    }
}
