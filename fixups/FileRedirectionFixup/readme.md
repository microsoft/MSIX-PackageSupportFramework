# File Redirection Fixup    
    
    
## Configuration    
The configuration for the File Redirection Fixup builds a collection of `[base path, relative path, pattern]` tuples. The `base path` is some filesystem path determined at runtime, e.g. the package path, the runtime value of `FOLDERID_SystemX64`, etc. The `relative path` is some static path relative to that directory. The pattern is a regular expression intended to match paths relative to the combination of `base path` and `relative path`. For example, to redirect accesses to `.txt` files under System32, you'd want the tuple `[FOLDERID_System, "", ".*\.txt"]`. This will match paths such as `C:\Windows\System32\foo.txt` or `C:\Windows\System32\subdir\bar.txt`. The `config` object inside of `config.json` for the File Redirection Fixup dll is expected to have the following form:    
    
`redirectedPaths` - This is the root element that all of these tuples are declared in. It is expected to be a value of type `object` whose properties correspond to some folder identifier. The allowed folder identifiers are:    
    
| Identifier | Description |    
| ---------- | ----------- |    
| `packageRelative` | Uses the package's installation root path as the base path. E.g. this is typically some path under `C:\Program Files\WindowsApps\`. This is expected to be a value of type `array` whose elements specify the relative paths/patterns described in more detail below |    
| `packageDriveRelative` | Uses the drive that the application is installed on as the base path (e.g. `C:\`). This is expected to be a value of type `array` whose elements specify the relative paths/patterns described in more detail below |    
| `knownFolders` | The base path in the tuple is resolved using `SHGetKnownFolderPath` for some known folder id. This is expected to be a value of type `array`. Each element is expected to be of type `object` whose properties are described in more detail below |    
    
The objects in the `knownFolders` array are expected to have the following form:    
    
| Property | Description |    
| -------- | ----------- |    
| `id` | A string identifier indicating the input to `SHGetKnownFolderPath`. This can be one of the strings listed below or a string of the form `"{GUID}"` for some `KNOWNFOLDERID` value. The supported non-GUID string identifiers mostly correspond to the [VFS redirected paths under the package root](https://docs.microsoft.com/en-us/windows/uwp/porting/desktop-to-uwp-behind-the-scenes), but with the `FOLDERID_` prefix removed.    
| `relativePaths` | This is expected to be a value of type `array` whose elements specify the relative paths/patterns described in more detail below |    
    
The list of supported named known folders are:    
    
| Name | `KNOWNFOLDERID` Equivalent |    
| ---- | -------------------------- |    
| `System` | `FOLDERID_System` |    
| `SystemX86` | `FOLDERID_SystemX86` |    
| `ProgramFilesX86` | `FOLDERID_ProgramFilesX86` |    
| `ProgramFilesX64` | `FOLDERID_ProgramFilesX64` |    
| `ProgramFilesCommonX86` | `FOLDERID_ProgramFilesCommonX86` |    
| `ProgramFilesCommonX64` | `FOLDERID_ProgramFilesCommonX64` |    
| `Windows` | `FOLDERID_Windows` |    
| `ProgramData` | `FOLDERID_ProgramData` |    
    
Each base path (known folder, package relative, drive relative) gets associated with a collection of relative paths and patterns. The format of each is identical and is an `object` whose properties are:    
    
| Property | Description |    
| -------- | ----------- |    
| `base` | Specifies the relative path portion of the tuple. This value gets appended to (with a directory separator) the base path to form a directory structure that's a candidate for redirection |    
| `patterns` | An `array` whose values are expected to be values of type `string`, each of which gets interpreted as a regular expression for matching paths relative to the base + relative path. |    
    
To make things simpler to understand, an example configuration object might look like:    
    
```json    
{    
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
                "id": "    {FDD39AD0-238F-46AF-ADB4-6C85480369C7}",    
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
    
In this example, the configuration is directing the File Redirection Fixup to redirect accesses to files with the `.log` extension under the `logs` folder that is relative to the package installation root. E.g. this would redirect an attempt to create the file `%ProgramFiles%\WindowsApps\Contoso.App_1.0.0.0_x64__wgeqdkkx372wm\logs\startup.log`. Similarly, the second configuration specifies that attempts to access _any_ file under the directory `temp` relative to the package root drive (e.g. `C:\temp\`) would get redirected. Moving on, the next configuration directs the fixup to redirect access to any file under the path created by combining the expansion of `FOLDERID_ProgramFilesX64` with `Contoso\config`. E.g. this would redirect an attempt to create the file `%ProgramFiles%\Contoso\config\foo.txt`. Finally, the last configuration is identical to the above, only it specifies the GUID of `FOLDERID_Documents`. E.g. this would redirect an attempt to create the file `%USERPROFILE%\Documents\MyApplication\settings.json`.    
    
In reality, most applications will only require redirecting access to either (1) the package path, or (2) a named known folder.    
    
## Redirected Paths    
Determining whether or not to redirect a path, and determining what that redirected path is, is a multi-step process. The first step in this process is to "normalize" the path. In essence, this primarily just involves expanding this path out to an absolute path (via `GetFullPathName`). It does _not_ perform any canonicalization; see the section on [Limitations](#limitations) for more information. Once the path is normalized, it is "de-virtualized." This involves mapping paths under the different package-relative `VFS` directories to their virtualized equivalent. E.g. a path under the `VFS\Windows` folder under the package path would get translated to the equivalent path under the expanded `FOLDERID_Windows` path. This is to ensure that references to the same file get redirected to the same location. Next, this path is compared to the set of configured paths. If the path "starts with" the configured path, then the remainder of the path is comopared to the configured regex pattern(s). If the remainder of the path matches the pattern, then the redirection kicks in. As a concrete example, consider the following scenario:    
    
> * The application makes an attempt to create the file `log.txt`    
> * The normalized path is `C:\Program Files\WindowsApps\Contoso.App_1.0.0.0_x64__wgeqdkkx372wm\VFS\ProgramFilesX64\Contoso\App\log.txt`    
> * The path gets de-virtualized to `C:\Program Files\Contoso\App\log.txt`    
> * The `config.json` includes the tuple `[FOLDERID_ProgramFilesX64, "Contoso\App", ".*\.txt"]`    
>   * This expands to the pair `base=C:\Program Files\Contoso\App, pattern=.*\.txt`    
> * The de-virtualized path "starts with" the base path. This leaves `log.txt` as the candidate for pattern matching    
> * `log.txt` _does_ match the pattern `.*\.txt` and therefore the create file attempt _will_ get redirected    
    
Once it is determined that the access should get redirected, the following steps are performed:    
    
> * The colon following the drive letter is replaced by a dollar sign. E.g. the path from before now becomes `C$\Program Files\Contoso\App\log.txt`    
> * The path is then appended to the local app data path (via `FOLDERID_LocalAppData`) appended with `VFS`. E.g. the resulting path would be something like `C:\Users\Bob\AppData\Local\VFS\C$\Program Files\Contoso\App\log.txt`    
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
