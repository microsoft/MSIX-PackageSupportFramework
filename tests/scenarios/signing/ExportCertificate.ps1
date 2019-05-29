<#
.DESCRIPTION
    Exports the certificate from CertStoreLocation with the specified FriendlyName.

.PARAMETER FriendlyName
    The friendly name given to the certificate and used to lookup a pre-existing cert. By default, this value is
    "CentennialFixups Test Signing Certificate"

.PARAMETER CertStoreLocation
    Target store to hold the created certificate. The default is "Cert:\LocalMachine\My"
#>

param (
	[Parameter(Mandatory=$True)]
	[string]$CertFriendlyName,
	
	[Parameter(Mandatory=$True)]
	[string]$CertStoreLocation
)

function TryGetCert
{
    write-host ("Cert friendly name is: " + $CertFriendlyName)
    $results = Get-ChildItem "$CertStoreLocation" | Where-Object { $_.FriendlyName -eq "$certFriendlyName" }
    if ($results.Count -gt 0)
    {
        return $results[0];
    }
	else
	{
		write-Host ("There is no certificate with the friendly name of " + $CertFriendlyName + " in location " + $CertStoreLocation)
		return $null
	}
}

$cert = TryGetCert

write-host "Exporting the certificate"
Export-Certificate -FilePath ../Appx/CentennialFixupsTestSigningCertificate.cer -Type CERT -Cert $cert
