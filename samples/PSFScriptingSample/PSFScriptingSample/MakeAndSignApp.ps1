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

makeappx pack /d bin\release /p bin\release\PSFScriptingSample.msix
signtool sign /p "Password12$" /a /v /fd SHA256 /f "bin\release\PSFScriptingSample_TemporaryKey.pfx" "bin\release\PSFScriptingSample.msix"