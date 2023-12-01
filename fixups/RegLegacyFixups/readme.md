# Registry Legacy Fixups
When injected into a process, RegLegacyFixups supports the ability to:
> * Modifies certain registry calls that do not work due to the MSIX container restrictions by modifying the call parameters to a form that would be allowed.

There is no guarantee that these changes will be enough to allow the application to perform properly, but often the change can solve application compatibility issues by removing request incompatible parameters that the application did not need.

# General Configuration
The configuration for the Registry Legacy Fixups is specified in the element `config` of the fixup structure within the `processes` section of the json file when RegLegacyFixups.dll is requested.

The `config` element for RegLegacyFixups.dll is an array of remediations.  Each remediation has two elements:

| Name | Purpose |
| ---- | ------- |
| `remediation` | An array of remediations. Structure for array elements is given in a tables for the specfic remediation type. |

Each remediation array object starts with a type field:
| `type` | Remediation type.  The values supported are given in a table below. |

| Remediation Type | Purpose |
| --------------- | ------- |
| `ModifyKeyAccess` | Allows for modification of access parameters in calls to open registry keys.  This remediation targets the `samDesired` parameter that specifies the permissions granted to the application when opening the key. This remediation type does not target calls for registry values.|
| `FakeDelete`      | Returns success to the application if it attempts to delete a key or registry item and "ACCESS_DENIED" occurs.  The app may or may not depend upon the delete occuring at some later point of running the application, so significant testing of the app is suggested when attempting this fixup.|
| `DeletionMarker`  | Allows for the hiding of specific registry keys or registry values in the virtual environment.|
| `Redirect`  | Allows for reading of registry values created in the context of a dependency |

## ModifyKeyAccess Remediation Type
The following Windows API calls are supported for this fixup type. 

> * RegCreateKeyEx
> * RegOpenKeyEx
> * RegOpenKeyTransacted
> * RegOpenCurrentUser

### Configuration for ModifyKeyAccess
When the `type` is specified as `ModifyKeyAccess`, the `remediation` element is an array of remediations with a structure shown here:

| Tag | Purpose |
| ------ | ------------------------------------- |
| `hive` | Specifies the registry hive targeted. |
| `patterns` | An array of regex strings. The pattern will be used against the path of the registry key being processed. |
| `access` | Defines the type of access to be modified and what it should be modified to. |

The value of the `hive` element may specified as shown in this table:

| Value | Purpose |
| ----- | ------- |
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

## FakeDelete Remediation Type
The following Windows API calls are supported for this fixup type. 

> * RegDeleteKey
> * RegDeleteKeyEx
> * RegDeleteKeyTransacted
> * RegDeleteValue

### Configuration for FakeDelete
When the `type` is specified as `FakeDelete`, the `remediation` element is an array of remediations with a structure shown here:

| Tag | Purpose |
| --- | ------- |
| `hive` | Specifies the registry hive targeted. |
| `patterns` | An array of regex strings. The pattern will be used against the path of the registry key being processed. |

The value of the `hive` element may specified as shown in this table:

| Value | Purpose |
| ----- | ------- |
| HKCU  | HKEY_CURRENT_USER |
| HKLM  | HKEY_LOCAL_MACHINE |

The value for the element `patterns` is a regex pattern string that specifies the registry paths to be affected.  This regex string is applied against the path of the key/subkey combination without the hive.

## DeletionMarker Remediation Type
The following Windows API calls are supported for this fixup type. 

> * RegGetValue
> * RegQueryValueEx
> * RegQueryMultipleValues
> * RegQueryInfoKey
> * RegEnumValue
> * RegEnumKeyEx
> * RegOpenKeyEx
> * RegOpenKeyTransacted

### Configuration for DeletionMarker
When the `type` is specified as `DeletionMarker`, the `remediation` element is an array of remediations with a structure shown here:
| Tag | Purpose |
| --- | ------- |
| `hive` | Specifies the registry hive targeted. |
| `key` | Contains a regex string. The key will be used against the path of the registry key being processed. |
| `values` | Contains an array of regex strings. The value will be used against the name of the registry value being processed in the key specified. This is an optional field. Omitting this field would hide the key specified. |

The value of the `hive` element may specified as shown in this table:

| Value | Purpose |
| ----- | ------- |
| HKCU  | HKEY_CURRENT_USER |
| HKLM  | HKEY_LOCAL_MACHINE |

The value for the element `key` is a regex pattern string that specifies the registry path to be affected.  This regex string is applied against the path of the key/subkey combination without the hive.

The value for the element `values` is a regex pattern string that specifies the registry-value to be affected.  This regex string is applied against the name of the registry value in the pattern key specified.

## Redirect Remediation Type
The following Windows API calls are supported for this fixup type. 

