# Fixup Authoring
The only two requirements are the two required dll exports: `PSFInitialize` and `PSFUninitialize`. Other than that, the fixup is relatively free to do whatever it wants.

## Fixup Loading
The fixup loading process is described in more detail [here](PsfRuntime/readme.md#fixup-loading), but in short, when the process starts up, the PSF Runtime will enumerate the set of dlls configured for the current process, loading them before calling `PSFInitialize` within a Detours transaction. Typical fixup behavior is to call `PSFRegister` for each function it wishes to detour at this time. This process can be somewhat automated by using the `DECLARE_FIXUP` and `DECLARE_STRING_FIXUP` macros. For example, consider the declarations for the `GetFileAttributes` functions:

```c++
DWORD WINAPI GetFileAttributesA(LPCSTR fileName);
DWORD WINAPI GetFileAttributesW(LPCWTR fileName);
```

Fixing a single function might look like:

```c++
auto GetFileAttributesWImpl = &::GetFileAttributesW;
DWORD __stdcall GetFileAttributesWFixup(LPCWSTR fileName)
{
    // ...
}
DECLARE_FIXUP(GetFileAttributesWImpl, GetFileAttributesWFixup);
```

Or fixing both with the same function might look like:

```c++
auto GetFileAttributesImpl = fixups::detoured_string_function(
    &::GetFileAttributesA,
    &::GetFileAttributesW);
template <typename CharT>
DWORD GetFileAttributesFixup(const CharT* fileName)
{
    // ...
}
DECLARE_STRING_FIXUP(GetFileAttributesImpl, GetFileAttributesFixup);
```

In either case, the `DECLARE_FIXUP`/`DECLARE_STRING_FIXUP` macros write the function pointers to a named section of memory that can be enumerated at runtime, typically by using the `psf::attach_all` and `psf::detach_all` functions inside the definitions of `PSFInitialize` and `PSFUninitialize` respectively. You can also optionally `#define PSF_DEFINE_EXPORTS` before `#include`-ing `psf_framework.h` _in a single translation unit_, which will define both of these functions for you as well as take care of exporting the functions with the correct names. For example, it is not uncommon to have a `main.cpp` that contains nothing more than:

```c++
#define PSF_DEFINE_EXPORTS
#include <psf_framework.h>
```

## Fixup Configuration
While a fixup is free to dictate and read its configuration however it wishes, the established pattern is to put the configuration alongside the fixup declaration in `config.json`. When this pattern is followed, the `PSFQueryCurrentDllConfig` function can be used to easily retrieve the already parsed JSON value from `config.json`. As a simple example, the following code demonstrates how to read a few configuration values:

```C++
// For this example, configuration is optional (PSFQueryCurrentDllConfig returns null)
if (auto configRoot = ::PSFQueryCurrentDllConfig())
{
    // NOTE: the "as_" functions throw on failure if the value is not of the desired type.
    // This example could be improved by providing better diagnostic information if the
    // types are different than what's expected.
    auto& config = configRoot->as_object(); // Throws if not an object

    if (auto enabledValue = config.try_get("enabled"))
    {
        g_enabled = enabledValue->as_boolean().get(); // Throws if not a boolean
    }

    if (auto logPathValue = config.try_get("logPath"))
    {
        g_logPath = logPathValue->as_string().wstring(); // Throws if not a string
    }
}
```

And a fixup declaration to go along with this might look like:

```json
{
    "dll": "MyFixup.dll",
    "config": {
        "enabled": true,
        "logPath": "logs/"
    }
}
```

## A Note on Fixup Reentrancy
Whenever a fixup invokes any external function other than the one being fixed, there is the possibility for reentrancy back into the fixup function. If we aren't careful and the fixup does not identify/handle this scenario, this recursion may continue indefinitely until the application crashes due to a stack overflow. As a concrete example, the fixup for `CreateFile` in the File Redirection Fixup may end up calling `CopyFile`, whose implementation just so happens to call `CreateFile`. If the `CreateFile` fixup were to have no mitigation in place, it may again attempt to call `CopyFile`, which would call `CreateFile`, and so on.

The [reentrancy_guard](include/reentrancy_guard.h) type exists as one available option to identify these scenarios. Example usage might look like:

```C++
void FooFixup()
{
    thread_local fixups::reentrancy_guard reentrancyGuard;
    auto guard = reentrancyGuard.enter();
    if (guard) { /*fixup code here*/ }
    return FooImpl();
}
```

A few considerations you may wish to take:

> * Reentrancy may be expected, and even okay at times. For example, if `CopyFile` were to call `CreateFile` with dramatically different function arguments, it may be the case that we still want to fixup that call as well
> * If COM reentrancy is a possible concern, this approach may not work since reentrant calls may be legitimate
> * While local testing may give some degree of confidence as to whether or not reentrancy may be a concern, future updates to the operating system may invalidate such assumptions
> * Interaction between two or more functions may need to be taken into account. E.g. the fixup for `CopyFile` may not want the fixup for `CreateFile` to have any effect when invoking the underlying implementation of `CopyFile`. In this case, a globally-scoped `reentrancy_guard` that's shared between the two functions may be more appropriate

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

In this case, the compiler will almost certainly inline the implementation of `CreateFileW` in `CreateFileA`. Similarly, the implementation of `CreateFileA` may get updated to call `DeleteFile2` directly, or improvements in the compiler's inlining logic may yield the same results. In any case, if you were to only detour `CreateFileW`, the application may stop working on future versions of the OS because your assumption is no longer true.

## A Note on Detouring `ntdll.dll`
It may be tempting to detour functions from `ntdll.dll` as a "catch all" solution. E.g. instead of detouring `CreateFileA`, `CreateFileW`, `CreateFile2`, `CopyFileA`, `CopyFileW`, and `CopyFile2` (and likely others), it's a very appealing solution to only detour `NtCreateFile`, which is not only less work, but it will also cover functions that you either forgot to detour, didn't want to spend the time to detour, didn't know about, or are added in the future. While not a terrible idea as a fallback (so long as you take measures to avoid/detect reentrancy/duplicate fixing), completely going this route is again relying on an implementation detail of the OS, which can change in future versions. Here be dragons.

## A Note on Exceptions
You will quickly notice that the different pieces of the PSF as well as multiple headers in the `include` directory use exceptions. Feel free to do so as well, but keep the following in mind:

> * Don't let exceptions leak out of your dll. That is, either catch exceptions at the end of your fixup functions, or declare them `noexcept` so that the process will terminate if an exception that would otherwise go unhandled gets thrown.
> * Applications can call `AddVectoredExceptionHandler` and add a vectored exception handler that does unpredictable things (e.g. such as terminate the application). Therefore, you should have the mindset that any exception thrown might crash the application _even if you handle it gracefully_.
