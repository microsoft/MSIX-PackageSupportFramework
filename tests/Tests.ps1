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