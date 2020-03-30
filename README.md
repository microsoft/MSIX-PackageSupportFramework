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

## Fixup Metadata
Each fixup and the PSF Launcher has a metadata file in xml format.  Each file contains the following  
 1. Version:  The version of the PSF is in MAJOR.MINOR.PATCH format according to [Sem Version 2](https://semver.org/)
 2. Minimum Windows Platform the the minimum windows version required for the fixup or PSF Launcher.
 3. Description: Short description of the fixup.
 4. WhenToUse: Heuristics on when you should apply the fixup.

Additionally, we have the XSD for the metadata files located in the solutions folder.

## Scripts
Scripting support is allowed on RS2 and higher builds of windows.  If PSf is executed on RS1, scripts will not execute even if they are specified in the configuration file.

## XML configuration files.
PSF allows you to write the configuration file in either json or xml.  Configuration files written in xml need to be translated to json before psf can use them. 

Because PSF uses json files for the configuration you will need to convert the xml configuration file to json.

To convert an xml configuration to json please use msxsl.exe located in the xmlToJsonConverter folder to convert your xml configuration to json.  Here is an example command.

`msxsl.exe [location of your xml file] -format.xsl -o config.json`

## Data/Telemetry
Telemetry datapoint has been hooked to collect usage data and sends it to Microsoft to help improve our products and services. Read Microsoft's [privacy statement to learn more](https://privacy.microsoft.com/en-US/privacystatement). However, data will be collected only when the PSF binaries are used from [Nuget package](https://www.nuget.org/packages?q=packagesupportframework) 
on Windows 10 devices and only if users have enabled collection of data. The Nuget package has binaries signed and will collect usage data from machine. When the binaries are built locally by cloning the repo or downloading the bits, then telemetry is not collected.
