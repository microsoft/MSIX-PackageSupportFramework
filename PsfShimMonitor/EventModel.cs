//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// NOTE: PsfMonitor is a "procmon"-like display of events captured via the PSF TraceShim.

using System;
using System.Collections.Generic;
using System.Windows;
using System.Collections.ObjectModel;  // ObservableCollection

namespace PsfMonitor
{
    public partial class MainWindow : Window
    {
        private ObservableCollection<EventItem> _ModelEventItems = new ObservableCollection<EventItem>();
        public ObservableCollection<EventItem> ModelEventItems
        {
            get { return _ModelEventItems; }
        }

        public List<EventItem> _TEventListItems = new List<EventItem>();
        public Object _TEventListsLock = new Object();


    }
}