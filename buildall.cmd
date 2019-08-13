@echo off
REM msbuild CentennialFixups.sln /p:platform=x86;configuration=debug
REM msbuild CentennialFixups.sln /p:platform=x86;configuration=release
REM msbuild CentennialFixups.sln /p:platform=x64;configuration=debug
REM msbuild CentennialFixups.sln /p:platform=x64;configuration=release
msbuild CentennialFixups.sln /p:platform="Any CPU";configuration=debug
msbuild CentennialFixups.sln /p:platform="Any CPU";configuration=release
REM pushd tests
REM msbuild tests.sln /p:platform=x86;configuration=debug
REM msbuild tests.sln /p:platform=x86;configuration=release
REM msbuild tests.sln /p:platform=x64;configuration=debug
REM msbuild tests.sln /p:platform=x64;configuration=release
REM popd
