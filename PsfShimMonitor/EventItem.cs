//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies. All rights reserved.path
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// NOTE: Class to hold an event item sent from the PSF TraceShim vie ETW. This class is used for displaying data as part of a DataGrid.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace TraceShimMonitor
{
    public class EventItem
    {
        // Event Data
        private int _Index;
        private DateTime _Timestamp;
        private string _ProcessName;
        private int _ProcessID;
        private int _ThreadID;
        private string _EventSource = "";
        private string _Event = "";
        private string _Inputs = "";
        private string _Result = "";
        private string _Outputs = "";
        private string _Caller = "";
        private Int64 _Start = 0;
        private Int64 _End = 0;

        // View Data
        private bool _IsResultHidden = false;
        private bool _IsEventCatHidden = false;
        private bool _IsPauseHidden = false;
        private bool _IsProcessIDHidden = false;

        private bool _IsHighlighted = false;

        private string _EventIsResultClass = "Normal";

        // Access view for Event Data
        public string IndexAsText { get { return _Index.ToString(); } }
        public string TimestampAsText { get { return _Timestamp.ToString(); } }
        public string ProcessName { get { return _ProcessName; } }
        public int ProcessID { get { return _ProcessID; } }
        public int ThreadID { get { return _ThreadID; } }
        public string EventSource { get { return _EventSource; } }
        public string Event { get { return _Event; } }
        public string Inputs { get { return _Inputs; } }
        public string Result { get { return _Result; } }
        public string Outputs { get { return _Outputs; } }
        public string Caller { get { return _Caller; } }
        public Int64 Start {  get { return _Start; } }
        public Int64 End {  get { return _End; } }
        public Int64 Duration {  get { return _End - _Start; } }
  
        // Access view for View Data
        public bool IsResultHidden { get { return _IsResultHidden; } set { _IsResultHidden = value; } }
        public bool IsEventCatHidden { get { return _IsEventCatHidden; } set { _IsEventCatHidden = value; } }
        public bool IsPauseHidden { get { return _IsPauseHidden; } set { _IsPauseHidden = value; } }
        public bool IsProcessIDHidden { get { return _IsProcessIDHidden; } set { _IsProcessIDHidden = value; } }
        public bool IsHidden
        {
            get
            {
                if (IsResultHidden)
                    return true;
                if (IsEventCatHidden)
                    return true;
                if (_IsPauseHidden)
                    return true;
                if (_IsProcessIDHidden)
                    return true;
                return false;
            }
        }
        public bool IsHighlighted { get { return _IsHighlighted; } set { _IsHighlighted = value; } }

        public string EventIsResultClass { get { return _EventIsResultClass; } set { _EventIsResultClass = value; } }

        public EventItem(int index, Int64 start, Int64 end, DateTime timestamp, string processname, int processid, int threadid, string eventsource, string sevent, string inputs, string result, string outputs, string caller)
        {
            _Index = index;
            _Start = start;
            _End = end;

            if (timestamp != null)
                _Timestamp = timestamp;
            else
                _Timestamp = DateTime.Now;
            if (processname != null && processname.Length>0)
                _ProcessName = processname;
            else
                _ProcessName = "unknown(" + processid.ToString() + ")";
            _ProcessID = processid;
            _ThreadID = threadid;
            if (eventsource != null)    
                _EventSource = eventsource;
            if (sevent != null)
                _Event = sevent;
            if (inputs != null)
                _Inputs = inputs;
            if (result != null)
                _Result = result;
            if (outputs != null)
            _Outputs = outputs;
            if (caller != null)
                _Caller = caller;
        }
    }
}
