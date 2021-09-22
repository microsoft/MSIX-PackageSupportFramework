<#
.PARAMETER ScriptPathAndArguments
	The location of the script to run and the arguments.
	
.PARAMETER $errorActionPreferenceForScript
    Sets the Error Action PRefrence for this script
#>

Param (
    [Parameter(Mandatory=$true)]
    [string]$ScriptPathAndArguments
)
	Write-host "Inside StartingScriptLauncher.ps1"
	###start-sleep 15
try
{
	invoke-expression $scriptPathAndArguments
}
catch
{
	write-host $_.Exception.Message
	#ERROR 774 refers to ERROR_ERRORS_ENCOUNTERED.
	#This error will be brought up the the user.
	###start-sleep 60
	exit(774)
}
write-host "Ending StartingScriptWrapper.ps1"
###start-sleep 60
exit(0)