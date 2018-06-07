
# Scenario Functional Tests

The projects here are intended to be packaged up and run as Desktop Bridge applications. Each one exercises functionality that, run normally, will fail. There are multiple scripts to perform most of the heavy lifting for you, but you can find more information about packaging Desktop Bridge applications at the following locations:

Information on the directory structure, etc. of a Desktop Bridge package: https://docs.microsoft.com/en-us/windows/uwp/porting/desktop-to-uwp-behind-the-scenes

Instructions for manually packaging a Desktop Bridge app: https://docs.microsoft.com/en-us/windows/uwp/porting/desktop-to-uwp-manual-conversion

## Project Layout
Each directory that contains scenario(s) to test will contain:

* A project file and source file(s) necessary for building the test binaries
* An `AppxManifest.xml` file. Inside will be multiple `Application` tags: one or more for un-shimmed scenarios that should fail, and one or more shimmed scenarios that should succeed
* A `FileMapping.txt` file that is used as input to `makeappx.exe`'s `/f` argument. This file specifies the mapping of files locally on disk to their corresponding location in the appx package.
* Shim configuration files necessary for running the executable in "shimmed" mode

## Building
The `tests.sln` solution file in the parent directory should reference all test projects that need to get built. The required architectures will vary from project-to-project, but in general most rely on 64-bit binaries. The only exception are those explicitly testing 32-bit functionality. _All_ tests rely on using the Debug configuration as these are tests after all. Thus, it is recommended that you build both 64-bit Debug and 32-bit Debug unless you are testing a specific set of scenarios that don't have such requirements.

### Creating Appx-es
The restrictions that Desktop Bridge places on applications is only fully realized when installed through the "normal" appx install path. I.e. attempting to register the package in development mode - e.g. through `Add-AppxPackage -Register` - while useful for quick and dirty tests, won't necessarily reflect reality. For example, the filesystem ACLs that prevent apps from being able to write to their install path won't be present unless installed normally. Therefore, all tests are set up with the expectation that testing will occur after genarating and installing an appx.

The `MakeAppx.ps1` script is useful for automating this process. In essence, it will create an appx, placing it under the `Appx` folder, generate a certificate using the `CreateCert.ps1` script in the `signing` directory (if necessary), and sign the appx using that certificate. Note that this script requires that both `makeappx.exe` and `signtool.exe` - both a part of the Windows SDK - be a part of the PATH. Typically the easiset way to satisfy this requirement is by running powershell from a Visual Studio developer command prompt. See the [Project Layout](#Project-Layout) section for more information on the prerequisites for directory layout. Run `Get-Help MakeAppx.ps1` from a PowerShell command window for more detailed information.

## Testing
As mentioned in the [Project Layout](#Project-Layout) section, each scenario will have multiple entry points: one or more for un-shimmed scenarios and one or more for shimmed scenarios. Each test is authored to make success or failure easily identifiable. Typically this is done by printing out a success or failure string to the command line, though this is not necessarily 100% the case. The display name for each will indicate the expected result, using words like "good," "success," "shimmed," etc. to indicate that the executable should run without experiencing any runtime failures, and words like "bad," "fail," "un-shimmed," etc. to indicate that the executable _should_ experience some runtime failure. Success is measured by the shimmed versions running successfully; the un-shimmed versions are controls to validate that the executable would otherwise fail at runtime.

In order to execute a test, follow the above for [creating an appx](#Creating-Appx-es) for the scenario(s) you wish to test and install the appx, e.g. by using [PowerShell's `Add-AppxPackage` cmdlet](https://docs.microsoft.com/en-us/powershell/module/appx/add-appxpackage?view=win10-ps), or by double clicking on the appx and using the [App Installer](https://www.microsoft.com/en-us/store/p/app-installer/9nblggh4nns1?activetab=pivot%3aoverviewtab). Once installed, multiple entries will be added to your start menu - one for each entry point in the manifest. Launch the shimmed version to validate that there are no runtime errors. If you are making modifications to a test, or if you are authoring a new test, it is also good to launch the un-shimmed version to validate that the executable fails at runtime (i.e. the shims are actually doing something).
