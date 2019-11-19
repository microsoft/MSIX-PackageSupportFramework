#Testing
Package Support Framework has several regression tests that are required to pass before your code will be pulled into the develop branch.

Below will be two sets of instructions.  One set will tell you how to run all the tests.  The second set will tell you how to debug tests.

##How Package Support Framework runs tests
Because Package Support Framework needs to hook into an application all tests are packaged into an application, installed on your computer, then ran.

At the end of the RunTests.ps1 script all the applications are removed.

##Running all tests.
Please follow the instructions to run all the tests and see if they all pass.

 1. Open a admin Visual Studio developer command prompt.
 2. Navigate to the root of the repository (MSIX-PackageSupportFramework)
 3. Run "buildall.cmd"
  1. If you have build errors please fix them before continuing.
 4. Open an amin powershell prompt.
 5. Navigate to the tests directory of the repository.
 6. run ".\runtests.ps1"
 
Wait for the tests to run and finish.  When the script is finished it will tell you how many tests failed.  If no tests failed you can make the pull request.

##Debugging tests
Sometimes our changes will make a test fail.  If that is the case you might need to debug a test to figure out why it failed so you can fix your code.

Please follow the below steps to debug tests.

 1. Run the tests in the "Running all tests" section to see 
 