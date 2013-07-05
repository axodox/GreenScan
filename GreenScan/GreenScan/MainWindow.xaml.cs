using System;
using System.Windows;
using System.Windows.Interop;
using Green.Graphics;
using System.Threading;
using Green.Kinect;
using System.Windows.Input;
using System.Windows.Threading;
using Microsoft.Win32;
namespace Green.Scan
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        ScanSettings SS;
        KinectManager KM;
        DispatcherTimer DT;
        SettingsWindow SW;
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
            MIShading.DataContext = SS.ShadingMode;

            DT = new DispatcherTimer();
            DT.Interval = new TimeSpan(0, 0, 0, 0, 10);
            DT.Tick += DT_Tick;
            DT.IsEnabled = true;
        }

        void InitSettings()
        {
            SS.KinectMode.ValueChanged += KinectMode_ValueChanged;
            SS.PreprocessingProperties.ValueChanged += (object sender, EventArgs e) => { SetPreprocessing(); };
            SS.ViewProperties.ValueChanged += (object sender, EventArgs e) => { SetView(); };
            SS.CameraProperties.ValueChanged += (object sender, EventArgs e) => { SetCameras(); };
            SS.ShadingProperties.ValueChanged += (object sender, EventArgs e) => { SetShading(); };
            SS.PerformanceProperties.ValueChanged += (object sender, EventArgs e) => { SetPerformance(); };
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
            SetPerformance();
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

        void SetPerformance()
        {
            DXC.SetPerformance(
                SS.TriangleGridWidth.Value,
                SS.TriangleGridHeight.Value);
        }
        
        void Settings_Click(object sender, RoutedEventArgs e)
        {
            if (SW == null)
            {
                SW = new SettingsWindow();
                SW.DataContext = SS;
                SW.Show();
                SW.Closed += (object S, EventArgs E) => { SW = null; };
            }
            else
            {
                SW.Focus();
            }
        }

        private void Stop_Click(object sender, RoutedEventArgs e)
        {
            KM.StopKinect();
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            if (SW != null)
            {
                SW.Close();
            }
            KM.CloseKinect();
        }

        const float ZoomStep = 1.1f;
        private void Window_MouseWheel(object sender, System.Windows.Input.MouseWheelEventArgs e)
        {
            if(e.Delta>0)
                SS.Scale.Value *= ZoomStep;
            else
                SS.Scale.Value /= ZoomStep;
        }

        bool ViewMouseManipulation = false;
        Point CursorStartPos;
        private void Window_MouseDown(object sender, MouseButtonEventArgs e)
        {
            CursorStartPos = Mouse.GetPosition(null);
            ViewMouseManipulation = true;
            Mouse.Capture(this);
        }

        void DT_Tick(object sender, EventArgs e)
        {
            if (!ViewMouseManipulation) return;
            if (Mouse.RightButton == MouseButtonState.Released && Mouse.LeftButton == MouseButtonState.Released)
            {
                ViewMouseManipulation = false;
                Mouse.Capture(null);
                return;
            }
            Point cursorPos = Mouse.GetPosition(null);
            float deltaX = (float)(cursorPos.X - CursorStartPos.X);
            float deltaY = (float)(cursorPos.Y - CursorStartPos.Y);

            if (Mouse.RightButton == MouseButtonState.Pressed)
            {
                SS.RotationY.Value += deltaX * 0.2f;
                SS.RotationX.Value -= deltaY * 0.2f;
            }

            if (Mouse.LeftButton == MouseButtonState.Pressed)
            {
                SS.MoveX.Value += deltaX * 0.002f / SS.Scale.Value;
                SS.MoveY.Value -= deltaY * 0.002f / SS.Scale.Value;
            }
            CursorStartPos = cursorPos;
        }

        private void ResetView_Click(object sender, RoutedEventArgs e)
        {
            SS.ViewProperties.ResetToDefault();
        }

        private void Window_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            SS.ViewProperties.ResetToDefault();
        }

        private void SaveRaw_Click(object sender, RoutedEventArgs e)
        {
            string fileName = "";
            if (SS.SaveLabel.Value != "")
                fileName += SS.SaveLabel.Value + "_";
            fileName += DateTime.Now.ToString("yyyy-MM-dd_HH-mm-ss") + ".gsr";
            SaveDialog(KM.SaveRaw(SS.SaveDirectory.AbsolutePath + fileName));
        }

        private void SaveDialog(bool ok)
        {
            if (!ok) MessageBox.Show("Saving was unsuccessful.", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
        }

        private void Open_Click(object sender, RoutedEventArgs e)
        {
            OpenFileDialog OFD = new OpenFileDialog();
            OFD.Filter = "GreenScan RAW files (*.gsr)|*.gsr";
            OFD.InitialDirectory = SS.SaveDirectory.AbsolutePath;
            OFD.Title = "Open file";
            if (OFD.ShowDialog(this) == true)
            {
                KM.StopKinect();
                OpenDialog(KM.OpenRaw(OFD.FileName));
            }
        }

        private void OpenDialog(bool ok)
        {
            if (!ok) MessageBox.Show("Opening was unsuccessful.", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
        }
    }
}
