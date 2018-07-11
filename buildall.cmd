@echo off
msbuild CentennialShims.sln /p:platform=x86;configuration=debug
msbuild CentennialShims.sln /p:platform=x86;configuration=release
msbuild CentennialShims.sln /p:platform=x64;configuration=debug
msbuild CentennialShims.sln /p:platform=x64;configuration=release
pushd tests
msbuild tests.sln /p:platform=x86;configuration=debug
msbuild tests.sln /p:platform=x86;configuration=release
msbuild tests.sln /p:platform=x64;configuration=debug
msbuild tests.sln /p:platform=x64;configuration=release
popd
