# Architecture Test
Tests cross-architecture fixups in a variety of ways. In particular, there are two tests of interest here: an "x86" test and an "x64" test. Although named this way, they _both_ involve multiple architectures. In particular, the "x86" test uses a 64-bit `PsfLauncher.exe` to execute a 32-bit `ArchitectureTest32.exe`, which in turn executes a 64-bit `ArchitectureTest64.exe`. The "x64" test does the opposite. It uses a 32-bit `PsfLauncher.exe` to execute the 64-bit `ArchitectureTest64.exe`, which in turn executes the 32-bit `ArchitectureTest32.exe`.

This test also uses a "test" fixup, again built for multiple architectures. The un-fixed versions will show a message that reads `"This message should have been fixed"`. The fixed versions should detour the `MultiByteToWideChar` API to instead replace the text with `"You've been fixed!"`.

_NOTE: In order to run this test, the test project and all fixups must be built for both x64 and x86 Debug_
