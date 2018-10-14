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
        private ObservableCollection<EventItem> _FilteredEventItems = new ObservableCollection<EventItem>();
        public ObservableCollection<EventItem> FilteredEventItems
        {
            get { return _FilteredEventItems; }
        }

        void UpdateFilteredViewList()
        {
            ObservableCollection<EventItem> _TFilteredKernelEventListItems = new ObservableCollection<EventItem>();
            foreach (EventItem ei in _ModelEventItems)
            {
                if (!ei.IsHidden)
                {
                    _TFilteredKernelEventListItems.Add(ei);
                }
            }
            _FilteredEventItems = _TFilteredKernelEventListItems;
            EventsGrid.ItemsSource = FilteredEventItems;
            EventsGrid.Items.Refresh();
            Update_Captured();
        }

        private void Update_Captured()
        {
            //int visible = 0;
            //foreach (EventItem ei in _EventItems)
            //{
            //    if (!ei.IsHidden)
            //        visible++;
            //}
            Captured.Text = _FilteredEventItems.Count.ToString() + " of " + _ModelEventItems.Count.ToString() + " Events";
            Other.Text = "Kernel KCBs=" + _KCBs.Count.ToString();
        }

    }
}