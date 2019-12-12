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

using System.Diagnostics.Eventing.Reader; // "regular" events

namespace PsfMonitor
{

    public partial class MainWindow : Window
    {

        #region DATA_SYSTEM
        public string ProviderName_SYSTEM = "System";
        private BackgroundWorker TraceBgWorker_SYSTEM = null;
        private bool WaitingForEventStart_SYSTEM = true;
        #endregion

        #region BACKGROUNDWORKER_SYSTEM
        private void ETWTraceInBackground_Start_SYSTEM(string etwclass)
        {
            //Check current BackgroundWorker status.
            if (TraceBgWorker_SYSTEM == null)
                TraceBgWorker_SYSTEM = new BackgroundWorker();
            else if (TraceBgWorker_SYSTEM.IsBusy)
                return;
            else
                TraceBgWorker_SYSTEM = new BackgroundWorker();


            // Do processing in the background
            TraceBgWorker_SYSTEM.WorkerSupportsCancellation = true;
            TraceBgWorker_SYSTEM.WorkerReportsProgress = true;
            TraceBgWorker_SYSTEM.DoWork += ETWTraceInBackground_DoWork_SYSTEM;
            TraceBgWorker_SYSTEM.ProgressChanged += ETWTraceInBackground_ProgressChanged_SYSTEM;
            TraceBgWorker_SYSTEM.RunWorkerCompleted += ETWTraceInBackground_RunWorkerCompleted_SYSTEM;

            TraceBgWorker_SYSTEM.RunWorkerAsync(etwclass);
        }  //ETWTraceInBackground_Start_SYSTEM()
        private void ETWTraceInBackground_DoWork_SYSTEM(object sender, DoWorkEventArgs e)
        {
            // This is the background thread
            int count = 0;
            string etwclass = e.Argument as string;
            BackgroundWorker worker = sender as BackgroundWorker;
            Thread.CurrentThread.Name = "ETWReaderSYSTEM";
            //Thread.CurrentThread.Priority = ThreadPriority.BelowNormal;

            try
            {
                string sQuery = "*[System/Level>0]";
                EventLogQuery Q_Operational = new EventLogQuery(etwclass, PathType.LogName, sQuery);
                EventBookmark Ev_OperationalBookmark = null;
                EventLogReader R_Operational;
                R_Operational = new EventLogReader(Q_Operational); // Walk through existing list to create a bookmark
                R_Operational.Seek(System.IO.SeekOrigin.End, 0);
                for (EventRecord eventInstance = R_Operational.ReadEvent();
                        null != eventInstance;
                        eventInstance = R_Operational.ReadEvent())
                {
                    Ev_OperationalBookmark = eventInstance.Bookmark;
                }
                R_Operational.Dispose();
                WaitingForEventStart_SYSTEM = false;

                worker.ReportProgress(count++);

                while (!worker.CancellationPending && !PleaseStopCollecting)
                {
                    Thread.Sleep(1000);
                    R_Operational = new EventLogReader(Q_Operational, Ev_OperationalBookmark);
                    for (EventRecord eventInstance = R_Operational.ReadEvent();
                            null != eventInstance;
                            eventInstance = R_Operational.ReadEvent())
                    {
                        Ev_OperationalBookmark = eventInstance.Bookmark;
                        try
                        {
                            DateTime et = eventInstance.TimeCreated.GetValueOrDefault();
                            EventItem eItem = new EventItem((int)et.Ticks, et.Ticks, et.Ticks, et, eventInstance.ProcessId.ToString(), (int)eventInstance.ProcessId, (int)eventInstance.ThreadId,
                                                            eventInstance.LogName, "System", eventInstance.Id.ToString(), eventInstance.LevelDisplayName, eventInstance.FormatDescription(),"");
                            //SYSTEM_ITEM item = new SYSTEM_ITEM(eventInstance.LevelDisplayName, eventInstance.FormatDescription(), eventInstance.ProviderName, eventInstance.Id, eventInstance.ProcessId);
                            worker.ReportProgress(count++, eItem);
                        }
                        catch
                        {
                            // app provider might be virtual or missing
                            string leveldisplayname = "";
                            string stuff = "Formatter not available. Details:";
                            int ProcessId = -1;
                            int ThreadId = -1;
                            switch (eventInstance.Level)
                            {
                                case 1:
                                    leveldisplayname = "Critical";
                                    break;
                                case 2:
                                    leveldisplayname = "Error";
                                    break;
                                case 3:
                                    leveldisplayname = "Warning";
                                    break;
                                case 4:
                                    leveldisplayname = "Information";
                                    break;
                                default:
                                    break;
                            }
                            foreach (EventProperty p in eventInstance.Properties)
                            {
                                stuff += p.Value.ToString() + "  ";
                            }
                            if (eventInstance.ProcessId != null)
                                ProcessId = (int)eventInstance.ProcessId;
                            if (eventInstance.ThreadId != null)
                                ThreadId = (int)eventInstance.ThreadId;
                            DateTime et = eventInstance.TimeCreated.GetValueOrDefault();
                            EventItem eItem = new EventItem((int)et.Ticks, et.Ticks, et.Ticks, et, eventInstance.ProcessId.ToString(), (int)eventInstance.ProcessId, (int)eventInstance.ThreadId,
                                                            eventInstance.LogName, "System", eventInstance.Id.ToString(), leveldisplayname, stuff, "");
                            worker.ReportProgress(count++, eItem);
                        }
                    }
                    R_Operational.Dispose();
                }
            }
            catch
            {
                WaitingForEventStart_SYSTEM = false;
            }
        } // ETWTraceInBackground_DoWork_SYSTEM()
        private void ETWTraceInBackground_ProgressChanged_SYSTEM(object sender, ProgressChangedEventArgs e)
        {
            WaitingForEventStart_SYSTEM = false;
            if (e.UserState != null)
            {
                EventItem eItem = (EventItem)e.UserState;
                //Event_System_List.Add((SYSTEM_ITEM)e.UserState);
                AppplyFilterToEventItem(eItem);
                if (IsPaused)
                {
                    eItem.IsPauseHidden = true;
                }
                _ModelEventItems.Add(eItem);
            }
        } // ETWTraceInBackground_ProgressChanged_SYSTEM()
        private void ETWTraceInBackground_RunWorkerCompleted_SYSTEM(object sender, RunWorkerCompletedEventArgs e)
        {
            // nothing to clean up
        }

        #endregion

    }

}