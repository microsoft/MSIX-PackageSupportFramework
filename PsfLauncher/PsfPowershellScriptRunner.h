#pragma once
#include "psf_runtime.h"
#include "StartProcessHelper.h"
#include <VersionHelpers.h>
#include <wil\resource.h>

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

    void Initialize(const psf::json_object* appConfig, const std::filesystem::path& currentDirectory)
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

        if (startScriptInformationObject)
        {
            this->startingScriptInformation = MakeScriptInformation(startScriptInformationObject, stopOnScriptError, currentDirectory);
            this->startingScriptInformation.doesScriptExistInConfig = true;
        }

        if (endScriptInformationObject)
        {
            //Ending script ignores stopOnScriptError.  Keep it the default value
            this->endingScriptInformation = MakeScriptInformation(endScriptInformationObject, false, currentDirectory);
            this->endingScriptInformation.doesScriptExistInConfig = true;

            //Ending script ignores this value.  Keep true to make sure
            //script runs on the current thread.
            this->endingScriptInformation.waitForScriptToFinish = true;
            this->endingScriptInformation.stopOnScriptError = false;
        }
    }

    //RunStartingScript should return an error only if stopOnScriptError is true
    void RunStartingScript()
    {
        RunScript(this->startingScriptInformation);
    }

    void RunEndingScript()
    {
        RunScript(this->endingScriptInformation);
    }

private:
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
        bool doesScriptExistInConfig = false;
        LPPROC_THREAD_ATTRIBUTE_LIST attributeList = nullptr;
    };

    ScriptInformation startingScriptInformation;
    ScriptInformation endingScriptInformation;

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
            HRESULT startScriptResult = StartProcess(nullptr, script.commandString.data(), script.currentDirectory.c_str(), script.showWindowAction, script.timeout, script.attributeList);

            if (script.stopOnScriptError)
            {
                THROW_IF_FAILED(startScriptResult);
            }
        }
        else
        {
            //We don't want to stop on an error and we want to run async
            std::thread(StartProcess, nullptr, script.commandString.data(), script.currentDirectory.c_str(), script.showWindowAction, script.timeout, script.attributeList);
        }
    }

    ScriptInformation MakeScriptInformation(const psf::json_object* scriptInformation, bool stopOnScriptError, std::filesystem::path currentDirectory)
    {
        ScriptInformation scriptStruct;
        scriptStruct.scriptPath = GetScriptPath(*scriptInformation);
        scriptStruct.commandString = MakeCommandString(*scriptInformation, scriptStruct.scriptPath);
        scriptStruct.timeout = GetTimeout(*scriptInformation);
        scriptStruct.shouldRunOnce = GetRunOnce(*scriptInformation);
        scriptStruct.showWindowAction = GetShowWindowAction(*scriptInformation);
        scriptStruct.waitForScriptToFinish = GetWaitForScriptToFinish(*scriptInformation);
        scriptStruct.stopOnScriptError = stopOnScriptError;
        scriptStruct.currentDirectory = currentDirectory;
        scriptStruct.attributeList = MakeAttributeList();

        //Async script run with a termination on failure is not a supported scenario.
        //Supporting this scenario would mean force terminating an executing user process
        //if the script fails.
        if (stopOnScriptError && !scriptStruct.waitForScriptToFinish)
        {
            THROW_HR_MSG(HRESULT_FROM_WIN32(ERROR_BAD_CONFIGURATION), "PSF does not allow stopping on a script error and running asynchronously.  Please either remove stopOnScriptError or add a wait");
        }

        return scriptStruct;
    }

    std::wstring MakeCommandString(const psf::json_object& scriptInformation, const std::wstring& scriptPath)
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
            THROW_HR_MSG(HRESULT_FROM_WIN32(createResult), "Error with getting the key to see if PowerShell is installed.");
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

    LPPROC_THREAD_ATTRIBUTE_LIST MakeAttributeList()
    {
        LPPROC_THREAD_ATTRIBUTE_LIST attributeList;
        SIZE_T AttributeListSize{};


        InitializeProcThreadAttributeList(nullptr, 1, 0, &AttributeListSize);
        attributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(
            GetProcessHeap(),
            0,
            AttributeListSize
        );

        THROW_LAST_ERROR_IF_MSG(
            !InitializeProcThreadAttributeList(
                attributeList,
                1,
                0,
                &AttributeListSize),
            "Could not initialize the proc thread attribute list.");

        DWORD attribute = 0x02;
        THROW_LAST_ERROR_IF_MSG(
            !UpdateProcThreadAttribute(
                attributeList,
                0,
                ProcThreadAttributeValue(18, FALSE, TRUE, FALSE),
                &attribute,
                sizeof(attribute),
                nullptr,
                nullptr),
            "Could not update Proc thread attribute.");

        return attributeList;
    }
};