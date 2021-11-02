//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <processthreadsapi.h>


void Log(const char* fmt, ...);

inline void Loghexdump(void* pAddressIn, long  lSize)
{
    char szBuf[128];
    long lIndent = 1;
    long lOutLen, lIndex, lIndex2, lOutLen2;
    long lRelPos;
    struct { char* pData; unsigned long lSize; } buf;
    unsigned char* pTmp, ucTmp;
    unsigned char* rememberPtmp;
    unsigned char* pAddress = (unsigned char*)pAddressIn;

    buf.pData = (char*)pAddress;
    buf.lSize = lSize;

while (buf.lSize > 0)
{
    pTmp = (unsigned char*)buf.pData;
    lOutLen = (int)buf.lSize;
    if (lOutLen > 16)
        lOutLen = 16;

    // create a 64-character formatted output line:
    sprintf_s(szBuf, 100, " >                            "
        "                      "
        "         ");
    rememberPtmp = pTmp;
    lOutLen2 = lOutLen;

    for (lIndex = 1 + lIndent, lIndex2 = 53 - 15 + lIndent, lRelPos = 0;
        lOutLen2;
        lOutLen2--, lIndex += 2, lIndex2++
        )
    {
        ucTmp = *pTmp++;

        sprintf_s(szBuf + lIndex, 100 - lIndex, "%02X ", (unsigned short)ucTmp);
        if (!isprint(ucTmp))  ucTmp = '.'; // nonprintable char
        szBuf[lIndex2] = ucTmp;

        if (!(++lRelPos & 3))     // extra blank after 4 bytes
        {
            lIndex++; szBuf[lIndex + 2] = ' ';
        }
    }

    if (!(lRelPos & 3)) lIndex--;

    sprintf_s(szBuf + lIndex, 100 - lIndex, "<    %08lx  ", (unsigned long)(rememberPtmp - pAddress));
    szBuf[lIndex + 14] = 0x0;

    ::OutputDebugStringA(szBuf);

    buf.pData += lOutLen;
    buf.lSize -= lOutLen;
}
}

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
        Log("\tdwflags=0x%x Size=0x%x Count=0x%x", attlist->dwflags, attlist->Size, attlist->Count);

        if ((attlist->dwflags & SIH_PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY) != 0)
        {
            Log("\tHas Desktop_App_Policy.");
            for (ULONG inx = 0; inx < attlist->Count; inx++)
            {
                SIH_PROC_THREAD_ATTRIBUTE_ENTRY Entry = attlist->Entry[inx];
                Log("\t\tIndex %d Attribute 0x%x Size=0x%x", inx, Entry.Attribute, Entry.cbSize);
                Loghexdump(Entry.lpvalue, (long)Entry.cbSize);
                if (Entry.Attribute == PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY)
                {
                    Log("\t\tIs Attribute_Desktop_App_Policy");
                    if (Entry.cbSize == 4)
                    {
                        DWORD attval = *((DWORD*)(Entry.lpvalue));
                        if ((attval & PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_ENABLE_PROCESS_TREE) != 0)
                        {
                            Log("\t\t\tPROCESS_CREATION_DESKTOP_APP_BREAKAWAY_ENABLE_PROCESS_TREE present.");
                        }
                        if ((attval & PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_DISABLE_PROCESS_TREE) != 0)
                        {
                            Log("\t\t\tPROCESS_CREATION_DESKTOP_APP_BREAKAWAY_DISABLE_PROCESS_TREE present.");
                        }
                        if ((attval & PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_OVERRIDE) != 0)
                        {
                            Log("\t\t\tPROCESS_CREATION_DESKTOP_APP_BREAKAWAY_OVERRIDE present.");
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
