//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// NOTE: PsfMonitor is a "procmon"-like display of events captured via the PSF TraceShim.

using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using Microsoft.Diagnostics.Tracing;  // consumer
using Microsoft.Diagnostics.Tracing.Session; // controller
using System.ComponentModel;  // backgroundworker
using System.Threading;


namespace PsfMonitor
{
    public class Provider
    {
        public string name {  get; set;}
        public Guid guid { get; set; }
        public Provider(string Name, Guid Guid)
        {
            name = Name;
            guid = Guid;
        }
    }

 

    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {

        // Temp list


        private bool IsPaused = false;
        private BackgroundWorker eventbgw = null;
        private BackgroundWorker kerneleventbgw = null;
        public TraceEventSession myTraceEventSession = null;
        // NOTE: The provider name and GUID must be kept in sync with PsfTrace/main.cpp
        //       The format of the provider name uses dashes here and dots in C++.
        //public Provider etwprovider = new Provider("Microsoft-Windows-PSFTrace",  new Guid(0x61F777A1, 0x1E59, 0x4BFC, 0xA6, 0x1A, 0xEF, 0x19, 0xC7, 0x16, 0xDD, 0xC0));
        public Provider etwprovider = new Provider("Microsoft-Windows-PSFTraceFixup", new Guid(0x7c1d7c1c, 0x544b, 0x5179, 0x28, 0x01, 0x39, 0x29, 0xf0, 0x01, 0x78, 0x02));
        public int EventCounter = 1;
        public bool EventTraceProviderEnablementResultCode, EventTraceProviderSourceResultCode;
        public int LastSearchIndex = -1;
        public string LastSearchString = "";
        public List<int> ProcIDsOfTarget = new List<int>();

        public const string ProgramTitle = "PSFMonitor";
        public const string UnexpectedErrorPrompt = "An unexpected error had occurred. Click Cancel to close program";

        public MainWindow()
        {
            InitializeComponent();

            // This is done to enable ETW Kernel Debugging
            EventsGrid.ItemsSource = FilteredEventItems;

            ETWTraceInBackground_Start(etwprovider);
            KernelTraceInBackground_Start();
            ETWTraceInBackground_Start_APPS(ProviderName_APPS);
            ETWTraceInBackground_Start_SYSTEM(ProviderName_SYSTEM);

            Status.Text = "Listening";
            Update_Captured();
            Closing += Dispose;
        }
        
        public void Dispose(object sender, CancelEventArgs e)
        {           
            PleaseStopCollecting = true;
            try
            {
                if (eventbgw != null)
                {
                    eventbgw.CancelAsync();
                    eventbgw = null;
                }
            }
            catch
            {
                // this is code attempting to clean up.  It should be allowed to continue to do so or a second launch
                // may have issues.
            }
            try
            {
                if (myTraceEventSession != null)
                {
                    myTraceEventSession.Source.StopProcessing();
                    myTraceEventSession.Source.Dispose();
                    myTraceEventSession.Dispose();
                    myTraceEventSession = null;
                }
            }
            catch
            {
                // this is code attempting to clean up.  It should be allowed to continue to do so or a second launch
                // may have issues.
            }
            DisableKernelTrace();
        }

        private void ETWTraceInBackground_Start(Provider etwp)
        {
            if (eventbgw == null)
            {
                eventbgw = new BackgroundWorker();
            }
            else if (eventbgw.IsBusy)
            {
                return;
            }
            else
            {
                eventbgw = new BackgroundWorker();
            }
            
            // Do processing in the background
            eventbgw.WorkerSupportsCancellation = true;
            eventbgw.WorkerReportsProgress = true;
            eventbgw.DoWork += Eventbgw_DoWork;
            eventbgw.ProgressChanged += Eventbgw_ProgressChanged;
            eventbgw.RunWorkerCompleted += Eventbgw_RunWorkerCompleted;

            eventbgw.RunWorkerAsync(etwp);
        } // ETWTraceInBackground_Start()

        private void Eventbgw_DoWork(object sender, DoWorkEventArgs e)
        {
            // This is the background thread
            Provider etwp = e.Argument as Provider;
            BackgroundWorker worker = sender as BackgroundWorker;
            Thread.CurrentThread.Name = "ETWReader";

            using (myTraceEventSession = new TraceEventSession(etwp.name, TraceEventSessionOptions.Create))
            {
                myTraceEventSession.StopOnDispose = true;
                myTraceEventSession.Source.Dynamic.All += delegate (TraceEvent data)           // Set Source (stream of events) from session.  
                {                                                                    // Get dynamic parser (knows about EventSources) 
                                                                                     // Subscribe to all EventSource events
                    string operation = "";
                    string inputs = "";
                    string result = "";
                    string outputs = "";
                    string caller = "";
                    Int64 start = 0;
                    Int64 end = 0;

                    try
                    {
                        operation = (string)data.PayloadByName("Operation");
                    }
                    catch
                    {
                        // expected possible condition
                    }
                    try
                    {
                        inputs = (string)data.PayloadByName("Inputs");
                    }
                    catch
                    {
                        // expected possible condition
                    }
                    try
                    {
                        result = (string)data.PayloadByName("Result");
                    }
                    catch
                    {
                        // expected possible condition
                    }
                    try
                    {
                        outputs = (string)data.PayloadByName("Outputs");
                    }
                    catch
                    {
                        // expected possible condition
                    }
                    try
                    {
                        caller = (string)data.PayloadByName("Caller");
                    }
                    catch
                    {
                        // expected possible condition
                    }
                    if (inputs == null && result == null && outputs == null)
                    {
                        try
                        {
                            outputs = (string)data.PayloadByName("Message");
                        }
                        catch
                        {
                            // expected possible condition
                        }
                    }
                    try
                    {
                        start = (Int64)data.PayloadByName("Start");
                    }
                    catch
                    {
                        // expected possible condition
                    }
                    try
                    {
                        end = (Int64)data.PayloadByName("End");
                    }
                    catch
                    {
                        // expected possible condition
                    }
                    EventItem ei = new EventItem((int)data.EventIndex,
                                                    start,
                                                    end,
                                                    data.TimeStamp,
                                                    data.ProcessName,  
                                                    data.ProcessID,
                                                    data.ThreadID,
                                                    data.ProviderName,
                                                    operation,
                                                    inputs,
                                                    result,
                                                    outputs,
                                                    caller
                                                  );

                    lock (_TEventListsLock)
                    {
                        _TEventListItems.Add(ei);
                        AddToProcIDsList(data.ProcessID);
                    }
                    worker.ReportProgress((int)data.EventIndex);
                };

                EventTraceProviderEnablementResultCode = myTraceEventSession.EnableProvider(etwp.guid);    
                if (!EventTraceProviderEnablementResultCode)
                {
                    // Attempt resetting for second run...
                    myTraceEventSession.DisableProvider(etwp.guid);
                    EventTraceProviderEnablementResultCode = myTraceEventSession.EnableProvider(etwp.guid);
                }
                EventTraceProviderSourceResultCode = myTraceEventSession.Source.Process();
            }
        } // Eventbgw_DoWork()
        private void AddToProcIDsList(int pid)
        {
            foreach (int match in ProcIDsOfTarget)
            {
                if (match == pid)
                {
                    return;
                }
            }
            ProcIDsOfTarget.Add(pid);
        }
        private bool IsPidInProdIDsList(int pid)
        {
            foreach (int match in ProcIDsOfTarget)
            {
                if (match == pid)
                {
                    return true;
                }
            }
            return false;
        }
        private void Eventbgw_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            try
            {
                lock (_TEventListsLock)
                {
                    if (_TEventListItems.Count > 0)
                    {
                        foreach (EventItem ei in _TEventListItems)
                        {
                            if (FilterOnProcessId < 0)
                            {
                                FilterOnProcessId = ei.ProcessID;
                            }
                            AppplyFilterToEventItem(ei);
                            if (IsPaused)
                            {
                                ei.IsPauseHidden = true;
                            }
                            _ModelEventItems.Add(ei);
                        }
                        _TEventListItems.Clear();

                        UpdateFilteredViewList();
                    }
                }
            }
            catch
            {
                ;
            }
        } // Eventbgw_ProgressChanged()

