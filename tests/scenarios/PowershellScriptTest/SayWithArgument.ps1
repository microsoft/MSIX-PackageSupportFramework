Param (
	[string]$text
)

New-Item -path $env:LOCALAPPDATA -Name "Argument.txt" -ItemType "file" -value $text