<#
.DESCRIPTION
    Creates and signs either a single appx or all appx-es under the "scenario" directory. Note that it is expected that
    all necessary binaries have already been built prior to running this command. Also note that this script expects
    'makeappx.exe' and 'signtool.exe' to be a part of the path. This is most easily accomplished by starting PowerShell
    from a Visual Studio developer command prompt.

.PARAMETER Name
    The name of the directory that contains the project you wish to generate an appx for. This directory is expected to
    contain a "FileMapping.txt" which serves as input to the 'makeappx' executable's '/f' argument

.PARAMETER All
    Creates an appx for all subdirectories under the "scenario" directory. It uses the presence/absence of the
    "FileMapping.txt" (see above) to determine which directories to generate appx-es for.
#>

[CmdletBinding()]
Param (
    [Parameter(Mandatory=$True, ParameterSetName='SingleUpdate')]
    [string]$Name,

    [Parameter(Mandatory=$True, ParameterSetName='MultipleUpdate')]
    [switch]$All
)

$appxRoot = "$PSScriptRoot\"

function UpdateSingle($name)
{
    function impl($arch, $config)
    {
        # NOTE: '86' does not have a directory like 'x64' does
        $archDir = if ($arch -eq "x86") { "Win32\" } else { "x64\" }
        $bitness = if ($arch -eq "x86") { "32" } else { "64" }

        $targetDir = "$PSScriptRoot\$name\$arch$config\"
        if (-not (Test-Path "$targetDir"))
        {
            New-Item -Path "$targetDir" -ItemType Directory | Out-Null
        }

        (Get-Content "$PSScriptRoot\$name\FileMapping.txt") `
            -replace '\$\{Architecture\}', $archDir `
            -replace '\$\{ArchFolderId\}', $arch.ToUpper() `
            -replace '\$\{Configuration\}', $config `
            -replace '\$\{Bitness\}', $bitness `
            | Set-Content "$targetDir\FileMapping.txt"
    }

    impl "x64" "Debug"
    impl "x64" "Release"
    impl "x86" "Debug"
    impl "x86" "Release"
}

function UpdateAll
{
    # Determine the set of projects by sub-directories with a FileMapping.txt file
    foreach ($dir in (Get-ChildItem "$PSScriptRoot"))
    {
        if (Test-Path "$dir\FileMapping.txt")
        {
            UpdateSingle $dir.Name
        }
    }
}

switch ($PSCmdlet.ParameterSetName)
{
    "SingleUpdate"
        {
            # Input should be a directory; normalize since the input may look like ".\name\" but we expect it to be of the form "name"
            UpdateSingle (Get-Item $Name).Name
        }
    "MultipleUpdate" { UpdateAll }
}
