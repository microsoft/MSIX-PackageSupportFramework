//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once
#include <wil\result.h>

// Define _PROC_THREAD_ATTRIBUTE_LIST as an empty struct because it's internal-only and variable-sized
struct _PROC_THREAD_ATTRIBUTE_LIST {};

struct ProcThreadAttributeList
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
	// 3. Any child process that is run with "InPackageContext" field as true will run in same context of package
	DWORD createInContainerAttribute = 0x02;
	std::unique_ptr<_PROC_THREAD_ATTRIBUTE_LIST> attributeList;

public:

	ProcThreadAttributeList()
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

	~ProcThreadAttributeList()
	{
		DeleteProcThreadAttributeList(attributeList.get());
	}

	LPPROC_THREAD_ATTRIBUTE_LIST get()
	{
		return attributeList.get();
	}

};
