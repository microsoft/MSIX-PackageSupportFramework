#pragma once


#include "psf_runtime.h"
#include "StartProcessHelper.h"
#include <future>
#include <memory>
#include <wil\resource.h>


class PsfPowershellScriptRunner
{
public:
	PsfPowershellScriptRunner(const psf::json_object* appConfig, const std::filesystem::path& currentDirectory) :
		currentDirectory(currentDirectory)
	{
		auto startScriptInformationObject = PSFQueryStartScriptInfo();
		auto endScriptInformationObject = PSFQueryEndScriptInfo();

		//If we want to run at least one script make sure powershell is installed on the computer.
		if (startScriptInformationObject || endScriptInformationObject)
		{
			THROW_HR_IF_MSG(ERROR_NOT_SUPPORTED, !CheckIfPowershellIsInstalled(), "PowerShell is not installed.  Please install PowerShell to run scripts in PSF");
		}

		auto stopOnScriptErrorObject = appConfig->try_get("stopOnScriptError");
		if (stopOnScriptErrorObject)
		{
			stopOnScriptError = stopOnScriptErrorObject->as_boolean().get();
		}

		if (startScriptInformationObject)
		{
			this->startScriptInformation.reset(startScriptInformationObject);
		}

		if (endScriptInformationObject)
		{
			this->endScriptInformation.reset(endScriptInformationObject);
		}


	}

	//RunStartingScript should return an error only if stopOnScriptError is true
	HRESULT RunStartingScript()
	{
		if (!startScriptInformation)
		{
			return ERROR_SUCCESS;
		}

		const std::wstring& scriptPath = GetScriptPath(*startScriptInformation);

		//This method throws an exception if the script does not exist
		ThrowIfScriptDoesNotExists(scriptPath);

		bool canScriptRun = false;
		THROW_IF_FAILED(CheckIfShouldRun(canScriptRun, *startScriptInformation));

		if (!canScriptRun)
		{
			return ERROR_SUCCESS;
		}


		bool waitForScriptToFinish = GetWaitForScriptToFinish(*startScriptInformation);

		//Don't run if the users wants the starting script to run asynch
		//AND to stop running PSF is the starting script encounters an error.
		//We stop here because we don't want to crash the application if the
		//starting script encountered an error.
		if (stopOnScriptError && !waitForScriptToFinish)
		{
			THROW_HR_MSG(ERROR_BAD_CONFIGURATION, "PSF does not allow stopping on a script error and running asynchronously.  Please either remove stopOnScriptError or add a wait");
		}

		std::wstring commandString = MakeCommandString(*startScriptInformation, scriptPath);
		DWORD scriptTimeout = GetTimeout(*startScriptInformation);


		int showWindowAction = GetShowWindowAction(*startScriptInformation);
		bool runInVirtualEnvironment = GetRunInVirtualEnvironment(*startScriptInformation);

		if (waitForScriptToFinish)
		{
			HRESULT startScriptResult = StartProcess(nullptr, commandString.data(), currentDirectory.c_str(), showWindowAction, runInVirtualEnvironment, scriptTimeout);

			if (stopOnScriptError)
			{
				THROW_IF_FAILED(startScriptResult);
			}
		}
		else
		{
			//We don't want to stop on an error and we want to run async
			std::thread(StartProcess, nullptr, commandString.data(), currentDirectory.c_str(), showWindowAction, runInVirtualEnvironment, scriptTimeout);
		}

		return {};
	}

	void RunEndingScript()
	{
		if (!endScriptInformation)
		{
			return;
		}

		const std::wstring& scriptPath = GetScriptPath(*endScriptInformation);

		//This method throws an exception if the script does not exist
		ThrowIfScriptDoesNotExists(scriptPath);

		bool canScriptRun = false;
		THROW_IF_FAILED(CheckIfShouldRun(canScriptRun, *endScriptInformation));

		if (canScriptRun)
		{
			std::wstring commandString = MakeCommandString(*endScriptInformation, scriptPath);
			DWORD scriptTimeout = GetTimeout(*endScriptInformation);


			int showWindowAction = GetShowWindowAction(*endScriptInformation);
			bool runInVirtualEnvironment = GetRunInVirtualEnvironment(*endScriptInformation);

			THROW_IF_FAILED(StartProcess(nullptr, commandString.data(), currentDirectory.c_str(), showWindowAction, runInVirtualEnvironment, scriptTimeout));
		}
	}

private:

	std::unique_ptr<const psf::json_object> startScriptInformation;
	std::unique_ptr<const psf::json_object> endScriptInformation;
	std::filesystem::path currentDirectory;
	bool stopOnScriptError = false;

	std::wstring MakeCommandString(const psf::json_object& scriptInformation, const std::wstring& scriptPath) noexcept
	{
		std::wstring commandString = L"Powershell.exe -file StartingScriptWrapper.ps1 ";
		commandString.append(L"\".\\");
		commandString.append(scriptPath);

		//Script arguments are optional.
		auto scriptArgumentsJObject = scriptInformation.try_get("scriptArguments");
		if (scriptArgumentsJObject)
		{
			commandString.append(L" ");
			commandString.append(scriptArgumentsJObject->as_string().wide());
		}

		//Add ending quote for the script inside a string literal.
		commandString.append(L"\"");

		return commandString;
	}

