//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <conio.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <vector>

#include <ShObjIdl.h>
#include <wrl/client.h>

#include <error_logging.h>
#include <test_runner.h>

#include "message_pipe.h"

using namespace Microsoft::WRL;
using namespace std::literals;

// It may be worth considering doing some enumeration of installed applications to find ones that we care about, but for
// now since the list is small and known, just declare them ahead of time. We'll just need to be careful to not treat
// missing applications as too serious
static constexpr const wchar_t* g_applications[] =
{
    L"ArchitectureTest_8wekyb3d8bbwe!Fixed32",
    L"ArchitectureTest_8wekyb3d8bbwe!Fixed64",
    L"CompositionTest_8wekyb3d8bbwe!Fixed",
    L"FileSystemTest_8wekyb3d8bbwe!Fixed",
    L"LongPathsTest_8wekyb3d8bbwe!Fixed",
    L"PowershellScriptTest_8wekyb3d8bbwe!PSNoScripts",
    L"PowershellScriptTest_8wekyb3d8bbwe!PSOnlyStart",
    L"PowershellScriptTest_8wekyb3d8bbwe!PSBothStartingFirst",
    L"PowershellScriptTest_8wekyb3d8bbwe!PSScriptWithArg",
    L"PowershellScriptTest_8wekyb3d8bbwe!PSWaitForScriptToFinish",
    L"DynamicLibraryTest_8wekyb3d8bbwe!Fixed32",
    L"DynamicLibraryTest_8wekyb3d8bbwe!Fixed64",
    L"RegLegacyTest_8wekyb3d8bbwe!Fixed32",
    L"RegLegacyTest_8wekyb3d8bbwe!Fixed64",
    L"EnvVarsATest_8wekyb3d8bbwe!Fixed32",
    L"EnvVarsATest_8wekyb3d8bbwe!Fixed64",
    L"EnvVarsWTest_8wekyb3d8bbwe!Fixed32",
    L"EnvVarsWTest_8wekyb3d8bbwe!Fixed64",
    L"ArgRedirectionTest_8wekyb3d8bbwe!Fixed32",
    L"ArgRedirectionTest_8wekyb3d8bbwe!Fixed64",
    L"PreventBreakAwayTest_8wekyb3d8bbwe!Fixed32",
    L"PreventBreakAwayTest_8wekyb3d8bbwe!Fixed64"
};

bool g_onlyPrintSummary = false;

DWORD CancelIOAndWait(HANDLE file, LPOVERLAPPED overlapped)
{
    if (!::CancelIoEx(file, overlapped))
    {
        return print_last_error("Failed to cancel the I/O operation");
    }

    DWORD unused;
    if (!::GetOverlappedResult(file, overlapped, &unused, true) && (::GetLastError() != ERROR_OPERATION_ABORTED))
    {
        return print_last_error("Failed to wait for the cancelled I/O operation to complete");
    }

    return ERROR_SUCCESS;
}

struct test
{
    std::string name;
    std::int32_t result = ERROR_CANCELLED;
};

struct test_app
{
    std::wstring package_family_name;
    std::string name;
    HRESULT activation_result;
    std::int32_t test_count = 0;
    std::int32_t success_count = 0;
    std::int32_t failure_count = 0;
    // NOTE: Missing tests deduced from (test_count - (success_count + failure_count))

    std::vector<test> tests;
};

struct state
{
    std::vector<test_app> test_apps;

    // NOTE: Pointers (as opposed to '.back()' calls) to identify extraneous messages and/or missing messages
    test_app* active_app = nullptr;
    test* active_test = nullptr;
};

state g_state;

void handle_message(const init_test_message* msg)
{
    if (g_state.active_app)
    {
        // Caller should ensure that apps that don't send a 'cleanup' message are properly cleaned up, so this should
        // imply that the same app sent multiple 'init' messages
        std::wcout << error_text() << "ERROR: Application sent more than one init message\n";
        return;
    }
    assert(!g_state.active_test);

    g_state.active_app = &g_state.test_apps.back();
    g_state.active_app->name = msg->name;
    g_state.active_app->test_count = msg->count;

    if (!g_onlyPrintSummary)
    {
        std::wcout << console::change_foreground(console::color::cyan) <<
            "================================================================================\n" <<
            "Test Initialize\n" <<
            "Test Name:  " << console::change_foreground(console::color::dark_cyan) << msg->name << console::revert_foreground() << "\n" <<
            "Test Count: " << console::change_foreground(console::color::dark_cyan) << msg->count << console::revert_foreground() << "\n" <<
            "================================================================================\n";
    }
}

