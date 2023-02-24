# PSF Launcher

The PSF Launcher acts as a wrapper for the application, it injects the `psfRuntime` to the application process and then activates it. 
To configure it, change the entry point of the app in the `AppxManifest.xml` and add a new `config.json` file to the package as shown below;<br/>
Based on the manifest's application ID, the `psfLauncher` looks for the app's launch configuration in the `config.json`.  The `config.json` can specify multiple application configurations via the applications array.  Each element maps the unique app ID to an executable which the `psfLauncher` will activate, using the Detours library to inject the `psfRuntime` library into the app process.  The app element must include the app executable.  It may optionally include a custom working directory, which otherwise defaults to Windows\System32.

PSF Launcher also supports running an additional "monitor" app, intended for PsfMonitor. You use PsfMonitor to view the output in conjuction with TraceFixup injection configured to output to events.

PSF Launcher also supports running external processes in package context which can be enabled by setting "inPackageContext" field. 

Finally, PsfLauncher also support some limited PowerShell scripting.

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

```xml
<?xml version="1.0" encoding="utf-8"?>
<configuration>
    <applications>
        <application>
            <id>PsfSample</id>
            <executable>PSFSampleApp/PrimaryApp.exe</executable>
            <workingDirectory>PSFSampleApp</workingDirectory>
        </application>
    </applications>
    <processes>
        <process>
            <executable>PsfLauncher.*</executable>
        </process>
        <process>
            <executable>PSFSample</executable>
            <fixups>
                <fixup>
                    <dll>FileRedirectionFixup.dll</dll>
                    <config>
                        <redirectedPaths>
                            <packageRelative>
                                <pathConfig>
                                    <base>PSFSampleApp/</base>
                                    <patterns>
                                        <pattern>
                                            .*\\/log
                                        </pattern>
                                    </patterns>
                                </pathConfig>
                            </packageRelative>
                        </redirectedPaths>
                    </config>
                </fixup>
            </fixups>
        </process>
    </processes>
</configuration>
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

```xml
<?xml version="1.0" encoding="utf-8"?>
<configuration>
    <applications>
        <application>
            <id>PSFLAUNCHERSixFour</id>
            <executable>PrimaryApp.exe</executable>
            <arguments>/AddedArg</arguments>
            <workingDirectory></workingDirectory>
            <monitor>
                <executable>PsfMonitor.exe</executable>
                <arguments></arguments>
                <asadmin>true</asadmin>
            </monitor>
        </application>
    </applications>
    <processes>
        <process>
            <executable>PrimaryApp$</executable>
            <fixups>
                <fixup>
                    <dll>TraceFixup64.dll</dll>
                    <config>
                        <traceMethod>eventLog</traceMethod>
                        <traceLevels>
                            <default>always</default>
                        </traceLevels>
                    </config>
                </fixup>
            </fixups>
        </process>
    </processes>
