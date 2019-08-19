#pragma once
#include <windows.h>
#include <wil\resource.h>

// These two macros don't exist in RS1.  Define them here to prevent build
// failures when building for RS1.
#ifndef PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_DISABLE_PROCESS_TREE
#    define PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_DISABLE_PROCESS_TREE PROCESS_CREATION_DESKTOP_APPX_OVERRIDE
#endif

#ifndef PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY
#    define PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY PROC_THREAD_ATTRIBUTE_CHILD_PROCESS_POLICY
#endif

HRESULT StartProcess(LPCWSTR applicationName, LPWSTR commandLine, LPCWSTR currentDirectory, int cmdShow, bool runInVirtualEnvironment, DWORD timeout)
{
	STARTUPINFOEXW startupInfoEx =
	{
		{
		sizeof(startupInfoEx)
		, nullptr // lpReserved
		, nullptr // lpDesktop
		, nullptr // lpTitle
		, 0 // dwX
		, 0 // dwY
		, 0 // dwXSize
		, 0 // swYSize
		, 0 // dwXCountChar
		, 0 // dwYCountChar
		, 0 // dwFillAttribute
		, STARTF_USESHOWWINDOW // dwFlags
		, static_cast<WORD>(cmdShow) // wShowWindow
		}
	};

	if (runInVirtualEnvironment)
	{
		SIZE_T AttributeListSize{};
		InitializeProcThreadAttributeList(nullptr, 1, 0, &AttributeListSize);
		startupInfoEx.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(
			GetProcessHeap(),
			0,
			AttributeListSize
		);

		RETURN_LAST_ERROR_IF_MSG(
			!InitializeProcThreadAttributeList(
				startupInfoEx.lpAttributeList,
				1,
				0,
				&AttributeListSize),
			"Could not initialize the proc thread attribute list.");

		DWORD attribute = PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_DISABLE_PROCESS_TREE;
		RETURN_LAST_ERROR_IF_MSG(
			!UpdateProcThreadAttribute(
				startupInfoEx.lpAttributeList,
				0,
				PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY,
				&attribute,
				sizeof(attribute),
				nullptr,
				nullptr),
			"Could not update Proc thread attribute.");
	}

	PROCESS_INFORMATION processInfo{};
	RETURN_LAST_ERROR_IF_MSG(
		!::CreateProcessW(
			applicationName,
			commandLine,
			nullptr, nullptr, // Process/ThreadAttributes
			true, // InheritHandles
			EXTENDED_STARTUPINFO_PRESENT, // CreationFlags
			nullptr, // Environment
			currentDirectory,
			(LPSTARTUPINFO)& startupInfoEx,
			&processInfo),
		"ERROR: Failed to create a process for %ws",
		applicationName);

	RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE), processInfo.hProcess == INVALID_HANDLE_VALUE);
	DWORD waitResult = ::WaitForSingleObject(processInfo.hProcess, timeout);
	RETURN_LAST_ERROR_IF_MSG(waitResult != WAIT_OBJECT_0, "Waiting operation failed unexpectedly.");
	CloseHandle(processInfo.hProcess);
	CloseHandle(processInfo.hThread);

	return ERROR_SUCCESS;
}

