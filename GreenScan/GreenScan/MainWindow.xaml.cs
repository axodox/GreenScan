using ExcaliburSecurity;
using Green.Branding;
using Green.Graphics;
using Green.Kinect;
using Green.Remoting;
using GreenScan;
using Microsoft.Win32;
using System;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media.Animation;
using System.Windows.Threading;

namespace Green.Scan
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window, INotifyPropertyChanged
    {
        ScanSettings Settings;
        KinectManager DeviceManager;
        DispatcherTimer MainDispatcher;
        RemotingServer Remote;
        
        public MainWindow()
        {
            InitializeComponent();
            ShowStatus("Loading...", true);

            BackgroundWorker SecurityWorker = new BackgroundWorker();
            SecurityWorker.DoWork += SecurityWorker_DoWork;
            SecurityWorker.RunWorkerCompleted += SecurityWorker_RunWorkerCompleted;
            SecurityWorker.RunWorkerAsync();
            
            Settings = new ScanSettings();
            DeviceManager = new KinectManager();
            GraphicsCore.Loaded += GraphicsCore_Loaded;
            InitGUI();
            InitSettings();
            InitRemoting();
        }

        void SecurityWorker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            if ((bool)e.Result == true)
            {
                ShowStatus("Ready.");
                Remote.IsEnabled = true;
                IsEnabled = true;
            }
            else
                Close();            
        }

        void SecurityWorker_DoWork(object sender, DoWorkEventArgs e)
        {
            SecurityClient SC = new SecurityClient();
            e.Result = SC.HasPermissionToRun;
        }

        void InitGUI()
        {
            MIStart.DataContext = DeviceManager;
            MIMode.DataContext = Settings.KinectMode;
            MITurntable.DataContext = TurntableScanner;
            MIExport.DataContext = DeviceManager;
            SMC.DataContext = Settings;

            MainDispatcher = new DispatcherTimer(DispatcherPriority.Send);
            MainDispatcher.Interval = new TimeSpan(0, 0, 0, 0, 30);
            MainDispatcher.Tick += DT_Tick;
            MainDispatcher.IsEnabled = true;
        }        

        void KinectMode_ValueChanged(object sender, System.EventArgs e)
        {
            if (DeviceManager.Processing)
            {
                DeviceManager.StartKinect(Settings.KinectMode.Value);
            }
        }

        private void Stop_Click(object sender, RoutedEventArgs e)
        {
            DeviceManager.StopKinect();
        }

        void GraphicsCore_Loaded(object sender, RoutedEventArgs e)
        {
            GraphicsCore.SetKinectDevice(DeviceManager);
            SetKinect();
            SetPreprocessing();
            SetView();
            SetCameras();
            SetShading();
            SetPerformance();
            SetSave();

            InitRotatingScanner();
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            Remote.Dispose();
            if (TurntableScanner != null) TurntableScanner.Dispose();
            if (TCW != null) TCW.Close();
            DeviceManager.CloseKinect();
            DeviceManager.Dispose();
            Settings.Save("Settings.ini");
        }

        void SetKinectToDepthAndColorMode(Action completedCallback)
        {
            ShowStatus("Preparing Kinect...", true);
            BackgroundWorker StartUpWorker = new BackgroundWorker();
            StartUpWorker.DoWork += StartUpWorker_DoWork;
            StartUpWorker.RunWorkerCompleted += StartUpWorker_RunWorkerCompleted;
            StartUpWorker.RunWorkerAsync(completedCallback);
        }

        void StartUpWorker_DoWork(object sender, DoWorkEventArgs e)
        {
            e.Result = e.Argument;
            if (DeviceManager.DeviceCount == 0) return;
            if (!DeviceManager.DeviceOpened)
                if (!DeviceManager.OpenKinect(0)) return;
            if (!DeviceManager.Processing || DeviceManager.Mode != KinectManager.Modes.DepthAndColor)
                if (!DeviceManager.StartKinect(KinectManager.Modes.DepthAndColor)) return;
        }

        void StartUpWorker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            (sender as BackgroundWorker).Dispose();
            if (DeviceManager.Processing && DeviceManager.Mode == KinectManager.Modes.DepthAndColor)
            {
                ShowStatus("Ready.");
                (e.Result as Action)();
            }
            else
                ShowStatus("Cannot connect to Kinect.");
        }

        #region Settings

        void InitSettings()
        {
            Settings.Load("Settings.ini");
            Settings.KinectMode.ValueChanged += KinectMode_ValueChanged;
            Settings.KinectProperties.ValueChanged += (object sender, EventArgs e) => { SetKinect(); };
            Settings.PreprocessingProperties.ValueChanged += (object sender, EventArgs e) => { SetPreprocessing(); };
            Settings.ViewProperties.ValueChanged += (object sender, EventArgs e) => { SetView(); };
            Settings.CameraProperties.ValueChanged += (object sender, EventArgs e) => { SetCameras(); };
            Settings.ShadingProperties.ValueChanged += (object sender, EventArgs e) => { SetShading(); };
            Settings.PerformanceProperties.ValueChanged += (object sender, EventArgs e) => { SetPerformance(); };
            Settings.SaveProperties.ValueChanged += (object sender, EventArgs e) => { SetSave(); };
            Settings.TurntableProperties.ValueChanged += (object sender, EventArgs e) => { SetTurntable(); };
        }

        void InitRemoting()
        {
            RoutedUICommand[] commands = {
                GreenScanCommands.Close, 
                GreenScanCommands.Export, 
                GreenScanCommands.Open, 
                GreenScanCommands.Save, 
                GreenScanCommands.Start, 
                GreenScanCommands.Stop,
                                     
                TurntableCommands.Export, 
                TurntableCommands.Open,
                TurntableCommands.Save,
                TurntableCommands.Scan,
                TurntableCommands.Stop, 
                TurntableCommands.ToOrigin
            };

            Remote = new RemotingServer(Settings, commands, this, 5656, 101);

            Remote.RemoteConnected += (object sender, RemoteEventArgs e) =>
            {
                if (Remote.RemoteCount == 1)
                    TBRemoting.Text = "Remoted from " + e.RemoteEndPoint.ToString();
                else
                    TBRemoting.Text = Remote.RemoteCount+ " remote(s) connected.";
                if (Remote.RemoteCount == 1)
                    (Resources["RemotingIn"] as Storyboard).Begin();
            };
            Remote.RemoteDisconnected += (object sender, RemoteEventArgs e) =>
            {
                TBRemoting.Text = Remote.RemoteCount + " remote(s) connected.";
                if (Remote.RemoteCount == 0)
                    (Resources["RemotingOut"] as Storyboard).Begin();
            };

        }

        void SetKinect()
        {
            DeviceManager.SetEmitter(Settings.EmitterEnabled.Value);
        }

        void SetPreprocessing()
        {
            GraphicsCore.SetPreprocessing(
                Settings.DepthAveraging.Value,
                Settings.DepthGaussIterations.Value,
                Settings.DepthGaussSigma.Value);
        }

        void SetView()
        {
            GraphicsCore.SetView(
                Settings.TranslationX.Value,
                Settings.TranslationY.Value,
                Settings.TranslationZ.Value,
                Settings.RotationX.Value,
                Settings.RotationY.Value,
                Settings.RotationZ.Value,
                Settings.Scale.Value,
                Settings.MoveX.Value,
                Settings.MoveY.Value,
                Settings.Rotation.Value);
        }

        void SetCameras()
        {
            GraphicsCore.SetCameras(
                Settings.InfraredIntrinsics.Value,
                Settings.InfraredDistortion.Value,
                Settings.InfraredDistortionCorrectionEnabled.Value,
                Settings.DepthToIRMapping.Value,
                Settings.ColorIntrinsics.Value,
                Settings.ColorRemapping.Value,
                Settings.ColorExtrinsics.Value,
                Settings.ColorDispositionX.Value,
                Settings.ColorDispositionY.Value,
                Settings.ColorScaleX.Value,
                Settings.ColorScaleY.Value);
        }

        void SetShading()
        {
            GraphicsCore.SetShading(
                Settings.ShadingMode.Value,
                Settings.DepthMaximum.Value,
                Settings.DepthMinimum.Value,
                Settings.ShadingPeriode.Value,
                Settings.ShadingPhase.Value,
                Settings.TriangleRemoveLimit.Value,
                Settings.WireframeShading.Value,
                Settings.UseModuleShading.Value);
        }

        void SetPerformance()
        {
            GraphicsCore.SetPerformance(
                Settings.TriangleGridResolution.Width,
                Settings.TriangleGridResolution.Height);
        }

        void SetSave()
        {
            GraphicsCore.SetSave(
                Settings.SaveModelResolution.Width,
                Settings.SaveModelResolution.Height,
                Settings.SaveTextureResolution.Width,
                Settings.SaveTextureResolution.Height);
        }

        void SetTurntable()
        {
            TurntableScanner.SetPerformance(
                Settings.TurntableModelResolution.Width,
                Settings.TurntableModelResolution.Height,
                Settings.TurntableTextureResolution.Width,
                Settings.TurntableTextureResolution.Height);
            TurntableScanner.SetCalibration(
                Settings.TurntableTransform.Value,
                Settings.TurntableClippingHeight.Value,
                Settings.TurntableClippingRadius.Value,
                Settings.TurntableCoreX.Value,
                Settings.TurntableCoreY.Value,
                Settings.TurntablePiSteps.Value);
            TurntableScanner.SetShading(
                Settings.TurntableView.Value);
        }

        #endregion

        #region View manipulation

        const float ZoomStep = 1.1f;
        private void Window_MouseWheel(object sender, System.Windows.Input.MouseWheelEventArgs e)
        {
            if(e.Delta>0)
                Settings.Scale.Value *= ZoomStep;
            else
                Settings.Scale.Value /= ZoomStep;
        }

        bool IsMouseOnCanvas
        {
            get
            {
                Point p = Mouse.GetPosition(GraphicsCore);
                return p.X > 0 && p.Y > 0 && p.X < GraphicsCore.ActualWidth && p.Y < GraphicsCore.ActualHeight;
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
                Settings.RotationY.Value += deltaX * 0.2f;
                Settings.RotationX.Value -= deltaY * 0.2f;
            }

            if (Mouse.LeftButton == MouseButtonState.Pressed)
            {
                Settings.MoveX.Value += deltaX * 0.002f / Settings.Scale.Value;
                Settings.MoveY.Value -= deltaY * 0.002f / Settings.Scale.Value;
            }
            CursorStartPos = cursorPos;
        }

        private void ResetView_Click(object sender, RoutedEventArgs e)
        {
            Settings.ViewProperties.ResetToDefault();
        }

        private void Window_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            if (!IsMouseOnCanvas) return;
            Settings.ViewProperties.ResetToDefault();
        }

        #endregion

        #region Save & Open

        private string GenerateFilename(string ext = "")
        {
            string fileName = "";
            if (Settings.SaveLabel.Value != "" && Settings.SaveNoTimestamp.Value)
                fileName = Settings.SaveLabel.Value;
            else
            {
                if (Settings.SaveLabel.Value != "")
                    fileName += Settings.SaveLabel.Value + "_";
                fileName += DateTime.Now.ToString("yyyy-MM-dd_HH-mm-ss");
            }
            fileName += ext;
            return Settings.SaveDirectory.AbsolutePath + fileName;
        }

        private void SaveRaw()
        {
            SaveDialog(DeviceManager.SaveRaw(GenerateFilename(".gsr")));
        }

        private void SaveImage()
        {
            SaveDialog(GraphicsCore.SaveImage(GenerateFilename(".png")));
        }

        private void SaveRawImage()
        {
            SaveDialog(GraphicsCore.SaveRawImage(GenerateFilename(".png")));
        }

        private void SaveDialog(bool ok)
        {
            if (ok)
                ShowStatus("Save complete.");
            else
            {
                ShowStatus("Ready.");
                MessageBox.Show("Saving was unsuccessful.", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        class SaveTask
        {
            public IModelSaver ModelSaver;
            public String Filename;
            public SaveFormats Format;
            public SaveTask(IModelSaver modelSaver, string filename, SaveFormats format)
            {
                ModelSaver = modelSaver;
                Filename = filename;
                Format = format;
            }
        }

        public bool SavingInProgress { get; private set; }
        private void SaveModel(IModelSaver modelSaver, SaveFormats format)
        {
            if (SavingInProgress) return;
            SavingInProgress = true;
            NotifyPropertyChanged("SavingInProgress");
            BackgroundWorker SaveWorker = new BackgroundWorker();
            SaveWorker.DoWork += SaveWorker_DoWork;
            SaveWorker.RunWorkerCompleted += SaveWorker_RunWorkerCompleted;
            SaveWorker.RunWorkerAsync(new SaveTask(modelSaver, GenerateFilename(), format));
            ShowStatus("Saving in progress...", true, double.NaN);
        }

        void SaveWorker_DoWork(object sender, DoWorkEventArgs e)
        {
            SaveTask ST = (SaveTask)e.Argument;
            e.Result = ST.ModelSaver.SaveModel(ST.Filename, ST.Format);
        }

        void SaveWorker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            SaveDialog((bool)e.Result);
            (sender as BackgroundWorker).Dispose();
            SavingInProgress = false;
            NotifyPropertyChanged("SavingInProgress");
        }

        private void OpenRaw(string path = null)
        {
            if (path == null)
            {
                OpenFileDialog OFD = new OpenFileDialog();
                OFD.Filter = "GreenScan RAW files (*.gsr)|*.gsr";
                OFD.InitialDirectory = Settings.SaveDirectory.AbsolutePath;
                OFD.Title = "Open file";
                if (OFD.ShowDialog(this) == true)
                {
                    path = OFD.FileName;                    
                }
            }

            if (path != null)
            {
                DeviceManager.StopKinect();
                OpenDialog(DeviceManager.OpenRaw(path));
            }
        }

        private void OpenDialog(bool ok)
        {
            if (ok)
                ShowStatus("File loaded successfully.");
            else
                MessageBox.Show("Opening was unsuccessful.", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
        }

        #endregion

        #region Rotating scanner

        RotatingScanner TurntableScanner;
        void InitRotatingScanner()
        {
            TurntableScanner = new RotatingScanner(DeviceManager, GraphicsCore);
            MITurntable.DataContext = TurntableScanner;
            TurntableScanner.PropertyChanged += TurntableScanner_PropertyChanged;
            TurntableScanner.StatusChanged += TurntableScanner_StatusChanged;
        }

        void TurntableScanner_StatusChanged(object sender, Extensions.StatusEventArgs e)
        {
            ShowStatus(e.Text, e.ShowProgress, e.Progress);
        }

        void TurntableScanner_PropertyChanged(object sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            switch (e.PropertyName)
            {
                case "IsConnected":
                    Settings.TurntableProperties.IsHidden = !TurntableScanner.IsConnected;
                    if (TurntableScanner.IsConnected)
                    {
                        SetTurntable();
                    }
                    break;
            }
        }

        TurntableCalibrationWindow TCW;
        private void CalibrateTurntable()
        {
            SetKinectToDepthAndColorMode(() =>
            {
                Settings.UseModuleShading.Value = false;
                TCW = new TurntableCalibrationWindow(GraphicsCore, Settings);
                TCW.Show();
            });
        }
        #endregion

        void ShowStatus(string text, bool showProgress = false, double progress = double.NaN)
        {
            SBIStatusText.Content = text;
            Visibility newVisibility = showProgress ? Visibility.Visible : Visibility.Collapsed;
            if (PBStatus.Visibility != newVisibility) PBStatus.Visibility = newVisibility;
            bool newIndeterminity = double.IsNaN(progress);
            if (PBStatus.IsIndeterminate != newIndeterminity) PBStatus.IsIndeterminate = newIndeterminity;
            PBStatus.IsIndeterminate = double.IsNaN(progress);
            if (PBStatus.Value != progress && !PBStatus.IsIndeterminate) PBStatus.Value = progress;
        }

        private void TurntableScan_Click(object sender, RoutedEventArgs e)
        {
            if (TurntableScanner.IsConnected)
            {
                TurntableScanner.Scan();
            }
        }

        #region Commanding
        void ProcessingCmdCanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = DeviceManager.Processing;
        }

        void StartCmdCanExecute(object target, CanExecuteRoutedEventArgs e)
        {
            int index;
            if (int.TryParse((string)e.Parameter, out index))
            {
                e.CanExecute = DeviceManager.DeviceCount > index;
            }
            else e.CanExecute = false;
        }

        void StartCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            int index;
            if (int.TryParse((string)e.Parameter, out index))
            {
                StartKinect(index);           
            }
        }

        public void StartKinect(KinectManager.Modes mode)
        {
            if (DeviceManager.Mode != mode) Settings.KinectMode.Value = mode;
            if (!DeviceManager.Processing) StartKinect(0);
        }

        public void StartKinect(int index)
        {
            ShowStatus("Starting...", true);
            BackgroundWorker StartWorker = new BackgroundWorker();
            StartWorker.DoWork += StartWorker_DoWork;
            StartWorker.RunWorkerCompleted += StartWorker_RunWorkerCompleted;
            StartWorker.RunWorkerAsync(index);     
        }

        void StartWorker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            (sender as BackgroundWorker).Dispose();
            if((bool)e.Result)
                ShowStatus("Ready.");
            else
                ShowStatus("The device can not be opened. May be on low power, brandwith or may be already in use.");
        }

        void StartWorker_DoWork(object sender, DoWorkEventArgs e)
        {
            DeviceManager.OpenKinect((int)e.Argument);
            e.Result = DeviceManager.StartKinect(Settings.KinectMode.Value);
        }

        void OpenCmdCanExecute(object target, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = !SavingInProgress;
            if (e.Parameter != null && e.Parameter is String)
            {
                e.CanExecute &= (e.Parameter as String).ToLower().EndsWith(".gsr") && File.Exists((string)e.Parameter);
            }
        }

        void OpenCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            OpenRaw((string)e.Parameter);
        }

        void SaveCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            SaveRaw();
        }

        void SaveCmdCanExecute(object target, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = DeviceManager.Processing && !SavingInProgress;
        }

        void BrowseCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            try { Process.Start(Settings.SaveDirectory.AbsolutePath); }
            catch { }
        }

        void ExportCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            switch ((string)e.Parameter)
            {
                case "PNG":
                    SaveImage();
                    break;
                case "PNG/raw":
                    SaveRawImage();
                    break; 
                default:
                    SaveModel(GraphicsCore, (SaveFormats)Enum.Parse(typeof(SaveFormats), (string)e.Parameter));
                    break;
            }
        }

        void ExportCmdCanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            switch ((string)e.Parameter)
            {
                case "PNG":
                    e.CanExecute = DeviceManager.ProvidesData && !SavingInProgress;
                    break;
                case "PNG/raw":
                    e.CanExecute = DeviceManager.ProvidesData && !SavingInProgress && (DeviceManager.Mode == KinectManager.Modes.Color || DeviceManager.Mode == KinectManager.Modes.Infrared);
                    break;
                default:
                    e.CanExecute = DeviceManager.ProvidesData && !SavingInProgress && DeviceManager.Mode == KinectManager.Modes.DepthAndColor;
                    break;
            }
        }

        void StopCmdCanExecute(object target, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = DeviceManager.Processing && !SavingInProgress;
        }

        void StopCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            ShowStatus("Stopping...", true);
            BackgroundWorker StopWorker = new BackgroundWorker();
            StopWorker.DoWork += StopWorker_DoWork;
            StopWorker.RunWorkerCompleted += StopWorker_RunWorkerCompleted;
            StopWorker.RunWorkerAsync();            
        }

        void StopWorker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            (sender as BackgroundWorker).Dispose();
            ShowStatus("Ready.");
        }

        void StopWorker_DoWork(object sender, DoWorkEventArgs e)
        {
            DeviceManager.StopKinect();
        }

        void CalibrateCmdCanExecute(object target, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = DeviceManager.DeviceCount > 0;
        }

        void CalibrateCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            KinectCalibrationWindow KCW = new KinectCalibrationWindow(Remote);
            KCW.ShowDialog();
        }

        void CloseCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            Close();
        }

        void AboutCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            AboutBox AB = new AboutBox();
            AB.ShowDialog();
        }

        void TurntableScanCmdCanExecute(object target, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = TurntableScanner.IsConnected && !TurntableScanner.IsScanning && TurntableScanner.IsAtOrigin;
        }

        void TurntableScanCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            SetKinectToDepthAndColorMode(() => { TurntableScanner.Scan(); });            
        }

        void TurntableStopCmdCanExecute(object target, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = TurntableScanner.IsConnected && TurntableScanner.IsScanning;
        }

        void TurntableStopCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            TurntableScanner.Stop();
        }

        void TurntableToOriginCmdCanExecute(object target, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = TurntableScanner.IsConnected && !TurntableScanner.IsScanning && !TurntableScanner.IsAtOrigin;
        }

        void TurntableToOriginCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            TurntableScanner.ReturnToOrigin();
        }

        void TurntableExportCmdCanExecute(object target, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = TurntableScanner.CanSave && !SavingInProgress;
        }

        void TurntableExportCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            SaveModel(TurntableScanner, (SaveFormats)Enum.Parse(typeof(SaveFormats), (string)e.Parameter));
        }

        void TurntableSaveCmdCanExecute(object target, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = TurntableScanner.CanSave && !SavingInProgress;
        }

        void TurntableSaveCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            SaveDialog(TurntableScanner.SaveRaw(GenerateFilename(".gtr")));
        }

        void TurntableOpenCmdCanExecute(object target, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = TurntableScanner.IsConnected && !TurntableScanner.IsScanning;
            if (e.Parameter != null && e.Parameter is String)
            {
                e.CanExecute &= (e.Parameter as String).ToLower().EndsWith(".gtr") && File.Exists((string)e.Parameter);
            }
        }

        void TurntableOpenCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            string path = (string)e.Parameter;
            if (path == null)
            {
                OpenFileDialog OFD = new OpenFileDialog();
                OFD.Filter = "GreenScan Turntable RAW files (*.gtr)|*.gtr";
                OFD.InitialDirectory = Settings.SaveDirectory.AbsolutePath;
                OFD.Title = "Open file";
                if (OFD.ShowDialog(this) == true)
                {
                    path = OFD.FileName;
                }
            }

            if (path != null)
            {
                DeviceManager.StopKinect();
                OpenDialog(TurntableScanner.OpenRaw(path));
            }
        }

        void TurntableCalibrateCmdCanExecute(object target, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = DeviceManager.Processing || DeviceManager.DeviceCount > 0;
        }

        void TurntableCalibrateCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            CalibrateTurntable();
        }
        #endregion

        public event PropertyChangedEventHandler PropertyChanged;
        void NotifyPropertyChanged(string name)
        {
            if (PropertyChanged != null) PropertyChanged(this, new PropertyChangedEventArgs(name));
        }
    }

    public static class GreenScanCommands
    {
        public static RoutedUICommand Open = new RoutedUICommand("Opens the specified model file.", "Open", typeof(MainWindow), new InputGestureCollection() { new KeyGesture(Key.O, ModifierKeys.Control) });
        public static RoutedUICommand Browse = new RoutedUICommand("Browses save directory.", "Browse", typeof(MainWindow));
        public static RoutedUICommand Save = new RoutedUICommand("Saves the current model.", "Save", typeof(MainWindow), new InputGestureCollection() { new KeyGesture(Key.S, ModifierKeys.Control) });
        public static RoutedUICommand Close = new RoutedUICommand("Closes the application.", "Close", typeof(MainWindow), new InputGestureCollection() { new KeyGesture(Key.Escape) });
        public static RoutedUICommand Export = new RoutedUICommand("Exports the current scene to the specified format. Supported formats: PNG, PNG/raw, STL, FBX, DAE, DXF, OBJ, FL4.", "Export", typeof(MainWindow));
        public static RoutedUICommand Start = new RoutedUICommand("Start the Kinect with the specified index.", "Start", typeof(MainWindow));
        public static RoutedUICommand Stop = new RoutedUICommand("Stops the Kinect.", "Stop", typeof(MainWindow));
        public static RoutedUICommand Calibrate = new RoutedUICommand("Starts calibration procedure.", "Calibrate", typeof(MainWindow));
        public static RoutedUICommand About = new RoutedUICommand("Displays the about box.", "About", typeof(MainWindow));
    }

    public static class TurntableCommands
    {
        public static RoutedUICommand Scan = new RoutedUICommand("Starts the scanning process. Turntable must be at origin. The command sets the Kinect to the required state.", "Turntable.Scan", typeof(MainWindow), new InputGestureCollection() { new KeyGesture(Key.F6) });
        public static RoutedUICommand Stop = new RoutedUICommand("Stops the scanning and the turntable.", "Turntable.Stop", typeof(MainWindow), new InputGestureCollection() { new KeyGesture(Key.F12) });
        public static RoutedUICommand Calibrate = new RoutedUICommand("Shows the calibration window.", "Turntable.Calibrate", typeof(MainWindow));
        public static RoutedUICommand ToOrigin = new RoutedUICommand("Returns the turntable to its origin.", "Turntable.ToOrigin", typeof(MainWindow), new InputGestureCollection() { new KeyGesture(Key.F7) });
        public static RoutedUICommand Export = new RoutedUICommand("Exports the model in various formats. The format should be given as parameter. Supported formats: STL, FBX, DAE, DXF, OBJ, FL4.", "Turntable.Export", typeof(MainWindow));
        public static RoutedUICommand Open = new RoutedUICommand("Opens the specified turntable model file.", "Turntable.Open", typeof(MainWindow));
        public static RoutedUICommand Save = new RoutedUICommand("Saves the model.", "Turntable.Save", typeof(MainWindow));
    }
}
