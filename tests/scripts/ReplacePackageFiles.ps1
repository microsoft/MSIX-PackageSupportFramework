
<#
.DESCRIPTION
    Replaces specified files in the package. Note that you must have permission to replace these files. You can run
    TakePackageOwnership.ps1 to give admin ownership. Both of these scripts must be run from an admin Powershell command
    window

.PARAMETER Name
    The package name (note: _not_ family name/full name) of the package you wish to replace binaries for. Wildcards are
    technically allowed, in which case this will execute for any package returned by 'Get-AppxPackage'

.PARAMETER Source
    The source file(s) you wish to copy into the package

.PARAMETER Destination
    The package-relative path you wish to copy the file to. When absent, copies the file(s) to the package root
#>

[CmdletBinding()]
Param (
    [Parameter(Mandatory=$True)]
    [string]$Name,

    [Parameter(Mandatory=$True)]
    [string]$Source,

    [Parameter(Mandatory=$False)]
    [alias("Dest")]
    [string]$Destination
)

function ReplaceFiles($pkg)
{
    $pkgRoot = $pkg.InstallLocation
    $dest = "$pkgRoot\$Destination"

    Copy-Item -Path "$Source" -Destination "$dest"
}

$packages = Get-AppxPackage -Name $Name

if ($packages -is [array])
{
    foreach ($pkg in $packages)
    {
        ReplaceFiles $pkg
    }
}
else
{
    ReplaceFiles $packages
}
