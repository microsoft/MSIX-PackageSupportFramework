//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <processthreadsapi.h>
#include "..\PsfLauncher\Globals.h"
#include "psf_logging.h"

/* These Attribute definitions are the form as stored in the dwflags field of the structure*/
#define SIH_PROC_THREAD_ATTRIBUTE_PARENT_PROCESS                    (1 << ProcThreadAttributeParentProcess)
#define SIH_PROC_THREAD_ATTRIBUTE_HANDLE_LIST                       (1 << ProcThreadAttributeHandleList)
#define SIH_PROC_THREAD_ATTRIBUTE_GROUP_AFFINITY                    (1 << ProcThreadAttributeGroupAffinity) 
#define SIH_PROC_THREAD_ATTRIBUTE_PREFERRED_NODE                    (1 << ProcThreadAttributePreferredNode)
#define SIH_PROC_THREAD_ATTRIBUTE_IDEAL_PROCESSOR                   (1 << ProcThreadAttributeIdealProcessor)
#define SIH_PROC_THREAD_ATTRIBUTE_UMS_THREAD                        (1 << ProcThreadAttributeUmsThread)
#define SIH_PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY                 (1 << ProcThreadAttributeMitigationPolicy)
#define SIH_PROC_THREAD_ATTRIBUTE_SECURITY_CAPABILITIES             (1 << ProcThreadAttributeSecurityCapabilities)
#define SIH_PROC_THREAD_ATTRIBUTE_PROTECTION_LEVEL                  (1 << ProcThreadAttributeProtectionLevel)
#define SIH_PROC_THREAD_ATTRIBUTE_JOB_LIST                          (1 << ProcThreadAttributeJobList)
#define SIH_PROC_THREAD_ATTRIBUTE_CHILD_PROCESS_POLICY              (1 << ProcThreadAttributeChildProcessPolicy)
#define SIH_PROC_THREAD_ATTRIBUTE_ALL_APPLICATION_PACKAGES_POLICY   (1 << ProcThreadAttributeAllApplicationPackagesPolicy)
#define SIH_PROC_THREAD_ATTRIBUTE_WIN32_KFILTER                     (1 << ProcThreadAttributeWin32kFilter)
#define SIH_PROC_THREAD_ATTRIBUTE_SAFE_OPEN_PROMPT_ORIGIN_CLAIM     (1 << ProcThreadAttributeSafeOpenPromptOriginClaim)
#define SIH_PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY                (1 << ProcThreadAttributeDesktopAppPolicy)
#define SIH_PROC_THREAD_ATTRIBUTE_PSEUDO_CONSOLE                    (1 << ProcThreadAttributePseudoConsole)



struct SIH_PROC_THREAD_ATTRIBUTE_ENTRY
{
    DWORD_PTR Attribute;
    size_t cbSize;
    PVOID  lpvalue;
};
struct SIH_PROC_THREAD_ATTRIBUTE_LIST
{
    DWORD dwflags;
    ULONG Size;
    ULONG Count;
    ULONG Reserved;
    PULONG Unknown;
    SIH_PROC_THREAD_ATTRIBUTE_ENTRY Entry[ANYSIZE_ARRAY];
};

inline void DumpStartupAttributes(SIH_PROC_THREAD_ATTRIBUTE_LIST* attlist)
{
    if (attlist != NULL)
    {
        Log("Attribute List Dump:");
        Log("\t\tdwflags=0x%x Size=0x%x Count=0x%x", attlist->dwflags, attlist->Size, attlist->Count);

        if ((attlist->dwflags & SIH_PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY) != 0)
        {
            Log("\t\tHas Desktop_App_Policy.");
            for (ULONG inx = 0; inx < attlist->Count; inx++)
            {
                SIH_PROC_THREAD_ATTRIBUTE_ENTRY Entry = attlist->Entry[inx];
                Log("\t\t\tIndex %d Attribute 0x%x Size=0x%x", inx, Entry.Attribute, Entry.cbSize);
                Loghexdump(Entry.lpvalue, (long)Entry.cbSize);
                if (Entry.Attribute == PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY)
                {
                    Log("\t\t\tIs Attribute_Desktop_App_Policy");
                    if (Entry.cbSize == 4)
                    {
                        DWORD attval = *((DWORD*)(Entry.lpvalue));
                        if ((attval & PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_ENABLE_PROCESS_TREE) != 0)
                        {
                            Log("\t\t\t\tPROCESS_CREATION_DESKTOP_APP_BREAKAWAY_ENABLE_PROCESS_TREE present.");
                        }
                        if ((attval & PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_DISABLE_PROCESS_TREE) != 0)
                        {
                            Log("\t\t\t\tPROCESS_CREATION_DESKTOP_APP_BREAKAWAY_DISABLE_PROCESS_TREE present.");
                        }
                        if ((attval & PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_OVERRIDE) != 0)
                        {
                            Log("\t\t\t\tPROCESS_CREATION_DESKTOP_APP_BREAKAWAY_OVERRIDE present.");
                        }
                    }
                }
            }
        }
        else
        {
            Loghexdump(attlist, 48);
        }
    }
}
inline bool DoesAttributeSpecifyInside(SIH_PROC_THREAD_ATTRIBUTE_LIST* attlist)
{
    bool bRet = true;
    if (attlist != NULL)
    {
        if ((attlist->dwflags & SIH_PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY) != 0)
        {
            for (ULONG inx = 0; inx < attlist->Count; inx++)
            {
                SIH_PROC_THREAD_ATTRIBUTE_ENTRY Entry = attlist->Entry[inx];
                if (Entry.Attribute == PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY)
                {
                    if (Entry.cbSize == 4)
                    {
                        DWORD attval = *((DWORD*)(Entry.lpvalue));
                        if ((attval & PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_OVERRIDE) != 0)
                        {
                            bRet = false;
                        }
                    }
                }
            }
        }
    }
    return bRet;
}

