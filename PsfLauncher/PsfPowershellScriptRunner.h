#pragma once
#include "psf_runtime.h"
#include "StartProcessHelper.h"
#include "Globals.h"
#include <wil\resource.h>
#include <known_folders.h>

#ifndef SW_SHOW
	#define SW_SHOW 5
#endif

#ifndef SW_HIDE
	#define SW_HIDE 0
#endif

class PsfPowershellScriptRunner
{
public:
	PsfPowershellScriptRunner() = default;

	void Initialize(const psf::json_object* appConfig, const std::filesystem::path& currentDirectory, const std::filesystem::path& packageRootDirectory)
	{
		auto startScriptInformationObject = PSFQueryStartScriptInfo();
		auto endScriptInformationObject = PSFQueryEndScriptInfo();

		//If we want to run at least one script make sure powershell is installed on the computer.
		if (startScriptInformationObject || endScriptInformationObject)
		{
			THROW_HR_IF_MSG(ERROR_NOT_SUPPORTED, !CheckIfPowershellIsInstalled(), "PowerShell is not installed.  Please install PowerShell to run scripts in PSF");
		}

		bool stopOnScriptError = false;
		auto stopOnScriptErrorObject = appConfig->try_get("stopOnScriptError");
		if (stopOnScriptErrorObject)
		{
			stopOnScriptError = stopOnScriptErrorObject->as_boolean().get();
		}

		std::wstring scriptExecutionMode = L"";
		auto scriptExecutionModeObject = appConfig->try_get("scriptExecutionMode");  // supports options like "-ExecutionPolicy ByPass"
		if (scriptExecutionModeObject)
		{
			scriptExecutionMode = scriptExecutionModeObject->as_string().wstring();
		}

		// Note: the following path must be kept in sync with the FileRedirectionFixup PathRedirection.cpp
		std::filesystem::path writablePackageRootPath = psf::known_folder(FOLDERID_LocalAppData) / std::filesystem::path(L"Packages") / psf::current_package_family_name() / LR"(LocalCache\Local\Microsoft\WritablePackageRoot)";

		if (startScriptInformationObject)
		{
			this->m_startingScriptInformation = MakeScriptInformation(startScriptInformationObject, stopOnScriptError, scriptExecutionMode, currentDirectory, packageRootDirectory, writablePackageRootPath);
			this->m_startingScriptInformation.doesScriptExistInConfig = true;
		}

		if (endScriptInformationObject)
		{
			//Ending script ignores stopOnScriptError.  Keep it the default value
			this->m_endingScriptInformation = MakeScriptInformation(endScriptInformationObject, false, scriptExecutionMode, currentDirectory, packageRootDirectory, writablePackageRootPath);
			this->m_endingScriptInformation.doesScriptExistInConfig = true;

			//Ending script ignores this value.  Keep true to make sure
			//script runs on the current thread.
			this->m_endingScriptInformation.waitForScriptToFinish = true;
			this->m_endingScriptInformation.stopOnScriptError = false;
		}
	}

	//RunStartingScript should return an error only if stopOnScriptError is true
	void RunStartingScript()
	{
		LogString("StartingScript commandString", this->m_startingScriptInformation.commandString.c_str());
		LogString("StartingScript currentDirectory", this->m_startingScriptInformation.currentDirectory.c_str());
		if (this->m_startingScriptInformation.waitForScriptToFinish)
		{
			Log("StartingScript waitForScriptToFinish=true");
		}
		else
		{
			Log("StartingScript waitForScriptToFinish=false");
		}
		RunScript(this->m_startingScriptInformation);
	}

