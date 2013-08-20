using ExcaliburSecurity;
using Green.Graphics;
using Green.Kinect;
using Green.Remoting;
using GreenScan;
using Microsoft.Win32;
using System;
using System.ComponentModel;
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
        ScanSettings SS;
        KinectManager KM;
        DispatcherTimer DT;
        SettingsWindow SW;
        RemoteReceiver RR;
        
        public MainWindow()
        {
            InitializeComponent();
            ShowStatus("Loading...", true);

            BackgroundWorker SecurityWorker = new BackgroundWorker();
            SecurityWorker.DoWork += SecurityWorker_DoWork;
            SecurityWorker.RunWorkerCompleted += SecurityWorker_RunWorkerCompleted;
            SecurityWorker.RunWorkerAsync();

            

            SS = new ScanSettings();
            KM = new KinectManager();
            GC.Loaded += GC_Loaded;
            InitGUI();
            InitSettings();
            InitRemoting();
        }

        void SecurityWorker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            if ((bool)e.Result == true)
            {
                ShowStatus("Ready.");
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
            MIStart.DataContext = KM;
            MIMode.DataContext = SS.KinectMode;
            MITurntable.DataContext = RS;
            MIExport.DataContext = KM;
            SMC.DataContext = SS;

            DT = new DispatcherTimer(DispatcherPriority.Send);
            DT.Interval = new TimeSpan(0, 0, 0, 0, 30);
            DT.Tick += DT_Tick;
            DT.IsEnabled = true;
        }        

        void KinectMode_ValueChanged(object sender, System.EventArgs e)
        {
            if (KM.Processing)
            {
                KM.StartKinect(SS.KinectMode.Value);
            }
        }

        private void Stop_Click(object sender, RoutedEventArgs e)
        {
            KM.StopKinect();
        }

        void GC_Loaded(object sender, RoutedEventArgs e)
        {
            GC.SetKinectDevice(KM);
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
            RR.Dispose();
            if (SW != null) SW.Close();
            if (RS != null) RS.Dispose();
            if (TCW != null) TCW.Close();
            KM.CloseKinect();
            KM.Dispose();
            SS.Save("Settings.ini");
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
            if (KM.DeviceCount == 0) return;
            if (!KM.DeviceOpened)
                if (!KM.OpenKinect(0)) return;
            if (!KM.Processing || KM.Mode != KinectManager.Modes.DepthAndColor)
                if (!KM.StartKinect(KinectManager.Modes.DepthAndColor)) return;
        }

        void StartUpWorker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            (sender as BackgroundWorker).Dispose();
            if (KM.Processing && KM.Mode == KinectManager.Modes.DepthAndColor)
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
            SS.Load("Settings.ini");
            SS.KinectMode.ValueChanged += KinectMode_ValueChanged;
            SS.PreprocessingProperties.ValueChanged += (object sender, EventArgs e) => { SetPreprocessing(); };
            SS.ViewProperties.ValueChanged += (object sender, EventArgs e) => { SetView(); };
            SS.CameraProperties.ValueChanged += (object sender, EventArgs e) => { SetCameras(); };
            SS.ShadingProperties.ValueChanged += (object sender, EventArgs e) => { SetShading(); };
            SS.PerformanceProperties.ValueChanged += (object sender, EventArgs e) => { SetPerformance(); };
            SS.SaveProperties.ValueChanged += (object sender, EventArgs e) => { SetSave(); };
            SS.TurntableProperties.ValueChanged += (object sender, EventArgs e) => { SetTurntable(); };
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

            RR = new RemoteReceiver(SS, commands, this, 5656, 101);

            RR.RemoteConnected += (object sender, RemoteEventArgs e) =>
            {
                if (RR.RemoteCount == 1)
                    TBRemoting.Text = "Remoted from " + e.RemoteEndPoint.ToString();
                else
                    TBRemoting.Text = RR.RemoteCount+ " remote(s) connected.";
                if (RR.RemoteCount == 1)
                    (Resources["RemotingIn"] as Storyboard).Begin();
            };
            RR.RemoteDisconnected += (object sender, RemoteEventArgs e) =>
            {
                TBRemoting.Text = RR.RemoteCount + " remote(s) connected.";
                if (RR.RemoteCount == 0)
                    (Resources["RemotingOut"] as Storyboard).Begin();
            };

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
                SS.InfraredDistortion.Value,
                SS.InfraredDistortionCorrectionEnabled.Value,
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
                SS.WireframeShading.Value,
                SS.UseModuleShading.Value);
        }

        void SetPerformance()
        {
            GC.SetPerformance(
                SS.TriangleGridResolution.Width,
                SS.TriangleGridResolution.Height);
        }

        void SetSave()
        {
            GC.SetSave(
                SS.SaveModelResolution.Width,
                SS.SaveModelResolution.Height,
                SS.SaveTextureResolution.Width,
                SS.SaveTextureResolution.Height);
        }

        void SetTurntable()
        {
            RS.SetPerformance(
                SS.TurntableModelResolution.Width,
                SS.TurntableModelResolution.Height,
                SS.TurntableTextureResolution.Width,
                SS.TurntableTextureResolution.Height);
            RS.SetCalibration(
                SS.TurntableTransform.Value,
                SS.TurntableClippingHeight.Value,
                SS.TurntableClippingRadius.Value,
                SS.TurntableCoreX.Value,
                SS.TurntableCoreY.Value);
            RS.SetShading(
                SS.TurntableView.Value);
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

        #endregion

        #region View manipulation

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

        #endregion

        #region Save & Open

        private string GenerateFilename(string ext = "")
        {
            string fileName = "";
            if (SS.SaveLabel.Value != "")
                fileName += SS.SaveLabel.Value + "_";
            fileName += DateTime.Now.ToString("yyyy-MM-dd_HH-mm-ss") + ext;
            return SS.SaveDirectory.AbsolutePath + fileName;
        }

        private void SaveRaw()
        {
            SaveDialog(KM.SaveRaw(GenerateFilename(".gsr")));
        }

        private void SaveImage()
        {
            SaveDialog(GC.SaveImage(GenerateFilename(".png")));
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

        private void OpenRaw()
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
            if (ok)
                ShowStatus("File loaded successfully.");
            else
                MessageBox.Show("Opening was unsuccessful.", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
        }

        #endregion

        #region Rotating scanner

        RotatingScanner RS;
        void InitRotatingScanner()
        {
            RS = new RotatingScanner(KM, GC);
            MITurntable.DataContext = RS;
            RS.PropertyChanged += RS_PropertyChanged;
            RS.StatusChanged += RS_StatusChanged;
        }

        void RS_StatusChanged(object sender, Extensions.StatusEventArgs e)
        {
            ShowStatus(e.Text, e.ShowProgress, e.Progress);
        }

        void RS_PropertyChanged(object sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            switch (e.PropertyName)
            {
                case "IsConnected":
                    SS.TurntableProperties.IsHidden = !RS.IsConnected;
                    if (RS.IsConnected)
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
                SS.UseModuleShading.Value = false;
                TCW = new TurntableCalibrationWindow(GC, SS);
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
            if (RS.IsConnected)
            {
                RS.Scan();
            }
        }

        #region Commanding
        void ProcessingCmdCanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = KM.Processing;
        }

        void StartCmdCanExecute(object target, CanExecuteRoutedEventArgs e)
        {
            int index;
            if (int.TryParse((string)e.Parameter, out index))
            {
                e.CanExecute = KM.DeviceCount > index;
            }
            else e.CanExecute = false;
        }

        void StartCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            int index;
            if (int.TryParse((string)e.Parameter, out index))
            {
                ShowStatus("Starting...", true);
                BackgroundWorker StartWorker = new BackgroundWorker();
                StartWorker.DoWork += StartWorker_DoWork;
                StartWorker.RunWorkerCompleted += StartWorker_RunWorkerCompleted;
                StartWorker.RunWorkerAsync(index);                
            }
        }

        void StartWorker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            (sender as BackgroundWorker).Dispose();
            if((bool)e.Result)
                ShowStatus("Ready.");
            else
                ShowStatus("The Kinect cannot be opened, may already be in use.");
        }

        void StartWorker_DoWork(object sender, DoWorkEventArgs e)
        {
            KM.OpenKinect((int)e.Argument);
            e.Result = KM.StartKinect(SS.KinectMode.Value);
        }

        void OpenCmdCanExecute(object target, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = !SavingInProgress;
        }

        void OpenCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            OpenRaw();
        }

        void SaveCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            SaveRaw();
        }

        void SaveCmdCanExecute(object target, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = KM.Processing && !SavingInProgress;
        }

        void ExportCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            switch ((string)e.Parameter)
            {
                case "PNG":
                    SaveImage();
                    break; 
                default:
                    SaveModel(GC, (SaveFormats)Enum.Parse(typeof(SaveFormats), (string)e.Parameter));
                    break;
            }
        }

        void ExportCmdCanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            switch ((string)e.Parameter)
            {
                case "PNG":
                    e.CanExecute = KM.Processing && !SavingInProgress;
                    break;
                default:
                    e.CanExecute = KM.Processing && KM.Mode == KinectManager.Modes.DepthAndColor && !SavingInProgress;
                    break;
            }
        }

        void StopCmdCanExecute(object target, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = KM.Processing && !SavingInProgress;
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
            KM.StopKinect();
        }

        void CloseCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            Close();
        }

        void TurntableScanCmdCanExecute(object target, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = RS.IsConnected && !RS.IsScanning && RS.IsAtOrigin;
        }

        void TurntableScanCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            SetKinectToDepthAndColorMode(() => { RS.Scan(); });            
        }

        void TurntableStopCmdCanExecute(object target, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = RS.IsConnected && RS.IsScanning;
        }

        void TurntableStopCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            RS.Stop();
        }

        void TurntableToOriginCmdCanExecute(object target, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = RS.IsConnected && !RS.IsScanning && !RS.IsAtOrigin;
        }

        void TurntableToOriginCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            RS.ReturnToOrigin();
        }

        void TurntableExportCmdCanExecute(object target, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = RS.CanSave && !SavingInProgress;
        }

        void TurntableExportCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            SaveModel(RS, (SaveFormats)Enum.Parse(typeof(SaveFormats), (string)e.Parameter));
        }

        void TurntableSaveCmdCanExecute(object target, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = RS.CanSave && !SavingInProgress;
        }

        void TurntableSaveCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            SaveDialog(RS.SaveRaw(GenerateFilename(".gtr")));
        }

        void TurntableOpenCmdCanExecute(object target, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = RS.IsConnected && !RS.IsScanning;
        }

        void TurntableOpenCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            OpenFileDialog OFD = new OpenFileDialog();
            OFD.Filter = "GreenScan Turntable RAW files (*.gtr)|*.gtr";
            OFD.InitialDirectory = SS.SaveDirectory.AbsolutePath;
            OFD.Title = "Open file";
            if (OFD.ShowDialog(this) == true)
            {
                KM.StopKinect();
                OpenDialog(RS.OpenRaw(OFD.FileName));
            }
        }

        void TurntableCalibrateCmdCanExecute(object target, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = KM.Processing || KM.DeviceCount > 0;
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
        public static RoutedUICommand Save = new RoutedUICommand("Saves the current model.", "Save", typeof(MainWindow), new InputGestureCollection() { new KeyGesture(Key.S, ModifierKeys.Control) });
        public static RoutedUICommand Close = new RoutedUICommand("Closes the application.", "Close", typeof(MainWindow), new InputGestureCollection() { new KeyGesture(Key.Escape) });
        public static RoutedUICommand Export = new RoutedUICommand("Exports the current scene to the specified format.", "Export", typeof(MainWindow));
        public static RoutedUICommand Start = new RoutedUICommand("Start the Kinect with the specified index.", "Start", typeof(MainWindow));
        public static RoutedUICommand Stop = new RoutedUICommand("Stops the Kinect.", "Stop", typeof(MainWindow));
    }

    public static class TurntableCommands
    {
        public static RoutedUICommand Scan = new RoutedUICommand("Starts the scanning process. Turntable must be at origin. The command sets the Kinect to the required state.", "Turntable.Scan", typeof(MainWindow), new InputGestureCollection() { new KeyGesture(Key.F6) });
        public static RoutedUICommand Stop = new RoutedUICommand("Stops the scanning and the turntable.", "Turntable.Stop", typeof(MainWindow), new InputGestureCollection() { new KeyGesture(Key.F12) });
        public static RoutedUICommand Calibrate = new RoutedUICommand("Shows the calibration window.", "Turntable.Calibrate", typeof(MainWindow));
        public static RoutedUICommand ToOrigin = new RoutedUICommand("Returns the turntable to its origin.", "Turntable.ToOrigin", typeof(MainWindow), new InputGestureCollection() { new KeyGesture(Key.F7) });
        public static RoutedUICommand Export = new RoutedUICommand("Exports the model in various formats. The format should be given as parameter.", "Turntable.Export", typeof(MainWindow));
        public static RoutedUICommand Open = new RoutedUICommand("Opens the specified turntable model file.", "Turntable.Open", typeof(MainWindow));
        public static RoutedUICommand Save = new RoutedUICommand("Saves the model.", "Turntable.Save", typeof(MainWindow));
    }
}
