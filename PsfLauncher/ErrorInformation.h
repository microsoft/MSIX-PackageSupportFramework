#pragma once
#include <string>
#include <windows.h>
#include <iostream>
#include "utilities.h"
class ErrorInformation
{
public:
    ErrorInformation();
    ErrorInformation(std::wstring customMessage, DWORD errorNumber);
    ~ErrorInformation();
    ErrorInformation(const ErrorInformation &toCopy);

    LPCWSTR Print();
    bool IsThereAnError();

private:
    std::wostringstream customMessage;
    DWORD errorNumber;
    std::wstring errorMessage;
    bool isThereAnError;
};

