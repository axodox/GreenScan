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

namespace Green.Settings.UI
{
    /// <summary>
    /// Interaction logic for SettingGroupControl.xaml
    /// </summary>
    public partial class SettingGroupControl : UserControl
    {
        public SettingGroupControl()
        {
            InitializeComponent();
        }

        public void Expand()
        {
            SP.Visibility = Visibility.Visible;
        }

        public void Collapse()
        {
            SP.Visibility = Visibility.Collapsed;
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            if (SP.Visibility == System.Windows.Visibility.Collapsed)
                SP.Visibility = System.Windows.Visibility.Visible;
            else
                SP.Visibility = System.Windows.Visibility.Collapsed;
        }
    }
}
