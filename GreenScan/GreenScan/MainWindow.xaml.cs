using System.Windows;
using System.Windows.Interop;
using Green.Graphics;
using System.Threading;
using Green.Kinect;
namespace Green.Scan
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        ScanSettings SS;
        KinectManager KM;
        
        public MainWindow()
        {
            InitializeComponent();
            KM = new KinectManager();
            SS = new ScanSettings();
            

            InitGUI();
            InitSettings();
        }

        void InitGUI()
        {
            KinectManagerToDeviceListConverter.ModeSetting = SS.KinectMode;

            DataContext = KM;
            MIMode.DataContext = SS.KinectMode;

        }

        void InitSettings()
        {
            SS.KinectMode.ValueChanged += KinectMode_ValueChanged;
            SS.ViewProperties.ValueChanged += ViewProperties_ValueChanged;
        }

        void KinectMode_ValueChanged(object sender, System.EventArgs e)
        {
            if (KM.Processing)
            {
                KM.StartKinect(SS.KinectMode.Value);
            }
        }

        void ViewProperties_ValueChanged(object sender, System.EventArgs e)
        {
            DXC.SetView(
                SS.TranslationX.Value,
                SS.TranslationY.Value,
                SS.TranslationZ.Value,
                SS.RotationX.Value,
                SS.RotationY.Value,
                SS.RotationZ.Value);
        }
        
        void Settings_Click(object sender, RoutedEventArgs e)
        {
            SettingsWindow SW = new SettingsWindow();
            SW.DataContext = SS;
            SW.Show();
        }
    }
}
