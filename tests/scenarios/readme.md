
# Scenario Functional Tests

The projects here are intended to be packaged up and run as Desktop Bridge applications. Each one exercises functionality that, run normally, will typically fail. There are multiple scripts to perform most of the heavy lifting for you, but you can find more information about packaging Desktop Bridge applications at the following locations:

Information on the directory structure, etc. of a Desktop Bridge package: https://docs.microsoft.com/en-us/windows/uwp/porting/desktop-to-uwp-behind-the-scenes

Instructions for manually packaging a Desktop Bridge app: https://docs.microsoft.com/en-us/windows/uwp/porting/desktop-to-uwp-manual-conversion

## Project Layout
Each directory that contains scenario(s) to test will contain:

> * A project file and source file(s) necessary for building the test binaries
> * An `AppxManifest.xml` file. Inside will be multiple `Application` tags: one or more for un-fixed scenarios that should fail, and one or more fixed scenarios that should succeed
> * A `FileMapping.txt` that's used as a "recipe" for creating an architecture/configuration-specific `FileMapping.txt` that is used as input to `makeappx.exe`'s `/f` argument. This file specifies the mapping of files locally on disk to their corresponding location in the appx package.
> * A `config.json` file

## Building
The `tests.sln` solution file in the parent directory should reference all test projects that need to get built. In general, most tests require a single architecture/configuration. The one exception is the architecture test, which requires both 32-bit and 64-bit binaries.

### Creating Appx-es
The restrictions that Desktop Bridge places on applications is only fully realized when installed through the "normal" appx install path. I.e. attempting to register the package in development mode - e.g. through `Add-AppxPackage -Register` - while useful for quick and dirty tests, won't necessarily reflect reality. For example, the filesystem ACLs that prevent apps from being able to write to their install path won't be present unless installed normally. Therefore, all tests are set up with the expectation that testing will occur after genarating and installing an appx.

The `MakeAppx.ps1` script is useful for automating this process. In essence, it will create an appx, placing it under the `Appx` folder, generate a certificate using the `CreateCert.ps1` script in the `signing` directory (if necessary), and sign the appx using that certificate. Note that this script requires that both `makeappx.exe` and `signtool.exe` - both a part of the Windows SDK - be a part of the PATH. Typically the easiset way to satisfy this requirement is by running powershell from a Visual Studio developer command prompt. See the [Project Layout](#project-layout) section for more information on the prerequisites for directory layout. Run `Get-Help MakeAppx.ps1` from a PowerShell command window for more detailed information.

## Testing
As mentioned in the [Project Layout](#project-layout) section, each scenario will have multiple entry points: one or more for un-fixed scenarios and one or more for fixed scenarios. The un-fixed versions should fail most, if not all tests when run whereas the fixed variants should pass all tests when run. The name of the application in the start menu will indicate which variant is which.

In order to execute a test, follow the above for [creating an appx](#creating-appx-es) for the scenario(s) you wish to test and install the appx, e.g. by using [PowerShell's `Add-AppxPackage` cmdlet](https://docs.microsoft.com/en-us/powershell/module/appx/add-appxpackage?view=win10-ps), or by double clicking on the appx and using the [App Installer](https://www.microsoft.com/en-us/store/p/app-installer/9nblggh4nns1?activetab=pivot%3aoverviewtab). Once installed, multiple entries will be added to your start menu - one for each entry point in the manifest. Launch the fixed version to validate that there are no runtime errors. If you are making modifications to a test, or if you are authoring a new test, it is also good to launch the un-fixed version to validate that the executable fails at runtime (i.e. the fixups are actually doing something).
