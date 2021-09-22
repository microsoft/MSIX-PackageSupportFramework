//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <filesystem>
#include <functional>

#include <test_config.h>

#include "attributes.h"
#include "common_paths.h"

static int DoCreateDirectoryTest(const std::filesystem::path& path, bool expectSuccess)
{
    clean_redirection_path();
    trace_messages(L"Creating directory: ", info_color, path.native(), new_line);
    trace_messages(L"   Expected result: ", info_color, (expectSuccess ? L"Success" : L"Failure"), new_line);
    trace_message(L"Calling with CreateDirectory...\n");
    if (::CreateDirectoryW(path.c_str(), nullptr))
    {
        if (!expectSuccess)
        {
            trace_message(L"ERROR: Successfully created directory, but we were expecting failure\n", error_color);
            return ERROR_ASSERTION_FAILURE;
        }

        // Trying to create the directory again should fail
        trace_message(L"Directory created successfully. Creating it again should fail...\n");
        if (::CreateDirectoryW(path.c_str(), nullptr))
        {
            trace_message(L"ERROR: Successfully created directory, but we were expecting failure\n", error_color);
            return ERROR_ASSERTION_FAILURE;
        }
    }
    else if (expectSuccess)
    {
        return trace_last_error(L"Failed to create directory, but we were expecting it to succeed");
    }

    clean_redirection_path();
	write_entire_file(L"TèƨƭTè₥ƥℓáƭè\\file.txt", "Testing text");
    trace_message(L"Now calling with CreateDirectoryEx...\n");
    if (::CreateDirectoryExW(L"TèƨƭTè₥ƥℓáƭè", path.c_str(), nullptr))
    {
        if (!expectSuccess)
        {
            trace_message(L"ERROR: Successfully created directory, but we were expecting failure\n", error_color);
            return ERROR_ASSERTION_FAILURE;
        }

        // The attributes of the two directories should match (As that's what CreateDirectoryEx does)
        auto attrFrom = ::GetFileAttributesW(L"TèƨƭTè₥ƥℓáƭè");
        auto attrTo = ::GetFileAttributesW(path.c_str());
        trace_messages(L"Template directory attributes: ", info_color, file_attributes(attrFrom), new_line);
        trace_messages(L"Created directory attributes: ", info_color, file_attributes(attrTo), new_line);
        if (attrFrom != attrTo)
        {
            trace_message(L"ERROR: Attributes should have matched!\n", error_color);
            return ERROR_ASSERTION_FAILURE;
        }

        clean_redirection_path();
        trace_message(L"Calling SetFileAttributesW against template folder, but this time with FILE_ATTRIBUTE_HIDDEN added to the attributes of the template directory\n");
        attrFrom |= FILE_ATTRIBUTE_HIDDEN;
        if (!::SetFileAttributesW(L"TèƨƭTè₥ƥℓáƭè", attrFrom))
        {
            return trace_last_error(L"Failed to set the file attributes");
        }
        else if (auto attr = ::GetFileAttributesW(L"TèƨƭTè₥ƥℓáƭè"); attr != attrFrom)
        {
            trace_message(L"ERROR: Setting attributes succeeded, but it doesn't appear to have had an effect\n", error_color);
            trace_messages(error_color, L"ERROR: Attributes are: ", error_info_color, file_attributes(attr), new_line);
            return ERROR_ASSERTION_FAILURE;
        }

        trace_message(L"Calling CreateDirectoryExW against this attributed folder\n");
        if (!::CreateDirectoryExW(L"TèƨƭTè₥ƥℓáƭè", path.c_str(), nullptr))
        {
            return trace_last_error(L"Failed to re-create the directory");
        }

        attrTo = ::GetFileAttributesW(path.c_str());
        trace_messages(L"Attributes are now: ", info_color, file_attributes(attrTo), new_line);
        if (attrTo != attrFrom)
        {
            trace_message(L"ERROR: FILE_ATTRIBUTE_HIDDEN should have been included in the directory's attributes!\n", error_color);
            return ERROR_ASSERTION_FAILURE;
        }
    }
    else if (expectSuccess)
    {
        return trace_last_error(L"Failed to create directory, but we were expecting it to succeed");
    }

    return ERROR_SUCCESS;
}

int CreateDirectoryTests()
{
    int result = ERROR_SUCCESS;

    // "Fôô" is not something that we should try and redirect, so creation should fail
    test_begin("Create Non-Configured Directory Test");
    auto testResult = DoCreateDirectoryTest(L"Fôô", false);
    result = result ? result : testResult;
    test_end(testResult);

    // "TestÐïřèçƭôř¥" _is_ something that we should try and redirect
    test_begin("Create Redirected Directory Test");
    testResult = DoCreateDirectoryTest(L"TèƨƭÐïřèçƭôř¥", true);
    result = result ? result : testResult;
    test_end(testResult);

    // NOTE: "Tèƨƭ" is a directory that exists in the package path. Therefore, it's reasonable to expect that an attempt
    //       to create that directory would fail. However, due to the limitations around file/directory deletion, we
    //       explicitly ensure that the opposite is true. That is, we give the application the benefit of the doubt that
    //       if it were to try and create the directory "Tèƨƭ" here that it had previously tried to delete it
    test_begin("Create Package Directory Test");
    testResult = DoCreateDirectoryTest(L"Tèƨƭ", true);
    result = result ? result : testResult;
    test_end(testResult);

    return result;
}
