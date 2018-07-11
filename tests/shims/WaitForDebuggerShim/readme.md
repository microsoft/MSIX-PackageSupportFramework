# Wait for Debugger Shim
This is a very simple shim that will do nothing more than hold the process during startup until a debugger is attached. This is often times more convenient than trying to debug from the initial app launch, which can often times be inconvenient (e.g. due to the need to child debug, etc.).

This functionality can be easily turned off/on inside of the `config.json` through a single `enabled` boolean (`true` by default). E.g. the dll configuration object might look something like:

```json
{
    "dll": "TraceShim.dll",
    "config": {
        "enabled": false
    }
}
```

## Limitations
Due to the fact that this "shim" is piggy-backing off of the shim infrastructure, no code in this dll will run until it has been loaded by the Shim Runtime. Therefore, this shim is not beneficial when trying to debug issues that occur earlier in the process startup sequence. E.g. if you are trying to debug an issue with Shim Runtime itself, or if the issue you are trying to investigate is actually being caused by something like a malformed `config.json`, this shim will not help (other than narrowing the issue down to something earlier in the initialization process).
