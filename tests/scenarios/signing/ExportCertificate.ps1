param (
	[Parameter(Mandatory=$True)]
	[string]$certFriendlyName,
	
	[Parameter(Mandatory=$True)]
	[string]$CertStoreLocation
)

function TryGetCert
{
	write-host ("Cert friendly name is: " + $certFriendlyName)
    $results = Get-ChildItem "$CertStoreLocation" | Where-Object { $_.FriendlyName -eq "$certFriendlyName" }
    if ($results.Count -gt 0)
    {
        return $results[0];
    }

    return $null
}

$cert = TryGetCert

write-host "Exporting the certificate"
Export-Certificate -FilePath ../Appx/CentennialFixupsTestSigningCertificate.cer -Type CERT -Cert $cert
