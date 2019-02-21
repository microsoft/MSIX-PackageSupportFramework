# PSF Launcher

The PSF Launcher acts as a wrapper for the application, it injects the `psfRuntime` to the application process and then activates it. 
To configure it, change the entry point of the app in the `AppxManifest.xml` and add a new `config.json` file to the package as shown below;<br/>
Based on the manifest's application ID, the `psfLauncher` looks for the app's launch configuration in the `config.json`.  The `config.json` can specify multiple application configurations via the applications array.  Each element maps the unique app ID to an executable which the `psfLauncher` will activate, using the Detours library to inject the `psfRuntime` library into the app process.  The app element must include the app executable.  It may optionally include a custom working directory, which otherwise defaults to Windows\System32.

`AppxManifest.xml` example:

```xml
<Package ...>
  ...
  <Applications>
    <Application Id="PSFSample"
                 Executable="PSFLauncher32.exe"
                 EntryPoint="Windows.FullTrustApplication">
      ...
    </Application>
  </Applications>
</Package>
```

 `config.json` example:

```json
{
    "applications": [
        {
            "id": "PSFSample",
            "executable": "PSFSampleApp/PSFSample.exe",
            "workingDirectory": "PSFSampleApp/"
        }
    ],
    "processes": [
        {
            "executable": "PSFSample",
            "fixups": [
                {
                    "dll": "FileRedirectionFixup.dll",
                    "config": {
                        "redirectedPaths": {
                            "packageRelative": [
                                {
                                    "base": "PSFSampleApp/",
                                    "patterns": [
                                        ".*\\.log"
                                    ]
                                }
                            ]
                        }
                    }
                }
            ]
        }
    ]
}
```

| Array | key | Value |
|-------|-----------|-------|
| applications | id |  Use the value of the `Id` attribute of the `Application` element in the package manifest. |
| applications | executable | The package-relative path to the executable that you want to start. In most cases, you can get this value from your package manifest file before you modify it. It's the value of the `Executable` attribute of the `Application` element. |
| applications | arguments | (Optional) Command line arguments for the executable.  If the PsfLauncher.exe receives any arguments, these will be appended to the command line after those from the config.json file. |
| applications | workingDirectory | (Optional) A package-relative path to use as the working directory of the application that starts. If you don't set this value, the operating system uses the `System32` directory as the application's working directory. |
| processes | executable | In most cases, this will be the name of the `executable` configured above with the path and file extension removed. |
| fixups | dll | Package-relative path to the fixup, .msix/.appx  to load. |
| fixups | config | (Optional) Controls how the fixup dl behaves. The exact format of this value varies on a fixup-by-fixup basis as each fixup can interpret this "blob" as it wants. |

The `applications`, `processes`, and `fixups` keys are arrays. That means that you can use the config.json file to specify more than one application, process, and fixup DLL.
