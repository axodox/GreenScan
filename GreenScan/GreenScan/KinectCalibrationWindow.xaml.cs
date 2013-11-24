using Green.Remoting;
using Green.Settings;
using System;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Threading;
using GreenResources = GreenScan.Properties.Resources;

namespace GreenScan
{
    /// <summary>
    /// Interaction logic for KinectCalibrationWindow.xaml
    /// </summary>
    public partial class KinectCalibrationWindow : Window
    {
        int SceneNumber;
        int Image;
        private RemotingServer Remote;
        public KinectCalibrationWindow(RemotingServer remotingServer)
        {
            InitializeComponent();
            Remote = remotingServer;
            Remote.StoreOption("Save.Directory");
            Remote.StoreOption("Save.Label");
            Remote.StoreOption("Save.NoTimestamp");
            Remote.StoreOption("Kinect.Mode");
            TBPath.Text = PathSetting.GetAbsolutePath(Remote.GetOption("Save.CalibrationDirectory"));            
            CheckBoxes = new CheckBox[] { CBInfrared, CBColor, CBDepth };
            Modes = new string[] { "Infrared", "Color", "Depth" };
            Commands = new string[] { "Export", "Export", "Save" };
            Arguments = new string[] { "PNG/raw", "PNG/raw", "" };
            Labels = new string[] { "Infrared", "Color", "Depth" };
            SAngle.ValueChanged += SAngle_ValueChanged;
        }

        void SAngle_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            int newAngle = (int)SAngle.Value;
            Remote.SetOption("Kinect.ElevationAngle", newAngle.ToString());
        }

        private void BPath_Click(object sender, RoutedEventArgs e)
        {
            using (System.Windows.Forms.FolderBrowserDialog FBD = new System.Windows.Forms.FolderBrowserDialog())
            {
                FBD.Description = GreenResources.KinectCalibrationSelectDirectory;
                FBD.SelectedPath = TBPath.Text;
                if (FBD.ShowDialog() == System.Windows.Forms.DialogResult.OK)
                {
                    Remote.SetOption("Save.CalibrationDirectory", FBD.SelectedPath);
                    TBPath.Text = FBD.SelectedPath;
                }
            }
        }

        string TargetPath;
        private void BStart_Click(object sender, RoutedEventArgs e)
        {
            if (!Remote.ExecuteCommand("Start", "0")) return;

            TargetPath = TBPath.Text;
            try
            {
                for (int i = 0; i < Modes.Length; i++)
                    Directory.CreateDirectory(TargetPath + Modes[i]);
            }
            catch
            {
                return;
            }
            BPath.IsEnabled = false;
            BStart.IsEnabled = false;
            BNext.IsEnabled = true;
            BClear.IsEnabled = true;
            SceneNumber = 0;
            Image = 0;
            PrepareStep();
            Remote.SetOption("Save.NoTimestamp", "True");
        }

        CheckBox[] CheckBoxes;
        string[] Modes, Commands, Arguments, Labels;

        private void PrepareStep()
        {
            TBProgress.Text = string.Format(GreenResources.KinectCalibrationStatus, SceneNumber);
            
            for (int i = 0; i < CheckBoxes.Length; i++)
            {
                if (i == Image)
                    CheckBoxes[i].IsChecked = null;
                else
                    CheckBoxes[i].IsChecked = Image > i ? true : false;
            }
            Remote.SetOption("Kinect.Mode", Modes[Image]);
            Remote.SetOption("Save.Label", Labels[Image] + string.Format("{0:D3}", (SceneNumber + 1)));
            Remote.SetOption("Save.Directory", TargetPath + Modes[Image]);
        }

        private void BNext_Click(object sender, RoutedEventArgs e)
        {
            Remote.ExecuteCommand(Commands[Image], Arguments[Image]);
            Image++;
            if (Image == 3)
            {
                Image = 0;
                SceneNumber++;
            }
            PrepareStep();
        }

        private void BClear_Click(object sender, RoutedEventArgs e)
        {
            Image = 0;
            PrepareStep();
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            Remote.RestoreOption("Save.Directory");
            Remote.RestoreOption("Save.Label");
            Remote.RestoreOption("Save.NoTimestamp");
            Remote.RestoreOption("Kinect.Mode");
        }
    }
}
