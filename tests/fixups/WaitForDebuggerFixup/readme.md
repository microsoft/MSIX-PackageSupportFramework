# Wait for Debugger Fixup
This is a very simple fixup that will do nothing more than hold the process during startup until a debugger is attached. This is often times more convenient than trying to debug from the initial app launch, which can often times be inconvenient (e.g. due to the need to child debug, etc.).

This functionality can be easily turned off/on inside of the `config.json` through a single `enabled` boolean (`true` by default). E.g. the dll configuration object might look something like:

```json
{
    "dll": "TraceFixup.dll",
    "config": {
        "enabled": false
    }
}
```

```xml
<fixup>
    <dll>TraceFixup.dll</dll>
    <config>
        <enabled>false</enabled>
    </config>
</fixup>
```

## Limitations
Due to the fact that this "fixup" is piggy-backing off of the fixup infrastructure, no code in this dll will run until it has been loaded by the Fixup Runtime. Therefore, this fixup is not beneficial when trying to debug issues that occur earlier in the process startup sequence. E.g. if you are trying to debug an issue with Fixup Runtime itself, or if the issue you are trying to investigate is actually being caused by something like a malformed `config.json`, this fixup will not help (other than narrowing the issue down to something earlier in the initialization process).
