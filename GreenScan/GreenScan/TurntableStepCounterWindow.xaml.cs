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
using System.Windows.Shapes;
using System.Windows.Threading;
using Turntables;
using GreenResources = GreenScan.Properties.Resources;

namespace GreenScan
{
    /// <summary>
    /// Interaction logic for TurntableStepCounterWindow.xaml
    /// </summary>
    public partial class TurntableStepCounterWindow : Window
    {
        public int PiSteps { get; private set; }
        DispatcherTimer Timer;
        const int PremovementSteps = 2000;
        enum States
        {
            None,
            MovingToOrigin,
            Premovement,
            WaitForSwitch
        } 
        States State;
        Turntable Table;
        int Steps, Passes, TotalSteps, AverageSteps;
        public TurntableStepCounterWindow(Turntable table)
        {
            InitializeComponent();
            Table = table;            
            State = States.None;
            Steps = Passes = TotalSteps = AverageSteps = 0;
            PiSteps = 0;
            Timer = new DispatcherTimer();
            Timer.Interval = new TimeSpan(0, 0, 0, 0, 100);
            Timer.Tick += Timer_Tick;
            Timer.IsEnabled = true;
        }

        void Timer_Tick(object sender, EventArgs e)
        {
            RSteps.Text = Table.PositionInSteps.ToString();
        }

        void ProcessStep()
        {
            switch (State)
            {
                case States.None:
                    Table.ToOrigin();
                    State = States.MovingToOrigin;
                    TBProgress.Text = GreenResources.TurntableStepCounterStatusMovingToOrigin;
                    break;
                case States.MovingToOrigin:
                    Table.Rotate(PremovementSteps);
                    State = States.Premovement;
                    TBProgress.Text = GreenResources.TurntableStepCounterStatusPremovement;
                    break;
                case States.Premovement:                    
                    Table.Rotate();
                    Table.StopAtMagneticSwitch = true;
                    State = States.WaitForSwitch;
                    TBProgress.Text = GreenResources.TurntableStepCounterStatusWaitingForSwitch;
                    break;
                case States.WaitForSwitch:
                    Steps = Table.PositionInSteps;
                    TotalSteps += Steps;                    
                    Steps = 0;
                    Passes++;
                    RPasses.Text = Passes.ToString();
                    AverageSteps = TotalSteps / Passes;
                    RStepsPerRotation.Text = AverageSteps.ToString();
                    Table.StopAtMagneticSwitch = false;
                    goto case States.None;
            }
        }

        void Table_MotorStopped(object sender, EventArgs e)
        {
            ProcessStep();
        }

        private void BStart_Click(object sender, RoutedEventArgs e)
        {
            Table.MotorStopped += Table_MotorStopped;
            ProcessStep();
            BStart.IsEnabled = false;
            PBProgress.Visibility = Visibility.Visible;
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            if (Passes > 0)
            {
                MessageBoxResult mbr = MessageBox.Show(GreenResources.TurntableStepCounterSaveResults, GreenResources.TurntableStepCounterCloseTitle, MessageBoxButton.YesNoCancel, MessageBoxImage.Question);
                switch (mbr)
                {
                    case MessageBoxResult.Yes:
                        PiSteps = AverageSteps / 2;
                        break;
                    case MessageBoxResult.Cancel:
                        e.Cancel = true;
                        return;
                }
            }
            Table.MotorStopped -= Table_MotorStopped;
            Table.StopAtMagneticSwitch = false;
            Table.ToOrigin();            
        }        
    }
}
