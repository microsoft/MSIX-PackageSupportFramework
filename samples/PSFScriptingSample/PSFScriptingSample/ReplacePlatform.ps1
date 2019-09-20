param (
	[Parameter(Mandatory=$True)]
	[string]$platform
)

if($platform -eq "x86")
{
	$platform = "x32"
}
$appxManifest = Get-Content "AppxManifest.xml"
$appxManifest = $appxManifest.replace("[PlatformPlaceHolder]", $platform.Substring(1))
$appxManifest | out-file -FilePath "bin\Release\AppxManifest.xml" -Encoding "UTF8"