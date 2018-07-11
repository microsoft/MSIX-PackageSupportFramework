//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <sstream>
#include <windows.h>

#include <test_config.h>

int wmain(int argc, const wchar_t** argv)
{
    auto result = parse_args(argc, argv);

    if (result == ERROR_SUCCESS)
    {
        test_initialize("Composition Tests", 1);
        test_begin("Composition Test");

        // NOTE: trace_message will call MultiByteToWideChar if we give it a non-wide string, hence the duplication
        //       below. Otherwise, we'll shim the message we're trying to print out!
        wchar_t buffer[256];
        constexpr char inputMessage[] = "This message should have been shimmed";
        trace_messages(L"Initial text: ", info_color, L"This message should have been shimmed", new_line);

        auto size = ::MultiByteToWideChar(
            CP_ACP,
            MB_ERR_INVALID_CHARS,
            inputMessage, static_cast<int>(std::size(inputMessage) - 1),
            buffer, static_cast<int>(std::size(buffer) - 1));
        if (!size)
        {
            result = trace_last_error(L"MultiByteToWideChar failed");
        }
        else
        {
            buffer[size] = '\0';
            trace_messages(L"Result text: ", info_color, buffer, new_line);

            constexpr wchar_t expectedOutputMessage[] = L"You've been shimmed! And you've been shimmed again!";
            if (std::wcsncmp(buffer, expectedOutputMessage, std::size(expectedOutputMessage) - 1) != 0)
            {
                result = ERROR_ASSERTION_FAILURE;
                trace_message(L"ERROR: Text did not match the expected output\n", error_color);
            }
            else
            {
                trace_message(L"SUCCESS: The text was modified successfully\n", success_color);
            }
        }

        test_end(result);

        test_cleanup();
    }

    if (!g_testRunnerPipe)
    {
        system("pause");
    }

    return result;
}
