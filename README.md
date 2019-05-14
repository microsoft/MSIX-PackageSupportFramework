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
PSF allows one powershell script to be ran before an exe runs, and one powershell script to be ran after the exe runs.

Each exe defined in the application manifest can have their own scripts.

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
 2. scriptArguments: Space delimited argument list.  The format is the same for a powershell script call.  This string gets appended to scriptPath to make a valid powershell.exe call.
 3. runInVirtualEnvironment: If the script should run in the same virtual enviorment that the packaged exe runs in.
 
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
		"runInAppEnviorment": "true"
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
		"runInAppEnviorment": "true"
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

