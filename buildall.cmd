@echo off
msbuild CentennialFixups.sln /p:platform=x86;configuration=debug
msbuild CentennialFixups.sln /p:platform=x86;configuration=release
msbuild CentennialFixups.sln /p:platform=x64;configuration=debug
msbuild CentennialFixups.sln /p:platform=x64;configuration=release
pushd tests
msbuild tests.sln /p:platform=x86;configuration=debug
msbuild tests.sln /p:platform=x86;configuration=release
msbuild tests.sln /p:platform=x64;configuration=debug
msbuild tests.sln /p:platform=x64;configuration=release
popd
