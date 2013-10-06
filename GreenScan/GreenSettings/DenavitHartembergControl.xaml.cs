using System;
using System.Collections.ObjectModel;
using System.Linq;
using System.ComponentModel;
using System.Windows;
using System.Windows.Controls;
using Green.Settings;
using System.Globalization;
using System.Windows.Input;

namespace Green.Settings.UI
{
    public partial class DenavitHartembergControl : UserControl
    {
        public DenavitHartembergControl()
        {
            InitializeComponent();
        }

        private void AddTransformation_Click(object sender, RoutedEventArgs e)
        {
            DenavitHartenbergSetting system = DataContext as DenavitHartenbergSetting;
            if (system == null) return;
            system.Items.Add(new DenavitHartenbergSetting.Joint());
        }

        private void RemoveTransformation_Click(object sender, RoutedEventArgs e)
        {
            DenavitHartenbergSetting system = DataContext as DenavitHartenbergSetting;
            if (system == null) return;
            system.Items.Remove((sender as Button).Tag as DenavitHartenbergSetting.Joint);
        }

        private void UserControl_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            if (DataContext is Setting) (DataContext as Setting).ResetValue();
        }
    }
}
