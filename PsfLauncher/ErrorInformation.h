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
    ErrorInformation& operator=(const ErrorInformation &toAssign);

    LPCWSTR Print();
    bool IsThereAnError();
    DWORD GetErrorNumber();
    void AddExeName(std::wstring exeNameToAdd);

private:
    std::wostringstream customMessage;
    DWORD errorNumber;
    std::wstring exeName;
    std::wstring errorMessage;
    bool isThereAnError;
};

