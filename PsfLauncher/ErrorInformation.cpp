#include "ErrorInformation.h"



ErrorInformation::ErrorInformation(std::wstring customMessage, DWORD errorNumber)
{
    this->customMessage = customMessage;
    this->errorNumber = errorNumber;

    this->errorMessage = widen(std::system_category().message(this->errorNumber));
    this->errorMessage.resize(this->errorMessage.length() - 3);
    this->isThereAnError = true;
}

ErrorInformation::ErrorInformation()
{
    this->customMessage = L"";
    this->errorMessage = L"";
    this->isThereAnError = false;
}

ErrorInformation::ErrorInformation(const ErrorInformation &toCopy)
{
    this->customMessage = toCopy.customMessage;
    this->errorMessage = toCopy.errorMessage;
    this->isThereAnError = toCopy.isThereAnError;
    this->errorNumber = toCopy.errorNumber;
}


ErrorInformation::~ErrorInformation()
{
}

bool ErrorInformation::IsThereAnError()
{
    return this->isThereAnError;
}

ErrorInformation ErrorInformation::GetInfoWithNoError()
{
    return ErrorInformation();
}
