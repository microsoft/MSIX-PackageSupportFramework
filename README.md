# Package Support Framework

This project provides tools, libraries, documentation and samples for creating app-compat fixups to enable classic Win32 applications to be packaged for distribution and execution as Microsoft Store apps.  For more information on creating and applying fixups, please see the complete [Microsoft Desktop UWP Fixup](https://docs.microsoft.com/windows/uwp/porting/desktop-to-uwp-fix) documentation.

## Package Layout
The Package Support Framework makes some assumptions about which files are present in the package, their location in the package, and what their file names are. These requirements are:

* `ShimLauncherXX.exe` - This is the entry point to the application that appears in the AppxManifest. There is no naming requirement imposed on it and the only path requirement is that it be able to find `ShimRuntimeXX.dll` in its dll search path. In fact, you can relatively easily replace this executable with your own if you wish. In general, it is suggested that you match this executable's architecture with that of the target executable to avoid unnecessary extra work. For more information on this executable, you can find its documentation [here](ShimLauncher/readme.md)
* `ShimRuntimeXX.dll` - This _must_ be named either `ShimRuntime32.dll` or `ShimRuntime64.dll` (depending on architecture), and _must_ be located at the package root. For more information on this dll, you can find its documentation [here](ShimRuntime/readme.md)
* `ShimRunDllXX.exe` - Its presence is only required if cross-architecture launches are a possibility. Otherwise, it _must_ be named either `ShimRunDll32.exe` or `ShimRunDll64.exe` (depending on architecture), and _must_ be located at the package root. For more information on this executable, you can find its documentation [here](ShimRunDll/readme.md)
* `config.json` - The configuration file _must_ be named `config.json` and _must_ be located at the package root. For more information, see [below](#JSON%20Configuration)
* Shim dlls - There is no naming or path requirement for the individual shim dlls, although they must also be able to find `ShimRuntimeXX.dll` in their dll search paths. It is also suggested that the name end with either `32` or `64` (more information can be found [here](ShimRuntime/readme.md#Shim%20Loading))

In general, it's probably safest/easiest to place all Package Support Framework related files and binaries directly under the package root.

## JSON Configuration
The `config.json` file serves two major purposes: instruct the Shim Launcher on which executable to launch for the current application id, and instruct the Shim Runtime on which shims to load for the current executable. These two roles are reflected as the two top level properties of the root object in the file:

* `applications` - This is an `array` whose elements are `object`s describing a mapping from app id to executable/working directory. The properties of each of these `object`s are:
  * `id` - The application id that this configuration applies to (the `Id` attribute of the `Application` tag in the AppxManifest). This is expected to be a value of type `string`
  * `executable` - The package-relative path to the executable that should get launched for the specified application id. This is expected to be a value of type `string`
  * `workingDirectory` - An optional `string` that specifies a package-relative path to use as the working directory for the launched application (typically set to be the directory of the executable). When this value is not present, the working directory is left unchanged (i.e. System32)
* `processes` - This is an `array` whose elements are `object`s describing a mapping from executable name to a set of shims that should get loaded for that executable. Note that the order of this array _is_ important: if more than one element matches the current executable, the first one "wins." The properties of each of these `object`s are:
  * `executable` - This is a `string` regular expression pattern that gets compared against the current executable's name (without the `.exe` suffix). If it matches, then the shim dlls listed in that object are loaded by the Shim Runtime.
  * `shims` - This is an `array` whose elements are `object`s that describe the dlls to load and the configuration to use for that dll. The properties of each of these `object`s are:
    * `dll` - The package-relative path to the shim dll to load. This is expected to be a value of type `string`
    * `config` - An optional value (typically an `object`) that's used to control how the shim behaves. The exact format and specifications of this value vary on a shim-by-shim basis as each shim is free to interpret this "blob" as it wishes. Note that this configuration is specific to the particular executable specified. That is, if you specify the same shim dll for multiple executables but wish to have the same shim configuration for all of them, you will need to copy the `config` between all of them.

An example `config.json` file might look like:

```json
{
    "applications": [
        {
            "id": "ContosoApp",
            "executable": "VFS\\ProgramFilesX64\\Contoso\\App.exe",
            "workingDirectory": "VFS\\ProgramFilesX64\\Contoso\\"
        }
    ],
    "processes": [
        {
            "executable": "ContosoApp",
            "shims": [
                {
                    "dll": "ContosoShim.dll",
                    "config": {
                        "enableFileRedirection": true
                    }
                }
            ]
        }
    ]
}
```

## Creating a New Shim Dll
General instructions for authoring your own shim dll can be found [here](Authoring.md).

## Testing/Debugging
Testing the Package Support Framework can often be fairly tricky. Not only does the application need to run with package identity (i.e. not a standalone executable), but it also needs to run with all of the full trust container restrictions in place (e.g. such as filesystem ACLs) to ensure that operations aren't succeeding when they otherwise wouldn't. This means that, in order to test the full end-to-end scenario, applications need to be packaged, signed, and installed. This can be quite tedious; you can find helper scripts for [taking package ownership](tests/scripts/TakePackageOwnership.ps1), [replacing files inside of a package](tests/scripts/ReplacePackageFiles.ps1), and [replacing binaries inside of a package with their locally built equivalent](tests/scripts/ReplacePackageShimBinaries.ps1) in this repository.

There are also several binaries included in this repository that leverage the shim framework that are intended to aid in debugging. The first is the [Wait for Debugger Shim](tests/shims/WaitForDebuggerShim/readme.md) that does exactly what it sound like: when present, it will hold the process at dll load until a debugger is attached to the process. The second is the [Trace Shim](tests/shims/TraceShim/readme.md), whose primary purpose is to detour a number of functions, conditionally (based off configuration) logging the call, its arguments, and its result. This is primarily useful when (1) trying to understand why an application is experiencing problems, and/or (2) identifying which function(s) need to get shimmed in order to get the application to work.
