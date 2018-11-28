# PSF Monitor
This sub-project provides for a Graphical User Interface tool, in the style of ProcessMonitor, that integrates events output by TraceFixup with those of kerenel level tracing.

Used in conjuction with an application launched by PsfLauncher and the TraceFixup dll configured to output events, this makes it easier to trace the application calls that may ultimately require fixups.

## Documentation
See the readme.md for PsfLauncher for documentation on how to use PsfLauncher in conjuction with TraceFixup to produce tracing events in your package.

The PsfLauncher documentation shows how to integrate the monitor inside your package and have it automatically run when you start the application to be traced.
You may, however, run PSF Monitor externally from your package as long as the package is configured with the TraceFixup shim.


## [License](https://github.com/Microsoft/MSIX-PackageSupportFramework/blob/master/LICENSE)
Code licensed under the [MIT License](https://github.com/Microsoft/MSIX-PackageSupportFramework/blob/master/LICENSE).

## Contribute
This project welcomes contributions and suggestions.  Most contributions require you to agree to a Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us the rights to use your contribution. For details, visit https://cla.microsoft.com.

Submit your own fixup(s) to the community:
1. Create a private fork for yourself
2. Make your changes in your private branch
3. Create a pull request into 'fork:Microsoft/MSIX-PackageSupportFramework' 'base:master'

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions provided by the bot. You will only need to do this once across all repos using our CLA.

Here is how you can contribute to the Package Support Framework:
* [Submit bugs](https://github.com/Microsoft/MSIX-PackageSupportFramework/issues) and help us verify fixups
* [Submit pull requests](https://github.com/Microsoft/MSIX-PackageSupportFramework/pulls) for bug fixups and features and discuss existing proposals

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
