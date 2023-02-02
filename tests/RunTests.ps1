
$global:failedTests = 0
#$PSScriptRoot = Split-Path -Path $MyInvocation.MyCommand.Definition -Parent
$pfxPath = "$PSScriptRoot\scenarios\signing\CentennialFixupsTestSigningCertificate.pfx"

function RunTest($Arch, $Config)
{
    Write-host "<<<<<<<<<<<<<<<<<<<< Test Pass for $($Arch)  $($Config) >>>>>>>>>>>>>>>>>>>>>"
    # For any directory under the "scenarios" directory that has a "FileMapping.txt", generate an appx for it
    Remove-Item "$PSScriptRoot\scenarios\Appx\*"
    foreach ($dir in (Get-ChildItem -Directory "$PSScriptRoot\scenarios"))
    {
        if (Test-Path "$($dir.FullName)\FileMapping.txt")
        {
            Push-Location $dir.FullName
            try
            {
                $appxPath = "..\Appx\$($dir.Name).appx"
                . makeappx.exe pack /o /p "$appxPath" /f "$Arch$Config\FileMapping.txt" > "$($appxPath).makeappx.txt"
                . signtool.exe sign /a /v /fd sha256 /f "$pfxPath" /p "CentennialFixupsTestSigning" "$appxPath" > "$($appxPath).signtool.txt"
            }
            finally
            {
                Pop-Location
            }
        }
    }

    # Install all of the applications
    try
    {
        Add-AppxPackage "$PSScriptRoot\scenarios\Appx\*.appx" | Out-Null

        # Finally, execute the actual test. Note that the architecture of the runner doesn't actually matter
        #. x64\Release\TestRunner.exe /onlyPrintSummary
        . x64\Release\TestRunner.exe
        $global:failedTests += $LASTEXITCODE
    }
    finally
    {
        # Uninstall all packages on exit. Ideally Add-AppxPackage would give us back something that we could use here,
        # but alas we must hard-code it
        $packagesToUninstall = @("ArchitectureTest", "CompositionTest", "FileSystemTest", "LongPathsTest", "WorkingDirectoryTest", "PowershellScriptTest", "DynamicLibraryTest", "RegLegacyTest", "EnvVarsATest", "EnvVarsWTest", "ArgRedirectionTest", "PreventBreakAwayTest")
        foreach ($pkg in $packagesToUninstall)
        {
            Get-AppxPackage $pkg | Remove-AppxPackage
        }
    }
}

write-host ""
write-host ""
write-host ""
write-host ""
write-host ""
write-host ""
write-host ""
write-host ""
write-host ""
write-host ""
write-host ""
write-host ""
write-host ""
write-host ""
write-host ""
write-host ""
write-host ""
write-host ""
write-host ""
write-host ""
write-host ""
write-host ""
write-host ""
write-host ""
write-host ("Checking to see if a cert exists at " + $pfxPath)
if (!(Test-Path "$pfxPath"))
{
    write-host "Invoking Create Cert"
    Invoke-Expression "$PSScriptRoot\scenarios\signing\CreateCert.ps1 -Install -PasswordAsPlainText CentennialFixupsTestSigning" | Out-Null
}

write-host "Cert exists"

if(!(Test-Path "$PSScriptRoot\scenarios\Appx"))
{
    New-Item -ItemType Directory "$PSScriptRoot\scenarios\Appx"
}

RunTest "x64" "Debug"
RunTest "x64" "Release"
RunTest "x86" "Debug"
RunTest "x86" "Release"


if ($TestConfigFromXML)
{
    write-host "Making config.json from config.xml"
    foreach ($dir in (Get-ChildItem -Directory "$PSScriptRoot\scenarios"))
    {
        if (Test-Path "$($dir.FullName)\FileMapping.txt")
        {
            push-location $dir.fullName
        
            foreach ($configurationFile in (get-childitem -file "Config*.json"))
            {
                Copy-Item $configurationFile -destination ("$configurationFile"+"_bak")
            
                $configFileName = $configurationFile.baseName
                start-process -FilePath ..\..\..\xmlToJsonConverter\msxsl.exe -ArgumentList "$configFileName.xml ..\..\..\xmlToJsonConverter\Format.xsl -o $configurationFile" -NoNewWindow
            }
            pop-location
        }
    }

    RunTest "x64" "Debug"
    RunTest "x64" "Release"
    RunTest "x86" "Debug"
    RunTest "x86" "Release"

    Write-host "Removing generated json files"
    foreach ($dir in (Get-ChildItem -Directory "$PSScriptRoot\scenarios"))
    {
        if (Test-Path "$($dir.FullName)\FileMapping.txt")
        {
            push-location $dir.fullName
        
        
            foreach ($configurationFile in (get-childitem -file "Config*.json_bak"))
            {
                $configFileName = $configurationFile.baseName
                Copy-Item $configurationFile -destination ("$configFileName.json")
            }

            remove-item "Config*.json_bak"
            pop-location
        }
    }
}
Write-Host "$failedTests tests have failed"

#Write-host "Removing generated json files"
#foreach ($dir in (Get-ChildItem -Directory "$PSScriptRoot\scenarios"))
#{
#    if (Test-Path "$($dir.FullName)\FileMapping.txt")
#    {
#        push-location $dir.fullName
#        
#        
#        foreach ($configurationFile in (get-childitem -file "Config*.json_bak"))
#        {
#            $configFileName = $configurationFile.baseName
#            Copy-Item $configurationFile -destination ("$configFileName.json")
#        }
#
#        remove-item "Config*.json_bak"
#        pop-location
#    }
#}
#
#Write-Host "$failedTests tests have failed"
#
#Exit $failedTests
