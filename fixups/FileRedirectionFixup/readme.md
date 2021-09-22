# File Redirection Fixup
When injected into a process, the File Redirection Fixup supports the ability to:
> * Cause certain files accessed in the package to be copied to a safe area where they may be locally manipulated as needed.
> * Specify file path/name patterns of folders and files that require special treatment when accessed by the application.
> * Specify file path/name patterns of folders and files that should be excluded from special treatment.
> * Specify rules for behavior of the appropriate Windows APIs when making file calls that match the pattern.
> * Once the redirection location is determined the behavior is usually:
> > * In the case of a file, if the file is not present there it is usually copied from the requested location to the destination (creating folder structures as needed), and from then on the redirected copy is used.
> > * In the case of a folder the redirected folder is created (including folder structures as needed) and used.
> * The rule may optionally have additional configuration to modify the behavior, such as to control the redirected location or exempt a file from redirection.

This configuration is specified in the `processes` se
## Configuration
The configuration for the File Redirection Fixup is specified under the element `config` of the fixup structure within the json file when FileRedirectionFixup.dll is requested.
This `config` element contains a single property named `redirectedPaths`.

`redirectedPaths` - This is the root PropertyName element that all of these configuration collections are declared in. 
The value of this property is expected to be of type `array`, containing up to three different types of optional objects. The supported PropertyNames allowed under `redirectedPaths` are:

