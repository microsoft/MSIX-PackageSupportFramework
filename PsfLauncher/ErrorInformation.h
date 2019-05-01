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
    ErrorInformation(std::wstring customMessage, DWORD errorNumber, std::wstring exeName);
    ~ErrorInformation();
    ErrorInformation(const ErrorInformation &toCopy);

    LPCWSTR Print();
    bool IsThereAnError();
    void AddExeName(std::wstring exeNameToAdd);

private:
    std::wostringstream customMessage;
    DWORD errorNumber;
    std::wstring exeName;
    std::wstring errorMessage;
    bool isThereAnError;
};