void cleanup_current_app(bool isForcedCleanup = false)
{
    assert(g_state.active_app);

    if (g_state.active_test)
    {
        if (!isForcedCleanup)
        {
            std::wcout << error_text() << "ERROR: Application sent cleanup message while there was an active test\n";
        }

        // Act like we got a failure message for the test and then continue cleanup as normal
        ++g_state.active_app->failure_count;
        g_state.active_test->result = ERROR_CANCELLED;
        g_state.active_test = nullptr;
    }

    auto app = g_state.active_app;
    auto blockedCount = (app->test_count - (app->success_count + app->failure_count));
    if (blockedCount)
    {
        std::wcout << error_text() << "ERROR: Reported test count does not match the number of test results\n";
    }

    if (!g_onlyPrintSummary)
    {
        std::wcout << console::change_foreground(console::color::cyan) <<
            "================================================================================\n" <<
            "Test Cleanup\n" <<
            "Test Name:  " << console::change_foreground(console::color::dark_cyan) << app->name.c_str() << console::revert_foreground() << "\n" <<
            "Test Count: " << console::change_foreground(console::color::dark_cyan) << app->test_count << console::revert_foreground() << "\n"
            "Succeeded:  " << console::change_foreground(console::color::dark_cyan) << app->success_count << console::revert_foreground() << "\n"
            "Failed:     " << console::change_foreground(console::color::dark_cyan) << app->failure_count << console::revert_foreground() << "\n"
            "Blocked:    " << console::change_foreground(console::color::dark_cyan) << blockedCount << console::revert_foreground() << "\n"
            "================================================================================\n";
    }

    g_state.active_app = nullptr;
}

void handle_message([[maybe_unused]] const cleanup_test_message* msg)
{
    if (!g_state.active_app)
    {
        std::wcout << error_text() << "ERROR: Application either sent multiple cleanup messages or didn't send an init message\n";
        assert(!g_state.active_test);
        return;
    }

    cleanup_current_app();
}

void handle_message(const test_begin_message* msg)
{
    if (!g_state.active_app)
    {
        std::wcout << error_text() << "ERROR: Unexpected test begin message\n";
        std::wcout << error_text() << "ERROR: Name is: " << error_info_text() << msg->name << "\n";
        return;
    }
    else if (g_state.active_test)
    {
        std::wcout << error_text() << "ERROR: Test begin message received while another test was already running\n";
        std::wcout << error_text() << "ERROR: Name is: " << error_info_text() << msg->name << "\n";
        return;
    }

    if (!g_onlyPrintSummary)
    {
        std::wcout << console::change_foreground(console::color::magenta) << "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n";
        std::wcout << "Test Begin: " << info_text() << msg->name << "\n";
        std::wcout << "--------------------------------------------------------------------------------\n";
    }

    g_state.active_app->tests.push_back(test{ msg->name });
    g_state.active_test = &g_state.active_app->tests.back();
}

void handle_message(const test_end_message* msg)
{
    if (!g_state.active_app)
    {
        std::wcout << error_text() << "ERROR: Unexpected test end message\n";
        return;
    }
    else if (!g_state.active_test)
    {
        std::wcout << error_text() << "ERROR: Test end message received while no test was in progress\n";
        return;
    }

    if (!g_onlyPrintSummary)
    {
        std::wcout << "--------------------------------------------------------------------------------\n";
        std::wcout << "Test End\n" << "Result: ";
    }

    if (msg->result)
    {
        ++g_state.active_app->failure_count;
        if (!g_onlyPrintSummary)
        {
            std::wcout << error_text() << "FAILED\n";
            std::wcout << "Error Code: " << error_text() << msg->result;
            std::wcout << " (" << error_info_text() << error_message(msg->result).c_str() << console::revert_foreground() << ")\n";
        }
    }
    else
    {
        ++g_state.active_app->success_count;
        if (!g_onlyPrintSummary)
        {
            std::wcout << success_text() << "SUCCESS\n";
        }
    }

    if (!g_onlyPrintSummary)
    {
        std::wcout << console::change_foreground(console::color::magenta) << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n";
    }

    g_state.active_test->result = msg->result;
    g_state.active_test = nullptr;
}

