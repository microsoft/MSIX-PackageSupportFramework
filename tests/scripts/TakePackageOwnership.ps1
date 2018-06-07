
<#
.DESCRIPTION
    Changes the ownership of all executables, dlls, and a few key files in a package's root path to the device's
    administrator group. Note that you must have permission to replace these files.This script must be run from an admin
    Powershell command window

.PARAMETER Name
    The package name (note: _not_ family name/full name) of the package you wish to replace binaries for. Wildcards are
    technically allowed, in which case this will execute for any package returned by 'Get-AppxPackage'
#>

[CmdletBinding()]
Param (
    [Parameter(Mandatory=$True)]
    [string]$Name
)

function TakeOwnership($pkg)
{
    # All files we want to take ownership of live at the root of the package path
    $pkgRoot = $pkg.InstallLocation

    $accessRule = New-Object System.Security.AccessControl.FileSystemAccessRule("Administrators", "FullControl", "Allow")

    function Impl($path)
    {
        Write-Verbose "Taking ownership of $path"

        # First take ownership
        takeown.exe /F "$path" /A | Out-Null
        if ($LASTEXITCODE -ne 0)
        {
            throw "Failed to take ownership of $path"
        }

        # Give full control to admin
        $acl = Get-Acl "$path"
        if ($acl)
        {
            $acl.SetAccessRule($accessRule)
            Set-Acl "$path" $acl
        }
        else
        {
            Write-Warning "Failed to get ACL for $path"
        }

        if ((Get-Item $path).Attributes -eq [System.IO.FileAttributes]::Directory)
        {
            foreach ($item in (Get-ChildItem $path))
            {
                Impl $item.FullName
            }
        }
    }

    # Change ownership permissions for all executables, dlls, and files we may want to replace in the root directory to admin
    Impl $pkg.InstallLocation
}

$packages = Get-AppxPackage -Name $Name

if ($packages -is [array])
{
    foreach ($pkg in $packages)
    {
        TakeOwnership $pkg
    }
}
else
{
    TakeOwnership $packages
}
