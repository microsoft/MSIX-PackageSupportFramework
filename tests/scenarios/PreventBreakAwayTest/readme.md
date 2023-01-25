# Prevent BreakAway Test
This test verifies working of running external processes spanned by the executable in the package context when "InPackageContext" field is set in config file. This is done by running external process (C:\\Windows\\System32\\cmd.exe) from our application and verifying that PsfRuntime dll is loaded which is possible only when cmd.exe is run in package context.
