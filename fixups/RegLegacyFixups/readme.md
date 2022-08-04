# Registry Legacy Fixups
When injected into a process, RegLegacyFixups supports the ability to:
> * Modifies certain registry calls that do not work due to the MSIX container restrictions by modifying the call parameters to a form that would be allowed.

There is no guarantee that these changes will be enough to allow the application to perform properly, but often the change can solve application compatibility issues by removing request incompatible parameters that the application did not need.

## Supported API calls
The following Windows API calls are supported. 

> * RegCreateKeyEx
> * RegOpenKeyEx
> * RegOpenKeyTransacted
> * RegOpenCurrentUser

Currently, there is only one remediation type:

| Remediation Type | Purpose |
| --------------- | ------- |
| `ModifyKeyAccess` | Allows for modification of access parameters in calls to open registry keys.  This remediation targets the `samDesired` parameter that specifies the permissions granted to the application when opening the key. This remediation type does not target calls for registry values.|

## Configuration
The configuration for the Registry Legacy Fixups is specified in the element `config` of the fixup structure within the `processes` section of the json file when RegLegacyFixups.dll is requested.

The `config` element for RegLegacyFixups.dll is an array of remediations.  Each remediation has three elements:

| Name | Purpose |
| ---- | ------- |
| `type` | Remediation type.  The values supported are given in a table above. |
| `remediation` | An array of remediations. Structure for array elements is given in a table below. |

When the `type` is specified as `ModifyKeyAccess`, the `remediation` element is an array of remediations with a structure shown here:

| `hive` | Specifies the registry hive targeted. |
| `patterns` | An array of regex strings. The pattern will be used against the path of the registry key being processed. |
| `access` | Defines the type of access to be modified and what it should be modified to. |

The value of the `hive` element may specified as shown in this table:

| Value | Purpose |
| HKCU  | HKEY_CURRENT_USER |
| HKLM  | HKEY_LOCAL_MACHINE |

The value for the element `patterns` is a regex pattern string that specifies the registry paths to be affected.  This regex string is applied against the path of the key/subkey combination without the hive.

The value of the `access` element is given in the following table:

| Access | Purpose |
| ------ | ------- |
| Full2RW | If the caller requested full access (for example STANDARD_RIGHTS_ALL), modify the call to remove KEY_CREATE_LINK. |
| Full2R  | If the caller requested full access (for example STANDARD_RIGHTS_ALL), modify the call to remove KEY_CREATE_LINK, KEY_CREATE_SUB_KEY, and KEY_CREATE_VALUE. |
| Full2MaxAllowed  | If the caller requested full access (for example STANDARD_RIGHTS_ALL), modify the call to request MAXIMUM_ALLOWED.|
| RW2R    | If the caller requested Read/Write access, modify the call to to remove KEY_CREATE_LINK, KEY_CREATE_SUB_KEY, and KEY_CREATE_VALUE. |
| RW2MaxAllowed  | If the caller requested Read/Write access, modify the call to request MAXIMUM_ALLOWED.|

# JSON Example
Here is an example of using this fixup to address an application that contains a vendor key under the HKEY_CURRENT_USER hive and the application requests for full access control to that key. While permissible in a native installation of the application, such a request is denied by some versions of the MSIX runtime (OS version specific) because the request would allow the applicaiton make modifications. The json file shown could address this by causing a change to the requested access to give the application contol for read/write purposes only.

```json
"fixups": [
	{
		"dll":"RegLegacyFixups.dll",
		"config": [
			{
				"type": "ModifyKeyAccess",
				"remediation": [
					{
						"hive": "HKCU",
						"patterns": [
							"^Software\\\\Vendor.*"
						],
						"access": "Full2RW"
					},
										{
						"hive": "HKLM",
						"patterns": [
							"^[Ss][Oo][Ff][Tt][Ww][Aa][Rr][Ee]\\\\Vendor.*"
						],
						"access": "RW2MaxAllowed"
					}
				]
			}
		]
	}
]
```

#XML Example
Here is the equivalent config section when using XML to specify.

```xml
<fixups>
	<fixup>
		<dll>RegLegacyFixups</dll>
		<config>
			<type>ModifyKeyAccess</type>
			<rediations>
				<remediation>
					<hive>HKCU</hive>
					<patterns>
						<patern>^Software\\\\Vendor.*</pattern>
					</patterns>
					<access>Full2RW</access>
				</remediation>
				<remediation>
					<hive>HKLM</hive>
					<patterns>
						<patern>^^[Ss][Oo][Ff][Tt][Ww][Aa][Rr][Ee]\\\\Vendor.*</pattern>
					</patterns>
					<access>RW2R</access>
				</remediation>
			</remediations>
		</config>
	</fixup>
</fixups>


