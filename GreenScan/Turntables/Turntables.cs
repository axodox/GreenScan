using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Threading;
using System.IO;

namespace Turntables
{
    public class Turntable : IDisposable
    {
        #region Device discovery
        private const string DeviceIdentifier = "Basch-step";
        private static SynchronizationContext SyncContext;
        private static Timer PollTimer;
        public static int DeviceCount { get; private set; }
        public static int OpenedDevices { get; private set; }
        public static event EventHandler DeviceConnected, DeviceDisconnected;

        static Turntable()
        {
            SyncContext = SynchronizationContext.Current;
            if (SyncContext == null)
                SyncContext = new SynchronizationContext();
            PollTimer = new Timer(DevicePollTimerCallback, null, 1000, 1000);
        }

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

                    SP.Write(CommandChars[(int)Commands.About].ToString());
                    if (SP.ReadLine().StartsWith(DeviceIdentifier))
                    {
                        devices.Add(SP.PortName);
                        SP.Write(CommandChars[(int)Commands.TurnOff].ToString());
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

        static void DevicePollTimerCallback(object o)
        {
            if (DeviceCount > 0) return;
            int deviceCount = GetDevices().Length;
            SyncContext.Post(DevicePollCallback, deviceCount);
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
        #endregion

        #region Commands
        public enum Commands : int 
        { 
            About = 0, 
            Stop, 
            StartUp, 
            StartDown, 
            Status, 
            Step, 
            StepBack, 
            ClearCounter, 
            TurnOff, 
            ToOrigin 
        };
        private static readonly char[] CommandChars = new char[] { '?', 'S', 'U', 'D', 'C', 'A', 'V', 'Z', 'E', 'O' };
        class Message
        {
            public Commands Command;
            public int Steps;
            public Message(Commands command, int steps = 0)
            {
                Command = command;
                Steps = steps;
            }

            public override string ToString()
            {
                return CommandChars[(int)Command] + string.Format("{0:X6}", Steps);
            }
        }

        public void SendCommandAsync(Commands command, int steps = 0)
        {
            lock (MessageQueue)
            {
                MessageQueue.Enqueue(new Message(command, steps));
            }
            SendARE.Set();            
        }

        public bool StopAtMagneticSwitch { get; set; }

        public void RotateTo(double angle)
        {
            int steps = (int)(angle / Math.PI) * PiSteps - PositionInSteps;
            if (steps > 0)
                SendCommandAsync(Turntable.Commands.Step, steps);
            else
                SendCommandAsync(Turntable.Commands.StepBack, -steps);
        }

        public void Rotate(int steps)
        {
            if (steps > 0)
                SendCommandAsync(Turntable.Commands.Step, steps);
            else
                SendCommandAsync(Turntable.Commands.StepBack, -steps);
        }

        public void RotateTo(int step)
        {
            int steps = step - PositionInSteps;
            Rotate(steps);
        }

        public void ToOrigin()
        {
            SendCommandAsync(Commands.ToOrigin);
        }

        public void TurnOnce()
        {
            SendCommandAsync(Commands.ClearCounter);
            SendCommandAsync(Turntable.Commands.Step, 2 * PiSteps);
        }

        public void Rotate()
        {
            SendCommandAsync(Commands.StartUp);
        }

        public void ResetCounter()
        {
            SendCommandAsync(Commands.ClearCounter);
        }

        public void Stop()
        {
            SendCommandAsync(Commands.Stop);
        }

        public void TurnOff()
        {
            SendCommandAsync(Commands.TurnOff);
        }
        #endregion

        #region State
        //Position
        public int PiSteps { get; set; }
        public int PositionInSteps { get; private set; }
        public double PositionInRadians { get { return (double)PositionInSteps / PiSteps * Math.PI; } }
        public double PositionInUnits { get { return (double)PositionInSteps / PiSteps / 2d; } }
        public double PositionInDegrees { get { return PositionInSteps * 360d / PiSteps / 2d; } }
        
        //Events
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

        //Other
        [Flags]
        private enum States : ushort { None = 0, Rotating = 0x0100, RotatingUp = 0x0200, MagneticSwitch = 0x0004 }
        public bool Rotating { get; private set; }
        public bool RotationDirection { get; private set; }
        public bool MagneticSwitch { get; private set; }
        public bool AtOrigin { get; private set; }
        public int StepCount { get; private set; }
        public string StatusWord { get; private set; }
        #endregion

        #region Communication
        private SerialPort Port;
        private bool CommunicationThreadOn;
        private Thread CommunicationThread;
        private Queue<Message> MessageQueue;
        private AutoResetEvent SendARE;
        public int PositionRefreshPeriod { get; set; }

        public Turntable(string portName)
        {
            PiSteps = 10989;
            Port = new SerialPort();
            Port.PortName = portName;
            InitPort(Port);
            Port.Open();
            OpenedDevices++;
            AtOrigin = false;

            MessageQueue = new Queue<Message>();
            SendARE = new AutoResetEvent(true);
            PositionRefreshPeriod = 10;
            StopAtMagneticSwitch = false;

            CommunicationThreadOn = true;
            CommunicationThread = new Thread(Communicate);
            CommunicationThread.Start();
            SendCommandAsync(Commands.Stop);
            SendCommandAsync(Commands.TurnOff);
        }

        private void Communicate()
        {
            Message message;
            string command, answer;
            bool rotating;
            States statusWord;
            Commands executingCommand = Commands.About;
            int targetStep = 0, origin = 0;
            int lastPositionInSteps = -1;
            try
            {
                while (CommunicationThreadOn)
                {
                    SendARE.WaitOne(PositionRefreshPeriod);
                    while (MessageQueue.Count > 0)
                    {
                        lock (MessageQueue) message = MessageQueue.Dequeue();
                        switch (message.Command)
                        {
                            case Commands.StartDown:
                            case Commands.StartUp:
                                Port.Write(CommandChars[(int)Commands.ClearCounter].ToString());
                                Port.ReadLine();
                                break;
                        }
                        executingCommand = message.Command;
                        Port.Write(message.ToString());
                        switch (message.Command)
                        {
                            case Commands.StartDown:
                            case Commands.StartUp:
                            case Commands.Step:
                            case Commands.StepBack:
                            case Commands.ToOrigin:
                                AtOrigin = false;
                                break;
                            case Commands.ClearCounter:
                                origin = PositionInSteps = 0;
                                break;
                        }

                        Port.ReadLine();
                        origin = PositionInSteps;
                        switch (message.Command)
                        {
                            case Commands.ToOrigin:
                                AtOrigin = true;
                                SendCommandAsync(Commands.ClearCounter);
                                origin = PositionInSteps = 0;
                                SyncContext.Post(MotorStoppedCallback, null);
                                break;
                            case Commands.Step:
                            case Commands.StepBack:
                                targetStep = message.Steps;
                                break;
                        }

                    }
                    Port.Write(CommandChars[(int)Commands.Status].ToString());
                    answer = Port.ReadLine();

                    StatusWord = answer;
                    statusWord = (States)Convert.ToInt16(answer.Replace(" ", "").Substring(0, 4), 16);
                    rotating = statusWord.HasFlag(States.Rotating);
                    MagneticSwitch = statusWord.HasFlag(States.MagneticSwitch);
                    RotationDirection = statusWord.HasFlag(States.RotatingUp);

                    int steps = Convert.ToInt32(answer.Substring(6, 6), 16);
                    if (steps > 32767) steps = (65536 - steps);

                    switch (executingCommand)
                    {
                        case Commands.Step:
                            steps = targetStep - steps + origin;
                            break;
                        case Commands.StepBack:
                            steps = targetStep - steps + origin;
                            break;
                        case Commands.StartUp:
                            steps = steps + origin;
                            break;
                        case Commands.StartDown:
                            steps = -steps + origin;
                            break;
                        default:
                            steps = origin;
                            break;
                    }

                    lastPositionInSteps = PositionInSteps;
                    PositionInSteps = steps;

                    if (PositionChanged != null && lastPositionInSteps != PositionInSteps) PositionChanged(this, null);
                    if (!MagneticSwitch && StopAtMagneticSwitch)
                    {
                        SendCommandAsync(Commands.Stop);
                        SendARE.Set();
                        SyncContext.Post(TurnCompleteCallback, null);
                    }

                    if (Rotating && !rotating)
                    {
                        SyncContext.Post(MotorStoppedCallback, null);
                        executingCommand = Commands.About;
                        origin = PositionInSteps;
                        Port.Write(CommandChars[(int)Commands.ClearCounter].ToString());
                        Port.ReadLine();
                    }
                    Rotating = rotating;
                }
                Port.Write(CommandChars[(int)Commands.Stop].ToString());
                Port.Write(CommandChars[(int)Commands.TurnOff].ToString());
            }
            catch
            {
            }
            Port.Dispose();
            Port = null;
            OpenedDevices--;
        }

        bool IsDisposed = false;
        public void Dispose()
        {
            IsDisposed = true;
            CommunicationThreadOn = false;
            SendARE.Set();
            CommunicationThread.Join(200);
        }

        ~Turntable()
        {
            if (!IsDisposed) Dispose();
        }
        #endregion
    }
}
