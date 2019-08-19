#pragma once


#include "psf_runtime.h"
#include "StartProcessHelper.h"
#include <future>
#include <wil\resource.h>


class PsfPowershellScriptRunner
{
public:
	PsfPowershellScriptRunner(const psf::json_object& scriptInformation, const std::filesystem::path& currentDirectory)
	{
		MakeCommandString(scriptInformation);
		CheckIfTheUserScriptExists(currentDirectory);
		CheckIfShowWindow(scriptInformation);
		CheckForRunOnce(scriptInformation);
		CheckForWaitForScriptToFinish(scriptInformation);
	}

	const std::wstring& GetCommandString() const
	{
		return this->commandString;
	}

	bool GetDoesScriptExist() const
	{
		return this->doesScriptExist;
	}

	bool GetRunInVirtualEnvironment() const
	{
		return this->runInVirtualEnvironment;
	}

	const std::wstring& GetScriptPath() const
	{
		return this->scriptPath;
	}

	const DWORD GetScriptTimeout() const
	{
		return this->timeout;
	}

	const bool GetRunOnce() const
	{
		return this->runOnce;
	}

	const bool GetShouldShowWindow() const
	{
		return this->showWindowAction;
	}

	const bool GetWaitForScriptToFinish() const
	{
		return this->waitForScriptToFinish;
	}

	void RunPfsScript(bool stopOnScriptError, std::filesystem::path currentDirectory)
	{
		bool canScriptRun = false;
		THROW_IF_FAILED(CheckIfShouldRun(canScriptRun));

		if (canScriptRun && stopOnScriptError)
		{
			if (stopOnScriptError)
			{
				THROW_IF_FAILED(StartProcess(nullptr, this->commandString.data(), currentDirectory.c_str(), this->showWindowAction, this->runInVirtualEnvironment, this->timeout));
			}
			else
			{
				//We don't want to stop on an error and we want to run async
				std::thread(StartProcess, nullptr, this->commandString.data(), currentDirectory.c_str(), this->showWindowAction, this->runInVirtualEnvironment, this->timeout);
			}
		}
	}



private:
	bool doesScriptExist = false;
	std::wstring commandString = L"Powershell.exe -file StartingScriptWrapper.ps1 ";
	bool runInVirtualEnvironment = false;
	std::wstring scriptPath = L"";
	DWORD timeout = INFINITE;
	bool runOnce = false;
	int showWindowAction = 0;
	bool waitForScriptToFinish = false;

	void MakeCommandString(const psf::json_object& scriptInformation) noexcept
	{
		this->scriptPath = scriptInformation.get("scriptPath").as_string().wide();
		this->commandString.append(L"\".\\");
		this->commandString.append(this->scriptPath);

		//Script arguments are optional.
		auto scriptArgumentsJObject = scriptInformation.try_get("scriptArguments");
		if (scriptArgumentsJObject)
		{
			this->commandString.append(L" ");
			this->commandString.append(scriptArgumentsJObject->as_string().wide());
		}

		this->commandString.append(L"\"");
		CheckForTimeout(scriptInformation);
	}

	void CheckIfTheUserScriptExists(const std::filesystem::path currentDirectory)
	{
		std::filesystem::path powershellScriptPath(this->scriptPath);

		//The file might be on a network drive.
		this->doesScriptExist = std::filesystem::exists(powershellScriptPath);

		//Check on local computer.
		if (!this->doesScriptExist)
		{
			this->doesScriptExist = std::filesystem::exists(currentDirectory / powershellScriptPath);
		}
	}

	void CheckIfRunInVirtualEnvironment(const psf::json_object& scriptInformation)
	{
		auto runInVirtualEnvironmentJObject = scriptInformation.try_get("runInVirtualEnvironment");

		if (runInVirtualEnvironmentJObject)
		{
			this->runInVirtualEnvironment = runInVirtualEnvironmentJObject->as_boolean().get();
		}
	}

	void CheckForTimeout(const psf::json_object& scriptInformation)
	{
		auto timeoutObject = scriptInformation.try_get("timeout");
		if (timeoutObject)
		{
			//Timout needs to be in milliseconds.
			//Multiple seconds from config by milliseconds.
			this->timeout = (DWORD)(1000 * timeoutObject->as_number().get_unsigned());
		}
	}

	void CheckIfShowWindow(const psf::json_object& scriptInformation)
	{
		auto showWindowObject = scriptInformation.try_get("showWindow");
		if (showWindowObject)
		{
			bool showWindow = showWindowObject->as_boolean().get();
			if (showWindow)
			{
				//5 in SW_SHOW.
				this->showWindowAction = 5;
			}

		}
	}

	void CheckForRunOnce(const psf::json_object& scriptInformation)
	{
		auto runOnceObject = scriptInformation.try_get("runOnce");
		if (runOnceObject)
		{
			this->runOnce = runOnceObject->as_boolean().get();
		}
	}

	void CheckForWaitForScriptToFinish(const psf::json_object& scriptInformation)
	{
		auto waitForStartingScriptToFinishObject = scriptInformation.try_get("wait");
		if (waitForStartingScriptToFinishObject)
		{
			this->waitForScriptToFinish = waitForStartingScriptToFinishObject->as_boolean().get();
		}
	}

	HRESULT CheckIfShouldRun(bool& canScriptRun)
	{
		canScriptRun = true;
		if (runOnce)
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
};