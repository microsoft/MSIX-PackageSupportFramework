# Shim Runtime
The Shim Runtime serves several purposes. The first is that it is responsible for detouring `CreateProcess` to ensure that any child process will also get the Shim Runtime injected into it. Additionally, it is responsible for loading and parsing the `config.json` DOM as well as loading any configured shim dlls for the current executable. Finally, it exposes a set of utility functions collectively referred to as the "Shim Framework" for use by the individual shim dlls. This includes helpers for interop with the Detours library, functions for querying information about the current package/app id, and a set of functions for querying information from the `config.json` DOM. See [shim_runtime.h](../include/shim_runtime.h) for a more complete idea of this API surface as well as [shim_config.h](../include/shim_config.h) for an idea of how the JSON data is exposed.

## Shim Loading
As mentioned above, one of the major responsibilities that the Shim Runtime has is loading the shims that are configured for the current executable. E.g. for a configuration that looks something like:

```json
{
    "processes": [
        {
            "executable": "ContosoApp",
            "shims": [
                {
                    "dll": "ContosoShim.dll"
                }
            ]
        }
    ]
}
```

The Shim Runtime will load `ContosoShim.dll` if the current executable name matches the regular expression `"ContosoApp"` (i.e. it must be an exact match for this example). Note that this pattern is missing the `".exe"` suffix. This is done because periods in regular expressions must be escaped, which in JSON requires a double back-slash to do so (i.e. `"name\\.exe"`), which complicates the configuration and harms readability.

When enumerating the list of shim dll names, Shim Runtime will make two attempts to load the dll, each made relative to the package root:

1. First using the name exactly as it is given (i.e. `"ContosoShim.dll"` in the example above)
1. If that fails, then it appends the current architecture bitness (`32` or `64`) to the end of the dll name (e.g. `"ContosoShim64.dll"` if the current process is a 64-bit process) and attempts to load that

This is done to support applications that contain both 32 and 64-bit executables, but need the same shims applied to both. This is why all of the inbox shims produce binaries named `_____32.dll` and `_____64.dll` by default. This naming convention also carries over to executables to prevent naming conflicts.

_NOTE: This naming scheme and functionality is taken from the Detours library, which has similar load/fallback logic, to be consistent_

Assuming the dll loads, the Shim Runtime calls `GetProcAddress`, expecting the two exports `ShimInitialize` and `ShimUninitialize`, treating failure the same as if the dll failed to load. Both functions are assumed to have the signature `int (__stdcall *)() noexcept`. It then calls `ShimInitialize` within a Detours transaction, failing out if the return value is non-zero (i.e. not `ERROR_SUCCESS`). Within the execution of `ShimInitialize`, the shim dll is free to call `ShimRegister`, which in turn calls `DetourAttach`. Calling `ShimRegister` at any other time will fail. When the initialize procedure returns, the transaction is completed, committing any function detours set up by the shim dll. When the Shim Runtime dll is being unloaded, it will enumerate the set of loaded shims _in reverse order_, calling `ShimUninitialize`. At this point in time, the shim dll is expected to call `ShimUnregister` for every prior call it made to `ShimRegister` (which calls `DetourDetach`) before getting unloaded to avoid later attempts to call back into an unloaded dll.

**NOTE: The exported names must _exactly_ match `ShimInitialize` and `ShimUninitialize`. This isn't automatic when using `__declspec(dllexport)` due to the "mangling" performed for 32-bit binaries**

If your shim dll is making use of the `DECLARE_SHIM`/`DECLARE_STRING_SHIM` macros, you can `#define SHIM_DEFINE_EXPORTS` before `#include`'ing `shim_framework.h` _in a single file_, which will define both of these functions for you as well as take care of the mangling issue.

## Runtime Requirements
As a part of its initialization, the Shim Runtime queries information about its environment that it then caches for later use. A few examples include parsing the `config.json`, caching the path to the package root, and caching the package name, among a couple other things. If any of these steps fail, e.g. because something is not present/cannot be found or any other failure, then the Shim Runtime dll will fail to load, which likely means that the process fails to start. Note that this implies the requirement that the application be running with package identity. There have been past conversations on adding support for a "debug" mode that works around this restriction (e.g. by using a fake package name, executable directory as the package root, etc.), but its benefit is questionable and has not yet been implemented. See [TODO](#TODO) for more information.
