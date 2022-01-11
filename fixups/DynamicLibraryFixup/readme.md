# Env Var Fixup
When injected into a process, the EnvVarFixup supports the ability to:
> * Define environment variables in the Config.Json file.
> * Specify that a named environment variable should be extracted from the application
> * hive registry.

MSIX packages do not normally implement any environment variables outside of what is normally set on the system outside of the container.
In essence, this fixup allows you to specify what would natively be a system or user environment variable as an application-specific environment variable.

## About Debugging this fixup
The Release build of this fixup produces no output to the debug console port for performance reasons.
Use of the Debug build will enable you to see the intercepts and what the fixup did.
That output is easily seen using the Sysinternals "DebugView" tool.

## Configuration
The configuration for the EnvVarFixup is specified under the element `config` of the fixup structure within the json file when EnvVarFixup.dll is requested.
This `config` element contains a an array called "EnvVars".  Each Envar contains three values:

| PropertyName | Description |
| ------------ | ----------- |
| `name` | Regex pattern for the name(s) of the environment variable being defined.|
| `value`| If the value is to be defined in the json, the value is entered here. Otherwise this may be specified as an empty string.|
| `useregistry`| A boolean, when set to true it instructs that the environment variable should be extracted from the package registry for Environment variables, first checking HKCU and then HKLM. When specified, the intercept will first look in the HKCU registry, then HKLM, and finally the `value` field in the JSON entry. |


# JSON Examples
To make things simpler to understand, here is a potential example configuration object that is not using the optional parameters:

```json
"config": {
    "EnvVars": [
        {
            "name" : "Var1Name",
            "value" : "SpecifiedValue1",
            "useregistry": "false"
        },
        {
            "name" : "Var2Name",
            "value" : "BackupValue2",
            "useregistry": "true"
        }
        ,
        {
            "name" : "MyName.\*",
            "value" : "",
            "useregistry": "true"
        }
    ]
}
```

Note that when using registry entries, the search looks at the registry as seen by the application inside the container.
Thus a HKCU search should see entries in the Application hive overlayed over the local system.
The HKLM search does the same, but on some systems (all at the time this was written) the HKLM\System entries in the Application hive might not be seen by the application.

When useregistry is not specified, any attempt by the application to set an environment variable will return an ERROR_ACCESS_DENIED event without attempting to set the value.

When useregistry is specified, regardless of where the value was located, it will be written to the HKCU\EnvironmentVariables key so that the write will succeed on systems that support it.

