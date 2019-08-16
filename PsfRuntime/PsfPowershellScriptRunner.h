#pragma once


#include "psf_runtime.h"
#include "psf_utils.h"
#include <wil\resource.h>
#include <wil\result.h>

class PsfPowershellScriptRunner
{
public:
	PsfPowershellScriptRunner(const psf::json_object& scriptInformation, const std::filesystem::path& currentDirectory)
	{
		MakeCommandString(scriptInformation);
		CheckIfTheUserScriptExists(currentDirectory);
		CheckIfShowWindow(scriptInformation);
		CheckForRunOnce(scriptInformation);
	}

	const std::wstring& GetCommandString() const
	{
		return this->commandString;
	}

	bool DoesScriptExist() const
	{
		return this->doesScriptExist;
	}

	bool RunInVirtualEnvironment() const
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

	const bool ShouldShowWindow() const
	{
		return this->showWindowAction;
	}

	HRESULT CheckIfShouldRun(bool shouldRunOnce, bool& canScriptRun)
	{
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

private:
	bool doesScriptExist = false;
	std::wstring commandString = L"Powershell.exe -file StartingScriptWrapper.ps1 ";
	bool runInVirtualEnvironment = false;
	std::wstring scriptPath = L"";
	DWORD timeout = INFINITE;
	bool runOnce = false;
	int showWindowAction = 0;

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
};