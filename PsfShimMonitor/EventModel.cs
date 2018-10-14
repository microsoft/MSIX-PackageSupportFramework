//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies. All rights reserved.path
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// NOTE: TraceShimMonitor is a "procmon"-like display of events captured via the PSF TraceShim.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;


using System.Collections.ObjectModel;  // ObservableCollection


namespace TraceShimMonitor
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