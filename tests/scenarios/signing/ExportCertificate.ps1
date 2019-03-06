[CmdletBinding(DefaultParameterSetName="CertCreation")]
Param (
	[X509Certificate2]$pfxCertificate
)

Export-Certificate -FilePath ../Appx/CentennialFixupsTestSigningCertificate.cer -cert $pfxCertificate -Type CERT