	void RunEndingScript()
	{
		LogString("EndingScript commandString", this->m_endingScriptInformation.commandString.c_str());
		LogString("EndingScript currentDirectory", this->m_endingScriptInformation.currentDirectory.c_str());
		RunScript(this->m_endingScriptInformation);
	}

private:
   struct MyProcThreadAttributeList
    {
    private:
        // To make sure the attribute value persists we cache it here.
        // 0x02 is equivalent to PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_DISABLE_PROCESS_TREE
        // The documentation can be found here:
        //https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-updateprocthreadattribute
        // This attribute tells PSF to
        // 1. Create Powershell in the same container as PSF.
        // 2. Any windows Powershell makes will also be in the same container as PSF.
        // This means that powershell, and any windows it makes, will have the same restrictions as PSF.
        DWORD createInContainerAttribute = 0x02;
        std::unique_ptr<_PROC_THREAD_ATTRIBUTE_LIST> attributeList;

    public:

        MyProcThreadAttributeList()
        {
            SIZE_T AttributeListSize{};
            InitializeProcThreadAttributeList(nullptr, 1, 0, &AttributeListSize);

            attributeList = std::unique_ptr<_PROC_THREAD_ATTRIBUTE_LIST>(reinterpret_cast<_PROC_THREAD_ATTRIBUTE_LIST*>(new char[AttributeListSize]));

            THROW_LAST_ERROR_IF_MSG(
                !InitializeProcThreadAttributeList(
                    attributeList.get(),
                    1,
                    0,
                    &AttributeListSize),
                "Could not initialize the proc thread attribute list.");

            // 18 stands for
            // PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY
            // this is the attribute value we want to add
            THROW_LAST_ERROR_IF_MSG(
                !UpdateProcThreadAttribute(
                    attributeList.get(),
                    0,
                    ProcThreadAttributeValue(18, FALSE, TRUE, FALSE),
                    &createInContainerAttribute,
                    sizeof(createInContainerAttribute),
                    nullptr,
                    nullptr),
                "Could not update Proc thread attribute.");
        }

        ~MyProcThreadAttributeList()
        {
            DeleteProcThreadAttributeList(attributeList.get());
        }

        LPPROC_THREAD_ATTRIBUTE_LIST get()
        {
            return attributeList.get();
        }

    };
  
	struct ScriptInformation
	{
		std::wstring scriptPath;
		std::wstring commandString;
		DWORD timeout = INFINITE;
		bool shouldRunOnce = true;
		int showWindowAction = SW_HIDE;
		bool waitForScriptToFinish = true;
		bool stopOnScriptError = false;
		std::filesystem::path currentDirectory;
		std::filesystem::path packageRoot;
		bool doesScriptExistInConfig = false;
	};

	ScriptInformation m_startingScriptInformation;
	ScriptInformation m_endingScriptInformation;
	MyProcThreadAttributeList m_AttributeList;