struct MyProcThreadAttributeList
{
private:
    // Implementation notes:
    //   Currently (Windows 10 21H1 and some previous releases plus Windows 11 21H2), the default
    //   behavior for most child processes is that they run inside the container.  The two known
    //   exceptions to this are the conhost and cmd.exe processes.  We generally don't care about
    //   the conhost processes.
    // 
    // The documentation on these can be found here:
    //https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-updateprocthreadattribute

    // The PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY attribute has some settings that can impact this.
    // 0x04 is equivalent to PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_OVERRIDE.  
    //     When used in a process creation call it affects only the new process being created, and 
    //     forces the new process to start inside the container, if possible.
    // 0x01 is equivalent to PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_ENABLE_PROCESS_TREE
    //     When used in a process creation call, it ONLY affects child processes of the process being
    //     created, meaning that grandshildren can/will break away from running inside the container used by
    //     that new process.
    // 0x02 is equivalent to PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_DISABLE_PROCESS_TREE 
    //     When used in a process creation call, it ONLY affects child processes of the process being
    //     created, meaning that grandshildren can/will NOT break away from running inside the container used by
    //     that new process.
    //
    // This PSF code uses the attribute to cause:
    //   1. Create a Powershell process in the same container as PSF.
    //   2. Any process that Powershell stats will also be in the same container as PSF.
    // This means that powershell, and any windows it makes, will have the same restrictions as PSF.
    DWORD createInContainerAttribute = 0x02;
    DWORD createOutsideContainerAttribute = 0x01;

    // Processes running inside the container run at a different level (Low) that uncontained processes,
    // and the default behavior is to have the child process run at the same level.  This can be overridden
    // by using PROC_THREAD_ATTRIBUTE_PROTECTION_LEVEL.
    //
    // The code here currently does not set this level as we prefer to keep the same level anyway.
    // UPDATE: If we provide an atttribute list we must include this for certain proesses anyway.
    DWORD attProtLevel = ProcThreadAttributeProtectionLevel;
    DWORD protectionLevel = PROTECTION_LEVEL_SAME;

    // Should it become neccessary to change more than one attribute, the number of attributes will need
    // to be modified in both initialization calls in the CTOR.

    std::unique_ptr<_PROC_THREAD_ATTRIBUTE_LIST> attributeList;

public:

    MyProcThreadAttributeList(bool setContainer, bool inside, bool setProtSame)
    {
        DWORD countAtt = 0;
        if (setContainer)
        {
            countAtt++;
        }
        if (setProtSame)
        {
            countAtt++;
        }
        // Ffor example of this code with two attributes see: https://github.com/microsoft/terminal/blob/main/src/server/Entrypoints.cpp
        SIZE_T AttributeListSize; //{};
        InitializeProcThreadAttributeList(nullptr, countAtt, 0, &AttributeListSize);
        attributeList = std::unique_ptr<_PROC_THREAD_ATTRIBUTE_LIST>(reinterpret_cast<_PROC_THREAD_ATTRIBUTE_LIST*>(new char[AttributeListSize]));
        //attributeList = std::unique_ptr<_PROC_THREAD_ATTRIBUTE_LIST>(reinterpret_cast<_PROC_THREAD_ATTRIBUTE_LIST*>(HeapAlloc(GetProcessHeap(), 0, AttributeListSize)));
        InitializeProcThreadAttributeList(
                attributeList.get(),
                countAtt,
                0,
                &AttributeListSize);

        if (setContainer)
        {
            // 18 stands for
            // PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY
            // this is the attribute value we want to add
            if (inside)
            {
                bool b = UpdateProcThreadAttribute(
                    attributeList.get(),
                    0,
                    ProcThreadAttributeValue(18, FALSE, TRUE, FALSE),
                    &createInContainerAttribute,
                    sizeof(createInContainerAttribute),
                    nullptr,
                    nullptr);
                if (!b)
                {
                    ;
                }
            }
            else
            {
                bool b = UpdateProcThreadAttribute(
                    attributeList.get(),
                    0,
                    ProcThreadAttributeValue(18, FALSE, TRUE, FALSE),
                    &createOutsideContainerAttribute,
                    sizeof(createOutsideContainerAttribute),
                    nullptr,
                    nullptr);
                if (!b)
                {
                    ;
                }
            }
        }

        if (setProtSame)
        {
            // 11 stands for
            // PROC_THREAD_ATTRIBUTE_PROTECTION_LEVEL
            // this is the attribute value we want to add
            //
            BOOL additive = FALSE;
            if (countAtt == 2)
                additive = TRUE;
            bool b2 = UpdateProcThreadAttribute(
                attributeList.get(),
                0,
                ProcThreadAttributeValue(attProtLevel, FALSE, TRUE, additive),
                &protectionLevel,
                sizeof(protectionLevel),
                nullptr,
                nullptr);
            if (!b2)
            {
                //	"Could not update Proc thread attribute for PROTECTION_LEVEL.");
                ;
            }
        }
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
