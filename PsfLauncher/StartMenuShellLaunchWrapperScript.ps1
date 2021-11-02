<#
.CMDLET  StartMenuShellLaunchWrapperScript.ps1

.PARAMETER PackageFamilyName
	The PackageFamilyName to be used for the container to run in (eg: PackageName_sighash).


.PARAMETER AppID
	One of the Application element ID values from the AppXManifest file of the package.
	Note: If multiple Application elements are present in the package, it does not matter which one you select.

.PARAMETER ScriptPath
	The location of the file to run using the local fta.  This should be the full path, without quotation marks.

.PARAMETER ScriptArguments
	Optionally arguments to the process started by the fta (add escaped quotations marks as needed).
#>

Param (

    [Parameter(Position=0, Mandatory=$true)]
    [string]$PackageFamilyName,

    [Parameter(Position=1, Mandatory=$true)]
    [string]$AppID,

    [Parameter(Position=2, Mandatory=$true)]
    [string]$FilePath,

    [Parameter(Position=3, Mandatory=$false)]
    [string]$ScriptArguments
)

try
{
	write-host "StartMenuShellLaunchWrapperScript.ps1: Launching file `"$($FilePath)`" in package $($PackageFamilyName)"
	invoke-CommandInDesktopPackage -PackageFamilyName $PackageFamilyName -AppID $AppID -Command "$($FilePath)" -Args "$($ScriptArguments)"
	write-host "StartMenuShellLaunchWrapperScript.ps1: returned."
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