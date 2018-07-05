
#include <algorithm>
#include <sstream>

#include <detour_transaction.h>
#include <shim_framework.h>
#include <shim_runtime.h>

#include "Config.h"

struct loaded_shim
{
    HMODULE module_handle = nullptr;
    ShimUninitializeProc uninitialize = nullptr;

    loaded_shim() = default;
    loaded_shim(const loaded_shim&) = delete;
    loaded_shim& operator=(const loaded_shim&) = delete;

    loaded_shim(loaded_shim&& other) noexcept
    {
        swap(other);
    }

    loaded_shim& operator=(loaded_shim&& other)
    {
        swap(other);
    }

    ~loaded_shim()
    {
        if (module_handle)
        {
            [[maybe_unused]] auto result = ::FreeLibrary(module_handle);
            assert(result);
        }
    }

    void swap(loaded_shim& other)
    {
        std::swap(module_handle, other.module_handle);
        std::swap(uninitialize, other.uninitialize);
    }
};
std::vector<loaded_shim> loaded_shims;

void load_shims()
{
    using namespace std::literals;

    if (auto config = ShimQueryCurrentExeConfig())
    {
        if (auto shims = config->try_get("shims"))
        {
            for (auto& shimConfig : shims->as_array())
            {
                auto& shim = loaded_shims.emplace_back();

                auto path = PackageRootPath() / shimConfig.as_object().get("dll").as_string().wide();
                shim.module_handle = ::LoadLibraryW(path.c_str());
                if (!shim.module_handle)
                {
                    path.replace_extension();
                    path.concat((sizeof(void*) == 4) ? L"32.dll" : L"64.dll");
                    shim.module_handle = ::LoadLibraryW(path.c_str());

                    if (!shim.module_handle)
                    {
                        auto message = narrow(path.c_str());
                        throw_last_error(message.c_str());
                    }
                }

                auto initialize = reinterpret_cast<ShimInitializeProc>(::GetProcAddress(shim.module_handle, "ShimInitialize"));
                if (!initialize)
                {
                    auto message = "ShimInitialize export not found in "s + narrow(path.c_str());
                    throw_win32(ERROR_PROC_NOT_FOUND, message.c_str());
                }
                auto uninitialize = reinterpret_cast<ShimUninitializeProc>(::GetProcAddress(shim.module_handle, "ShimUninitialize"));
                if (!uninitialize)
                {
                    auto message = "ShimUninitialize export not found in "s + narrow(path.c_str());
                    throw_win32(ERROR_PROC_NOT_FOUND, message.c_str());
                }

                auto transaction = detours::transaction();
                check_win32(::DetourUpdateThread(::GetCurrentThread()));

                // Only set the uninitialize pointer if the transaction commits successfully since that's our cue to clean
                // it up, which will attempt to call DetourDetach
                check_win32(initialize());
                transaction.commit();
                shim.uninitialize = uninitialize;
            }
        }
    }
}

void unload_shims()
{
    while (!loaded_shims.empty())
    {
        auto transaction = detours::transaction();
        check_win32(::DetourUpdateThread(::GetCurrentThread()));

        if (loaded_shims.back().uninitialize)
        {
            [[maybe_unused]] auto result = loaded_shims.back().uninitialize();
            assert(result == ERROR_SUCCESS);
        }

        transaction.commit();
        loaded_shims.pop_back();
    }
}

using EntryPoint_t = int (__stdcall*)();
EntryPoint_t ApplicationEntryPoint = nullptr;
static int __stdcall ShimmedEntryPoint() noexcept try
{
    load_shims();
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

    // Call DetourAttach for all APIs that ShimRuntime detours
    shims::attach_all();

    // We can't call LoadLibrary in DllMain, so hook the application's entry point and do initialization then
    ApplicationEntryPoint = reinterpret_cast<EntryPoint_t>(::DetourGetEntryPoint(nullptr));
    if (!ApplicationEntryPoint)
    {
        throw_last_error();
    }
    check_win32(::DetourAttach(reinterpret_cast<void**>(&ApplicationEntryPoint), ShimmedEntryPoint));

    transaction.commit();
}

void detach()
{
    // Unload in the reverse order as we initialized
    unload_shims();

    auto transaction = detours::transaction();
    check_win32(::DetourUpdateThread(::GetCurrentThread()));
    shims::detach_all();
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
            unload_shims();
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
    std::wostringstream ss;
    auto error_message{ widen(message_from_caught_exception()) };
    auto error_code{ win32_from_caught_exception() };
    ss << error_message << " (" << std::hex << error_code << ")";
    ::ShimReportError(ss.str().c_str());
    ::SetLastError(error_code);
    return FALSE;
}