	const std::wstring GetScriptPath(const psf::json_object& scriptInformation) const
	{
		//Get throws if the key does not exist.
		return scriptInformation.get("scriptPath").as_string().wide();
	}

	DWORD GetTimeout(const psf::json_object& scriptInformation)
	{
		DWORD timeout = INFINITE;
		auto timeoutObject = scriptInformation.try_get("timeout");
		if (timeoutObject)
		{
			//Timout needs to be in milliseconds.
			//Multiple seconds from config by milliseconds.
			timeout = (DWORD)(1000 * timeoutObject->as_number().get_unsigned());
		}

		return timeout;
	}

	void ThrowIfScriptDoesNotExists(const std::wstring& scriptPath)
	{
		std::filesystem::path powershellScriptPath(scriptPath);
		bool doesScriptExist = false;
		//The file might be on a network drive.
		doesScriptExist = std::filesystem::exists(powershellScriptPath);

		//Check on local computer.
		if (!doesScriptExist)
		{
			doesScriptExist = std::filesystem::exists(currentDirectory / powershellScriptPath);
		}

		THROW_HR_IF_MSG(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), !doesScriptExist, "The starting script does not exist");
	}

	bool GetRunOnce(const psf::json_object& scriptInformation)
	{
		bool shouldRunOnce = false;
		auto runOnceObject = scriptInformation.try_get("runOnce");
		if (runOnceObject)
		{
			shouldRunOnce = runOnceObject->as_boolean().get();
		}

		return shouldRunOnce;
	}

	int GetShowWindowAction(const psf::json_object& scriptInformation)
	{
		//0 is to minimize
		int showWindowAction = 0;
		auto showWindowObject = scriptInformation.try_get("showWindow");
		if (showWindowObject)
		{
			bool showWindow = showWindowObject->as_boolean().get();
			if (showWindow)
			{
				//5 in SW_SHOW.
				showWindowAction = 5;
			}
		}

		return showWindowAction;
	}

	bool GetRunInVirtualEnvironment(const psf::json_object& scriptInformation)
	{
		bool runInVirtualEnvironment = true;
		auto runInVirtualEnvironmentJObject = scriptInformation.try_get("runInVirtualEnvironment");

		if (runInVirtualEnvironmentJObject)
		{
			runInVirtualEnvironment = runInVirtualEnvironmentJObject->as_boolean().get();
		}

		return runInVirtualEnvironment;
	}

	bool GetWaitForScriptToFinish(const psf::json_object& scriptInformation)
	{
		bool waitForScriptToFinish = false;
		auto waitForStartingScriptToFinishObject = scriptInformation.try_get("wait");
		if (waitForStartingScriptToFinishObject)
		{
			waitForScriptToFinish = waitForStartingScriptToFinishObject->as_boolean().get();
		}

		return waitForScriptToFinish;
	}

	HRESULT CheckIfShouldRun(bool& canScriptRun, const psf::json_object& scriptInformation)
	{
		bool shouldRunOnce = GetRunOnce(scriptInformation);
		canScriptRun = true;
		if (shouldRunOnce)
		{
			std::wstring runOnceSubKey = L"SOFTWARE\\";
			runOnceSubKey.append(psf::current_package_full_name());
			runOnceSubKey.append(L"\\runOnce");

			DWORD keyDisposition;
			wil::unique_hkey registryHandle;
			LSTATUS createResult = RegCreateKeyExW(HKEY_CURRENT_USER, runOnceSubKey.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &registryHandle, &keyDisposition);

			if (createResult != ERROR_SUCCESS)
			{
				return createResult;
			}

			if (keyDisposition == REG_OPENED_EXISTING_KEY)
			{
				canScriptRun = false;
			}
		}

		return{};
	}

	bool CheckIfPowershellIsInstalled()
	{
		wil::unique_hkey registryHandle;
		LSTATUS createResult = RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\PowerShell\\1", 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_READ, nullptr, &registryHandle, nullptr);

		if (createResult == ERROR_FILE_NOT_FOUND)
		{
			// If the key cannot be found, powershell is not installed
			return false;
		}
		else if (createResult != ERROR_SUCCESS)
		{
			THROW_HR_MSG(createResult, "Error with getting the key to see if PowerShell is installed.");
		}

		DWORD valueFromRegistry = 0;
		DWORD bufferSize = sizeof(DWORD);
		DWORD type = REG_DWORD;
		THROW_IF_WIN32_ERROR_MSG(RegQueryValueExW(registryHandle.get(), L"Install", nullptr, &type, reinterpret_cast<BYTE*>(&valueFromRegistry), &bufferSize),
			"Error with querying the key to see if PowerShell is installed.");

		if (valueFromRegistry != 1)
		{
			return false;
		}

		return true;
	}
};