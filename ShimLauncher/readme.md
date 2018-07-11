# Shim Launcher
The Shim Launcher executable is provided to be the entry point to the target application whose sole purpose is to ensure that the Shim Runtime gets loaded into the target process. The Shim Launcher executable does little more than read from `config.json` when launched to determine which executable to launch, based off the current application id. For example, an AppxManifest that initially looks something like:

```xml
<Package ...>
  ...
  <Applications>
    <Application Id="ContosoApp"
                 Executable="VFS\ProgramFilesX64\ContosoApp.exe"
                 EntryPoint="Windows.FullTrustApplication">
      ...
    </Application>
  </Applications>
</Package>
```

Would become:

```xml
<Package ...>
  ...
  <Applications>
    <Application Id="ContosoApp"
                 Executable="ShimLauncher64.exe"
                 EntryPoint="Windows.FullTrustApplication">
      ...
    </Application>
  </Applications>
</Package>
```

With a corresponding `config.json` that looks like:

```json
{
    "applications": [
        {
            "id": "ContosoApp",
            "executable": "VFS\\ProgramFilesX64\\ContosoApp.exe"
        }
    ],
    ...
}
```
