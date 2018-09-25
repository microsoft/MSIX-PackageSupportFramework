# Electron Fixup
The electron fixup is designed to allow a basic application using the electron framework to launch in an application container. The electron fixup is not designed as a one size fits all to allow every feature of an electron application to be executed in an app container.

## Usage
There is no specific configuration required for the fixup. Only the basic PSF dependencies are required as outlined in the [psfruntime usage readme](../../PsfRuntime/readme.md) under fixup loading. Also see [step-by-step instructions](https://docs.microsoft.com/en-us/windows/uwp/porting/package-support-framework) on PSF.

## Restrictions
Electron fixup relies on CreateFileFromAppW API which requires Windows SDK version 1803 (10.0.17134) or greater.
