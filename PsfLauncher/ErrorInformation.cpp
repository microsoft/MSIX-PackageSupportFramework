#include "ErrorInformation.h"

ErrorInformation::ErrorInformation(std::wstring customMessage, DWORD errorNumber, std::wstring exeName) 
    : ErrorInformation(customMessage, errorNumber)
{
    this->exeName = exeName;
}

ErrorInformation::ErrorInformation(std::wstring customMessage, DWORD errorNumber)
{
    this->customMessage << customMessage;
    this->errorNumber = errorNumber;
    this->exeName = exeName;

    this->errorMessage = widen(std::system_category().message(this->errorNumber));
    this->errorMessage.resize(this->errorMessage.length() - 3);
    this->isThereAnError = true;
}

ErrorInformation::ErrorInformation()
{
    this->customMessage << L"";
    this->errorMessage = L"";
    this->isThereAnError = false;
}

ErrorInformation::ErrorInformation(const ErrorInformation &toCopy)
{
    this->customMessage << toCopy.customMessage.str();
    this->errorMessage = toCopy.errorMessage;
    this->isThereAnError = toCopy.isThereAnError;
    this->errorNumber = toCopy.errorNumber;
}

LPCWSTR ErrorInformation::Print()
{
    this->customMessage << L"Message: " << this->errorMessage << "\r\n";
    this->customMessage << L"Error number: " << this->errorNumber << "\r\n";

    return this->customMessage.str().c_str();
}


ErrorInformation::~ErrorInformation()
{
}

bool ErrorInformation::IsThereAnError()
{
    return this->isThereAnError;
}

void ErrorInformation::AddExeName(std::wstring exeNameToAdd)
{
    this->exeName = exeNameToAdd;
}