#pragma once
#include <string>
#include <windows.h>
#include <iostream>
#include "utilities.h"

class ErrorInformation
{
public:
    ErrorInformation()
    {
    }


    ErrorInformation(std::wstring customMessage, DWORD errorNumber) :
        ErrorInformation(customMessage, errorNumber, L"")
    {
    }

    ErrorInformation(std::wstring customMessage, DWORD errorNumber, std::wstring exeName)
        : customMessage(customMessage)
        , errorNumber(errorNumber)
        , errorMessage(widen(std::system_category().message(errorNumber)))
        , isThereAnError(true)
        , exeName(exeName)
    {
    }

    ErrorInformation(const ErrorInformation &toCopy)
    {
        this->customMessage = toCopy.customMessage;
        this->errorMessage = toCopy.errorMessage;
        this->isThereAnError = toCopy.isThereAnError;
        this->errorNumber = toCopy.errorNumber;
        this->exeName = toCopy.exeName;
    }

    ErrorInformation& operator=(const ErrorInformation &toAssign)
    {
        if (this == &toAssign)
        {
            return *this;
        }

        this->customMessage.clear();
        this->customMessage = toAssign.customMessage;
        this->errorMessage = toAssign.errorMessage;
        this->isThereAnError = toAssign.isThereAnError;
        this->errorNumber = toAssign.errorNumber;
        this->exeName = toAssign.exeName;

        return *this;
    }

    std::wstring Print()
    {
        std::wstring errorToPrint;

        if (!this->customMessage.empty())
        {
            errorToPrint.append(this->customMessage);
            errorToPrint.append(L"\r\n");
        }

        errorToPrint.append(L"Message: " + this->errorMessage + L"\r\n");
        errorToPrint.append(L"Error number: " + this->errorNumber);
        errorToPrint.append(L"\r\n");

        if(!this->exeName.empty())
        {
            errorToPrint.append(L"ExeName: " + this->exeName + L"\r\n");
        }

        return errorToPrint;
    }

    bool IsThereAnError()
    {
        return this->isThereAnError;
    }

    DWORD GetErrorNumber()
    {
        return this->errorNumber;
    }

    void AddExeName(std::wstring exeNameToAdd)
    {
        this->exeName = exeNameToAdd;
    }

private:
    std::wstring customMessage;
    DWORD errorNumber;
    std::wstring exeName;
    std::wstring errorMessage;
    bool isThereAnError = false;
};

