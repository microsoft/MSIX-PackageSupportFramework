## Package Layout
The Package Support Framework makes some assumptions about which files are present in the package, their location in the package, and what their file names are. These requirements are:

| File Name | Requirements |
| --------- | ------------ |
| ShimLauncher32.exe<br>ShimLauncher64.exe | This is the entry point to the application that appears in the AppxManifest. There is no naming requirement imposed on it and the only path requirement is that it be able to find ShimRuntimeXX.dll in its dll search path. In fact, you can relatively easily replace this executable with your own if you wish. In general, it is suggested that you match this executable's architecture with that of the target executable to avoid unnecessary extra work. You can find more information on [MSDN](https://docs.microsoft.com/windows/uwp/porting/package-support-framework#create-a-configuration-file) |
| ShimRuntime32.dll<br>ShimRuntime64.exe | This _must_ be named either `ShimRuntime32.dll` or `ShimRuntime64.dll` (depending on architecture), and _must_ be located at the package root. For more information on this dll, you can find its documentation [here](ShimRuntime/readme.md) |
| ShimRunDll32.exe<br>ShimRunDll64.exe | Its presence is only required if cross-architecture launches are a possibility. Otherwise, it _must_ be named either `ShimRunDll32.exe` or `ShimRunDll64.exe` (depending on architecture), and _must_ be located at the package root. For more information on this executable, you can find its documentation [here](ShimRunDll/readme.md) |
| config.json | The configuration file _must_ be named `config.json` and _must_ be located at the package root. For more information, see [the documentation on MSDN](https://docs.microsoft.com/windows/uwp/porting/package-support-framework#create-a-configuration-file) |
| Shim dlls | There is no naming or path requirement for the individual shim dlls, although they must also be able to find `ShimRuntimeXX.dll` in their dll search paths. It is also suggested that the name end with either `32` or `64` (more information can be found [here](ShimRuntime/readme.md#shim-loading)) |

In general, it's probably safest/easiest to place all Package Support Framework related files and binaries directly under the package root.
