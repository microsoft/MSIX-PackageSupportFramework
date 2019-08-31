param (
	[string]$SomethingOne,
	[string]$SomethingTwo,
	[boolean]$flag
)

if($PSBoundParameters.ContainsKey('SomethingOne'))
{
	Write-host ("Say " + $SomethingOne)
}
else
{
	Write-host "Nothing to say"
}

if($PSBoundParameters.ContainsKey('SomethingTwo'))
{
	Write-host ("Say " + $SomethingTwo)
}
else
{
	Write-host "Nothing to else to say"
}

if($flag)
{
	write-host "Flag was true"
}
else
{
	write-host "Flag was false"
}
