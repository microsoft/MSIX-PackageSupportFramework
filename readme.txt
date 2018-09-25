========================================================================
The PSF nuget package provides support for creating both:
- PSF Fixup DLL projects, to include in a PSF-enabled Desktop Bridge app
- PSF Application projects, including PSF redistributables and fixups
========================================================================

These in turn can be collected up with a Windows Application Packaging (WAP)
project in order to build, debug, and deploy a PSF-enabled Desktop Bridge app.

To create a PSF solution from within Visual Studio:
1. Create a solution for building your PSF-enabled packaged app
2. Add a project containing your classic Win32 target app requiring app-compat fixups
3. Add a Visual C++ | Windows Desktop | DLL project for your fixup logic
   a. Add a reference to the PSF nuget package, to enable building with the PSF SDK 
   b. Implement your fixup logic
4. Add a Visual C++ | General | Empty project for your application entry point
   a. Add a PSF nuget package reference, to supply PSF redistributables 
   b. Add a project reference to your fixup DLL project(s), to include in the package
   c. Right-click the fixup DLL project reference node and open the property page
   d. Set the Copy Local property to True, and both Library Dependency properties to False
   e. Right-click the project node and open the property page
   f. Under General, set the Target Name to PsfLauncher32 (Win32) and PsfLauncher64 (x64)
5. Add a Visual C++ | Windows Universal | Windows Application Packaging project
   a. Add application references to both your target app (2) and the PSF redist (4)
   b. Right-click the PSF redist project and set it as the app entry point
   c. Add a config.json file to provide app fixup configuration, and set its Package Action property to Content.
   d. Ensure that the appid in the config.json matches that in the package appx manifest
   e. Right-click the project node, select Edit, and add this to the bottom of the project:
	  <Target Name="PSFRemoveSourceProject" AfterTargets="ExpandProjectReferences" BeforeTargets="_ConvertItems">
		<ItemGroup>
		  <FilteredNonWapProjProjectOutput Include="@(_FilteredNonWapProjProjectOutput)">
			<SourceProject Condition="'%(_FilteredNonWapProjProjectOutput.SourceProject)'=='PSF'" />
		  </FilteredNonWapProjProjectOutput>
		  <_FilteredNonWapProjProjectOutput Remove="@(_FilteredNonWapProjProjectOutput)" />
		  <_FilteredNonWapProjProjectOutput Include="@(FilteredNonWapProjProjectOutput)" />
		</ItemGroup>
	  </Target>

The resulting solution structure looks like this:
PSFApp: EXE project providing target app and content 
PSFFixup: DLL project providing app-compat fixup logic
  References: PSF Nuget package
PSFRedist: EXE project providing PSF redistributables and entry point
  References: PSF Nuget package, fixup DLL project(s)
PSFPackage 
  Applications: PSFApp, PSFRedist (set as entry point)
  
========================================================================
For more information, including a PSF sample application, visit:
https://github.com/Microsoft/MSIX-PackageSupportFramework/
========================================================================
