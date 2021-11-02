<#
.CMDLET  StartMenuCmdWrapperScript.ps1

.PARAMETER PackageFamilyName
	The PackageFamilyName to be used for the container to run in (eg: PackageName_sighash).


.PARAMETER AppID
	One of the Application element ID values from the AppXManifest file of the package.
	Note: If multiple Application elements are present in the package, it does not matter which one you select.

.PARAMETER ScriptPath
	The location of the cmd/bat script to run.  This should be the full path, without quotation marks.

.PARAMETER ScriptArguments
	Optionally arguments to the cmd file (don't include a /c as these are arguments to the script itself, add escaped quotations marks as needed).
#>

Param (

    [Parameter(Position=0, Mandatory=$true)]
    [string]$PackageFamilyName,

    [Parameter(Position=1, Mandatory=$true)]
    [string]$AppID,

    [Parameter(Position=2, Mandatory=$true)]
    [string]$ScriptPath,

    [Parameter(Position=3, Mandatory=$false)]
    [string]$ScriptArguments
)

try
{
	write-host "StartMenuCmdWrapper.ps1: Launching script `"$($scriptPath)`" in package $($PackageFamilyName)"
	invoke-CommandInDesktopPackage -PackageFamilyName $PackageFamilyName -AppID $AppID -Command "C:\Windows\System32\cmd.exe" -Args "/c `"$($scriptPath)`" $($ScriptArguments)"
	#invoke-expression "C:\Windows\System32\cmd.exe /c `"$($scriptPath)`" $($ScriptArguments)"
	write-host "StartMenuCmdWrapper.ps1: returned."
	#start-sleep 30
}
catch
{
	write-host $_.Exception.Message
	#ERROR 774 refers to ERROR_ERRORS_ENCOUNTERED.
	#This error will be brought up the the user.
	start-sleep 20
	exit(774)
}
	
exit(0)