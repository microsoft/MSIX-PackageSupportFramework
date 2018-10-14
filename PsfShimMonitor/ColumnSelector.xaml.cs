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
using System.Windows.Shapes;

namespace TraceShimMonitor
{
    /// <summary>
    /// Interaction logic for ColumnSelector.xaml
    /// </summary>
    public partial class ColumnSelector : Window
    {
        private DataGrid _SourceDG;
        private List<CheckBox> _CbList = new List<CheckBox>();
        
        public ColumnSelector(DataGrid sdg)
        {
            InitializeComponent();

            if (sdg != null)
            {
                _SourceDG = sdg;
                foreach (DataGridColumn dgc in _SourceDG.Columns)
                {
                    CheckBox cb = new CheckBox();
                    cb.Content = dgc.Header;
                    cb.IsThreeState = false;
                    cb.IsChecked = (dgc.Visibility == Visibility.Visible);
                    cb.Checked += Cb_Event_Checked;
                    cb.Unchecked += Cb_Event_UnChecked;
                    cb.FontSize = 12;
                    cb.Height = 20;
                    _CbList.Add(cb);
                }
                ItemsList.ItemsSource = _CbList;
                //ItemsList.UpdateLayout();
                //this.Height = 35 + _CbList.Count * 25;  // temp
                this.Loaded += WhenLoaded;
            }
        }
        
        private void WhenLoaded(object sender, RoutedEventArgs e)
        {
            this.Height = 35 + ItemsList.ActualHeight;           
        }
        private void Cb_Event_Checked(object sender, RoutedEventArgs e)
        {
            try
            {
                CheckBox cb = sender as CheckBox;
                foreach (DataGridColumn dgc in _SourceDG.Columns)
                {
                    if (dgc.Header as string == cb.Content as string)
                    {
                        dgc.Visibility = Visibility.Visible;
                    }
                }
            }
            catch
            {
                ;
            }
        }
        private void Cb_Event_UnChecked(object sender, RoutedEventArgs e)
        {
            try
            {
                CheckBox cb = sender as CheckBox;
                foreach (DataGridColumn dgc in _SourceDG.Columns)
                {
                    if (dgc.Header as string == cb.Content as string)
                    {
                        dgc.Visibility = Visibility.Collapsed;
                    }
                }
            }
            catch
            {
                ;
            }
        }
    }
}
