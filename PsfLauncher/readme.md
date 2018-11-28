# PSF Launcher

The PSF Launcher acts as a wrapper for the application, it injects the `psfRuntime` to the application process and then activates it. 
To configure it, change the entry point of the app in the `AppxManifest.xml` and add a new `config.json` file to the package as shown below;<br/>
Based on the manifest's application ID, the `psfLauncher` looks for the app's launch configuration in the `config.json`.  The `config.json` can specify multiple application configurations via the applications array.  Each element maps the unique app ID to an executable which the `psfLauncher` will activate, using the Detours library to inject the `psfRuntime` library into the app process.  The app element must include the app executable.  It may optionally include a custom working directory, which otherwise defaults to Windows\System32.

PSF Launcher also supports running an additional "monitor" app, intended for PsfMonitor. You use PsfMonitor to view the output in conjuction with TraceFixup injection configured to output to events.

## Configuration
PSF Launcher uses a config.json file to configure the behavior.

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
    <Application Id="PSFSample"
                 Executable="PSFLauncher64.exe"
                 EntryPoint="Windows.FullTrustApplication">
      ...
    </Application>
  </Applications>
</Package>
```

A possible `config.json` example would be:


```json
{
	"applications": [
	{
		"id": "PSFLAUNCHERSixFour",
      		"executable": "PrimaryApp.exe",
      		"workingDirectory": "",
      		"monitor": {
			"executable": "PsfMonitor.exe",
			"arguments": "",
			"asadmin": true,
        		"wait": false
      		}
    	}
  	],
	"processes": [
	{
		"executable": ".*PrimaryApp.*",
		"shims": [ 
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

In this example, the configuration is directing the PsfLauncher to start PsfMonitor and then the referenced Primary App. PsfMonitor to be run using RunAs, followed by PrimaryApp once the monitoring app is stable. The root folder of the package is for the CurrentDirectory.  In the processes section of the example, the json further configures Trace Fixup to be injected into the Primary app, and that is to capture all levels of trace to the event log.


| Array | key | Value |
|-------|-----------|-------|
| applications | id |  Use the value of the `Id` attribute of the `Application` element in the package manifest. |
| applications | executable | The package-relative path to the executable that you want to start. In most cases, you can get this value from your package manifest file before you modify it. It's the value of the `Executable` attribute of the `Application` element. |
| applications | workingDirectory | (Optional) A package-relative path to use as the working directory of the application that starts. If you don't set this value, the operating system uses the `System32` directory as the application's working directory. If you supply a value in the form of an empty string, it will use the directory of the referenced executable. |
| applications | monitor | (Optional) If present, the monitor identifies a secondary program that is to be launched prior to starting the primary application.  A good example might be `PsfMonitor.exe`.  The monitor configuration consists of the following items:
	`'executable'` - This is the name of the executable relative to the root of the package.
	`'arguments'`  - This is a string containing any command line arguments that the monitor executable requires.
	`'asadmin'` - This is a boolean (0 or 1) indicating if the executable needs to be launched as an admin.  To use this option you must mark the package with the RunAsAdministrator capability.  If the monitor executable has a manifest (internal or external) it is ignored.  If not expressed, this defaults to a 0.
	`'wait'` - This is a boolean (0 or 1) indicating if the launcher should wait for the monitor program to exit prior to starting the primary application.  When not set, the launcher will WaitForInputIdle on the monitor before launching the primary application. This option is not normally used and defaults to 0. |
| processes | executable | In most cases, this will be the name of the `executable` configured above with the path and file extension removed. |
| fixups | dll | Package-relative path to the fixup, .msix/.appx  to load. |
| fixups | config | (Optional) Controls how the fixup dl behaves. The exact format of this value varies on a fixup-by-fixup basis as each fixup can interpret this "blob" as it wants. |

The `applications`, `processes`, and `fixups` keys are arrays. That means that you can use the config.json file to specify more than one application, process, and fixup DLL.


