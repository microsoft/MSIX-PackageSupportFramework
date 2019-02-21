@echo off
msbuild CentennialFixups.sln /p:platform=x86;configuration=debug
msbuild CentennialFixups.sln /p:platform=x86;configuration=release
msbuild CentennialFixups.sln /p:platform=x64;configuration=debug
msbuild CentennialFixups.sln /p:platform=x64;configuration=release
msbuild CentennialFixups.sln /p:platform=AnyCPU;configuration=debug
msbuild CentennialFixups.sln /p:platform=AnyCPU;configuration=release
pushd tests
msbuild tests.sln /p:platform=x86;configuration=debug
msbuild tests.sln /p:platform=x86;configuration=release
msbuild tests.sln /p:platform=x64;configuration=debug
msbuild tests.sln /p:platform=x64;configuration=release
popd
