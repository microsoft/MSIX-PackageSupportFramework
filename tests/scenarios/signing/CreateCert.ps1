<#
.DESCRIPTION
    Creates or removes a self-signed certificate that can be used for appx signing, optionally installing/uninstalling
    it from the trusted root store.

.PARAMETER FriendlyName
    The friendly name given to the certificate and used to lookup a pre-existing cert. By default, this value is
    "CentennialFixups Test Signing Certificate"

.PARAMETER Subject
    Subject used when creating the certificate. The default is "CN=Microsoft Corporation, O=Microsoft Corporation, L=Redmond, S=Washington, C=US".
    Note that when using the certificate for appx signing this must match the AppxManifest, so using something other
    than the default for the test appx-es is not recommended

.PARAMETER CertStoreLocation
    Target store to hold the created certificate. The default is "Cert:\LocalMachine\My"

.PARAMETER FileName
    The output ".pfx" file name. The default is "CentennialFixupsTestSigningCertificate.pfx". Note that other scripts may
    try and reference this hardcoded name, so using the default is recommended.

.PARAMETER Force
    Forces the (re-)creation of the certificate, even if one with the same friendly name already exists. This is
    effectively the same as running with -Clean followed by a normal create call

.PARAMETER Install
    Installs the certificate to the machine's trusted root store. Note that this is necessary in order to install appx-es
    that are signed with using the generated certificate

.PARAMETER Uninstall
    Uninstalls the certificate from the machine's trusted root store

.PARAMETER Clean
    Removes the certificate from the machine's trusted root store, the specified certificate store, and deletes the
    generated ".pfx" file from the filesystem
#>

[CmdletBinding(DefaultParameterSetName="CertCreation")]
Param (
    [Parameter(ParameterSetName='CertCreation')]
    [Parameter(ParameterSetName='CertUninstallation')]
    [Parameter(ParameterSetName='Clean')]
    [string]$FriendlyName="CentennialFixups Test Signing Certificate",

    [Parameter(ParameterSetName='CertCreation')]
    [string]$Subject="CN=Microsoft Corporation, O=Microsoft Corporation, L=Redmond, S=Washington, C=US",

    [Parameter(ParameterSetName='CertCreation')]
    [string]$CertStoreLocation="Cert:\LocalMachine\My",

    [Parameter(ParameterSetName='CertCreation')]
    [string]$FileName="CentennialFixupsTestSigningCertificate.pfx",
	
	[Parameter(ParameterSetName='CertCreation')]
	[string]$passwordAsPlainText,

    [Parameter(ParameterSetName='CertCreation')]
    [switch]$Force,

    [Parameter(ParameterSetName='CertCreation')]
    [switch]$Install,

    [Parameter(ParameterSetName='CertUninstallation')]
    [switch]$Uninstall,

    [Parameter(ParameterSetName='Clean')]
    [switch]$Clean
)

$certFile = "$PSScriptRoot\$FileName"

function TryGetCert
{
    $results = Get-ChildItem "$CertStoreLocation" | Where-Object { $_.FriendlyName -eq "$FriendlyName" }
    if ($results.Count -gt 0)
    {
        return $results[0];
    }

    return $null
}

function Cleanup
{
    Get-ChildItem "Cert:\LocalMachine\Root" | Where-Object { $_.FriendlyName -eq "$FriendlyName" } | Remove-Item
    Get-ChildItem "$CertStoreLocation" | Where-Object { $_.FriendlyName -eq "$FriendlyName" } | Remove-Item

    if (Test-Path "$certFile")
    {
        Get-ChildItem "$certFile" | Remove-Item
    }
}

function CreateCert()
{
    if ($Force)
    {
        Cleanup
    }

    # Create a cert in the specified store if one does not already exist
    $cert = TryGetCert
    if ($cert -eq $null)
    {
        $cert = New-SelfSignedCertificate -Type Custom -Subject "$Subject" -KeyUsage DigitalSignature -FriendlyName "$FriendlyName" -CertStoreLocation "$CertStoreLocation"
    }

    $Password = ConvertTo-SecureString $passwordAsPlainText -AsPlainText -Force
    if (-not (Test-Path "$certFile"))
    {
        Export-PfxCertificate -Cert $cert -FilePath $certFile -Password $Password | Out-Null
    }

    if ($Install)
    {
        Import-PfxCertificate -FilePath "$certFile" -CertStoreLocation "Cert:\LocalMachine\Root" -Password $Password
    }
}

function UninstallCert()
{
    Get-ChildItem "Cert:\LocalMachine\Root" | Where-Object { $_.FriendlyName -eq "$FriendlyName" } | Remove-Item
}

switch ($PSCmdlet.ParameterSetName)
{
    "CertCreation" { CreateCert }
    "CertUninstallation" { UninstallCert }
    "Clean" { Cleanup }
}
