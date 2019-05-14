#pragma once
#include <string>
#include <windows.h>
#include <iostream>
#include "utilities.h"

class ErrorInformation
{
public:
    ErrorInformation() : customMessage(L""), errorMessage(L""), isThereAnError(false)
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

        return *this;
    }

    std::wstring Print()
    {
        this->customMessage.clear();
        this->customMessage.append(L"\r\nMessage: " + this->errorMessage + L"\r\n");
        this->customMessage.append(L"Error number: " + this->errorNumber);
        this->customMessage.append(L"\r\n");

        return this->customMessage;
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
    bool isThereAnError;
};

