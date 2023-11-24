#pragma once

#include <optional>

#include <winrt/base.h>
#include <winrt/Windows.Management.Deployment.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Foundation.Collections.h>

// Helper for querying installed package. Should be called from packge which has
// restricted capability "packageQuery" in their Manifest

using namespace winrt::Windows::Management::Deployment;
using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::Foundation::Collections;

constexpr const wchar_t* VCLibsName = L"Microsoft.VCLibs.140.00_8wekyb3d8bbwe";

struct PackageModel
{
    std::wstring pkg_name;
    std::wstring pkg_version;
    std::wstring pkg_publisher;
    std::wstring pkg_install_path;
};

inline std::optional<PackageModel> query_package_with_name(std::wstring_view pkgFamilyName)
{
    auto currPkgArchitecture = Package::Current().Id().Architecture();

    PackageManager pkgManager;
    
    Package currPkg = nullptr;
    PackageVersion maxPkgVersion = { 0 };

    IIterable<Package> possiblePackages = pkgManager.FindPackagesForUserWithPackageTypes(L"", pkgFamilyName, PackageTypes::Framework);

    for (auto&& pkg : possiblePackages)
    {
        if (pkg.Id().Architecture() != currPkgArchitecture)
        {
            continue;
        }

        std::wstring pkg_name = pkg.Id().Name().c_str();
        auto pkgVersion = pkg.Id().Version();

        // If this is 1st candidate package, set currPkg and maxPkgVersion to this package
        if (currPkg == nullptr)
        {
            currPkg = pkg;
            maxPkgVersion = pkgVersion;
        }
        // For others, compare version and set currPkg and maxPkgVersion to the package with highest version
        else
        {
            if (maxPkgVersion.Major < pkgVersion.Major ||
                (maxPkgVersion.Major == pkgVersion.Major && maxPkgVersion.Minor < pkgVersion.Minor) ||
                (maxPkgVersion.Major == pkgVersion.Major && maxPkgVersion.Minor == pkgVersion.Minor && maxPkgVersion.Build < pkgVersion.Build) ||
                (maxPkgVersion.Major == pkgVersion.Major && maxPkgVersion.Minor == pkgVersion.Minor && maxPkgVersion.Build == pkgVersion.Build && maxPkgVersion.Revision < pkgVersion.Revision))
            {
                maxPkgVersion = pkgVersion;
                currPkg = pkg;
            }
        }
    }

    if (!currPkg)
    {
        return std::nullopt;
    }

    PackageModel model;
    model.pkg_name = currPkg.Id().Name().c_str();
    model.pkg_version = std::to_wstring(maxPkgVersion.Major) + L"." + std::to_wstring(maxPkgVersion.Minor) + L"." + std::to_wstring(maxPkgVersion.Build) + L"." + std::to_wstring(maxPkgVersion.Revision);
    model.pkg_publisher = currPkg.Id().Publisher().c_str();
    model.pkg_install_path = std::wstring(currPkg.EffectivePath());

    return model;
}


