using System;
using System.Windows;
using System.Windows.Interop;
using Green.Graphics;
using System.Threading;
using Green.Kinect;
using System.Windows.Input;
using System.Windows.Threading;
using Microsoft.Win32;
using GreenScan;
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
            GC.Loaded += GC_Loaded;
            InitGUI();
            InitSettings();
        }

        void InitGUI()
        {
            KinectManagerToDeviceListConverter.ModeSetting = SS.KinectMode;

            DataContext = KM;
            MIMode.DataContext = SS.KinectMode;
            //MIShading.DataContext = SS.ShadingMode;
            SMC.DataContext = SS;

            DT = new DispatcherTimer();
            DT.Interval = new TimeSpan(0, 0, 0, 0, 10);
            DT.Tick += DT_Tick;
            DT.IsEnabled = true;
        }

        void InitSettings()
        {
            SS.Load("Settings.ini");
            SS.KinectMode.ValueChanged += KinectMode_ValueChanged;
            SS.PreprocessingProperties.ValueChanged += (object sender, EventArgs e) => { SetPreprocessing(); };
            SS.ViewProperties.ValueChanged += (object sender, EventArgs e) => { SetView(); };
            SS.CameraProperties.ValueChanged += (object sender, EventArgs e) => { SetCameras(); };
            SS.ShadingProperties.ValueChanged += (object sender, EventArgs e) => { SetShading(); };
            SS.PerformanceProperties.ValueChanged += (object sender, EventArgs e) => { SetPerformance(); };
            SS.SaveProperties.ValueChanged += (object sender, EventArgs e) => { SetSave(); };
        }

        void KinectMode_ValueChanged(object sender, System.EventArgs e)
        {
            if (KM.Processing)
            {
                KM.StartKinect(SS.KinectMode.Value);
            }
        }

        void GC_Loaded(object sender, RoutedEventArgs e)
        {
            SetPreprocessing();
            SetView();
            SetCameras();
            SetShading();
            SetPerformance();
            SetSave();
        }

        void SetPreprocessing()
        {
            GC.SetPreprocessing(
                SS.DepthAveraging.Value,
                SS.DepthGaussIterations.Value,
                SS.DepthGaussSigma.Value);
        }

        void SetView()
        {
            GC.SetView(
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
            GC.SetCameras(
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
            GC.SetShading(
                SS.ShadingMode.Value,
                SS.DepthMaximum.Value,
                SS.DepthMinimum.Value,
                SS.ShadingPeriode.Value, 
                SS.ShadingPhase.Value, 
                SS.TriangleRemoveLimit.Value,
                SS.WireframeShading.Value);
        }

        void SetPerformance()
        {
            GC.SetPerformance(
                SS.TriangleGridWidth.Value,
                SS.TriangleGridHeight.Value);
        }

        void SetSave()
        {
            GC.SetSave(
                SS.SaveWidth.Value,
                SS.SaveHeight.Value,
                SS.SaveTextureWidth.Value,
                SS.SaveTextureHeight.Value);
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
            SS.Save("Settings.ini");
        }

        const float ZoomStep = 1.1f;
        private void Window_MouseWheel(object sender, System.Windows.Input.MouseWheelEventArgs e)
        {
            if(e.Delta>0)
                SS.Scale.Value *= ZoomStep;
            else
                SS.Scale.Value /= ZoomStep;
        }

        bool IsMouseOnCanvas
        {
            get
            {
                Point p = Mouse.GetPosition(GC);
                return p.X > 0 && p.Y > 0 && p.X < GC.ActualWidth && p.Y < GC.ActualHeight;
            }
        }

        bool ViewMouseManipulation = false;
        Point CursorStartPos;
        private void Window_MouseDown(object sender, MouseButtonEventArgs e)
        {
            if (!IsMouseOnCanvas) return;
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
            if (!IsMouseOnCanvas) return;
            SS.ViewProperties.ResetToDefault();
        }

        private string GenerateFilename(string ext)
        {
            string fileName = "";
            if (SS.SaveLabel.Value != "")
                fileName += SS.SaveLabel.Value + "_";
            fileName += DateTime.Now.ToString("yyyy-MM-dd_HH-mm-ss") + ext;
            return SS.SaveDirectory.AbsolutePath + fileName;
        }

        private void SaveRaw_Click(object sender, RoutedEventArgs e)
        {
            SaveDialog(KM.SaveRaw(GenerateFilename(".gsr")));
        }

        private void SaveImage_Click(object sender, RoutedEventArgs e)
        {
            SaveDialog(GC.SaveImage(GenerateFilename(".png")));
        }

        private void SaveSTL_Click(object sender, RoutedEventArgs e)
        {
            SaveModel(GraphicsCanvas.SaveFormats.STL);
        }

        private void SaveFBX_Click(object sender, RoutedEventArgs e)
        {
            SaveModel(GraphicsCanvas.SaveFormats.FBX);
        }

        private void SaveDXF_Click(object sender, RoutedEventArgs e)
        {
            SaveModel(GraphicsCanvas.SaveFormats.DXF);
        }

        private void SaveDAE_Click(object sender, RoutedEventArgs e)
        {
            SaveModel(GraphicsCanvas.SaveFormats.DAE);
        }

        private void SaveOBJ_Click(object sender, RoutedEventArgs e)
        {
            SaveModel(GraphicsCanvas.SaveFormats.OBJ);
        }

        private void SaveDialog(bool ok)
        {
            if (!ok) MessageBox.Show("Saving was unsuccessful.", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
        }

        private void SaveModel(GraphicsCanvas.SaveFormats format)
        {
            SaveWindow saveWindow = new SaveWindow(GC, GenerateFilename(""), format);
            SaveDialog((bool)saveWindow.ShowDialog());
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

        private void TurntableCalibrate_Click(object sender, RoutedEventArgs e)
        {
            TurntableCalibrationWindow TCW = new TurntableCalibrationWindow(GC, SS);
            TCW.Show();
        }
    }
}
