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
            SS.PreprocessingProperties.ValueChanged += (object sender, EventArgs e) => { SetPreprocessing(); };
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
            SetPreprocessing();
            SetView();
            SetCameras();
            SetShading();
        }

        void SetPreprocessing()
        {
            DXC.SetPreprocessing(
                SS.DepthAveraging.Value,
                SS.DepthGaussIterations.Value,
                SS.DepthGaussSigma.Value);
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
            DXC.SetCameras(
                SS.InfraredIntrinsics.Value, 
                SS.DepthToIRMapping.Value,
                SS.ColorIntrinsics.Value,
                SS.ColorRemapping.Value,
                SS.ColorExtrinsics.Value,
                SS.ColorDispositionX.Value,
                SS.ColorDispositionY.Value,
                SS.ColorScaleX.Value,
                SS.ColorScaleY.Value);
        }

        void SetShading()
        {
            DXC.SetShading(
                SS.ShadingMode.Value, 
                SS.DepthLimit.Value, 
                SS.ShadingPeriode.Value, 
                SS.ShadingPhase.Value, 
                SS.TriangleRemoveLimit.Value);
        }
        
        void Settings_Click(object sender, RoutedEventArgs e)
        {
            SettingsWindow SW = new SettingsWindow();
            SW.DataContext = SS;
            SW.Show();
        }

        private void Stop_Click(object sender, RoutedEventArgs e)
        {
            KM.StopKinect();
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            KM.CloseKinect();
        }
    }
}
