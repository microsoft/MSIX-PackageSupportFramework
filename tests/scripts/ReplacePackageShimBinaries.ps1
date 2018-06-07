
<#
.DESCRIPTION
    Replaces shim binaries in a package with the same binaries built locally. Note that this attempts to do so for both
    x86 and x64, so if files with the same name exist in both output paths, one will win, and it may not be the
    architecture that you desired. Also note that you must have permission to replace these files. You can run
    TakePackageOwnership.ps1 to give admin ownership. Both of these scripts must be run from an admin Powershell command
    window

.PARAMETER Name
    The package name (note: _not_ family name/full name) of the package you wish to replace binaries for. Wildcards are
    technically allowed, in which case this will execute for any package returned by 'Get-AppxPackage'
#>

[CmdletBinding()]
Param (
    [Parameter(Mandatory=$True)]
    [string]$Name
)

function ReplaceBinaries($pkg)
{
    # All files we want to replace live at the root of the package path
    $pkgRoot = $pkg.InstallLocation
    $x64Path = "$PSScriptRoot\..\..\x64\Debug"
    $x86Path = "$PSScriptRoot\..\..\win32\Debug"

    function DoCopy($binary)
    {
        function Impl($src)
        {
            if (Test-Path "$src")
            {
                Copy-Item -Path "$src" -Destination "$pkgRoot\$binary"
            }
        }

        Impl "$x64Path\$binary"
        Impl "$x86Path\$binary"
    }

    foreach ($exe in (Get-ChildItem -Path "$pkgRoot" -Filter "*.exe"))
    {
        DoCopy $exe
    }

    foreach ($dll in (Get-ChildItem -Path "$pkgRoot" -Filter "*.dll"))
    {
        DoCopy $dll
    }
}

$packages = Get-AppxPackage -Name $Name

if ($packages -is [array])
{
    foreach ($pkg in $packages)
    {
        ReplaceBinaries $pkg
    }
}
else
{
    ReplaceBinaries $packages
}