	void RunScript(ScriptInformation& script)
	{
		if (!script.doesScriptExistInConfig)
		{
			return;
		}

		THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), !DoesScriptExist(script.scriptPath, script.currentDirectory));

		bool canScriptRun = false;
		THROW_IF_FAILED(CheckIfShouldRun(script.shouldRunOnce, canScriptRun));

		if (!canScriptRun)
		{
			return;
		}

		if (script.waitForScriptToFinish)
		{
			HRESULT startScriptResult = StartProcess(nullptr, script.commandString.data(), script.currentDirectory.c_str(), script.showWindowAction, script.timeout, m_AttributeList.get());

			if (script.stopOnScriptError)
			{
				THROW_IF_FAILED(startScriptResult);
			}
		}
		else
		{
			//We don't want to stop on an error and we want to run async
			std::thread(StartProcess, nullptr, script.commandString.data(), script.currentDirectory.c_str(), script.showWindowAction, script.timeout, m_AttributeList.get());
		}
	}

	ScriptInformation MakeScriptInformation(const psf::json_object* scriptInformation, bool stopOnScriptError, std::wstring scriptExecutionMode, std::filesystem::path currentDirectory, std::filesystem::path packageRoot, std::filesystem::path packageWritableRoot)
	{
		ScriptInformation scriptStruct;
		scriptStruct.scriptPath = ReplacePsuedoRootVariables(GetScriptPath(*scriptInformation), packageRoot, packageWritableRoot);
		scriptStruct.commandString = ReplacePsuedoRootVariables(MakeCommandString(*scriptInformation, scriptExecutionMode, scriptStruct.scriptPath), packageRoot, packageWritableRoot);
		scriptStruct.timeout = GetTimeout(*scriptInformation);
		scriptStruct.shouldRunOnce = GetRunOnce(*scriptInformation);
		scriptStruct.showWindowAction = GetShowWindowAction(*scriptInformation);
		scriptStruct.waitForScriptToFinish = GetWaitForScriptToFinish(*scriptInformation);
		scriptStruct.stopOnScriptError = stopOnScriptError;
		scriptStruct.currentDirectory = currentDirectory;
		scriptStruct.packageRoot = packageRoot;

		//Async script run with a termination on failure is not a supported scenario.
		//Supporting this scenario would mean force terminating an executing user process
		//if the script fails.
		if (stopOnScriptError && !scriptStruct.waitForScriptToFinish)
		{
			THROW_HR_MSG(HRESULT_FROM_WIN32(ERROR_BAD_CONFIGURATION), "PSF does not allow stopping on a script error and running asynchronously.  Please either remove stopOnScriptError or add a wait");
		}

		return scriptStruct;
	}


	std::wstring ReplacePsuedoRootVariables(std::wstring inString, std::filesystem::path packageRoot, std::filesystem::path packageWritableRoot)
	{
		//Allow for a substitution in the strings for a new pseudo variable %MsixPackageRoot% so that arguments can point to files
		//inside the package using a syntax relative to the package root rather than rely on VFS pathing which can't kick in yet.
		std::wstring outString = inString;
		std::wstring var2rep1 = L"%MsixPackageRoot%";
		std::wstring var2rep2 = L"%MsixWritablePackageRoot%";

		std::wstring::size_type pos1 = 0u;
		std::wstring repargs1 = packageRoot.c_str();
		while ((pos1 = outString.find(var2rep1, pos1)) != std::string::npos) {
			outString.replace(pos1, var2rep1.length(), repargs1);
			pos1 += repargs1.length();
		}
		std::wstring::size_type pos2 = 0u;
		std::wstring repargs2 = packageWritableRoot.c_str();
		while ((pos2 = outString.find(var2rep2, pos2)) != std::string::npos) {
			outString.replace(pos2, var2rep2.length(), repargs2);
			pos2 += repargs2.length();
		}
		return outString;
	}

	std::wstring Dequote(std::wstring inString)
	{
		// Remove quotation-like marks around edges of a string reference to a file, if present.
		if (inString.length() > 2)
		{
			if (inString[0] == L'\"' && inString[inString.length() - 1] == L'\"')
			{
				return inString.substr(1, inString.length() - 2);
			}
			if (inString[0] == L'\'' && inString[inString.length() - 1] == L'\'')
			{
				return inString.substr(1, inString.length() - 2);
			}
			if (inString.length() > 4)
			{
				// Check for Powershell style references too
				if (inString[0] == L'`' && inString[1] == L'\'' && inString[inString.length() - 2] == L'`' && inString[inString.length() - 1] == L'\'')
				{
					return inString.substr(2, inString.length() - 4);
				}
				if (inString[0] == L'`' && inString[1] == L'\"' && inString[inString.length() - 2] == L'`' && inString[inString.length() - 1] == L'\"')
				{
					return inString.substr(2, inString.length() - 4);
				}
			}
		}
		return inString;
	}

	std::wstring EscapeFilenameForPowerShell(std::filesystem::path inputPath)
	{
		std::wstring outString = inputPath.c_str();
		std::wstring::size_type pos = 0u;
		std::wstring var2rep = L" ";
		std::wstring repargs = L"` ";
		while ((pos = outString.find(var2rep, pos)) != std::string::npos) {
			outString.replace(pos, var2rep.length(), repargs);
			pos += repargs.length();
		}
		return outString;

	}

	std::wstring MakeCommandString(const psf::json_object& scriptInformation, const std::wstring& scriptExecutionMode, const std::wstring& scriptPath)
	{
		std::wstring commandString = L"Powershell.exe ";
		commandString.append(scriptExecutionMode);
		commandString.append(L" -file StartingScriptWrapper.ps1 ");
		commandString.append(L"\"");

		// ScriptWrapper uses invoke-expression so we need the expression to launch another powershell to run a file with arguments.
		commandString.append(L"Powershell.exe ");
		commandString.append(scriptExecutionMode);
		commandString.append(L" -file ");

		const std::filesystem::path dequotedScriptPath = Dequote(scriptPath);
		std::wstring fixed4PowerShell = dequotedScriptPath; // EscapeFilenameForPowerShell(dequotedScriptPath);
		if (dequotedScriptPath.is_absolute())
		{
			commandString.append(L"\'");
			commandString.append(fixed4PowerShell);
			commandString.append(L"\'");
		}
		else
		{
			commandString.append(L"\'");
			commandString.append(L".\\");
			commandString.append(fixed4PowerShell);
			commandString.append(L"\'");
		}

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
		//.get throws if the key does not exist.
		return scriptInformation.get("scriptPath").as_string().wide();
	}

	DWORD GetTimeout(const psf::json_object& scriptInformation)
	{
		auto timeoutObject = scriptInformation.try_get("timeout");
		if (timeoutObject)
		{
			//Timout needs to be in milliseconds.
			//Multiple seconds from config by milliseconds.
			return (DWORD)(1000 * timeoutObject->as_number().get_unsigned());
		}

		return INFINITE;
	}

	bool DoesScriptExist(const std::wstring& scriptPath, std::filesystem::path currentDirectory)
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

		return doesScriptExist;
	}

	bool GetRunOnce(const psf::json_object& scriptInformation)
	{
		auto runOnceObject = scriptInformation.try_get("runOnce");
		if (runOnceObject)
		{
			return runOnceObject->as_boolean().get();
		}

		return true;
	}

	int GetShowWindowAction(const psf::json_object& scriptInformation)
	{
		auto showWindowObject = scriptInformation.try_get("showWindow");
		if (showWindowObject)
		{
			bool showWindow = showWindowObject->as_boolean().get();
			if (showWindow)
			{
				//5 in SW_SHOW.
				return SW_SHOW;
			}
		}

		return SW_HIDE;
	}

	bool GetWaitForScriptToFinish(const psf::json_object& scriptInformation)
	{
		auto waitForStartingScriptToFinishObject = scriptInformation.try_get("waitForScriptToFinish");
		if (waitForStartingScriptToFinishObject)
		{
			return waitForStartingScriptToFinishObject->as_boolean().get();
		}

		return true;
	}

	HRESULT CheckIfShouldRun(bool shouldRunOnce, bool& shouldScriptRun)
	{
		shouldScriptRun = true;
		if (shouldRunOnce)
		{
			std::wstring runOnceSubKey = L"SOFTWARE\\";
			runOnceSubKey.append(psf::current_package_full_name());
			runOnceSubKey.append(L"\\PSFScriptHasRun ");

			DWORD keyDisposition;
			wil::unique_hkey registryHandle;
			LSTATUS createResult = RegCreateKeyExW(HKEY_CURRENT_USER, runOnceSubKey.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &registryHandle, &keyDisposition);

			if (createResult != ERROR_SUCCESS)
			{
				return createResult;
			}

			if (keyDisposition == REG_OPENED_EXISTING_KEY)
			{
				shouldScriptRun = false;
			}
		}
		
		return S_OK;
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
			// Certain systems lack the 1 key but have the 3 key (both point to same path)
			createResult = RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\PowerShell\\3", 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_READ, nullptr, &registryHandle, nullptr);
			if (createResult == ERROR_FILE_NOT_FOUND)
			{
				return false;
			}
			else if (createResult != ERROR_SUCCESS)
			{
				THROW_HR_MSG(HRESULT_FROM_WIN32(createResult), "Error with getting the key to see if PowerShell is installed.");
			}
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