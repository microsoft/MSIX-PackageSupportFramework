//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// NOTE: PsfMonitor is a "procmon"-like display of events captured via the PSF TraceShim.


using System.Windows;
using System.Collections.ObjectModel;  // ObservableCollection

namespace PsfMonitor
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
            Captured.Text = _FilteredEventItems.Count.ToString() + " of " + _ModelEventItems.Count.ToString() + " Events";
            Other.Text = "Kernel KCBs=" + _KCBs.Count.ToString();
        }

    }
}