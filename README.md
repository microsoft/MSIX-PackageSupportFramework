# Package Support Framework
This project provides tools, libraries, documentation and samples for creating app-compat fixups to enable classic Win32 applications to be distributed and executed as packaged apps.

## Documentation
Check out our [step by step guide](https://docs.microsoft.com/en-us/windows/uwp/porting/package-support-framework), it will walk you through the main PSF workflows and provides the key documentation.

See also:
* [Package Support Framework package layout](layout.md)
* [Package Support Framework Nuget package install](https://www.nuget.org/packages/Microsoft.PackageSupportFramework)
* [Instructions for authoring your own shim dll](Authoring.md)

## [License](https://github.com/Microsoft/MSIX-PackageSupportFramework/blob/master/LICENSE)
Code licensed under the [MIT License](https://github.com/Microsoft/MSIX-PackageSupportFramework/blob/master/LICENSE).

## Branch structure
Package Support Framework adopts a development and master branching style.

### Master
This branch represents the source in the most recent nuget package.  The code in this branch is the most stable.  Please do not fork off this branch for development.

### Develop
This branch has the latest code. Keep in mind that there might be features in this branch that is not yet in master.  Please make a private fork off this branch to make any contributions to Package Support Framework.

## Contribute
This project welcomes contributions and suggestions.  Most contributions require you to agree to a Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us the rights to use your contribution. For details, visit https://cla.microsoft.com.

## Script support
PSF allows one PowerShell script to be run before an exe runs, and one PowerShell script to be ran after the exe runs.

Each exe defined in the application manifest can have their own scripts.

### Stopping application on a script error.
PSF gives users the option to stop the application if the starting script encounters an error. To disable further PSF script runs on error, add the option "stopOnScriptError":true.
 The default PSF value for stopOnScriptError is false.  This pair can be specified on a per exe basis.

If `stopOnScriptError` is either false or not defined the behavior is to run the application and endingScript (if defined) even if there is an error with the starting script.

#### Determining what constitutes an error for scripts.
PSF does not alter the Error Action Preference for powershell.  If you need to treat non-terminating errors as terminating errors please change the Error Action Preference in each
of your powershell scripts.  You can find more information about setting the Error Action Preference here: [MSDN setting the Error Actio Preference.](https://docs.microsoft.com/en-us/dotnet/api/system.management.automation.psinvocationsettings.erroractionpreference?view=pscore-6.2.0)

### Hiding the script windows
PSF allows the user to hide the PowerShell window for both the starting script and ending script.  To do so add `"showWindow": true` to the script object.  The default behavior is to hide the window.

### Running the script once
PSF allows the starting script and the ending script to be run once per application version.  To enable this run once feature, add `"runOnce" : true` to any script object.  Adding this indicates that the script should only execute once per package version lifetime for each user.  The default value for this pair is false.

### Timeout
Both the starting script and ending script support timeouts in seconds.  Timeouts are optional.

If no timeout is supplied PSF will wait indefinitely for the script to exit. The user application will not start until the script runs to completion or is terminated.

If a timeout is supplied PSF will terminate the script after the supplied number of seconds have elapsed.  Terminating a script is not considered an error.

### Wait.
The starting script accepts a wait pair.  This pair is optional and defaults to false if not supplied.  Wait is used to tell PSF if the application should run in parallel with the starting script.

Wait = true tells PSF to run the application after the starting script has finished running.

The endingScript ignores wait.  PSF will wait for the endingScript to finish before exiting.

If you want the application and starting script to run at the same time either provide `"wait":false` or do not include this pair.


### Combining stopOnScriptError and Wait
With the introduction of running asynchronously certain combinations of stopOnScriptError and wait are not allowed.  This prevents the application from crashing in the middle of execution if the starting script has an error.

Below are all combinations of stopOnScriptError and wait along with what PSF will do in each case.

| stopOnScriptError | wait  | behavior                                                                                                                                                    |
|-------------------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------|
| true              | false | Not allowed because this would cause the application to terminate in the middle of execution if the starting script encountered an error.                   |
| true              | true  | The application will start only after the starting script has exited.  If the script encountered an error PSF would exit and the application would not run. |
| false             | false | The application and starting script will run in parallel.  No error will be reported if the starting script encounters an error.                            |
| fale              | true  | The application will start only after the starting script has exited.  No error will be reported if the starting script encountered an error.


### Prerequisite to allow scripts to run
In order to allow scripts to run you need to set the execution policy to unrestricted or RemoteSigned.  The execution policy needs to be set for both the 64-bit powershell executable and the 32-bit powershell executable.

Here are the locations of each executable.
* If on a 64-bit computer
  * 64-bit: %SystemRoot%\system32\WindowsPowerShell\v1.0\powershell.exe
  * 32-bit: %SystemRoot%\SysWOW64\WindowsPowerShell\v1.0\powershell.exe
* If on a 32-bit computer
  * 32-bit: %SystemRoot%\system32\WindowsPowerShell\v1.0\powershell.exe
  
[More information about PowerShell Execution Policy](https://docs.microsoft.com/en-us/powershell/module/microsoft.powershell.core/about/about_execution_policies?view=powershell-6)


### Configuration changes
In order to specify what scripts will run for each packaged exe you will need to modify the config.json file.  To tell PSF to run a script before the execution of the pacakged exe add an object called "startScript".  To tell PSF to run a script after the packaged exe finishes add an object called "endScript".
Both objects use the same three keys.

| Key name                | Value type | Required?             |
|-------------------------|------------|-----------------------|
| scriptPath              | string     | Yes                   |
| scriptArguments         | string     | No                    |
| runInVirtualEnvironment | boolean    | No (defaults to true) |
 
 #### Key descriptions
 1. scriptPath: The path to the script including the name and extension.  The Path starts from the root directory of the application.
 2. scriptArguments: Space delimited argument list.  The format is the same for a PowerShell script call.  This string gets appended to scriptPath to make a valid PowerShell.exe call.
 3. runInVirtualEnvironment: If the script should run in the same virtual environment that the packaged exe runs in.
 
### Flow of PSF with scripts
Below is the flow of PSF with scripting support.  
The flow from beginning to end is [Starting Script] -> [Monitor] -> [Packaged exe] -> [Ending script]


### Visual representation of the flow
<pre>
+------------+ Success, or  +-----------------+              +-----------------+
|  Starting  | Script error |   Monitor and   |   Success    |      Ending     |
|   Script   |------------->|   Packaged exe  |------------> |      Script     |
+------------+              +-----------------+              +-----------------+
     | Create                        | Create                         | Create
     | Process                       | Process                        | Process
     | Error                         | Error                          | Error
     V                               V                                V
+-------------+             +-----------------+   Create Process +----------------+
| Throw error |             |    Run ending   |       Error      |   Throw error  | 
|  and exit   |             |     script      |----------------->|    and exit.   |
+-------------+             +-----------------+                  +----------------+
                                    | 
                                    | Success
                                    |
                                    V
                             +--------------+
                             |  Throw error |
                             |   and exit   |
                             +--------------+
</pre>

## Sample configuration
Here is a sample configuration using two different exes.
<pre>
    {
  "applications": [
    {
      "id": "Sample",
      "executable": "Sample.exe",
      "workingDirectory": "",
	  "startScript":
	  {
		"scriptPath": "RunMePlease.ps1",
		"scriptArguments": "ThisIsMe.txt",
		"runInVirtualEnvironment": true
	  },
	  "endScript":
	  {
		"scriptPath": "RunMeAfter.ps1",
		"scriptArguments": "ThisIsMe.txt"
	  }
    },
	{
      "id": "CPPSample",
      "executable": "CPPSample.exe",
      "workingDirectory": "",
	  "startScript":
	  {
		"scriptPath": "CPPStart.ps1",
		"scriptArguments": "ThisIsMe.txt",
		"runInVirtualEnvironment": true
	  },
	  "endScript":
	  {
		"scriptPath": "CPPEnd.ps1",
		"scriptArguments": "ThisIsMe.txt"
	  }
    }
  ],
  "processes": [
    ...(taken out for brevity)
  ]
}
</pre>

## Fixup Metadata
Each fixup and the PSF Launcher has a metadata file in xml format.  Each file contains the following  
 1. Version:  The version of the PSF is in MAJOR.MINOR.PATCH format according to [Sem Version 2](https://semver.org/)
 2. Minimum Windows Platform the the minimum windows version required for the fixup or PSF Launcher.
 3. Description: Short description of the Fixup.
 4. WhenToUse: Heuristics on when you should apply the fixup.

Addtionally, we have the XSD for the metadata files.  THe XSD is located in the solution folder.

Submit your own fixup(s) to the community:
1. Create a private fork for yourself
2. Make your changes in your private branch
3. For new files that you are adding include the following Copyright statement.\
//-------------------------------------------------------------------------------------------------------\
// Copyright (c) #YOUR NAME#. All rights reserved.\
// Licensed under the MIT license. See LICENSE file in the project root for full license information.\
//-------------------------------------------------------------------------------------------------------
4. Create a pull request into 'fork:Microsoft/MSIX-PackageSupportFramework' 'base:develop'

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions provided by the bot. You will only need to do this once across all repos using our CLA.

Here is how you can contribute to the Package Support Framework:

* [Submit bugs](https://github.com/Microsoft/MSIX-PackageSupportFramework/issues) and help us verify fixes
* [Submit pull requests](https://github.com/Microsoft/MSIX-PackageSupportFramework/pulls) for bug fixes and features and discuss existing proposals

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## Data/Telemetry
Telemetry datapoint has been hooked to collect usage data and sends it to Microsoft to help improve our products and services. Read Microsoft's [privacy statement to learn more](https://privacy.microsoft.com/en-US/privacystatement). However, data will be collected only when the PSF binaries are used from [Nuget package](https://www.nuget.org/packages?q=packagesupportframework) 
on Windows 10 devices and only if users have enabled collection of data. The Nuget package has binaries signed and will collect usage data from machine. When the binaries are built locally by cloning the repo or downloading the bits, then telemetry is not collected.