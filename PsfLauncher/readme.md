# PSF Launcher

The PSF Launcher acts as a wrapper for the application, it injects the `psfRuntime` to the application process and then activates it. 
To configure it, change the entry point of the app in the `AppxManifest.xml` and add a new `config.json` file to the package as shown below;<br/>
Based on the manifest's application ID, the `psfLauncher` looks for the app's launch configuration in the `config.json`.  The `config.json` can specify multiple application configurations via the applications array.  Each element maps the unique app ID to an executable which the `psfLauncher` will activate, using the Detours library to inject the `psfRuntime` library into the app process.  The app element must include the app executable.  It may optionally include a custom working directory, which otherwise defaults to Windows\System32.

PSF Launcher also supports running an additional "monitor" app, intended for PsfMonitor. You use PsfMonitor to view the output in conjuction with TraceFixup injection configured to output to events.

## Configuration
PSF Launcher uses a config.json file to configure the behavior.

### Example 1 - Package using FileRedirectionFixup
Given an application package with an `AppxManifest.xml` file containing the following:

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

A possible `config.json` example that includes fixups would be:

```json
{
    "applications": [
        {
            "id": "PSFSample",
            "executable": "PSFSampleApp/PrimaryApp.exe",
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
                                        ".*\\/log"
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
In this example, the configuration is directing the PsfLauncher to start PsfSample.exe. The CurrentDirectory for that process is set to the folder containing PSFSample.exe.  In the processes section of the example, the json further configures the FileRedirection Fixup to be injected into the PSFSample, and that fixup is configured to perform file redirecton on log files in a particular folder. (See FileRedirectionFixup for additional details and examples on configuring this fixup).


### Example 2
This example shows using PsfLauncher with the TraceFixup and PsfMonitor. This might be used as part of a repackaging effort in order to understand whether fixups might be required for full funtionality of the package.

Given an application package with an `AppxManifest.xml` file containing the following:

```xml
<Package ...>
  ...
  <Applications>
    <Application Id="PrimaryApp"
                 Executable="PrimaryApp.exe"
                 EntryPoint="Windows.FullTrustApplication">
      ...
    </Application>
  <Applications>
    <Application Id="PSFLAUNCHERSixFour"
                 Executable="PSFLauncher64.exe"
                 EntryPoint="Windows.FullTrustApplication">
      ...
    </Application>
  </Applications>
</Package>
```

A possible `config.json` example that uses the Trace Fixup along with PsfMonitor would be:

```json
{
        "applications": [
        {
            "id": "PSFLAUNCHERSixFour",
            "executable": "PrimaryApp.exe",
            "arguments": "/AddedArg"
            "workingDirectory": "",
            "monitor": {
                "executable": "PsfMonitor.exe",
                "arguments": "",
                "asadmin": true,
   	    }
	}
  	],
        "processes": [
        {
            "executable": "PrimaryApp$",
            "fixups": [ 
            { 
                "dll": "TraceFixup64.dll",
                "config": {
                    "traceMethod": "eventlog",
                    "traceLevels": {
                        "default": "always"
                    }
                }
            } 
            ]
        }
        ]
}
```

In this example, the configuration is directing the PsfLauncher to start PsfMonitor and then the referenced Primary App. PsfMonitor to be run using RunAs (enabling the monitor to also trace at the kernel level), followed by PrimaryApp once the monitoring app is stable. The root folder of the PrimaryApp is used for the CurrentDirectory of the PrimaryApp process.  In the processes section of the example, the json further configures Trace Fixup to be injected into the PrimaryApp, and that is to capture all levels of trace to the event log.

### Json Schema

| Array | key | Value |
|-------|-----------|-------|
| applications | id |  Use the value of the `Id` attribute of the `Application` element in the package manifest. |
| applications | executable | The package-relative path to the executable that you want to start. In most cases, you can get this value from your package manifest file before you modify it. It's the value of the `Executable` attribute of the `Application` element. |
| applications | arguments | (Optional) Command line arguments for the executable.  If the PsfLauncher.exe receives any arguments, these will be appended to the command line after those from the config.json file. |
| applications | workingDirectory | (Optional) A package-relative path to use as the working directory of the application that starts. If you don't set this value, the operating system uses the `System32` directory as the application's working directory. If you supply a value in the form of an empty string, it will use the directory of the referenced executable. |
| applications | monitor | (Optional) If present, the monitor identifies a secondary program that is to be launched prior to starting the primary application.  A good example might be `PsfMonitor.exe`.  The monitor configuration consists of the following items: |
| | |   `'executable'` - This is the name of the executable relative to the root of the package. |
| | |   `'arguments'`  - This is a string containing any command line arguments that the monitor executable requires. |
| | |   `'asadmin'` - This is a boolean (0 or 1) indicating if the executable needs to be launched as an admin.  To use this option set to 1, you must also mark the package with the RunAsAdministrator capability.  If the monitor executable has a manifest (internal or external) it is ignored.  If not expressed, this defaults to a 0. |
| | |   `'wait'` - This is a boolean (0 or 1) indicating if the launcher should wait for the monitor program to exit prior to starting the primary application.  When not set, the launcher will WaitForInputIdle on the monitor before launching the primary application. This option is not normally used for tracing and defaults to 0. |
| processes | executable | In most cases, this will be the name of the `executable` configured above with the path and file extension removed. |
| fixups | dll | Package-relative path to the fixup, .msix/.appx  to load. |
| fixups | config | (Optional) Controls how the fixup dl behaves. The exact format of this value varies on a fixup-by-fixup basis as each fixup can interpret this "blob" as it wants. |

The `applications`, `processes`, and `fixups` keys are arrays. That means that you can use the config.json file to specify more than one application, process, and fixup DLL.

=======
Submit your own fixup(s) to the community:
1. Create a private fork for yourself
2. Make your changes in your private branch
3. For new files that you are adding include the following Copyright statement.\
//-------------------------------------------------------------------------------------------------------\
// Copyright (c) #YOUR NAME#. All rights reserved.\
// Licensed under the MIT license. See LICENSE file in the project root for full license information.\
//-------------------------------------------------------------------------------------------------------
4. Create a pull request into 'fork:Microsoft/MSIX-PackageSupportFramework' 'base:master'
