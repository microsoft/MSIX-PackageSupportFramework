# Package Support Framework
This project provides tools, libraries, documentation and samples for creating runtime fixes (also called *fixups*) for compatibility issues that enable Windows desktop applications to be distributed and executed as MSIX packaged apps.

Here are some common examples where you can find the Package Support Framework (PSF) useful:

* Your app can't find some DLLs when launched. You may need to set your current working directory. You can learn about the required current working directory in the original shortcut before you converted to MSIX.
* The app writes to the install folder. This issue typically shows up as ACCESS DENIED errors in [Process Monitor](https://docs.microsoft.com/sysinternals/downloads/procmon) as demonstrated in the following image.
    ![ProcMon Logfile](procmon_logfile.png)
* Your app needs to pass arguments to the executable when launched. You can easily configure arguments by using [PSF Launcher](https://github.com/microsoft/MSIX-PackageSupportFramework/tree/master/PsfLauncher).

You can learn more about how to identify compatibility issues [here](https://docs.microsoft.com/windows/msix/psf/package-support-framework#identify-packaged-application-compatibility-issues). If you have feedback, please create [an issue](https://github.com/Microsoft/MSIX-PackageSupportFramework/issues) or post a message in our [tech community](https://techcommunity.microsoft.com/t5/Package-Support-Framework/bd-p/Package-Support).

## Documentation
See these articles for core documentation about using the Package Support Framework:

* [Apply runtime fixes by using the Package Support Framework](https://docs.microsoft.com/windows/uwp/porting/package-support-framework): This article provides step-by-step instructions for the main Package Support Framework workflows.
* [Run scripts with the Package Support Framework](https://docs.microsoft.com/windows/msix/psf/run-scripts-with-package-support-framework): This article demonstrates how to run scripts to customize an application dynamically to the user's environment after it is packaged using MSIX.

The following additional resources provide additional information about specific scenarios:
* [Package Support Framework package layout](layout.md)
* [Package Support Framework NuGet package install](https://www.nuget.org/packages/Microsoft.PackageSupportFramework)
* [Instructions for authoring your own fixup DLL](Authoring.md)

## Get the pre-built Package Support Framework binaries
Download the Package Support Framework binaries from [Nuget.org](https://www.nuget.org/packages/Microsoft.PackageSupportFramework). To extract the binaries, rename the package extension to .zip, unzip the file, and locate the binaries in the /bin folder. In a future release we are planning to make it easier to locate the binaries directly on GitHub.

## Branch structure
Package Support Framework adopts a **develop** and **master** branching style.

### Master
The **master** branch represents the source in the most recent NuGet package. The code in this branch is the most stable. Do not fork off this branch for development.

### Develop
The **develop** branch has the latest code. Keep in mind that there might be features in this branch that is not yet in **master**. Make a private fork off the **develop** branch to make your own contributions to Package Support Framework.

## Fixup metadata
Each fixup and the PSF Launcher has a metadata file in XML format. Each file contains the following:

* `Version`: The version of the Package Support Framework is in MAJOR.MINOR.PATCH format according to [Sem Version 2](https://semver.org/).
* `MinimumWindowsPlatform`: The minimum windows version required for the fixup or PSF Launcher.
* `Description`: A short description of the fixup.
* `WhenToUse`: Heuristics on when you should apply the fixup.

The metadata file schema is provided [here](MetadataSchema.xsd).

## Data/Telemetry
The Package Support Framework includes telemetry that collects usage data and sends it to Microsoft to help improve our products and services. Read Microsoft's [privacy statement to learn more](https://privacy.microsoft.com/en-US/privacystatement). However, data will be collected only when both of the following conditions are met:

* The Package Support Framework binaries are used from the [NuGet package](https://www.nuget.org/packages/Microsoft.PackageSupportFramework) on a Windows 10 computer.
* The user has enabled collection of data on the computer.

The NuGet package contains signed binaries and will collect usage data from the computer. Telemetry is not collected when the binaries are built locally by cloning the repo or downloading the binaries directly.

## License
The Package Support Framework code is licensed under the [MIT License](https://github.com/Microsoft/MSIX-PackageSupportFramework/blob/master/LICENSE).

## Contribute
This project welcomes contributions and suggestions. Most contributions require you to agree to a Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us the rights to use your contribution. For details, visit https://cla.microsoft.com.

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

* [Submit bugs](https://github.com/Microsoft/MSIX-PackageSupportFramework/issues) and help us verify fixes.
* [Submit pull requests](https://github.com/Microsoft/MSIX-PackageSupportFramework/pulls) for bug fixes and features and discuss existing proposals.

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
        "scriptArguments": "ThisIsMe.txt"
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
