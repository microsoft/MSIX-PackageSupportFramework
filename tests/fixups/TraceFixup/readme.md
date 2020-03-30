# Trace Fixup
The "Trace Fixup" is provided as a debugging/diagnostics mechanism for identifying which function call(s) are made and whether or not they succeed. Despite its name, it is not actually a "fixup" and does not modify any runtime behavior (apart from logging, etc.), but it does use the same underlying technology to identify function calls. This "fixup" has multiple uses:

The first useful thing that this fixup can be used for is to identify _what_ the application is doing - or trying to do - and whether or not it succeeded _on a function call/API level_. That is, it will tell you exactly what function was called by the application (and optionally what module called it). This is in contrast to other monitoring systems that may only operate at the kernel level and give back coarser grained information.

Another useful thing that this fixup can be used for is to validate the functionality of _other_ fixups. For example, if you are trying to validate that `FindFirstFile`/`FindNextFile` return back a redirected file, you can monitor the results of those calls to validate that the expected file is present.

## Limitations
Note that there are obvious limitations with this approach. For starters, it will only show the function calls for the functions that have been fixed. E.g. if the application is attempting to call some obscure filesystem API that has not been fixed yet, this fixup will not see/know about it.

Additionally, developer intent is of course missing. Some function calls may be expected to fail, as is common with filesystem APIs. Similarly, some application failures may originate from function calls that succeed, e.g. as a result of unexpected file contents.

Asynchronous/overlapped I/O is also a scenario that may be tricky to identify/detect as failures may occur at a later time when context is missing.

## Suggested Usage
Since most of this fixup's usefulness comes from identifying what the _application_ is trying to do, as opposed to what other fixups are trying to do, it is suggested that this fixup get loaded as the final fixup in the `config.json` file. E.g. the dll configuration may look something like:

```json
{
    "dll": "TraceFixup.dll",
    "config": {
        "traceMethod": "eventlog",
        "traceLevels": {
            "default": "allFailures"
        },
        "breakOn": {
            "filesystem": "unexpectedFailures"
        }
    }
}
```

```xml
<fixup>
    <dll>TraceFixup.dll</dll>
    <config>
        <traceMethod>eventLog</traceMethod>
        <traceLevels>
            <traceLevel level="default">allFailures</traceLevel>
        </traceLevels>
        <breakOn>
            <break level="fileSystem">unexpectedFailures</break>
        </breakOn>
    </config>
</fixup>
```

## Configuration
The fixup can be configured to trace calls in a variety of ways and in a variety of situations via the fixup `config` element inside of `config.json`. This is expected to be an object with the following (optional) values:

| Property | Description |
| -------- | ----------- |
| `traceMethod` | Defines the method of tracing. This is expected to be a value of type `string`. Allowed values are:<br>`printf` - Uses `printf` (i.e. console output) for tracing.<br>`eventlog` - Uses Event Trace for Windows to output events that may be consumed using PSFShimMonitor.<br>`outputDebugString` - Uses `OutputDebugString` for tracing. This is the default. |
| `waitForDebugger` | Specifies whether or not to hold the process until a debugger is attached in the `DLL_PROCESS_ATTACH` callback. This is expected to be a value of type `boolean`. The default value is `false`. This option is most useful when `traceMethod` is set to `outputDebugString`. |
| `traceFunctionEntry` | Specifies whether or not to trace function entry. This is useful when trying to reason about function call order and composition since functions are logged in the reverse order (see [Log Ordering](#log-ordering) for more information). This is expected to be a value of type `boolean`. The default value is `false`. Note that this logging is done independent of function success/failure and the `traceLevels` configuration since success/failure is not known at function entry. |
| `traceCallingModule` | Defines whether or not to include the calling module in the output. This is expected to be a value of type `boolean`. The default value is `true`. This is potentially useful for identifying possible risks of recursion (one API implemented using another). There's no real harm with leaving this option always enabled, but can help reduce output noise when turned off. |
| `ignoreDllLoad` | Specifies whether or not to ignore calls to `NtCreateFile` for dlls. This is expected to be a value of type `boolean`. The default value is `true`. |
| `traceLevels` | Used to determine whether or not a function call should get logged, based off function result. E.g. you can configure calls to always get logged, only logged for unexpected failures, or logged for any failure. This is expected to be a value of type `object`. The format is described in more detail below |
| `breakOn` | Similar to `traceLevels`, but used to determine whether or not to issue a `DebugBreak` in particular scenarios. Its format is identical to `traceLevels`, however the `default` level is `ignore` (i.e. _never_ issue a `DebugBreak`) |

For the `traceLevels` and `breakOn` objects, each property specifies the function type/classification that the trace level applies to. The expected values are:

| Property | Description |
| -------- | ----------- |
| `default` | Specifies the default trace level, used when no more specific trace level is specified for the function type. The default value is `unexpectedFailures`, described below. |
| `filesystem` | Specifies the trace level for file management functions. In general, this corresponds to the functions listed at https://msdn.microsoft.com/en-us/library/windows/desktop/aa364232(v=vs.85).aspx. |
| `registry` | Specifies the trace level for registry functions. In general, this corresponds to the functions listed at https://msdn.microsoft.com/en-us/library/windows/desktop/ms724875(v=vs.85).aspx. |
| `processAndThread` | Specifies the trace level for process and thread functions. In general, this corresponds to the functions listed at https://msdn.microsoft.com/en-us/library/windows/desktop/ms684847(v=vs.85).aspx. |
| `dynamicLinkLibrary` | Specifies the trace level for dynamic-link library functions. In general this corresponds to the functions listed at https://msdn.microsoft.com/en-us/library/windows/desktop/ms682599(v=vs.85).aspx |

And the values specify which function results to trace, based off the degree of success/failure. The expected values are:

| Value | Description |
| ----- | ----------- |
| `always` | Logs all function calls, irrespective of success/failure |
| `ignoreSuccess` | Logs all failures and any function call where success/failure cannot be determined (e.g. `void` returning function) |
| `allFailures` | Logs all function calls where failure was reported |
| `unexpectedFailures` | Logs only failures that are not considered to be "expected" - such as "file not found", "buffer overflow", etc. |
| `ignore` | Does not log output for any function call, regardless of success/failure |

The configuration that's best to use will depend on the scenario. For example, you likely don't want to use a `traceMethod` of `printf` unless the target application is a console application. E.g. the test applications in this project are mostly console applications, however most "real world" applications probably are not. Similarly, a value of `unexpectedFailures` for the default trace level may be a reasonable starting place to reduce noise, but this isn't always an indication of issue(s) due to the previously mentioned [Limitations](#limitations).

## Log Ordering
Since the majority purpose of this fixup is to identify API call failures, tracing must be done _after_ the invocation of the implementation function returns. This means that if a single function is written in terms of one or more other functions, then they will appear in reverse order in the output. E.g. `CreateFile` is written in terms of `NtCreateFile`, so if both functions are fixed, then you will see output for the call to `NtCreateFile` _before_ the output for the call to `CreateFile`.

## Multi-Threaded Applications
The only synchronization that the trace fixup does is to ensure that all output for a single call is grouped "together." It does _not_ synchronize calls that originate from one another (e.g. `FindFirstFile` calling `FindFirstFileEx`), nor does it synchronize the function entry traces configured via `traceFunctionEntry`. Therefore output may appear intertwined if two different calls were to happen at the same time.
