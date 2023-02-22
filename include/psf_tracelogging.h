#pragma once

#include <TraceLoggingProvider.h>
#include "Telemetry.h"
#include "RemovePII.h"
#include "psf_config.h"
#include "psf_constants.h"

TRACELOGGING_DECLARE_PROVIDER(g_Log_ETW_ComponentProvider);

namespace psf
{
    inline void TraceLogApplicationsConfigdata(std::string& currentExeFixes)
    {
        auto configRoot = PSFQueryConfigRoot();

        if (auto applications = configRoot->as_object().try_get("applications"))
        {
            for (auto& applicationsConfig : applications->as_array())
            {
                auto exeStr = applicationsConfig.as_object().try_get("executable")->as_string().wide();
                auto idStr = applicationsConfig.as_object().try_get("id")->as_string().wide();
                auto workingDirStr = applicationsConfig.as_object().try_get("workingDirectory") ? RemovePIIfromFilePath(applicationsConfig.as_object().try_get("workingDirectory")->as_string().wide()) : L"Null";
                auto argStr = applicationsConfig.as_object().try_get("arguments") ? RemovePIIfromFilePath(applicationsConfig.as_object().try_get("arguments")->as_string().wide()) : L"Null";
                bool inPackageCtxt = applicationsConfig.as_object().try_get("inPackageContext") ? applicationsConfig.as_object().try_get("inPackageContext")->as_boolean().get() : false;
                std::string ScriptUsed = "Null";
                if (applicationsConfig.as_object().try_get("startScript"))
                {
                    ScriptUsed = "StartScript";
                    if (applicationsConfig.as_object().try_get("endScript"))
                    {
                        ScriptUsed += "; EndScript";
                    }
                }
                if (applicationsConfig.as_object().try_get("endScript"))
                {
                    ScriptUsed = "EndScript";
                }
                auto PsfMonitorExecutable = applicationsConfig.as_object().try_get("monitor") ? (applicationsConfig.as_object().try_get("monitor")->as_object().try_get("executable")->as_string().wide()) : L"Null";

                TraceLoggingWrite(
                    g_Log_ETW_ComponentProvider,
                    "ApplicationsConfigdata",
                    TraceLoggingWideString(psf::current_package_full_name().c_str(), "PackageName"),
                    TraceLoggingWideString(exeStr, "applications_executable"),
                    TraceLoggingWideString(idStr, "applications_id"),
                    TraceLoggingWideString(workingDirStr, "WorkingDirectory"),
                    TraceLoggingWideString(argStr, "Arguments"),
                    TraceLoggingBool(inPackageCtxt, "InPackageContext"),
                    TraceLoggingString(ScriptUsed.c_str(), "ScriptUsed"),
                    TraceLoggingWideString(PsfMonitorExecutable, "PSFMonitor"),
                    TraceLoggingWideString(psf::current_version, "PSFVersion"),
                    TraceLoggingBoolean(TRUE, "UTCReplace_AppSessionGuid"),
                    TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage),
                    TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));

                if (!(std::wcscmp(idStr, psf::current_application_id().c_str())))
                {
                    currentExeFixes += ((std::wcscmp(workingDirStr, L"Null")) ? "WorkingDirectory; " : "");
                    currentExeFixes += ((std::wcscmp(argStr, L"Null")) ? "Arguments; " : "");
                    currentExeFixes += ((inPackageCtxt != false) ? "InPackageContext; " : "");
                    currentExeFixes += ((ScriptUsed != "Null") ? (ScriptUsed + "; ") : "");

                    if (auto config = PSFQueryExeConfig(std::filesystem::path(exeStr).stem().c_str()))
                    {
                        if (auto fixups = config->try_get("fixups"))
                        {
                            for (auto& fixupConfig : fixups->as_array())
                            {
                                currentExeFixes += (fixupConfig.as_object().get("dll").as_string().string());
                                currentExeFixes += "; ";
                            }
                        }
                    }
                }
            }
        }
    }

    inline void TraceLogPerformance(std::string& currentExeFixes, double elapsedTime)
    {
        TraceLoggingWrite(
            g_Log_ETW_ComponentProvider,
            "Performance",
            TraceLoggingWideString(L"PSFLoadRunTime", "EvalType"),
            TraceLoggingString(currentExeFixes.c_str(), "Fixes"),
            TraceLoggingFloat64(elapsedTime, "ElapsedTimeMS"),
            TraceLoggingBoolean(TRUE, "UTCReplace_AppSessionGuid"),
            TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
    }

    template<class CharT>
    inline void TraceLogExceptions(const char* Type, const CharT* Message)
    {
        if (std::is_same<CharT, char>::value)
        {
            TraceLoggingWrite(
                g_Log_ETW_ComponentProvider,
                "Exceptions",
                TraceLoggingString(Type, "Type"),
                TraceLoggingString((char*)Message, "Message"),
                TraceLoggingBoolean(TRUE, "UTCReplace_AppSessionGuid"),
                TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
        }
        else
        {
            TraceLoggingWrite(
                g_Log_ETW_ComponentProvider,
                "Exceptions",
                TraceLoggingString(Type, "Type"),
                TraceLoggingWideString((wchar_t*)Message, "Message"),
                TraceLoggingBoolean(TRUE, "UTCReplace_AppSessionGuid"),
                TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
        }

    }

    inline void TraceLogScriptInformation(const wchar_t* ScriptType, const wchar_t* ScriptPath, const wchar_t* CmdString, bool WaitForScriptToFinish, DWORD Timeout, bool ShouldRunOnce, int ShowWindowAction, DWORD ExitCode)
    {
        TraceLoggingWrite(
            g_Log_ETW_ComponentProvider,
            "ScriptInformation",
            TraceLoggingWideString(ScriptType, "ScriptType"),
            TraceLoggingWideString(ScriptPath, "ScriptPath"),
            TraceLoggingWideString(CmdString, "ScriptArguments"),
            TraceLoggingBoolean(WaitForScriptToFinish, "WaitForScriptToFinish"),
            TraceLoggingULong(Timeout, "Timeout"),
            TraceLoggingBoolean(ShouldRunOnce, "RunOnce"),
            TraceLoggingInt32(ShowWindowAction, "ShowWindow"),
            TraceLoggingULong(ExitCode, "ExitCode"),
            TraceLoggingBoolean(TRUE, "UTCReplace_AppSessionGuid"),
            TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
    }

    inline void TraceLogPSFMonitorConfigData(const wchar_t* PsfMonitorConfig)
    {
        TraceLoggingWrite(
            g_Log_ETW_ComponentProvider,
            "PSFMonitorConfigData",
            TraceLoggingWideString(PsfMonitorConfig, "PSFMonitorConfig"),
            TraceLoggingBoolean(TRUE, "UTCReplace_AppSessionGuid"),
            TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
    }

    template<class CharT>
    inline void TraceLogArgumentRedirection(const CharT* commanddLine, const CharT* cnvtCommandLine)
    {
        TraceLoggingWrite(g_Log_ETW_ComponentProvider, // handle to my provider
            "ArgumentRedirection",
            TraceLoggingValue(commanddLine, "commandLine"),
            TraceLoggingValue(cnvtCommandLine, "Converted commandLine"),
            TraceLoggingBoolean(TRUE, "UTCReplace_AppSessionGuid"),
            TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES)
        );
    }

    inline void TraceLogFixupConfig(const char* FixupType, const wchar_t * FixupConfigData)
    {
        TraceLoggingWrite(
            g_Log_ETW_ComponentProvider,
            "FixupConfig",
            TraceLoggingWideString(psf::current_package_full_name().c_str(), "PackageName"),
            TraceLoggingWideString(psf::current_application_id().c_str(), "ApplicationID"),
            TraceLoggingString(FixupType, "FixupType"),
            TraceLoggingWideString(FixupConfigData, "FixupConfigData"),
            TraceLoggingBoolean(TRUE, "UTCReplace_AppSessionGuid"),
            TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
    }

}
