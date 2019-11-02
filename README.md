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

### Testing your changes.
Before making a pull request please run the regression tests to make sure your changes did not break existing behavior.

Please head to the tests solution and follow the instructions inside readme.md.

## Script support
Scripts allow IT Pros to customize the app for the user environment dynamically. The scripts typically change registry keys or perform file modifications based on the machine or server configuration.

Each exe defined in the application manifest can have their own scripts. PSF allows one PowerShell script to be run before an exe runs, and one PowerShell script to be ran after the exe runs.

### Prerequisite to allow scripts to run
In order to allow scripts to run you need to set the execution policy to unrestricted or RemoteSigned.

Run the one of the two following powershell commands to set the execution policy: `Set-ExecutionPolicy -ExecutionPolicy RemoteSigned` or `Set-ExecutionPolicy -ExecutionPolicy Unrestricted` depending if you want the execution policy to be remoteSigned or unrestricted.

The execution policy needs to be set for both the 64-bit powershell executable and the 32-bit powershell executable.  Make sure to open each version of powershell and run the command shown above.

Here are the locations of each executable.
* If on a 64-bit computer
  * 64-bit: %SystemRoot%\system32\WindowsPowerShell\v1.0\powershell.exe
  * 32-bit: %SystemRoot%\SysWOW64\WindowsPowerShell\v1.0\powershell.exe
* If on a 32-bit computer
  * 32-bit: %SystemRoot%\system32\WindowsPowerShell\v1.0\powershell.exe
  
[More information about PowerShell Execution Policy](https://docs.microsoft.com/en-us/powershell/module/microsoft.powershell.core/about/about_execution_policies?view=powershell-6)

### How to enable scripts to run with PSF
In order to specify what scripts will run for each packaged exe you will need to modify the config.json file.  To tell PSF to run a script before the execution of the packaged exe add a configuration item called "startScript".  To tell PSF to run a script after the packaged exe finishes add a configuration item called "endScript".

#### Script configuration items
The following are the configuration items available for the scripts.  The ending script ignores the following configuration items: waitForScriptToFinish, and stopOnScriptError.

| Key name                | Value type | Required? | Default  | Description
|-------------------------|------------|-----------|----------|---------|
| scriptPath              | string     | Yes       | N/A      | The path to the script including the name and extension.  The Path starts from the root directory of the application.
| scriptArguments         | string     | No        | empty    | Space delimited argument list.  The format is the same for a PowerShell script call.  This string gets appended to scriptPath to make a valid PowerShell.exe call.
| runInVirtualEnvironment | boolean    | No        | true     | If the script should run in the same virtual environment that the packaged exe runs in.
| runOnce                 | boolean    | No        | true     | If the script should run once per user, per version.
| showWindow              | boolean    | No        | false    | If the powershell window is shown.
| waitForScriptToFinish   | boolean    | No        | true     | If the packaged exe should wait for the starting script to finish before starting.
| timeout                 | DWORD      | No        | INFINITE | How long the script will be allowed to execute.  If elapsed the script will be stopped.

### Sample configuration
Here is a sample configuration using two different exes.
<pre>
    {
  "applications": [
    {
      "id": "Sample",
      "executable": "Sample.exe",
      "workingDirectory": "",
      "stopOnScriptError": false,
	  "startScript":
	  {
		"scriptPath": "RunMePlease.ps1",
		"scriptArguments": "\\\"First argument\\\" secondArgument",
		"runInVirtualEnvironment": true,
		"showWindow": true,
		"waitForScriptToFinish": false
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
		"scriptArguments": "ThisIsMe.txt",
    "runOnce": false
	  }
    }
  ],
  "processes": [
    ...(taken out for brevity)
  ]
}
</pre>

 ### Enabling the app to exit if the starting script encounters an error.
 The scripting change allows users to tell PSF to exit the application if the starting script fails.  To do this, add the pair "stopOnScriptError": true to the application configuration (not the script configuration).
 
 ### The combination of `stopOnScriptError: true` and `waitForScriptToFinish: false` is not supported
 This is an illegal configuration combination and PSF will throw the error ERROR_BAD_CONFIGURATION.

## Fixup Metadata
Each fixup and the PSF Launcher has a metadata file in xml format.  Each file contains the following  
 1. Version:  The version of the PSF is in MAJOR.MINOR.PATCH format according to [Sem Version 2](https://semver.org/)
 2. Minimum Windows Platform the the minimum windows version required for the fixup or PSF Launcher.
 3. Description: Short description of the fixup.
 4. WhenToUse: Heuristics on when you should apply the fixup.

Additionally, we have the XSD for the metadata files.  THe XSD is located in the solution folder.

## Data/Telemetry
Telemetry datapoint has been hooked to collect usage data and sends it to Microsoft to help improve our products and services. Read Microsoft's [privacy statement to learn more](https://privacy.microsoft.com/en-US/privacystatement). However, data will be collected only when the PSF binaries are used from [Nuget package](https://www.nuget.org/packages?q=packagesupportframework) 
on Windows 10 devices and only if users have enabled collection of data. The Nuget package has binaries signed and will collect usage data from machine. When the binaries are built locally by cloning the repo or downloading the bits, then telemetry is not collected.