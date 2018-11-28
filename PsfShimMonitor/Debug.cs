//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// NOTE: PsfMonitor is a "procmon"-like display of events captured via the PSF TraceShim.

using System;
using System.Windows;
using System.Runtime.InteropServices;  // debugprivs

namespace PsfMonitor
{

    public partial class MainWindow : Window
    {
        [StructLayout(LayoutKind.Sequential)]
        public struct LUID
        {
            public int LowPart;
            public int HighPart;
        }
        [StructLayout(LayoutKind.Sequential)]
        public struct TOKEN_PRIVILEGES
        {
            public LUID Luid;
            public int Attributes;
            public int PrivilegeCount;
        }

        [DllImport("advapi32.dll", SetLastError = true)]
        static extern int OpenProcessToken(
                                        System.IntPtr ProcessHandle, // handle to process
                                        int DesiredAccess, // desired access to process
                                        ref IntPtr TokenHandle // handle to open access token
                                        );

        [DllImport("advapi32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        public extern static int LookupPrivilegeValue(string lpsystemname, string lpname, [MarshalAs(UnmanagedType.Struct)] ref LUID lpLuid);


        [DllImport("advapi32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        public extern static int AdjustTokenPrivileges(int tokenhandle, int disableprivs, [MarshalAs(UnmanagedType.Struct)] ref TOKEN_PRIVILEGES Newstate, int bufferlength, int PreviousState, int Returnlength);


        [DllImport("kernel32", SetLastError = true)]
        static extern bool CloseHandle(IntPtr handle);


        [DllImport("kernel32.dll", ExactSpelling = true)]
        internal static extern IntPtr GetCurrentProcess();
        internal const int SE_PRIVILEGE_ENABLED = 0x00000002;
        internal const int TOKEN_QUERY = 0x00000008;
        internal const int TOKEN_ADJUST_PRIVILEGES = 0x00000020;
        internal const string SE_DEBUG_NAME = "SeDebugPrivilege";
        private void RaiseDebug()
        {
            // Raising the debug privilege allows the application to (amoung other things) receive kernel ETW messages. 
            // You already need elevation for this to work.
            IntPtr hcurrproc;
            IntPtr hToken = new IntPtr();
            LUID luidSEDebugNameValue = new LUID();
            TOKEN_PRIVILEGES tkpPrivileges = new TOKEN_PRIVILEGES();
            int retval;

            hcurrproc = GetCurrentProcess();
            if (hcurrproc != null)
            {
                retval = OpenProcessToken(hcurrproc, (int)(TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY), ref hToken);
                retval = LookupPrivilegeValue(null, SE_DEBUG_NAME, ref luidSEDebugNameValue);

                tkpPrivileges.PrivilegeCount = 1;
                tkpPrivileges.Luid = luidSEDebugNameValue;
                tkpPrivileges.Attributes = SE_PRIVILEGE_ENABLED;

                retval = AdjustTokenPrivileges((int)hToken, 0, ref tkpPrivileges, 1024, 0, 0);
                if (retval != 0)
                {
                    MessageBox.Show("Error on AdjustTokenPrivileges=0x" + Marshal.GetLastWin32Error().ToString("x"), "Debug");
                }
                CloseHandle(hToken);
            }
            else
                MessageBox.Show("no curr proc", "Debug");
        } // RaiseDebug

    }

}