void handle_message(const test_trace_message* msg)
{
    // We shouldn't be trying to trace output while a test isn't running
    if (!g_state.active_test || g_onlyPrintSummary)
    {
        return;
    }

    std::wcout << console::change_foreground(msg->color) << msg->text.get();
    if (msg->print_new_line)
    {
        std::wcout << L"\n";
    }
}

int wmain(int argc, const wchar_t** argv)
{
    // Display UTF-16 correctly...
    // NOTE: The CRT will assert if we try and use 'cout' with this set
    _setmode(_fileno(stdout), _O_U16TEXT);

    for (int i = 1; i < argc; ++i)
    {
        if (argv[i] == L"/onlyPrintSummary"sv)
        {
            g_onlyPrintSummary = true;
        }
        else
        {
            std::wcout << error_text() << "ERROR: Unknown argument: " << error_info_text() << argv[i] << "\n";
            return ERROR_INVALID_PARAMETER;
        }
    }

    if (auto hr = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED); FAILED(hr))
    {
        return print_error(hr, "COM Initialization failed");
    }

    ComPtr<IApplicationActivationManager> activationManager;
    if (auto hr = ::CoCreateInstance(CLSID_ApplicationActivationManager, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&activationManager));
        FAILED(hr))
    {
        return print_error(hr, "Failed to activate ApplicationActivationManager");
    }

    message_pipe pipe;
    HANDLE waitArray[2] = { pipe.wait_handle() };

    for (auto& aumid : g_applications)
    {
        if (!g_onlyPrintSummary)
        {
            std::wcout << "\nLaunching: " << info_text() << aumid << "\n";
        }

        g_state.test_apps.emplace_back();
        auto& currentApp = g_state.test_apps.back();
        currentApp.package_family_name.assign(aumid, std::wcschr(aumid, L'!'));

        DWORD pid;
        currentApp.activation_result = activationManager->ActivateApplication(aumid, L"/mode:test", AO_NONE, &pid);
        if (FAILED(currentApp.activation_result))
        {
            print_error(currentApp.activation_result, "Failed to activate application");
        }
        else
        {
            if (!g_onlyPrintSummary)
            {
                std::wcout << "Process created with process id: " << info_text() << pid << "\n";
            }

            unique_handle process(::OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION, false, pid));
            if (!process)
            {
                return print_last_error("Failed to open process handle");
            }

            // NOTE: WaitForMultipleObjects will return the index of the first signalled handle in the array, so the
            //       process handle must be last so that we process all data it sends back before continuing
            waitArray[1] = process.get();

            while (true)
            {
                auto waitResult = ::WaitForMultipleObjects(
                    static_cast<DWORD>(std::size(waitArray)),
                    waitArray,
                    false,
                    INFINITE);
                if ((waitResult >= WAIT_OBJECT_0) && (waitResult < WAIT_ABANDONED_0))
                {
                    auto index = waitResult - WAIT_OBJECT_0;
                    assert(index < std::size(waitArray));

                    if (index == 1)
                    {
                        // Process terminated
                        break;
                    }
                    else
                    {
                        pipe.on_signalled([](const test_message* msg)
                        {
                            switch (msg->type)
                            {
                            case test_message_type::init:
                                handle_message(reinterpret_cast<const init_test_message*>(msg));
                                break;

                            case test_message_type::cleanup:
                                handle_message(reinterpret_cast<const cleanup_test_message*>(msg));
                                break;

                            case test_message_type::begin:
                                handle_message(reinterpret_cast<const test_begin_message*>(msg));
                                break;

                            case test_message_type::end:
                                handle_message(reinterpret_cast<const test_end_message*>(msg));
                                break;

                            case test_message_type::trace:
                                handle_message(reinterpret_cast<const test_trace_message*>(msg));
                                break;

                            default:
                                assert(false);
                            }
                        });
                    }
                }
                else if (waitResult == WAIT_FAILED)
                {
                    return print_last_error("Failed to wait for process to send data or exit");
                }
                else
                {
                    assert(false);
                    std::wcout << error_text() << "ERROR: Unexpected failure waiting for process to send data or exit\n";
                    return ERROR_ASSERTION_FAILURE;
                }
            }

            assert(pipe.state() != pipe_state::connected);

            DWORD exitCode;
            if (!::GetExitCodeProcess(process.get(), &exitCode))
            {
                return print_last_error("Failed to get process exit code");
            }

            if (!currentApp.test_count)
            {
                assert(!g_state.active_app);
                std::wcout << error_text() << "ERROR: Application terminated without sending an init message\n";
            }
            else if (g_state.active_app)
            {
                std::wcout << error_text() << "ERROR: Application terminated without sending a cleanup message\n";
                cleanup_current_app(true);
            }

            if (!g_onlyPrintSummary)
            {
                std::wcout << "Process exited with code: " << info_text() << exitCode << "\n";
            }

            waitArray[1] = nullptr;
        }

        if (!g_onlyPrintSummary)
        {
            std::wcout << "\n\n";
        }
    }

    std::wcout << console::change_foreground(console::color::cyan) <<
        "********************************************************************************\n"
        "******************************      Summary       ******************************\n"
        "********************************************************************************\n";
    int failureCount = 0;
    for (auto& testApp : g_state.test_apps)
    {
        auto succeeded = SUCCEEDED(testApp.activation_result) && testApp.test_count && (testApp.success_count == testApp.test_count);
        auto blockedCount = testApp.test_count - (testApp.success_count + testApp.failure_count);

        std::wcout << console::change_foreground(console::color::cyan) << "Package Family Name: " <<
            console::change_foreground(console::color::dark_cyan) << testApp.package_family_name << "\n";

        std::wcout << console::change_foreground(console::color::cyan) << "             Result: ";
        if (succeeded)
        {
            std::wcout << success_text() << "SUCCEEDED!\n";
        }
        else
        {
            std::wcout << error_text() << "FAILED!\n";
        }

        if (testApp.test_count)
        {
            std::wcout << console::change_foreground(console::color::cyan) << "        Total Count: " <<
                console::change_foreground(console::color::dark_cyan) << testApp.test_count << "\n";
            std::wcout << console::change_foreground(console::color::cyan) << "            Success: " <<
                console::change_foreground(console::color::dark_cyan) << testApp.success_count << "\n";
            std::wcout << console::change_foreground(console::color::cyan) << "            Failure: " <<
                console::change_foreground(console::color::dark_cyan) << testApp.failure_count << "\n";
            std::wcout << console::change_foreground(console::color::cyan) << "            Blocked: " <<
                console::change_foreground(console::color::dark_cyan) << blockedCount << "\n";
        }

        if (!succeeded)
        {
            std::wcout << console::change_foreground(console::color::cyan) << "             Reason: ";
            if (FAILED(testApp.activation_result))
            {
                ++failureCount; // Don't know how many tests there were, so just assume one
                std::wcout << console::change_foreground(console::color::dark_cyan) << "Application failed to activate\n";
                std::wcout << console::change_foreground(console::color::cyan) << "              Error: " <<
                    console::change_foreground(console::color::dark_cyan) << error_message(testApp.activation_result).c_str() << "\n";
            }
            else if (!testApp.test_count)
            {
                ++failureCount; // Don't know how many tests there were, so just assume one
                std::wcout << console::change_foreground(console::color::dark_cyan) << "Application failed to communicate test results\n";
            }
            else
            {
                failureCount += testApp.test_count - testApp.success_count;
                std::wcout << console::change_foreground(console::color::dark_cyan) <<
                    (blockedCount ? "The test terminated unexpectedly" : "Test had failures") << "\n";

                if (testApp.failure_count)
                {
                    std::wcout << console::change_foreground(console::color::cyan) << "      Failing Tests:";

                    const char* prefix = " ";
                    for (auto& test : testApp.tests)
                    {
                        if (test.result)
                        {
                            std::wcout << console::change_foreground(console::color::dark_cyan) << prefix <<
                                test.name.c_str() << " [" << error_text() << error_message(test.result).c_str() <<
                                console::revert_foreground() << "]\n";
                            prefix = "                     ";
                        }
                    }
                }
            }
        }

        std::wcout << "\n";
    }

    std::wcout << "Overall Result: ";
    if (!failureCount)
    {
        std::wcout << success_text() << "SUCCEEDED!\n";
    }
    else
    {
        std::wcout << error_text() << L"FAILED!\n\n";
    }

    std::wcout << "\n\n";

    return failureCount;
}
