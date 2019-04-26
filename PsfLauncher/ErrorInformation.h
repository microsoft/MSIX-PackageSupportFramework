#pragma once
#include <string>
#include <minwindef.h>
#include "utilities.h"
class ErrorInformation
{
public:
    ErrorInformation(std::wstring customMessage, DWORD errorNumber);
    ~ErrorInformation();
    ErrorInformation(const ErrorInformation &toCopy);

    ErrorInformation GetInfoWithNoError();
    bool IsThereAnError();

private:
    ErrorInformation();
    std::wstring customMessage;
    DWORD errorNumber;
    std::wstring errorMessage;
    bool isThereAnError;
};

