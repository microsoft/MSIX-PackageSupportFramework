#pragma once

// Removes PII user info from PCWSTR version - returns what's after :\Users\userName if a user path is found.
inline static PCWSTR RemovePIIfromFilePath(
    _In_ PCWSTR path)
{
    if (path == NULL)
    {
        return path;
    }

    PCWSTR userString = L":\\Users\\";
    PCWSTR userStringPointer = wcsstr(path, userString);
    if (userStringPointer == NULL)
    {
        return path;
    }

    PCWSTR validPath = wcsstr(userStringPointer + wcslen(userString), L"\\");

    // In cases where validPath is NULL, there is no userName after :\Users\, we are going err on the side
    // of caution and return null in this case.  So, returning validPath is fine in both cases where
    // userName is found vs not
    return validPath;
}