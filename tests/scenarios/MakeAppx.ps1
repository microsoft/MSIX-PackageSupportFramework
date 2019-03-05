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
    [Parameter(Mandatory=$True, ParameterSetName='SingleAppx')]
    [string]$Name,

    [Parameter(Mandatory=$True, ParameterSetName='MultipleAppx')]
    [switch]$All,

    [Parameter(Mandatory=$False)]
    [ValidateSet('x86','x64')]
    [Alias('Arch')]
    [string]$Architecture = "x64",

    [Parameter(Mandatory=$False)]
    [ValidateSet('Debug','Release')]
    [Alias('Config')]
    [string]$Configuration = "Debug"
)

$appxRoot = "$PSScriptRoot\Appx\"

function CreateSingle($name)
{
    Write-Host -NoNewline "Creating appx for "
    Write-Host -ForegroundColor Green $name

    if (-not (Test-Path "$appxRoot"))
    {
        mkdir $appxRoot
    }

    Push-Location $name
    try
    {
        $appxPath = "$appxRoot$name.appx"
        . makeappx.exe pack /o /p "$appxPath" /f "$PSScriptRoot\$name\$Architecture$Configuration\FileMapping.txt"
        if ($LastExitCode -ne 0)
        {
            throw "ERROR: Appx creation failed!"
        }

        Write-Host -NoNewline "Appx created successfully! [Output: "
        Write-Host -NoNewline -ForegroundColor Green "$appxPath"
        Write-Host "]"

		$passwordAsPlainText = "CentennialFixupsTestSigning"
        # Ensure that a signing certificate has been created
        Write-Host "Signing the appx..."
        . "$PSScriptRoot\signing\CreateCert.ps1" -Install -passwordAsPlainText $passwordAsPlainText
        . signtool.exe sign /p $passwordAsPlainText /a /v /fd sha256 /f "$PSScriptRoot\signing\CentennialFixupsTestSigningCertificate.pfx" "$appxPath"
    }
    finally
    {
        Pop-Location
    }
}

function CreateAll
{
    # Determine the set of projects by sub-directories with a FileMapping.txt file
    foreach ($dir in (Get-ChildItem "$PSScriptRoot"))
    {
        if (Test-Path "$dir\FileMapping.txt")
        {
            CreateSingle $dir.Name
        }
    }
}

switch ($PSCmdlet.ParameterSetName)
{
    "SingleAppx"
        {
            # Input should be a directory; normalize since the input may look like ".\name\" but we expect it to be of the form "name"
            CreateSingle (Get-Item $Name).Name
        }
    "MultipleAppx" { CreateAll }
}
