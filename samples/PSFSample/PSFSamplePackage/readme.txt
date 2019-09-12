========================================================================
    Package Support Framework (PSF) Sample Application
========================================================================

This sample demonstrates a simple PSF application.  For details on the
solution structure, including how to assemble a PSF app from scratch,
please see the readme.txt included in the Package Support Framework 
Nuget package, which can be installed from Visual Studio, or from here:
https://www.nuget.org/packages/Microsoft.PackageSupportFramework

The PSF Sample Application demonstrates fixups for two typical app compat 
issues:
1. Executing a packaged app with the wrong current working directory.
	This issue arises because the app is no longer launched from a 
	shortcut that would normally specify the executable path as the 
	current working directory.  Instead, the system path is supplied, 
	and this may cause errors for an app executable expecting to find 
	files relative to its own location, via the current working directory.
	The fixup for this is simply to configure the config.json entry for
	working_directory.
2. Attempting to write to files in the package path, which is protected.
	This issue may arise for apps which installed into a custom folder,
	or which relied on UAC file/registry virtualization to enable writes 
	to the program folder.  When executing as a package app, this is no 
	longer permitted.  In this case, a fixup DLL is required to redirect
	such writes to a suitable location such as the app's local cached 
	data folder.

This sample contains a single fixup DLL which fixes two Win32 functions:
1. MessageBoxWFixup replaces the MessageBoxW caption with "Fixup Message".
    This is demonstrated indirectly when the app calls MessageBox.Show,
    with the caption "App Message".
2. CreateFileWFixup monitors calls to CreateFileW that attempt to create
    a file with extension "log".  When such an attempt is detected, the
    fixup redirects the path to the app's ApplicationData LocalCacheFolder.
    Again, the app initiates this behavior indirectly with a call to 
    System.IO.File.CreateText.  