</configuration>
```

In this example, the configuration is directing the PsfLauncher to start PsfMonitor and then the referenced Primary App. PsfMonitor to be run using RunAs (enabling the monitor to also trace at the kernel level), followed by PrimaryApp once the monitoring app is stable. The root folder of the PrimaryApp is used for the CurrentDirectory of the PrimaryApp process.  In the processes section of the example, the json further configures Trace Fixup to be injected into the PrimaryApp, and that is to capture all levels of trace to the event log.



### Example 3
This example shows an alternative for the json used in the prior example. This might be used when command line arguments for the target executable include file paths and you want to reference those paths as full path references to their ultimate location in the deployed package.


```json
{
        "applications": [
        {
            "id": "PSFLAUNCHERSixFour",
            "executable": "PrimaryApp.exe",
            "arguments": "%MsixPackageRoot%\filename.ext %MsixPackageRoot%\VFS\LocalAppData\Vendor\file.ext"
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


In this example, the pseudo-variable %MsixPackageRoot% would be replaced by the folder path that the package was installed to. This pseudo-variable is only available for string replacement by PsfLauncher in the arguments field. The example shows a reference to a file that was placed at the root of the package and another that will exist using VFS pathing. Note that as long as the LOCALAPPDATA file is the only file required from the package LOCALAPPDATA folder, the use of FileRedirectionFixup would not be mandated.

### Example 4
This example shows an alternative for the json used in the prior example. This might be used when an additional script is to be run upon first launch of the application. Such scripts are sometimes used for per-user configuration activities that must be made based on local conditions.

To implement scripting PsfLauncher uses a PowerShell script as an intermediator.  PshLauncher will expect to find a file StartingScriptWrapper.ps1, which is included as part of the PSF, to call the PowerShell script that is referenced in the Json.  The wrapper script should be placed in the package as the same folder used by the launcher.


```json
  "applications": [
    {
      "id": "Sample",
      "executable": "Sample.exe",
      "workingDirectory": "",
      "stopOnScriptError": false,
      "scriptExecutionMode": "-ExecutionPolicy Bypass",
	  "startScript":
	  {
		"waitForScriptToFinish": true,
		"timeout": 30000,
		"runOnce": true,
		"showWindow": false,
		"scriptPath": "PackageStartScript.ps1",
		"waitForScriptToFinish": true
		"scriptArguments": "%MsixWritablePackageRoot%\\VFS\LocalAppData\\Vendor",
	  },
	  "endScript":
	  {
		"scriptPath": "\\server\scriptshare\\RunMeAfter.ps1",
		"scriptArguments": "ThisIsMe.txt"
	  }
    }
  ],
  "processes": [
    ...(taken out for brevity)
  ]
}
```

In this example two scripts are defined. The startScript is configured so that PsfLauncher will run this script only the first time that this user runs the application. It will wait for completion of the script before starting the executable associated with the application.  PsfLauncher will resolve the %MsixWritablePackageRoot% pseudo-variable allowing the script to write the configuration into the package redirected area (necessary since package scripts do not inject the FileRedirectionFixup).  The endScript example is configured to run after each time the application is run.

Note that the scriptPath must point to a powershell ps1 file; it may be a full path reference or a reference relative to the package root folder.  In addition to this script file, the package must also include a file named "StartingScriptWrapper.ps1", which is included in the Psf sources.  This script is run by PsfLauncher for any startScript or endScript, and the wrapper will invoke the script defined in the json configuration.

The use of scriptExecutionMode may only be necessary in environments when Group Policy setting of default PowerShell ExecutionPolicy is expressed. 

### Example 5
This example shows using "inPackageContext" feature to run every process spanned by an executable in same package context. 

```json
{
        "applications": [
        {
            "id": "Sample",
            "executable": "Sample.exe",
            "inPackageContext": true
    	}
  	    ]
}
```

```xml
<?xml version="1.0" encoding="utf-8"?>
<configuration>
    <applications>
        <application>
            <id>Sample</id>
            <executable>Sample.exe</executable>
            <inPackageContext>true</inPackageContext>
        </application>
    </applications>
</configuration>
```
This allows every process spanned by Sample.exe to run in the same package context. For example, if Sample.exe triggers cmd.exe outside of the package(C:\Windows\System32\cmd.exe). This configuration allows to create cmd.exe in the same package context of current application.

### Json Schema

| Array | key | Value |
|-------|-----------|-------|
| applications | id |  Use the value of the `Id` attribute of the `Application` element in the package manifest. |
| applications | waitForDebugger | (Optional, default=false) Boolean. This option is for debugging of the launcher itself and is only available in debug builds of the launcher, it is silently ignored if present in a release build. When set to true, the launcher will wait to receive a signal that a debugger has attached to the process. The wait occurs immediately after reading enough of the json file to detect this setting, before processing other settings, launching scripts, monitor, and target. |
| applications | executable | The path to the executable that you want to start. This path is typically specified relative to the package root folder. In most cases, you can get this value from your package manifest file before you modify it. It's the value of the `Executable` attribute of the `Application` element. Pseudo-variables and Environment variables are supported for this path. |
| applications | arguments | (Optional) Command line arguments for the executable.  If the PsfLauncher.exe receives any arguments, these will be appended to the command line after those from the config.json file. Pseudo-variables and Environment variables are supported in the arguments. |
| applications | workingDirectory | (Optional) A path to use as the working directory of the application that starts. This is typically a relative path of the package root folder, however full paths may be specified. If you don't set this value, the operating system uses the `System32` directory as the application's working directory. If you supply a value in the form of an empty string, it will use the directory of the referenced executable. Pseudo-variables and Environment variables are supported in the workingDirectory. |
| applications | monitor | (Optional) If present, the monitor identifies a secondary program that is to be launched prior to starting the primary application.  A good example might be `PsfMonitor.exe`.  The monitor configuration consists of the following items: |
| | |   `'executable'` - This is the name of the executable relative to the root of the package. |
| | |   `'arguments'`  - This is a string containing any command line arguments that the monitor executable requires. Any use of the string "%MsixPackageRoot%" in the arguments will be replaced by the a string containing the actual package root folder at runtime. |
| | |   `'asadmin'` - This is a boolean (0 or 1) indicating if the executable needs to be launched as an admin.  To use this option set to 1, you must also mark the package with the RunAsAdministrator capability.  If the monitor executable has a manifest (internal or external) it is ignored.  If not expressed, this defaults to a 0. |
| | |   `'wait'` - This is a boolean (0 or 1) indicating if the launcher should wait for the monitor program to exit prior to starting the primary application.  When not set, the launcher will WaitForInputIdle on the monitor before launching the primary application. This option is not normally used for tracing and defaults to 0. |
| applications | stopOnScriptError| (Optional) Boolean. Indicates that if a startScript returns an error then the launch of the application should be skipped. |
| applications | ScriptExecutionMode | (Optional) String value that will be added to the powershell launch of any startScript or endScript. |
| applications | startScript | (Optional) If present, used to define a PowerShell script that will be run prior running the application executable. |
| | |  `'waitForScriptToFinish'` - (Optional, default=true) Boolean. When true, PsfLauncher will wait for the script to complete or timeout before running the application executable. |
| | | `'timeout'` - (Optional, default is none) Expressed in ms.  Only applicable if waitForScriptToFinish is true.  If a timeout occurs it is treated as an error for the purpose of `'stopOnScriptError'`. The value 0 means an immediate timeout, if you do not want a timeout do not specify a value. |
| | | `'runOnce'` - (Optional, default=true) Boolean. When true, the script will only be run the first time the user runs the application. If script fails to run, it will be run on every launch of application till it runs successfully once. |
| | | `'showWindow'` - (Optional, default=true). Boolean. When false, the PowerShell window is hidden. |
| | | `'scriptPath'` - Relative or full path to a ps1 file. May be in package or on a network share. Use of pseudo-variables or environment variables are supported. |
| | | `'scriptArguments'` - (Optional) Arguments for the `'scriptPath'` PowerShell file.  Use of pseudo-variables or environment variables are supported. Multiple arguments(and arguments with space) are provided by enclosing each argument in single quote. ("scriptArguments": "'<Arg1>' '<Arg2>'"). |
| applications | endScript | (Optional) If present, used to define a PowerShell script that will be run after completion of the application executable. |
| | | `'runOnce'` - (Optional, default=true) Boolean. When true, the script will only be run the first time the user runs the application. If script fails to run, it will be run on every launch of application till it runs successfully once. |
| | | `'showWindow'` - (Optional, default=true). Boolean. When false, the PowerShell window is hidden. |
| | | `'scriptPath'` - Relative or full path to a ps1 file. May be in package or on a network share. Use of pseudo-variables or environment variables are supported. |
| | | `'scriptArguments'` - (Optional) Arguments for the `'scriptPath'` PowerShell file.  Use of pseudo-variables or environment variables are supported. |
| applications | inPackageContext | (Optional, default=false) Boolean. When true, all the processes spanned by this executable will be run in the same package context. |
| processes | executable | In most cases, this will be the name of the `executable` configured above with the path and file extension removed. Multiple arguments(and arguments with space) are provided by enclosing each argument in single quote. ("scriptArguments": "'<Arg1>' '<Arg2>'").|
| fixups | dll | Package-relative path to the fixup, .msix/.appx  to load. |
| fixups | config | (Optional) Controls how the fixup dl behaves. The exact format of this value varies on a fixup-by-fixup basis as each fixup can interpret this "blob" as it wants. |

The `applications`, `processes`, and `fixups` keys are arrays. That means that you can use the config.json file to specify more than one application, process, and fixup DLL.

An entry in `processes` has a value named `executable`, the value for which should be formed as a RegEx string to match the name of an executable process without path or file extension. The launcher will expect Windows SDK std:: library RegEx ECMAList syntax for the RegEx string.

### PsfLauncher Pseudo-Variables
The PSF Launcher supports the use of two special purpose "pseudo-variables". These pseudo-variables provide package specific locations are present on the target system.  These pseudo-variables may be used on the application executable, arguments, and workingDirectory fields, as well as in startScript/endScript scriptPath and scriptArguments.  The PSF Launcher will derefernce these variables (and any specified environment variables) prior to launching the script/application. The PSF Launcher pseudo-vairables are:

| Variable| Value |
|---------|-------|
| %MsixPackageRoot% | The root folder of the package. While nominally this would be a subfolder under "C:\\Program Files\\WindowsApps" it is possible for the volume to be mounted in other locations. |
| %MsixWritablePackageRoot% | The package specific redirection location for this user when the FileRedirectionFixup is in use. | 

### inPackageContext
This Configuration allows all created process from a running executable to operate in same package context. This includes any child processes that might not be part of the package. This might be useful in situations where applications span processes that are outside package but needs to be run in the same package context, or when PSF fixups need to be applied to processes external to the package.

When an external process is launched by an executable, all of its app data is stored in its native app data folder and hence not containerized. This feature allows the app data of the external process to be virtualized in per user per app data folder of the package.

This configuration is not needed when process spanned by the executable are already part of the current package. This feature can be enabled when there is a need of running external process in the current package context.

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
