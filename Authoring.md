# Shim Authoring
The only two requirements are the two required dll exports: `ShimInitialize` and `ShimUninitialize`. Other than that, the shim is relatively free to do whatever it wants.

## Shim Loading
The shim loading process is described in more detail [here](ShimRuntime/readme.md#Shim%20Loading), but in short, when the process starts up, the Shim Runtime will enumerate the set of dlls configured for the current process, loading them before calling `ShimInitialize` within a Detours transaction. Typical shim behavior is to call `ShimRegister` for each function it wishes to detour at this time.

## Shim Configuration
While a shim is free to dictate and read its configuration however it wishes, [the established pattern is to put the configuration alongside the shim declaration in `config.json`](readme.md#JSON%20Configuration). When this pattern is followed, the `ShimQueryCurrentDllConfig` function can be used to easily retrieve the already parsed JSON value from `config.json`. As a simple example, the following code demonstrates how to read a few configuration values:

```C++
// For this example, configuration is optional (ShimQueryCurrentDllConfig returns null)
if (auto configRoot = ::ShimQueryCurrentDllConfig())
{
    // NOTE: the "as_" functions throw on failure, if the value is not of the desired type.
    // This example could be made better by providing better diagnostic information if the
    // types are different than what's expected.
    auto& config = configRoot->as_object(); // Throws if not an object

    if (auto enabledValue = config.try_get("enabled"))
    {
        g_enabled = enabledValue->as_boolean().get(); // Throws if not a boolean
    }

    if (auto logPathValue = config.try_get("logPath"))
    {
        g_logPath = enabledValue->as_string().wstring(); // Throws if not a string
    }
}
```

## Shim Reentrancy: A Cautionary Tale
When detouring function calls in another dll, we must be careful to avoid situations where a shimmed function ends up calling function(s) other than the one being shimmed, as these other function(s) may in turn end up calling back into the shim dll. As a concrete example, the shim for `CreateFile` in the [File Redirection Shim](FileRedirectionShim/readme.md) may end up calling `CopyFile`, whose implementation just so happens to consume `CreateFile`. In order to avoid an infinite cycle, the File Redirection Shim keeps track of a global, thread local boolean (via the helper [reentrancy_guard](include/reentrancy_guard.h) type) that it uses to detect reentrancy. If reentrancy is detected, then the function doesn't perform any of its shimming logic and instead forwards the call onto the underlying implementation.

While not a tremendously great or elegant solution, it is arguably the safest. Even if you can determine that the current implementation of a function doesn't invoke a shimmed function, there's no guarantee that this will hold for future updates to the OS. Note however, that there is a slight risk if COM reentrancy is a possible concern since a reentrant call may come from an unrelated call.

## A Note on "Wide" vs "ANSI" Functions
Many functions in the Windows API surface that accept string arguments have "wide" (`wchar_t*`) and "narrow" (`char*`) variants. In general, the "narrow" versions of these functions are implemented in terms of their "wide" variant. E.g. they are implemented as-if by the following:

```C++
BOOL DeleteFileA(LPCSTR fileName)
{
    BOOL result = false;
    if (auto unicodeString = ConvertToUnicodeString(fileName))
    {
        result = DeleteFileW(unicodeString);
        FreeUnicodeString(unicodeString);
    }

    return result;
}
```

Thus, it is tempting to only detour the "wide" version of the function. Unfortunately, this can get you into trouble as you are relying on both an implementation detail of the "narrow" function as well as the assumption that the compiler isn't going to inline the call. E.g. consier the following hypothetical future scenario:

```C++
// Hypothetical function added in the future
BOOL DeleteFile2(LPCWSTR fileName, DWORD flags);

BOOL DeleteFileW(LPCWSTR fileName)
{
    return DeleteFile2(fileName, 0);
}
```

In this case, the compiler will almost certainly inline the implementation of `CreateFileW` into `CreateFileA`. Thus, in this scenario, if you were to only detour `CreateFileW`, the application may stop working on future versions of the OS because your assumption is no longer true.

## A Note on Detouring `ntdll.dll`
It may be tempting to detour functions from `ntdll.dll` as a "catch all" solution. E.g. instead of detouring `CreateFileA`, `CreateFileW`, `CreateFile2`, `CopyFileA`, `CopyFileW`, and `CopyFile2` (and likely others), it's a very appealing solution to only detour `NtCreateFile`, which is not only less work, but it will also cover functions that you either forgot to detour, didn't want to spend the time to detour, or didn't know about. While not a terrible idea as a fallback (so long as you take measures to avoid/detect reentrancy), completely going this route is again relying on an implementation detail of the OS, which can change in future versions. Here be dragons.

## A Note on Exceptions
You will quickly notice that the different pieces of the PSF as well as multiple headers in the `include` directory use exceptions. Feel free to do so as well, but keep the following in mind:

* Don't let exceptions leak out of your dll. That is, either catch exceptions at the end of your shim functions, or declare them `noexcept` so that the process will terminate if an exception that would otherwise go unhandled gets thrown.
* Applications can call `AddVectoredExceptionHandler` and add a vectored exception handler that does unpredictable things (e.g. such as terminate the application). Therefore, you should have the mindset that any exception thrown might crash the application _even if you handle it gracefully_.
