Param (
	[string]$text,
	[string]$path
)

if($path -eq 'LocalAppData')
{
New-Item -path $env:LOCALAPPDATA -Name "Argument.txt" -ItemType "file" -value $text
}