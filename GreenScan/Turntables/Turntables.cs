using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Threading;
using System.IO;

namespace Turntables
{
    public class Turntable : IDisposable
    {
        static SynchronizationContext SC;
        static Timer PollTimer;
        public static int DeviceCount { get; private set; }
        public static int OpenedDevices { get; private set; }
        public static event EventHandler DeviceConnected, DeviceDisconnected;
        static Turntable()
        {
            SC = SynchronizationContext.Current;
            PollTimer = new Timer(DevicePollTimerCallback, null, 1000, 1000);
        }

        static void DevicePollTimerCallback(object o)
        {
            if (DeviceCount > 0) return;
            int deviceCount = GetDevices().Length;
            SC.Post(DevicePollCallback, deviceCount);
        }

        static void DevicePollCallback(object o)
        {
            int oldDeviceCount = DeviceCount;
            DeviceCount = (int)o + OpenedDevices;
            if (DeviceCount > oldDeviceCount && DeviceConnected != null) DeviceConnected(null, null);
            if (DeviceCount < oldDeviceCount && DeviceDisconnected != null) DeviceDisconnected(null, null);
        }

        public static Turntable DefaultDevice
        {
            get
            {
                if (DeviceCount == 0) return null;
                Turntable T = null;
                try
                {
                    T = new Turntable(GetDevices()[0]);
                    return T;
                }
                catch
                {
                    if (T != null) T.Dispose();
                    return null;
                }
            }
        }

        [Flags]
        private enum States : ushort { None = 0, Rotating = 0x0100, RotatingUp = 0x0200, MagneticSwitch = 0x0004}
        public enum Commands : int { About = 0, Stop, StartUp, StartDown, Status, Step, StepBack, ClearCounter, TurnOff, ToOrigin };
        private static readonly string[] CommandStrings = new string[] { "?", "S", "U", "D", "C", "A", "V", "Z", "E", "O" };
        private const string DeviceSignal = "Basch-step";
        private SerialPort Port;
        private int Origin;
        public int TargetStep { get; private set; }
        public int PositionInSteps { get; private set; }
        public double PositionInRadians 
        {
            get
            {
                return (double)PositionInSteps / PiSteps*Math.PI;
            }
        }

        public double PositionInUnits
        {
            get
            {
                return (double)PositionInSteps / PiSteps / 2d;
            }
        }

        public double PositionInDegrees
        {
            get
            {
                double degrees = PositionInSteps * 360d / PiSteps / 2d;
                if (degrees > 360) return degrees - 360d;
                if (degrees < 0) return degrees + 360d;
                return degrees;
            }
        }

        public const int PiSteps = 10989;

        private static void InitPort(SerialPort port)
        {
            port.BaudRate = 9600;
            port.DataBits = 8;
            port.StopBits = StopBits.One;
            port.Parity = Parity.None;
            port.Handshake = Handshake.None;
            port.RtsEnable = true;
            port.DtrEnable = true;
            port.ReadTimeout = Timeout.Infinite;
            port.NewLine = "\r\n";
        }
        public static string[] GetDevices()
        {
            SerialPort SP = new SerialPort();
            InitPort(SP);

            List<string> devices = new List<string>();
            string[] ports = SerialPort.GetPortNames();

            int i;
            for (i = 0; i < ports.Length; i++)
            {
                SP.PortName = ports[i];
                try
                {
                    SP.Open();
                    SP.DiscardInBuffer();

                    SP.Write(CommandStrings[(int)Commands.About]);
                    if (SP.ReadLine().StartsWith(DeviceSignal))
                    {
                        devices.Add(SP.PortName);
                        SP.Write(CommandStrings[(int)Commands.TurnOff]);
                    }
                }
                catch (UnauthorizedAccessException) { }
                catch (IOException) { }
                finally
                {
                    if (SP.IsOpen) SP.Close();
                }
            }
            SP.Dispose();
            return devices.ToArray();
        }

        public void Stop()
        {
            SendCommandAsync(Commands.Stop);
        }

        public void TurnOff()
        {
            SendCommandAsync(Commands.TurnOff);
        }