| PropertyName | Description |
| ------------ | ----------- |
| `packageRelative` | Uses the package's installation root path as the base path. E.g. this is typically some path under `C:\Program Files\WindowsApps\`. This is expected to be a value of type `array` whose elements specify the relative paths/patterns described in more detail below |
| `packageDriveRelative` | Uses the drive that the application is installed on as the base path (e.g. `C:\`). This is expected to be a value of type `array` whose elements specify the relative paths/patterns described in more detail below |
| `knownFolders` | The base path in the tuple is resolved using `SHGetKnownFolderPath` for some known folder id. This is expected to be a value of type `array`. Each element is expected to be of type `object` whose properties are described in more detail below |

While all three PropertNames may be specified, typically only one of these will be used depending on the form that you want to use when you specify file paths.  
It is also possible, but atypical, to specify a given PropertyName more than once. 

The value of each of these properties is itself of type `array`. 
This array contains a collection of unnamed value objects that can be thought of as redirection control objects. 
These redirection control objects are used in identifying potential redirections and has source and destination identifiers along with special options that can control specifics of the rename operation.
While the configuration structure of the three types of `redirectedPaths` are similar, there are sufficient differences that they should be individually described.

Note that in general, the processing of file operations that are trapped will scan the configuration top-down, looking for the first redirection control that matches the file/directory being operated on. 
When a matching redirection control is found for the source of file operation, that matching redirection control will be used and further scanning of the configuration is terminated. 
This allows for an exclusion rule to be placed early in the array that has a specific pattern match with the exclusion redirection control to be followed by a more inclusive pattern match with a different redirection control.

### Configuration for `packageRelative` object array elements
Each unnamed element of the array has the following structure:

| PropertyName | Description |
| ------------ | ----------- |
| `base` | Base folder designation of potential sources, to be used in pattern matching. The form of the value is described below. |
| `patterns` | An array of RegEx strings used on sources under the `base` for matching. |
| `redirectTargetBase` | (Optional) Base folder designation for destination. When not specified, the VFS folder under the user's LocalAppData folder will be used.|
| `isExclusion` | (Optional) When true, redirection operations are not allowed on this pattern match. This value defaults to false when not present, but when set to true to implement an exclusion as described below. |
| `isReadOnly` | (Optional) When true, a copy on access and redirection operation is performed, but the copy should be read-only to prevent changes. |

The value of `base` is to be a path that is relative to the installation folder of the package. 
The full path for any pattern matching will start from a runtime determined construction of the installation root of the package, followed by the value of `base`.
Special IDs for known folders are not permitted to be specified in the `base` of `packageRelative` objects.

### Configuration for 'packageDriveRelative` object array elements
Each unnamed element of the array has the following structure:

| PropertyName | Description |
| ------------ | ----------- |
| `base` | Base folder designation of potential sources, to be used in pattern matching. The form of the value is described below. |
| `patterns` | An array of RegEx strings used on sources under the `base` for matching. |
| `redirectTargetBase` | (Optional) Base folder designation for destination. When not specified, the VFS folder under the user's LocalAppData folder will be used.|
| `isExclusion` | (Optional) When true, redirection operations are not allowed on this pattern match. This value defaults to false when not present, but when set to true to implement an exclusion as described below. |
| `isReadOnly` | (Optional) When true, a copy on access and redirection operation is performed, but the copy should be read-only to prevent changes. |

The value of `base` is to be a folder path relative to the drive that the package is installed to (nominally C:\).
The full path for any pattern matching will start from a runtime determined construction of the drive of the installed package, followed by the value of `base`.
Special IDs for known folders are not permitted to be specified in the `base` of `packageDriveRelative` objects.

### Configuration for `knownFolders` object array elements
Each unnamed element of the array has the following structure:

| PropertyName | Description |
| ------------ | ----------- |
| `id` | A string identifier indicating the input to `SHGetKnownFolderPath`. This can be one of the strings listed below or a string of the form `"{GUID}"` for some `KNOWNFOLDERID` value. The supported non-GUID string identifiers mostly correspond to the [VFS redirected paths under the package root](https://docs.microsoft.com/en-us/windows/uwp/porting/desktop-to-uwp-behind-the-scenes), but with the `FOLDERID_` prefix removed.
| `relativePaths` | This is expected to be a value of type `array` whose elements specify the relative paths/patterns described in more detail below |
| `relativePaths` | This is expected to be a value of type `array` whose unnamed elements specify the relative paths/patterns described in more detail below |


The list of supported named known folder ids are:

| `id` | `KNOWNFOLDERID` Equivalent |
| ---- | -------------------------- |
| `System` | `FOLDERID_System` |
| `SystemX86` | `FOLDERID_SystemX86` |
| `ProgramFilesX86` | `FOLDERID_ProgramFilesX86` |
| `ProgramFilesX64` | `FOLDERID_ProgramFilesX64` |
| `ProgramFilesCommonX86` | `FOLDERID_ProgramFilesCommonX86` |
| `ProgramFilesCommonX64` | `FOLDERID_ProgramFilesCommonX64` |
| `Windows` | `FOLDERID_Windows` |
| `ProgramData` | `FOLDERID_ProgramData` |
| `LocalAppData` | `FOLDERID_LocalAppData` |
| `RoamingAppData` | `FOLDERID_RoamingAppData` |


The value for `base` is expected to be a known folder identifier, specified either in the form of `FOLDERID_xxx` or in GUID form. The full path for any pattern matching will start from a runtime determined value provided by `SHGetKnownFolderPath` against this base.

Each element of the of the `knownFolders` type has the following structure:

| PropertyName | Description |
| ------------ | ----------- |
| `base` | Base folder designation of potential sources, to be used in pattern matching. The form of the value is described below. |
| `patterns` | An array of RegEx strings used on sources under the `base` for matching. |
| `redirectTargetBase` | (Optional) Base folder designation for destination. When not specified, the VFS folder under the user's LocalAppData folder will be used.|
| `isExclusion` | (Optional) When true, redirection operations are not allowed on this pattern match. This value defaults to false when not present, but when set to true to implement an exclusion as described below. |
| `isReadOnly` | (Optional) When true, a copy on access and redirection operation is performed, but the copy should be read-only to prevent changes. |

The value of `base` is to be a path that is relative to the known folder id already specified. 
The full path for any pattern matching will start from a runtime determined construction of the local value of the knownFolder `id`, followed by the value of `base`.
Special IDs for known folders are not permitted to be specified in the `base` in this situation.

## Configuration of `Patterns` Parameter
In each type of redirection described above, `Patterns` is expected to be an object type of array.  Each unnamed element of the array is a string that is to be used in RegEx based pattern matching.  
The RegEx algorithm used if from the std library and uses the default syntax for ECMAScript (see https://docs.microsoft.com/en-us/cpp/standard-library/regular-expressions-cpp?view=vs-2017). 

The pattern may include folders (keeping in mind that this is relative to the base) if desired. Some examples of patterns:

> * `.\` A simple way to specify that all files and folders under the base, no matter how many levels deep will match.
> * `Foo\\.txt$` A way to specify that only a file named exactly as Foo.txt in a case sensitive match directly in the base folder will match.
> * `.*\\.[iI][nN][iI]$` A way to specify that only all files with file extensions of ini in upper or lower case.

## Configuration for `redirectTargetBase` Parameter
In each type of redirection described above, the value of `redirectTargetBase` is expected to be a string.  
Normally, this is expected to be a known folder id in the form of `FOLDERID_xxx` or a GUID (including the `{` and `}` characters) of a known folder as supported by `SHGetKnownFolderPath`.

Absolute target paths, while discouraged, may also be specified. 
The specification of absolute target paths might be required to support redirection to locations not managed by MSIX, such as the local caching folder of cloud based storage or network shares.

When not specified, the `redirectTargetBase` value will default to `FOLDERID_LocalAppData`.
When the value of `redirecteargetBase` is either the `FOLDERID_LocalAppData` or `FOLDERID_RoamingAppData`, the effective redirectTargetBase will have a `VFS` folder added to it.

The ultimate redirection location of a given source file matched to a redirection rule is complicated, and will be described after some examples. 
For now, consider that it will end up in a destination constructed from the effective redirectTargetBase, package identification, and path that the source was relative to the source base. 

## Configuration of the `unmanagedRetarget` Parameter
The value of this parameter is a boolean, and defaults to false when not present.  Specifying this as true allows redirection to locations not managed by the MSIX runtime.  
This allows for redirection to common locations such as home drives, file shares, and cloud based storage.

# JSON Examples
To make things simpler to understand, here is a potential example configuration object that is not using the optional parameters:

```json
"config": {
    "redirectedPaths": {
        "packageRelative": [
            {
                "base": "logs",
                "patterns": [
                    ".*\\.log"
                ]
            }
        ],
        "packageDriveRelative": [
            {
                "base": "temp",
                "patterns": [
                    ".*"
                ]
            }
        ],
        "knownFolders": [
            {
                "id": "ProgramFilesX64",
                "relativePaths": [
                    {
                        "base": "Contoso\\config",
                        "patterns": [
                            ".*"
                        ]
                    }
                ]
            },
            {
                "id": " {FDD39AD0-238F-46AF-ADB4-6C85480369C7}",
                "relativePaths": [
                    {
                        "base": "MyApplication",
                        "patterns": [
                            ".*"
                        ]
                    }
                ]
            }
        ]
    }
}
```

#XML Example
Here is the config section of the File Redirection Fixup for xml.

```xml
<config>
    <redirectedPaths>
        <packageRelative>
            <pathConfig>
                <base>logs</base>
                <patterns>
                    <pattern>.*\\.log</pattern>
                </patterns>
            </pathConfig>
        </packageRelative>
        <packageDriveRelative>
            <pathConfig>
                <base>temp</base>
                <patterns>
                    <pattern>.*</pattern>
                </patterns>
            </pathConfig>
        </packageDriveRelative>
        <knownFolders>
            <knownFolder>
                <id>ProgramFilesX64</id>
                <relativePaths>
                    <relativePath>
                        <base>Contoso\\config</base>
                        <patterns>
                            <pattern>.*</pattern>
                        </patterns>
                    </relativePath>
                </relativePaths>
            </knownFolder>
        </knownFolders>
    </redirectedPaths>
</config>
```


In the example above, the configuration is directing the File Redirection Fixup to redirect accesses to files with the `.log` extension under the `logs` folder that is relative to the package installation root. E.g. this would redirect an attempt to create the file `%ProgramFiles%\WindowsApps\Contoso.App_1.0.0.0_x64__wgeqdkkx372wm\logs\startup.log`. Similarly, the second configuration specifies that attempts to access _any_ file under the directory `temp` relative to the package root drive (e.g. `C:\temp\`) would get redirected. Moving on, the next configuration directs the fixup to redirect access to any file under the path created by combining the expansion of `FOLDERID_ProgramFilesX64` with `Contoso\config`. E.g. this would redirect an attempt to create the file `%ProgramFiles%\Contoso\config\foo.txt`. Finally, the last configuration is identical to the above, only it specifies the GUID of `FOLDERID_Documents`. E.g. this would redirect an attempt to create the file `%USERPROFILE%\Documents\MyApplication\settings.json`.

In reality, most applications will only require redirecting access from either (1) the relative package path, or (2) a named known folder.

ere is a second example:

```json
"config": {
    "redirectedPaths": {
        "packageRelative": [
            {
                "base": "configs",
                "patterns": [
                    "AutoUpdate\\.ini$"
                ]
				"redirectTargetBase": "H:"
				"IsReadOnly": "true" 
            },
            {
                "base": "configs",
                "patterns": [
                    ".*\\.[eE][xX][eE]$"
                    ".*\\.[dD][lL][lL]$"
                    ".*\\.[tT][lL][bB]$"
                    ".*\\.[cC][oO][mM]$"
                ]
				"IsExclusion": "true" 
            },
			{
                "base": "configs",
                "patterns": [
                    ".*"
                ]
				"redirectTargetBase": "H:"
            },
			{
                "base": "",
                "patterns": [
                    ".*"
                ]
            }
		]
    }
}
```
To read the example, it might help to read from the bottom element up. In this example, we enable:

> * Most files for copy on write operation, redirecting to a safe place the default LocalAppData folder.
> * Except for files that were under the configs subfolder of the package. for these:
> > * Most will be redirected to a safe place in the user's home drive (PackageCache\Packagename).
> > * Except for certain Windows PE file types
> > * And except for one named AutoUpdate.ini, which is to remain as read only as provided in the package to prevent the end-user from configuring updates back on.

```xml
<config>
    <redirectedPaths>
        <packageRelative>
            <pathConfig>
                <base>configs</base>
                <patterns>
                    <pattern>AutoUpdate\\.ini$</pattern>
                </patterns>
                <redirectedTargetBase>H:</redirectedTargetBase>
                <IsReadOnly>true</IsReadOnly>
            </pathConfig>
            <pathConfig>
                <base>configs</base>
                <patterns>
                    <pattern>.*\\.[eE][xX][eE]$</pattern>
                    <pattern>.*\\.[dD][lL][lL]$</patterns>
                    <pattern>.*\\.[tT][lL][bB]$</pattern>
                    <pattern>.*\\.[cC][oO][mM]$</pattern>
                </patterns>
                <IsExclusion>true</IsExclusion>
            </pathConfig>
            <pathConfig>
                <base>configs</base>
                <patterns>
                    <pattern>.*</pattern>
                <patterns>
                <redirectTargetBase>H:</redirectTargetBase>
            </pathConfig>
            <pathConfig>
                <base></base>
                <patterns>
                    <pattern>.*</pattern>
                </patterns>
            </pathConfig>
        </packageRelative>
    </redirectedPaths>
</config>
```

# Redirected Path Calculations
Determining whether or not to redirect a path, and determining what that redirected path is, is a multi-step process. 

The first step in this process is to "normalize" the path. 
In essence, this primarily just involves expanding this path out to an absolute path (via `GetFullPathName`). It does _not_ perform any canonicalization; see the section on [Limitations](#limitations) for more information. 

Once the path is normalized, it is "de-virtualized." 
This involves mapping paths under the different package-relative `VFS` directories to their virtualizion equivalent path. 
For example, a path under the package path `VFS\Windows` folder would get translated to the equivalent path under the expanded `FOLDERID_Windows` path. 
This is to ensure that references to the same file get redirected to the same location. 

Next, this path is compared to the set of redirection rules base paths. If the path "starts with" the redirection rule base path, then the remainder of the path is compared to the regex pattern(s) of that rule. 
If the remainder of the path matches the pattern, then the redirection kicks in as configured by that rule. As a concrete example, consider the following scenario:

> * The application makes an attempt to create the file `log.txt`
> * The normalized path is `C:\Program Files\WindowsApps\Contoso.App_1.0.0.0_x64__wgeqdkkx372wm\VFS\ProgramFilesX64\Contoso\App\log.txt`
> * The path gets de-virtualized to `C:\Program Files\Contoso\App\log.txt`
> * The `config.json` includes a `knownFolders` redirection rule `[FOLDERID_ProgramFilesX64, "Contoso\App", ".*\.txt", "FOLDERID_LocalAppData","false"]`
> * This expands to the rule `base=C:\Program Files\Contoso\App, pattern=.*\.txt`, "C:\Users\Username\AppData\Local", "false" ]
> * The de-virtualized path "starts with" the base path of this rule. This leaves `log.txt` as the candidate for pattern matching
> * `log.txt` _does_ match the pattern `.*\.txt` and therefore the create file attempt _will_ get redirected to the user appdata local area.

Redirecting paths in the package root path.  

In order to make sure packages that write to their package root don't break on an upgrade, path redirection will redirect paths to one of two places  

> 1. %LOCAL_APP_DATA%\Packages\[Package family name]\LocalCache\Local\Microsoft\WritablePackageRoot
> 1. %LOCAL_APP_DATA%\VFS

If the path to redirect is in the package root path then the redirected path is to the writable package root (option 1).  
For all other paths, the redirected path is to VFS (option 2).

Once it is determined that the access should get redirected, the following steps are performed:

> * The colon following the drive letter is replaced by a dollar sign. E.g. the path from before now becomes `C$\Program Files\Contoso\App\log.txt`
> * The path is then appended to the local app data path (via `FOLDERID_LocalAppData`) appended with `VFS`. E.g. the resulting path would be something like `C:\Users\Username\AppData\Local\VFS\C$\Program Files\Contoso\App\log.txt`
> * After the FileRedirectionShim does this, MSIX will further redirect this by substituting the LocalCache folder for this package, so it would be something like `C:\Users\Username\AppData\Local\Packages\Packagename\VFS\C$\Program Files\Contoso\App\log.txt`.
> * The directory structure is conditionally constructed, if the calling function indicates that it should be (e.g. `CreateFile` would need the directory structure to exist, but `DeleteFile` wouldn't)
> * The file - if it exists - is conditionally copied to this location if it hasn't already been and the calling function indicates that it should be (e.g. `CreateFile` with the intent to modify the file would require that the file be copied, but `DeleteFile` wouldn't)

## Limitations
Since the File Redirection Fixup runs in user mode and works with file paths, it is inherently imperfect. For example, all of the following paths may actually refer to the same file:

> 1. `foo.txt`
> 1. `C:foo.txt`
> 1. `C:\path\to\foo.txt`
> 1. `C:/path/to/foo.txt`
> 1. `\path\to\foo.txt`
> 1. `G:\this\doesnt\look\like\the\path\to\foo.txt` (in the case of symlinks/hardlinks)
> 1. `\\?\C:\path\to\foo.txt`
> 1. `\\.\C:\path\to\foo.txt`
> 1. `\\localhost\C$\path\to\foo.txt`
> 1. `\\127.0.0.1\C$\path\to\foo.txt`
> 1. `\\?\UNC\localhost\C$\path\to\foo.txt`
> 1. `\\?\HarddiskVolume2\path\to\foo.txt`
> 1. `\\?\GLOBALROOT\Device\Harddisk0\Partition2\path\to\foo.txt`

And the list could go on forever... Accounting for these scenarios is primarily a question of tradeoffs and probability. For example, it's reasonable to expect an application to reference a file using methods 1-5, and possibly even 7 or 8, so we make sure to properly handle these inputs. The others are considerably less likely - some more so than others - and handling them would introduce additional complexities and almost certainly performance penalties, so we opt not to handle these scenarios.

### Deleting Files/Directories
Re-directing file reads and writes is relatively simple since we can ignore any equivalent file in the non-redirected location, with the exception of copying it initially, if needed. This is not true for deleting a file since the application expects that subsequent attempts to reference that file will either fail or create a new file, depending on the operation being performed. Thus we can't just delete the file in the redirected location, but must also delete any equivalent non-redirected file that may exist. This is an issue since we _can't_ delete such a file; that's the whole point of the fixup. At the moment, this scenario is not handled and we will only make an attempt to delete the file using the redirected path. In the future, one possibility is to maintain a list of deleted files that get ignored during the copy-on-read step, which will make it appear as if the file doesn't exist to the application.

### Changing Directories
The Package Support Framework does not currently handle scenarios where an application attempts to change its current directory to one whose creation was redirected. That is, `SetCurrentDirectory` is not fixed. Adding support likely wouldn't be all that difficult - all paths would effectively have to undergo an initial "de-redirection" step similar to the "de-virtualization" step - but the cost/risk/benefit of such a change isn't well enough understood at this time.


### RoamingAppData
The MSIX runtime, which operates after the PSF redirections, will automatically redirect any writes made by contained processes that are aimed at the user's AppData\Roaming folder, whether requested directly by the application or if redirected by this shim.  These writes are re-redirected to the user's AppData\Local\Packages\PackageName\LocalCache\Roaming folder. 

Developers can use a different API to persist data in the user's AppData\Local\Packages\PackageName\RoamingState folder. For more information on this folder, how it is cloud backed, and limitations, see the blog post: https://blogs.windows.com/buildingapps/2016/05/04/roaming-app-data-and-the-user-experience/#4AuZVb5pUlw4O21b.97 