
#include <conio.h>
#include <filesystem>
#include <iostream>

#include <Windows.h>

#include <error_logging.h>

int run()
{
    if (std::filesystem::exists(L"foo.txt"))
    {
        std::cout << success_text() << "SUCCESS: Working directory set correctly!\n";
    }
    else
    {
        std::cout << error_text() << "ERROR: Working directory not set correctly!\n";
        return ERROR_FILE_NOT_FOUND;
    }

    return 0;
}

int main()
{
    auto result = run();

    std::cout << "Press any key to continue . . . ";
    _getch();
    return result;
}
