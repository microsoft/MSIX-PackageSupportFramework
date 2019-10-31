//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <algorithm>
#include <sstream>

#include <detour_transaction.h>
#include <psf_framework.h>
#include <psf_runtime.h>

#include "Config.h"

void Log(const char* fmt, ...);

struct loaded_fixup
{
    HMODULE module_handle = nullptr;
    PSFUninitializeProc uninitialize = nullptr;

    loaded_fixup() = default;
    loaded_fixup(const loaded_fixup&) = delete;
    loaded_fixup& operator=(const loaded_fixup&) = delete;

    loaded_fixup(loaded_fixup&& other) noexcept
    {
        swap(other);
    }

    loaded_fixup& operator=(loaded_fixup&& other) noexcept
    {
        swap(other);
    }

    ~loaded_fixup()
    {
        if (module_handle)
        {
            [[maybe_unused]] auto result = ::FreeLibrary(module_handle);
            assert(result);
        }
    }

    void swap(loaded_fixup& other)
    {
        std::swap(module_handle, other.module_handle);
        std::swap(uninitialize, other.uninitialize);
    }
};
std::vector<loaded_fixup> loaded_fixups;

void load_fixups()
{
    using namespace std::literals;

    if (auto config = PSFQueryCurrentExeConfig())
    {
        if (auto fixups = config->try_get("fixups"))
        {
            for (auto& fixupConfig : fixups->as_array())
            {
                auto& fixup = loaded_fixups.emplace_back();

                auto path = PackageRootPath() / fixupConfig.as_object().get("dll").as_string().wide();
                fixup.module_handle = ::LoadLibraryW(path.c_str());
                if (!fixup.module_handle)
                {
                    path.replace_extension();
                    path.concat((sizeof(void*) == 4) ? L"32.dll" : L"64.dll");
                    fixup.module_handle = ::LoadLibraryW(path.c_str());

                    if (!fixup.module_handle)
                    {
                        auto message = narrow(path.c_str());
                        throw_last_error(message.c_str());
                    }
                }
                Log("\tInject into current process: %ls\n", path.c_str());

                auto initialize = reinterpret_cast<PSFInitializeProc>(::GetProcAddress(fixup.module_handle, "PSFInitialize"));
                if (!initialize)
                {
                    auto message = "PSFInitialize export not found in "s + narrow(path.c_str());
                    throw_win32(ERROR_PROC_NOT_FOUND, message.c_str());
                }
                auto uninitialize = reinterpret_cast<PSFUninitializeProc>(::GetProcAddress(fixup.module_handle, "PSFUninitialize"));
                if (!uninitialize)
                {
                    auto message = "PSFUninitialize export not found in "s + narrow(path.c_str());
                    throw_win32(ERROR_PROC_NOT_FOUND, message.c_str());
                }

                auto transaction = detours::transaction();
                check_win32(::DetourUpdateThread(::GetCurrentThread()));

                // Only set the uninitialize pointer if the transaction commits successfully since that's our cue to clean
                // it up, which will attempt to call DetourDetach
                check_win32(initialize());
                transaction.commit();
                fixup.uninitialize = uninitialize;
            }
        }
    }
}

void unload_fixups()
{
    while (!loaded_fixups.empty())
    {
        auto transaction = detours::transaction();
        check_win32(::DetourUpdateThread(::GetCurrentThread()));

        if (loaded_fixups.back().uninitialize)
        {
            [[maybe_unused]] auto result = loaded_fixups.back().uninitialize();
            assert(result == ERROR_SUCCESS);
        }

        transaction.commit();
        loaded_fixups.pop_back();
    }
}

using EntryPoint_t = int (__stdcall*)();
EntryPoint_t ApplicationEntryPoint = nullptr;
static int __stdcall FixupEntryPoint() noexcept try
{
    load_fixups();
    return ApplicationEntryPoint();
}
catch (...)
{
    return win32_from_caught_exception();
}

void attach()
{
    LoadConfig();

    // Restore the contents of the in memory import table that DetourCreateProcessWithDll* modified
    ::DetourRestoreAfterWith();

    auto transaction = detours::transaction();
    check_win32(::DetourUpdateThread(::GetCurrentThread()));

    // Call DetourAttach for all APIs that PsfRuntime detours
    psf::attach_all();

    // We can't call LoadLibrary in DllMain, so hook the application's entry point and do initialization then
    ApplicationEntryPoint = reinterpret_cast<EntryPoint_t>(::DetourGetEntryPoint(nullptr));
    if (!ApplicationEntryPoint)
    {
        throw_last_error();
    }
    check_win32(::DetourAttach(reinterpret_cast<void**>(&ApplicationEntryPoint), FixupEntryPoint));

    transaction.commit();
}

void detach()
{
    // Unload in the reverse order as we initialized
    unload_fixups();

    auto transaction = detours::transaction();
    check_win32(::DetourUpdateThread(::GetCurrentThread()));
    psf::detach_all();
    transaction.commit();
}

BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID) noexcept try
{
    // Per detours documentation, immediately return true if running in a helper process
    if (::DetourIsHelperProcess())
    {
        return TRUE;
    }

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        try
        {
            attach();
        }
        catch (...)
        {
            unload_fixups();
            throw;
        }
        break;

    case DLL_PROCESS_DETACH:
        detach();
        break;
    }

    return TRUE;
}
catch (...)
{
    ::PSFReportError(widen(message_from_caught_exception()).c_str());
    ::SetLastError(win32_from_caught_exception());
    return false;
}
