$windowKits = get-childitem "C:\Program Files (x86)\Windows Kits\10\bin" -filter "10.0*" | sort-object -property name -descending

$foundAWindowKit = $false
$windowKitIndex = 0
while(-not $foundAWindowKit -and $windowKitIndex -le $windowKits.count)
{
	if(test-path($windowKits[$windowKitIndex].fullName + "\x64"))
	{
		$env:Path += (";" + $windowKits[$windowKitIndex] + "\x64")
		$foundAWindowKit = $true
	}

	$windowKitIndex++
}

if($foundAWindowKit)
{
	makeappx pack /d bin\release /p bin\release\PSFScriptingSample.msix
signtool sign /p "Password12$" /a /v /fd SHA256 /f "bin\release\PSFScriptingSample_TemporaryKey.pfx" "bin\release\PSFScriptingSample.msix"
}
else
{
	write-host "No window SDK's were found at C:\program files(x86)\Windows Kits\10\bin"
	write-host "Please go to https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk to download the most recent windows SDK"
	read-host "Press any key to exit"
}