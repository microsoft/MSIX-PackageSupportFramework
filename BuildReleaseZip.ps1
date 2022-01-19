# Script to create the release zip file
Set-Location $PSScriptRoot

if (!(Test-Path .\ZipRelease))
{
    new-item -ItemType Directory -Name .\ZipRelease
}
if (Test-Path .\ZipRelease\ReleasePsf.zip)
{
    Remove-Item .\ZipRelease\ReleasePsf.zip
}
if (Test-Path .\ZipRelease\DebugPsf.zip)
{
    Remove-Item .\ZipRelease\DebugPsf.zip
}




Get-ChildItem -Path PsfLauncher\*.ps1 | Compress-Archive -DestinationPath .\ZipRelease\ReleasePsf.zip
Get-ChildItem -Path Win32\Release\*.dll | Compress-Archive -DestinationPath .\ZipRelease\ReleasePsf.zip -Update
Get-ChildItem -Path Win32\Release\*.exe | Compress-Archive -DestinationPath .\ZipRelease\ReleasePsf.zip -Update
Get-ChildItem -Path Win32\Release\x86\*.dll | Compress-Archive -DestinationPath .\ZipRelease\ReleasePsf.zip -Update
Get-ChildItem -Path x64\Release\*.dll | Compress-Archive -DestinationPath .\ZipRelease\ReleasePsf.zip -Update
Get-ChildItem -Path x64\Release\*.exe | Compress-Archive -DestinationPath .\ZipRelease\ReleasePsf.zip -Update
Get-ChildItem -Path x64\Release\amd64\*.dll | Compress-Archive -DestinationPath .\ZipRelease\ReleasePsf.zip -Update

Get-ChildItem -Path PsfLauncher\*.ps1 | Compress-Archive -DestinationPath .\ZipRelease\DebugPsf.zip
Get-ChildItem -Path Win32\Debug\*.dll | Compress-Archive -DestinationPath .\ZipRelease\DebugPsf.zip -Update
Get-ChildItem -Path Win32\Debug\*.exe | Compress-Archive -DestinationPath .\ZipRelease\DebugPsf.zip -Update
Get-ChildItem -Path Win32\Debug\x86\*.dll | Compress-Archive -DestinationPath .\ZipRelease\DebugPsf.zip -Update
Get-ChildItem -Path x64\Debug\*.dll | Compress-Archive -DestinationPath .\ZipRelease\DebugPsf.zip -Update
Get-ChildItem -Path x64\Debug\*.exe | Compress-Archive -DestinationPath .\ZipRelease\DebugPsf.zip -Update
Get-ChildItem -Path x64\Debug\amd64\*.dll | Compress-Archive -DestinationPath .\ZipRelease\DebugPsf.zip -Update

$yyyy = (Get-Date).Year
$mm = (Get-Date).Month
$dd = (Get-Date).day
$outname = ".\ZipRelease.zip-v$($yyyy)-$($mm)-$($dd).zip"
Get-ChildItem -Path  .\ZipRelease\*.zip | Compress-Archive -DestinationPath $outname