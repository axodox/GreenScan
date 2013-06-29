using System;
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
            DXC.Loaded += DXC_Loaded;


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
            SS.ViewProperties.ValueChanged += (object sender, EventArgs e) => { SetView(); };
            SS.CameraProperties.ValueChanged += (object sender, EventArgs e) => { SetCameras(); };
            SS.ShadingProperties.ValueChanged += (object sender, EventArgs e) => { SetShading(); };
        }

        void KinectMode_ValueChanged(object sender, System.EventArgs e)
        {
            if (KM.Processing)
            {
                KM.StartKinect(SS.KinectMode.Value);
            }
        }

        void DXC_Loaded(object sender, RoutedEventArgs e)
        {
            SetView();
            SetCameras();
            SetShading();
        }

        void SetView()
        {
            DXC.SetView(
                SS.TranslationX.Value,
                SS.TranslationY.Value,
                SS.TranslationZ.Value,
                SS.RotationX.Value,
                SS.RotationY.Value,
                SS.RotationZ.Value,
                SS.Scale.Value,
                SS.MoveX.Value,
                SS.MoveY.Value,
                SS.Rotation.Value);
        }

        void SetCameras()
        {
            DXC.SetCameras(SS.InfraredCameraMatrix.Value, SS.DepthToIRMapping.Value);
        }

        void SetShading()
        {
            DXC.SetShading(SS.DepthLimit.Value);
        }
        
        void Settings_Click(object sender, RoutedEventArgs e)
        {
            SettingsWindow SW = new SettingsWindow();
            SW.DataContext = SS;
            SW.Show();
        }
    }
}
