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

try
{
	invoke-expression $scriptPathAndArguments
}
catch
{
	write-host $_.Exception.Message
	#ERROR 774 refers to ERROR_ERRORS_ENCOUNTERED.
	#This error will be brought up the the user.
	exit(774)
}

exit(0)