        private SynchronizationContext SyncContext;
        private bool CommunicationThreadOn;
        private Thread CommunicationThread;
        private Queue<string> CommandQueue;
        private AutoResetEvent SendARE;
        private void Communicate()
        {
            string command, answer;
            bool rotating;
            States statusWord;

            while (CommunicationThreadOn)
            {
                SendARE.WaitOne(PositionRefreshPeriod);
                while (CommandQueue.Count > 0)
                {
                    lock (CommandQueue) command = CommandQueue.Dequeue();
                    Port.Write(command);
                    Port.ReadLine();
                    if (command == CommandStrings[(int)Commands.ToOrigin])
                    {
                        AtOrigin = true;
                        CommandQueue.Enqueue(CommandStrings[(int)Commands.ClearCounter]);
                        SyncContext.Post(MotorStoppedCallback, null);                        
                    }
                }
                Port.Write(CommandStrings[(int)Commands.Status]);
                answer = Port.ReadLine();
                int h;

                int ss = Convert.ToInt32(answer.Substring(6, 6), 16);
                if (TargetStep > 0)
                {
                    if (ss > 32767)
                        h = TargetStep - (65536 - ss);
                    else
                        h = TargetStep - ss;
                    if (h <= TargetStep)
                        PositionInSteps = Origin + h;
                }
                else
                {
                    if (ss > 32767)
                        h = TargetStep + (65536 - ss);
                    else
                        h = TargetStep + ss;
                    if (h >= TargetStep)
                        PositionInSteps = Origin + h;
                }
                if (PositionChanged != null) PositionChanged(this, null);
                
                StatusWord = answer;
                statusWord = (States)Convert.ToInt16(answer.Replace(" ","").Substring(0, 4), 16);
                rotating = statusWord.HasFlag(States.Rotating);
                MagneticSwitch = statusWord.HasFlag(States.MagneticSwitch);
                if (PositionInSteps >= delay && DelayEnded!=null)
                {
                    DelayEnded(this, null);
                    delay = int.MaxValue;
                }
                if (PositionInSteps >= stopOn || (!MagneticSwitch && StopAtMagneticSwitch))
                {
                    stopOn = int.MaxValue;
                    CommandQueue.Enqueue(CommandStrings[(int)Commands.Stop]);
                    SendARE.Set();
                    SyncContext.Post(TurnCompleteCallback, null);
                }
                if (Rotating && !rotating) SyncContext.Post(MotorStoppedCallback, null);
                Rotating = rotating;
                RotationDirection = statusWord.HasFlag(States.RotatingUp);
            }
            Port.Write(CommandStrings[(int)Commands.Stop]);
            Port.Write(CommandStrings[(int)Commands.TurnOff]);
        }

        int stopOn = int.MaxValue;
        int delay = int.MaxValue;
        public void TurnOnce()
        {
            stopOn = PiSteps * 2;
            SendCommandAsync(Commands.ClearCounter);
            SendCommandAsync(Commands.StartUp);
            delay = int.MaxValue;
        }

        private const int DelayDivider = 10;
        public event EventHandler DelayEnded;
        public void TurnOnceWithDelay()
        {

            stopOn = PiSteps * 2;
            delay = PiSteps/20;
            stopOn += delay;
            PositionInSteps = 0;
            SendCommandAsync(Commands.ClearCounter);
            SendCommandAsync(Commands.StartUp);
        }

        public void Rotate()
        {
            SendCommandAsync(Commands.StartUp);
        }

        public void ResetCounter()
        {
            SendCommandAsync(Commands.ClearCounter);
        }

        public event EventHandler PositionChanged;
        public event EventHandler MotorStopped;
        private void MotorStoppedCallback(object o)
        {
            if (MotorStopped != null) MotorStopped(this, null);
        }

        public event EventHandler TurnComplete;
        private void TurnCompleteCallback(object o)
        {
            if (TurnComplete != null) TurnComplete(this, null);
        }

        public int PositionRefreshPeriod { get; set; }
        public int StepCount { get; private set; }
        public bool Rotating { get; private set; }
        public bool RotationDirection { get; private set; }
        public bool MagneticSwitch { get; private set; }
        public bool AtOrigin { get; private set; }
        public bool StopAtMagneticSwitch { get; set; }
        public string StatusWord { get; private set; }
        public Turntable(string portName)
        {
            Port = new SerialPort();
            Port.PortName = portName;
            InitPort(Port);
            Port.Open();
            OpenedDevices++;
            AtOrigin = false;

            CommandQueue = new Queue<string>();
            SendARE = new AutoResetEvent(true);
            SyncContext = SynchronizationContext.Current;
            PositionRefreshPeriod = 10;
            StopAtMagneticSwitch = false;

            CommunicationThreadOn = true;
            CommunicationThread = new Thread(Communicate);
            CommunicationThread.Start();
            SendCommandAsync(Commands.Stop);
            SendCommandAsync(Commands.TurnOff);
        }

        public void SendCommandAsync(Commands command, int steps = 0)
        {
            switch (command)
            {
                case Commands.StartDown:
                case Commands.StartUp:
                case Commands.Step:
                case Commands.StepBack:
                case Commands.ToOrigin:
                    AtOrigin = false;
                    break;               
            }
            string s = CommandStrings[(int)command];
            if (steps != 0)
            {
                s += string.Format("{0:X6}", steps);
                switch (command)
                {
                    case Commands.Step:
                        Origin = PositionInSteps;
                        TargetStep = steps;
                        break;
                    case Commands.StepBack:
                        Origin = PositionInSteps;
                        TargetStep = -steps;
                        break;
                }
            }
            lock (CommandQueue) CommandQueue.Enqueue(s);
            SendARE.Set();
        }

        public void RotateTo(double angle)
        {
            int steps = (int)(angle / Math.PI) * PiSteps - PositionInSteps;
            if (steps > 0)
                SendCommandAsync(Turntable.Commands.Step, steps);
            else
                SendCommandAsync(Turntable.Commands.StepBack, -steps);
        }

        public void RotateTo(int step)
        {
            int steps = step - PositionInSteps;
            if (steps > 0)
                SendCommandAsync(Turntable.Commands.Step, steps);
            else
                SendCommandAsync(Turntable.Commands.StepBack, -steps);
        }

        public void ToOrigin()
        {
            SendCommandAsync(Commands.ToOrigin);
        }

        bool disposed = false;
        public void Dispose()
        {
            disposed = true;
            CommunicationThreadOn = false;
            SendARE.Set();
            CommunicationThread.Join();
            Port.Dispose();
            Port = null;
            OpenedDevices--;
        }

        ~Turntable()
        {
            if (!disposed) Dispose();
        }

        private States statusWord1 { get; set; }
    }
}
