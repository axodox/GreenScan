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
using Green.Remoting;

namespace GreenRemoteModuleTest
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        RemoteControlTester RC;
        public MainWindow()
        {
            InitializeComponent();
            RemoteControlTester.Init();
            
        }

        private void Connect_Click(object sender, RoutedEventArgs e)
        {
            RC = new RemoteControlTester();
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            if (RC != null)
            {
                RC.Close();
                RC.Disconnect();
            }
            RemoteControlTester.Shutdown();
        }

        private void Close_Click(object sender, RoutedEventArgs e)
        {
            if (RC != null)
            {
                RC.Disconnect();
            }
        }

        private void Setting_Click(object sender, RoutedEventArgs e)
        {
            if (RC != null)
                RC.TestSetting();
        }


    }
}
