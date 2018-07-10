//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <iostream>
#include <sstream>

#include <Windows.h>

#include <test_config.h>

using namespace std::literals;

int wmain(int argc, const wchar_t** argv)
{
    std::map<std::wstring_view, std::wstring> allowedArgs;
    allowedArgs.emplace(L"/launchChild"sv, L"true");
    auto result = parse_args(argc, argv, allowedArgs);

#ifdef _M_IX86
    constexpr const wchar_t targetExe[] = L"ArchitectureTest64.exe";
    constexpr const char name[] = "x86 Test";
#else
    constexpr const wchar_t targetExe[] = L"ArchitectureTest32.exe";
    constexpr const char name[] = "x64 Test";
#endif

    auto isInitialTest = allowedArgs[L"/launchChild"] == L"true";
    if (result == ERROR_SUCCESS)
    {
        if (isInitialTest)
        {
            test_initialize("Architecture Tests", 2);
        }

        test_begin(name);

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

            constexpr wchar_t expectedOutputMessage[] = L"You've been shimmed!";
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

        if (isInitialTest)
        {
            STARTUPINFO si{};
            PROCESS_INFORMATION pi;

            std::wstring launchString = targetExe + L" /launchChild:false"s;
            auto isTestMode = static_cast<bool>(g_testRunnerPipe);
            if (isTestMode)
            {
                // Relenquish the pipe so that our child process can connect
                g_testRunnerPipe.reset();
                launchString += L" /mode:test";
            }

            std::wcout << L"Launching \"" << targetExe << L"\"\n";
            if (::CreateProcessW(nullptr, launchString.data(), nullptr, nullptr, false, 0, nullptr, nullptr, &si, &pi))
            {
                ::WaitForSingleObject(pi.hProcess, INFINITE);

                if (!isTestMode)
                {
                    // This is a bit of a hack, but when we're not running in test mode, the child process isn't
                    // communicating test success/failure to anyone, except through the exit code. Since the child
                    // process will only be running a single test, this is fairly convenient for us and allows us to
                    // use the exit code to deduce the success/failure of the test
                    DWORD exitCode;
                    if (::GetExitCodeProcess(pi.hProcess, &exitCode) && !exitCode)
                    {
                        ++g_successCount;
                    }
                    else
                    {
                        ++g_failureCount;
                    }
                }

                ::CloseHandle(pi.hProcess);
                ::CloseHandle(pi.hThread);
            }
            else
            {
                print_last_error("Failed to launch target executable");
            }

            if (isTestMode)
            {
                g_testRunnerPipe = test_client_connect();
            }

            test_cleanup();
        }
    }

    if (!g_testRunnerPipe && isInitialTest)
    {
        system("pause");
    }

    return result;
}
