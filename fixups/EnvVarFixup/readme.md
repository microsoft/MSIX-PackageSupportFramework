# DynamicLibrary Fixup
When injected into a process, the DynamicLibraryFixup supports the ability to:
> * Define mappings between dll name and location within the package in the Config.Json file.
> * Ensure that the named dll will be found within the package anytime the application tries to load the dll.

There are a bunch of reasons that the traditional ways of apps locating their dlls fail under MSIX.
Although there are specific fixes for many of these issues, this fixup creates a sure-fire way to get it loaded.

## About Debugging this fixup
The Release build of this fixup produces no output to the debug console port for performance reasons.
Use of the Debug build will enable you to see the intercepts and what the fixup did.
That output is easily seen using the Sysinternals "DebugView" tool.

## Configuration
The configuration for the EnvVarFixup is specified under the element `config` of the fixup structure within the json file when EnvVarFixup.dll is requested.
This `config` element contains a an array called "EnvVars".  Each Envar contains three values:

| PropertyName | Description |
| ------------ | ----------- |
| `forcePackageDllUse` | Boolean.  Set to true.|
| `relativeDllPaths` | An array. See below. |

Each element of the array has the following structure:

| PropertyName | Description |
| ------------ | ----------- |
| `name`| This is the name as requested by the application. This will be the name of the file, without any path information and without the filename extension.|
| `filepath`| The filepath relative to the root folder of the package. |


# JSON Examples
To make things simpler to understand, here is a potential example configuration object that is not using the optional parameters:

```json
"config": {
    "forcePackageDllUse": "true",
    "relativeDllPaths": [
        {
            "name" : "DllName_without_DotDll",
            "filepath" : "RelativePathToFile_including_DllName_without_DotDll.dll"
        },
        {
            "name" : "DllName2",
            "filepath" : "VFS\ProgramFilesX84\Vendor\App\Subfolder\DllName2.dll"
        }
        ,
        {
            "name" : "DllNameX",
            "filepath" : "filepathX"
        }
    ]
}
```

