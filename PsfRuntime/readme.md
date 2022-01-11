# PSF Runtime
The PSF Runtime serves several purposes. The first is that it is responsible for detouring `CreateProcess` to ensure that any child process will also get the PSF Runtime injected into it. Additionally, it is responsible for loading and parsing the `config.json` DOM as well as loading any configured fixup dlls for the current executable. Finally, it exposes a set of utility functions collectively referred to as the "PSF Framework" for use by the individual fixup dlls. This includes helpers for interop with the Detours library, functions for querying information about the current package/app id, and a set of functions for querying information from the `config.json` DOM. See [psf_runtime.h](../include/psf_runtime.h) for a more complete idea of this API surface as well as [psf_config.h](../include/psf_config.h) for an idea of how the JSON data is exposed.

## Processes to be fixed up
To ensure that all processes that require fixups are handled, PsfRuntime intercepts the processes creation API CreateProcess.  
Ultimately, this covers cases of calls to CreateProcess by the PsfLauncher, 
but also CreateProcess, StartProcess, and some of the managed code and private implementions by the target application that must eventually call this API.

Generally, we want all such child processes to run inside the container, however there is an exception
for the case of a CONHOST process which may not.
Currently:
> * Most child processes will run inside the container by default and do not require fixing.
> * Some child processes do not supply the Extended information to request whether to run inside or not.  Except for excluded processes (Conhost currently)
> * we will add the extended information to the request to ensure it runs inside the container.
> * Some child processes do supply extended information, but don't request either inside or outside. These are currently not fixed up but should run inside.
> * Some child processes do supply extended information specifying either inside or outside.  These are also not adjusted currently.

## Fixup Loading
As mentioned above, one of the major responsibilities that the PSF Runtime has is loading the fixups that are configured for the current executable. E.g. for a configuration that looks something like:

```json
{
    "processes": [
        {
            "executable": "ContosoApp",
            "fixups": [
                {
                    "dll": "ContosoFixup.dll"
                }
            ]
        }
    ]
}
```

```xml
<processes>
    <process>
        <executable>ContosoApp</executable>
        <fixups>
            <fixup>
                <dll>ContosoFixup.dll</dll>
            </fixup>
        </fixups>
    </process>
</processes>
```

The PSF Runtime will load `ContosoFixup.dll` if the current executable name matches the regular expression `"ContosoApp"` (i.e. it must be an exact match for this example). Note that this pattern is missing the `".exe"` suffix. This is done because periods in regular expressions must be escaped, which in JSON requires a double back-slash to do so (i.e. `"name\\.exe"`), which complicates the configuration and harms readability.

When enumerating the list of fixup dll names, PSF Runtime will make two attempts to load the dll, each made relative to the package root:

> 1. First using the name exactly as it is given (e.g. `"ContosoFixup.dll"` in the example above)
> 1. If that fails, then it appends the current architecture bitness (`32` or `64`) to the end of the dll name (e.g. `"ContosoFixup64.dll"` if the current process is a 64-bit process) and attempts to load that

This is done to support applications that contain both 32 and 64-bit executables, but need the same fixups applied to both. This is why all of the inbox fixups produce binaries named `_____32.dll` and `_____64.dll` by default. This naming convention also carries over to executables to prevent naming conflicts.

> _NOTE: This naming scheme and functionality is taken from the Detours library, which has similar load/fallback logic, to be consistent_

Assuming the dll loads, the PSF Runtime calls `GetProcAddress`, expecting the two exports `PSFInitialize` and `PSFUninitialize`, treating failure the same as if the dll failed to load. Both functions are assumed to have the signature:

```c++
int (__stdcall *)() noexcept`
```

The PSF Runtime then calls `PSFInitialize` within a Detours transaction, failing out if the return value is non-zero (i.e. not `ERROR_SUCCESS`). Within the execution of `PSFInitialize`, the fixup dll is free to call `PSFRegister`, which in turn calls `DetourAttach`. Calling `PSFRegister` at any other time will fail. When the initialize procedure returns, the transaction is completed, committing any function detours set up by the fixup dll. When the PSF Runtime dll is being unloaded, it will enumerate the set of loaded fixups _in reverse order_, calling `PSFUninitialize`. At this point in time, the fixup dll is expected to call `PSFUnregister` for every prior call it made to `PSFRegister` (which calls `DetourDetach`) before getting unloaded to avoid later attempts to call back into an unloaded dll.

> **IMPORTANT: The exported names must _exactly_ match `PSFInitialize` and `PSFUninitialize`. This isn't automatic when using `__declspec(dllexport)` due to the "mangling" performed for 32-bit binaries**

> TIP: In most cases you can leverage the `PSF_DEFINE_EXPORTS` macro to define/export these functions for you with the correct names. See [here](../Authoring.md#fixup-loading) for more information

## Runtime Requirements
As a part of its initialization, the PSF Runtime queries information about its environment that it then caches for later use. A few examples include parsing the `config.json`, caching the path to the package root, and caching the package name, among a couple other things. If any of these steps fail, e.g. because something is not present/cannot be found or any other failure, then the PSF Runtime dll will fail to load, which likely means that the process fails to start. Note that this implies the requirement that the application be running with package identity. There have been past conversations on adding support for a "debug" mode that works around this restriction (e.g. by using a fake package name, executable directory as the package root, etc.), but its benefit is questionable and has not yet been implemented.