        private void Eventbgw_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            Status.Text = "NoTrace";
            Other.Text = EventTraceProviderEnablementResultCode.ToString() + "  " + EventTraceProviderSourceResultCode.ToString();
        } // Eventbgw_RunWorkerCompleted()

        #region UIFilter
        private void AppplyFilterToEventItem(EventItem ei)
        {
            ApplyFilterResultToEventItem(ei);
            ApplyFilterCategoryEventToEventItem(ei);
            ApplyFilterProcessIdToEventItem(ei);
        }
        private bool ApplyFilterResultToEventItem(EventItem ei)
        {
            bool washidden = ei.IsHidden;
            if (ei.Result.StartsWith("Success"))
            {
                if ((bool)cbSuccesss.IsChecked)
                {
                    ei.IsResultHidden = false;
                }
                else
                {
                    ei.IsResultHidden = true;
                }
                ei.EventIsResultClass = "Normal";
            }
            else if (ei.Result.StartsWith("Unknown") ||
                ei.Result.StartsWith("Indeterminate"))
            {
                if ((bool)cbIntermediate.IsChecked)
                {
                    ei.IsResultHidden = false;
                }
                else
                {
                    ei.IsResultHidden = true;
                }
                ei.EventIsResultClass = "Warning";
            }
            else if (ei.Result.StartsWith("Expected Failure"))
            {
                if ((bool)cbExpectedFailure.IsChecked)
                {
                    ei.IsResultHidden = false;
                }
                else
                {
                    ei.IsResultHidden = true;
                }
                ei.EventIsResultClass = "Warning";
            }
            else if (ei.Result.StartsWith("Failure"))
            {
                if ((bool)cbFailure.IsChecked)
                {
                    ei.IsResultHidden = false;
                }
                else
                {
                    ei.IsResultHidden = true;
                }
                ei.EventIsResultClass = "Failure";
            }
            else
            {
                ei.EventIsResultClass = "Normal"; // TODO: Reults starting with other stuff
            }

            if (ei.IsHidden != washidden)
            {
                return true;
            }
            else
            {
                return false;
            }
        }  
        private bool ApplyFilterCategoryEventToEventItem(EventItem ei)
        {
            bool washidden = ei.IsHidden;
            switch (ei.Event)
            {
                case "CreateProcess":
                case "CreateProcessAsUser":
                    if ((bool)cbCatProcess.IsChecked)
                    {
                        ei.IsEventCatHidden = false;
                    }
                    else
                    {
                        ei.IsEventCatHidden = true;
                    }
                    break;

                case "CreateFile":
                case "CreateFile2":
                case "CopyFile":
                case "CopyFile2":
                case "CopyFileEx":
                case "CreateHardLink":
                case "CreateSymbolicLink":
                case "DeleteFile":
                case "MoveFile":
                case "MoveFileEx":
                case "ReplaceFile":
                case "FindFirstFile":
                case "FindFirstFileEx":
                case "FindNextFile":
                case "FindClose":
                case "CreateDirectory":
                case "CreateDirectoryEx":
                case "RemoveDirectory":
                case "SetCurrentDirectory":
                case "GetCurrentDirectory":
                case "GetFileAttributes":
                case "SetFileAttributes":
                case "GetFileAttributesEx":
                    if ((bool)cbCatFile.IsChecked)
                    {
                        ei.IsEventCatHidden = false;
                    }
                    else
                    {
                        ei.IsEventCatHidden = true;
                    }
                    break;

                case "RegCreateKey":
                case "RegCreateKeyEx":
                case "RegOpenKey":
                case "RegOpenKeyEx":
                case "RegGetValue":
                case "RegQueryValue":
                case "RegQueryValueEx":
                case "RegSetKeyValue":
                case "RegSetValue":
                case "RegSetValueEx":
                case "RegDeleteKey":
                case "RegDeleteKeyEx":
                case "RegDeleteKeyValue":
                case "RegDeleteValue":
                case "RegDeleteTree":
                case "RegCopyTree":
                case "RegEnumKey":
                case "RegEnumKeyEx":
                case "RegEnumValue":
                    if ((bool)cbCatReg.IsChecked)
                    {
                        ei.IsEventCatHidden = false;
                    }
                    else
                    {
                        ei.IsEventCatHidden = true;
                    }
                    break;

                // NT Level
                case "NtCreateFile":
                case "NtOpenFile":
                case "NtCreateDirectoryObject":
                case "NtOpenDirectoryObject":
                case "NtQueryDirectoryObject":
                case "NtOpenSymbolicLinkObject":
                case "NtQuerySymbolicLinkObject":
                    if ((bool)cbCatNTFile.IsChecked)
                    {
                        ei.IsEventCatHidden = false;
                    }
                    else
                    {
                        ei.IsEventCatHidden = true;
                    }
                    break;

                case "NtCreateKey":
                case "NtOpenKey":
                case "NtOpenKeyEx":
                case "NtSetValueKey":
                case "NtQueryValueKey":
                    if ((bool)cbCatNTReg.IsChecked)
                    {
                        ei.IsEventCatHidden = false;
                    }
                    else
                    {
                        ei.IsEventCatHidden = true;
                    }
                    break;

                case "AddDllDirectory":
                case "LoadLibrary":
                case "LoadLibraryEx":
                case "LoadModule":
                case "LoadPackagedLibrary":
                case "RemoveDllDirectory":
                case "SetDefaultDllDirectories":
                case "SetDllDirectory":
                    if ((bool)cbCatDll.IsChecked)
                    {
                        ei.IsEventCatHidden = false;
                    }
                    else
                    {
                        ei.IsEventCatHidden = true;
                    }
                    break;

                // Kernel traces
                case "Process/Start":
                case "Process/Stop":
                    if ((bool)cbCatKernelProcess.IsChecked)
                    {
                        ei.IsEventCatHidden = false;
                    }
                    else
                    {
                        ei.IsEventCatHidden = true;
                    }
                    break;
                case "Image/Load":
                    if ((bool)cbCatKernelImageLoad.IsChecked)
                    {
                        ei.IsEventCatHidden = false;
                    }
                    else
                    {
                        ei.IsEventCatHidden = true;
                    }
                    break;

                case "FileIO/Query":
                case "FileIO/QueryInfo":
                case "FileIO/Create":
                case "FileIO/FileCreate":
                case "FileIO/Read":
                case "FileIO/Write":
                case "FileIO/Close":
                case "FileIO/Cleanup":
                case "FileIO/OperationEnd":
                case "FileIO/DirEnum":
                case "FileIO/SetInfo":
                case "FileIO/Rename":
                case "FileIO/Delete":
                case "FileIO/FileDelete":
                case "FileIO/Flush":
                    if ((bool)cbCatKernelFile.IsChecked)
                    {
                        ei.IsEventCatHidden = false;
                    }
                    else
                    {
                        ei.IsEventCatHidden = true;
                    }
                    break;

                case "DiskIO/Read":
                case "DiskIO/Write":
                    if ((bool)cbCatKernelDisk.IsChecked)
                    {
                        ei.IsEventCatHidden = false;
                    }
                    else
                    {
                        ei.IsEventCatHidden = true;
                    }
                    break;

                case "Registry/Open":
                case "Registry/Query":
                case "Registry/QueryValue":
                case "Registry/SetInformation":
                case "Registry/Close":
                case "Registry/Create":
                case "Registry/SetValue":
                case "Registry/EnumerateKey":
                case "Registry/Delete":
                case "Registry/DeleteValue":
                case "Registry/EnumerateValueKey":
                    if ((bool)cbCatKernelRegistry.IsChecked)
                    {
                        ei.IsEventCatHidden = false;
                    }
                    else
                    {
                        ei.IsEventCatHidden = true;
                    }
                    break;

                case "Application":
                    if ((bool)cbCatApplicationLog.IsChecked)
                    {
                        ei.IsEventCatHidden = false;
                    }
                    else
                    {
                        ei.IsEventCatHidden = true;
                    }
                    break;
                case "System":
                    if ((bool)cbCatSystemLog.IsChecked)
                    {
                        ei.IsEventCatHidden = false;
                    }
                    else
                    {
                        ei.IsEventCatHidden = true;
                    }
                    break;
                default:
                    if ((bool)cbCatOther.IsChecked)
                    {
                        ei.IsEventCatHidden = false;
                    }
                    else
                    {
                        ei.IsEventCatHidden = true;
                    }
                    break;
            }
            if (ei.IsHidden != washidden)
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        private void ApplyFilterProcessIdToEventItem(EventItem ei)
        {
            if (FilterOnProcessId > 0)
            {
                if (ei.ProcessID != FilterOnProcessId)
                {
                    ei.IsProcessIDHidden = true;
                }
            }
        }

        private void Cb_Result_Checked_or_Unchecked(object sender, RoutedEventArgs e)
        {
            try
            {
                if (_ModelEventItems != null)
                {
                    bool anychanged = false;
                    foreach (EventItem ei in _ModelEventItems)
                    {
                        if (ApplyFilterResultToEventItem(ei))
                        {
                            anychanged = true;
                        }
                    }
                    if (anychanged)
                    {
                        UpdateFilteredViewList();
                    }
                }
            }
            catch (Exception ex)
            {
                if (MessageBox.Show(UnexpectedErrorPrompt, ProgramTitle, MessageBoxButton.OKCancel, MessageBoxImage.Error) == MessageBoxResult.Cancel)
                {
                    throw ex;
                }
            }
        }
        private void Cb_Event_Checked_or_Unchecked(object sender, RoutedEventArgs e)
        {
            try
            {
                if (_ModelEventItems != null)
                {
                    bool anychanged = false;
                    foreach (EventItem ei in _ModelEventItems)
                    {
                        if (ApplyFilterCategoryEventToEventItem(ei))
                        {
                            anychanged = true;
                        }
                    }
                    if (anychanged)
                    {
                        UpdateFilteredViewList();
                    }
                }
            }
            catch (Exception ex)
            {
                if (MessageBox.Show(UnexpectedErrorPrompt, ProgramTitle, MessageBoxButton.OKCancel, MessageBoxImage.Error) == MessageBoxResult.Cancel)
                {
                    throw ex;
                }
            }
        }


        #endregion

        #region UI_ACTIONS
        private void bClearList_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                _ModelEventItems.Clear();
                _FilteredEventItems.Clear();
                EventsGrid.Items.Refresh();
                Update_Captured();
                LastSearchIndex = -1;
                LastSearchString = "";
            }
            catch (Exception ex)
            {
                if (MessageBox.Show(UnexpectedErrorPrompt, ProgramTitle, MessageBoxButton.OKCancel, MessageBoxImage.Error) == MessageBoxResult.Cancel)
                {
                    throw ex;
                }
            }
        }


        private void SearchStringBut_Click(object sender, RoutedEventArgs e)
        {
            SearchForString();
        }
        private void SearchStringTB_KeyUp(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                SearchForString();
            }
        }
        private void SearchForString()
        { 
            int totalfound = 0;
            int onfound = 0;
            try
            {
                string search = SearchStringTB.Text.ToUpper();
                bool newsearch = !search.Equals(LastSearchString);

                int nextfound = -1;
                if (!newsearch && LastSearchIndex >= 0)
                {
                    nextfound = LastSearchIndex;
                }

                int index = 0;
                foreach (EventItem ei in _FilteredEventItems)
                {
                    ei.IsCurrentSearchItem = false;
                    if (search.Length > 0)
                    {
                        if (ei.Event != null)
                        {
                            if (ei.ProcessID.ToString().Contains(search))
                            {
                                ei.IsHighlighted = true;
                            }
                            else if (ei.ProcessName.ToUpper().Contains(search))
                            {
                                ei.IsHighlighted = true;
                            }
                            else if (ei.Event.ToUpper().Contains(search))
                            {
                                ei.IsHighlighted = true;
                            }
                            else if (ei.Inputs.ToUpper().Contains(search))
                            {
                                ei.IsHighlighted = true;
                            }
                            else if (ei.Result.ToUpper().Contains(search))
                            {
                                ei.IsHighlighted = true;
                            }
                            else if (ei.Outputs.ToUpper().Contains(search))
                            {
                                ei.IsHighlighted = true;
                            }
                            else if (ei.Caller.ToUpper().Contains(search))
                            {
                                ei.IsHighlighted = true;
                            }
                            else
                            {
                                ei.IsHighlighted = false;
                            }
                        }
                        else
                        {
                            ei.IsHighlighted = false;
                        }
                    }
                    else
                    {
                        ei.IsHighlighted = false;
                    }
                    if (ei.IsHighlighted)
                    {
                        totalfound++;
                        if (!ei.IsHidden)
                        {
                            if (newsearch && nextfound == -1)
                            {
                                nextfound = index;
                                ei.IsCurrentSearchItem = true;
                                onfound = totalfound;
                            }
                            else if (!newsearch && nextfound <= LastSearchIndex)
                            {
                                nextfound = index;
                                ei.IsCurrentSearchItem = true;
                                onfound = totalfound;
                            }
                        }
                    }
                    index++;
                }
                EventsGrid.Items.Refresh();
                if (nextfound >= 0)
                {
                    EventsGrid.ScrollIntoView(_FilteredEventItems[nextfound]);
                }

                if (totalfound > 0)
                {
                    SearchFoundCount.Text = onfound.ToString() + " of " + totalfound.ToString();
                }
                else
                {
                    SearchFoundCount.Text = "0 found";
                }

                LastSearchIndex = nextfound;
                LastSearchString = search;
            }
            catch (Exception ex)
            {
                if (MessageBox.Show(UnexpectedErrorPrompt, ProgramTitle, MessageBoxButton.OKCancel, MessageBoxImage.Error) == MessageBoxResult.Cancel)
                {
                    throw ex;
                }
            }
        }

        private void DataGridColumnHeader_MouseRightButtonDown(object sender, MouseButtonEventArgs e)
        {
            try
            { 
                // Popup dialog with "hide" and "add/remove columns" entries.
                //       Hide just hides the selected column.
                //       add/remove uses Popup window with list of column names and checkbox with visibility.  Apply and Cancel buttons               
                ColumnSelector popup = new ColumnSelector(EventsGrid);
                popup.ShowDialog();
            }
            catch (Exception ex)
            {
                if (MessageBox.Show(UnexpectedErrorPrompt, ProgramTitle, MessageBoxButton.OKCancel, MessageBoxImage.Error) == MessageBoxResult.Cancel)
                {
                    throw ex;
                }
            }
        }

        private void bEvents_Click(object sender, RoutedEventArgs e)
        {
            Button but = sender as Button;
            ContextMenu contextMenu = but.ContextMenu;
            contextMenu.PlacementTarget = but;
            if (!contextMenu.IsOpen)
            {
                contextMenu.IsOpen = true;
            }
            else
            {
                contextMenu.IsOpen = false;
            }
        }
        private void bResults_Click(object sender, RoutedEventArgs e)
        {
            Button but = sender as Button;
            ContextMenu contextMenu = but.ContextMenu;
            contextMenu.PlacementTarget = but;
            if (!contextMenu.IsOpen)
            {
                contextMenu.IsOpen = true;
            }
            else
            {
                contextMenu.IsOpen = false;
            }
        }



        private void bPauseContinue_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (IsPaused)
                {
                    bPauseContinue.Content = "Pause";
                    IsPaused = false;
                    if (_ModelEventItems != null &&
                        EventsGrid != null &&
                        EventsGrid.Items != null)
                    {
                        foreach (EventItem ei in _ModelEventItems)
                        {
                            ei.IsPauseHidden = false;
                        }
                        UpdateFilteredViewList();
                    }
                }
                else
                {
                    bPauseContinue.Content = "Continue";
                    IsPaused = true;
                }
            }
            catch (Exception ex)
            {
                if (MessageBox.Show(UnexpectedErrorPrompt, ProgramTitle, MessageBoxButton.OKCancel, MessageBoxImage.Error) == MessageBoxResult.Cancel)
                {
                    throw ex;
                }
            }
        }
        #endregion
    }
}