> * RegOpenKeyEx
> * RegOpenKeyTransacted
> * RegGetValue
> * RegEnumValue
> * RegQueryValueEx
> * RegQueryMultipleValues

### Configuration for Redirect
When the `type` is specified as `Redirect`, the `remediation` element is an array of remediations with a structure shown here:
| Tag | Purpose |
| --- | ------- |
| `dependency` | The dependency to use when resolving the registry data in `data` field. |
| `data` | Contains an array of objects which specifies keys and values to emulate `dependency`. |

The `data` field is an array of objects with the following structure:
| Tag | Purpose |
| --- | ------- |
| `key` | The full name of the `key` which will be used by the application to query about the `dependency`. |
| `values` | Contains a objects which specifies the values read by the application to get information about a `dependency`. The field name specifies the value name and the field value specifies the data of that value. |

The `key` can contain meta-variable **%dependency_version%** which will be replaced with the version of the `dependency`. The field values in `values` can contain meta-variable **%dependency_root_path%** and **%dependency_version%** which will be replaced with the root path and version of the `dependency` respectively. These entries are created during the loading of RegLegacyFixups.dll in HKCU and are deleted during the unloading. When application tries to read registry values specified in the config, the call is redirected to HKCU.

# JSON Example
Here is an example of using this fixup to address an application that contains a vendor key under the HKEY_CURRENT_USER hive and the application requests for full access control to that key. While permissible in a native installation of the application, such a request is denied by some versions of the MSIX runtime (OS version specific) because the request would allow the applicaiton make modifications. The json file shown could address this by causing a change to the requested access to give the application contol for read/write purposes only.

```json
"fixups": [
	{
		"dll":"RegLegacyFixups.dll",
		"config": [
		  {
			"remediation": [
				{
					"type": "ModifyKeyAccess",
					"hive": "HKCU",
					"patterns": [
						"^Software\\\\Vendor.*"
					],
					"access": "Full2RW"
				},
				{
					"type": "ModifyKeyAccess",
					"hive": "HKLM",
					"patterns": [
						"^[Ss][Oo][Ff][Tt][Ww][Aa][Rr][Ee]\\\\Vendor.*"
					],
					"access": "RW2MaxAllowed"
				},
				{
					"type": "FakeDelete",
					"hive": "HKCU",
					"patterns": [
						"^[Ss][Oo][Ff][Tt][Ww][Aa][Rr][Ee]\\\\Vendor\\\\.*"
					],
				},
				{
					"type": "DeletionMarker",
					"hive": "HKCU",
					"key": "^[Ss][Oo][Ff][Tt][Ww][Aa][Rr][Ee]\\\\Vendor\\\\.*"
				},
				{
					"type": "DeletionMarker",
					"hive": "HKLM",
					"key": "^Software\\\\Vendor.*",
					"values": [
						"^SubKey"
					]
				},
				{
					"type": "Redirect",
					"dependency": "java",
					"data": [
						{
							"key": "Software\\random_key\\abc\\%dependency_version%",
							"values": {
								"version": "%dependency_version%-SNAPSHOT"
								"path": "%dependency_root_path%\\bin"
							}
						}
					]
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
			<rediations>
				<remediation>
					<type>ModifyKeyAccess</type>
					<hive>HKCU</hive>
					<patterns>
						<patern>^Software\\\\Vendor.*</pattern>
					</patterns>
					<access>Full2RW</access>
				</remediation>
				<remediation>
					<type>ModifyKeyAccess</type>
					<hive>HKLM</hive>
					<patterns>
						<patern>^[Ss][Oo][Ff][Tt][Ww][Aa][Rr][Ee]\\\\Vendor.*</pattern>
					</patterns>
					<access>RW2R</access>
				</remediation>
				<remediation>
					<type>FakeDelete</type>
					<hive>HKCU</hive>
					<patterns>
						<patern>^[Ss][Oo][Ff][Tt][Ww][Aa][Rr][Ee]\\\\Vendor\\\\.*</pattern>
					</patterns>
					<access>RW2R</access>
				</remediation>
				<remediation>
					<type>DeletionMarker</type>
					<hive>HKCU</hive>
					<key>"^[Ss][Oo][Ff][Tt][Ww][Aa][Rr][Ee]\\\\Vendor\\\\.*"</key>
				</remediation>
				<remediation>
					<type>DeletionMarker</type>
					<hive>HKLM</hive>
					<key>"^Software\\\\Vendor.*"</key>
					<values>
						<value>"^SubKey"</value>
					</values>
				</remediation>
				<remediation>
					<type>Redirect</type>
					<dependency>java</dependency>
					<data>
						<key>Software\\random_key\\abc\\%dependency_version%</key>
						<values>
							<version>%dependency_version%-SNAPSHOT</version>
							<path>%dependency_root_path%\\bin</path>
						</values>
					</data>
				</remediation>
			</remediations>
		</config>
	</fixup>
</fixups>


