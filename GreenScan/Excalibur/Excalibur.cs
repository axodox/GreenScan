#define Win
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Net;
using System.Net.Sockets;
using System.IO;
using System.Threading;
using System.Xml;
using System.ComponentModel;
using System.Collections.ObjectModel;

namespace Excalibur
{
    public class ExcaliburSeeker: IDisposable
    {
        private const int TIMEOUT = 2000;
        Timer DiscoveryTimer;
        int Port;
        long ProtocolIdentifier;
        Socket SeekerSocket;
        EndPoint BroadcastEndPoint;
        byte[] SeekPacket;
        bool DiscoveryEnabled;
        Thread DiscoveryThread;
        SynchronizationContext SyncContext;
        public ExcaliburSeeker(int port, long protocolIdentifier)
        {
            if (SynchronizationContext.Current != null)
            {
                SyncContext = SynchronizationContext.Current;
            }
            else
            {
                SyncContext = new SynchronizationContext();
            }
            Port = port;
            ProtocolIdentifier = protocolIdentifier;
            SeekerSocket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
            SeekerSocket.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.Broadcast, true);
            SeekerSocket.SetSocketOption(SocketOptionLevel.IP, SocketOptionName.IpTimeToLive, 10);
            SeekerSocket.ReceiveTimeout = TIMEOUT;
            SeekerSocket.SendTimeout = TIMEOUT;
            BroadcastEndPoint = new IPEndPoint(IPAddress.Broadcast, Port);
            SeekPacket = BitConverter.GetBytes(ProtocolIdentifier);
            DiscoveryTimer = new Timer(Seek, null, 1000, 1000);
            DiscoveryEnabled = true;
            DiscoveryThread = new Thread(DiscoveryWorker);
            DiscoveryThread.Start();
        }

        private void Seek(object o)
        {
            SeekerSocket.SendTo(SeekPacket, BroadcastEndPoint);
        }

        private void DiscoveryWorker()
        {
            Thread.CurrentThread.Name = "Discovery on: " + Port.ToString();
            byte[] buffer = new byte[12];
            EndPoint remoteEndPoint = new IPEndPoint(IPAddress.Any, 0);
            int bytes;
            while (DiscoveryEnabled)
            {
                try
                {
                    bytes=SeekerSocket.ReceiveFrom(buffer, ref remoteEndPoint);
                    if (BitConverter.ToInt64(buffer, 0) == ProtocolIdentifier)
                    {
                        if (bytes == 10) (remoteEndPoint as IPEndPoint).Port = BitConverter.ToInt16(buffer, 8);
                        SyncContext.Post(EndPointFoundCallback, remoteEndPoint);
                        
                    }
                }
                catch
                {

                }
            }
            SeekerSocket.Dispose();
        }

        public class EndPointFoundEventArgs : EventArgs
        {
            public IPEndPoint EndPoint;
            public EndPointFoundEventArgs(IPEndPoint endPoint)
            {
                EndPoint = endPoint;
            }
        }
        public delegate void EndPointFoundEventHandler(object sender, EndPointFoundEventArgs e);
        public event EndPointFoundEventHandler EndPointFound;
        private void EndPointFoundCallback(object o)
        {
            if (EndPointFound != null) EndPointFound(this, new EndPointFoundEventArgs(o as IPEndPoint));
        }


        bool Disposed = false;
        public void Dispose()
        {
            if (!Disposed)
            {
                DiscoveryEnabled = false;
                Disposed = true;
                DiscoveryTimer.Dispose();
            }
        }
    }

    public class ExcaliburServer
    {
        private const int TIMEOUT = 2000;
        
        SynchronizationContext SyncContext;
        Socket ListenerSocket, AnnounceSocket;
        Thread ListenerThread, AnnounceThread;
        bool ListeningEnabled;
        long ProtocolIdentifier;
        public List<ExcaliburClient> Clients;
        public int Timeout
        {
            get
            {
                return ListenerSocket.ReceiveTimeout;
            }
            set
            {
                ListenerSocket.ReceiveTimeout = value;
            }
        }
        public ExcaliburServer(int port, long protocolIdentifier, bool discovery = true)
        {
            if (SynchronizationContext.Current != null)
            {
                SyncContext = SynchronizationContext.Current;
            }
            else
            {
                SyncContext = new SynchronizationContext();
            }
            ListenerSocket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            ListenerSocket.Bind(new IPEndPoint(IPAddress.Any, port));
            ListeningEnabled = true;
            ProtocolIdentifier = protocolIdentifier;
            Timeout = TIMEOUT;
            Clients = new List<ExcaliburClient>();
            ListenerThread = new Thread(ListeningWorker);
            ListenerThread.Start();

            AnnounceSocket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
            AnnounceSocket.Bind(new IPEndPoint(IPAddress.Any, port));
            AnnounceSocket.ReceiveTimeout = Timeout;
            AnnounceThread = new Thread(AnnounceWorker); 
            AnnounceThread.Start();
        }

        public void SendMessageToAll(string text, int id = -1)
        {
            foreach (ExcaliburClient client in Clients)
            {
                try
                {
                    client.SendMessage(text, id);
                }
                catch
                {

                }
            }
        }

        public void DisconnectAll()
        {
            foreach (ExcaliburClient client in Clients)
            {
                client.Disconnect();
            }
        }

        public int Port
        {
            get
            {
                return (ListenerSocket.LocalEndPoint as IPEndPoint).Port;
            }
        }

        public void StopListening()
        {
            ListeningEnabled = false;
            ListenerSocket.Close();
        }

        private void ListeningWorker()
        {
            Thread.CurrentThread.Name = "Listening on: " +ListenerSocket.LocalEndPoint.ToString();
            while (ListeningEnabled)
            {
                try
                {
                    ListenerSocket.Listen(1);
                    Socket ClientSocket = ListenerSocket.Accept();
                    ClientSocket.ReceiveTimeout = ClientSocket.SendTimeout = Timeout;
                    byte[] identifier = new byte[8];
                    ClientSocket.Receive(identifier);
                    if (BitConverter.ToInt64(identifier, 0) == ProtocolIdentifier)
                    {
                        ClientSocket.Send(identifier);
                        ExcaliburClient EC = new ExcaliburClient(ClientSocket, ProtocolIdentifier, SyncContext);
                        EC.Disconnected += new ExcaliburClient.DisconnectEventHandler(EC_Disconnected);
                        Clients.Add(EC);
                        SyncContext.Post(ClientConnectedCallback, EC);
                    }
                    else
                    {
                        ClientSocket.Close();
                    }
                }
                catch
                {

                }
            }
            ListenerSocket.Dispose();
        }

        public IPAddress[] GetLocalIPAddresses()
        {
            List<IPAddress> addresses = new List<IPAddress>();
            addresses.AddRange(Dns.GetHostAddresses(Dns.GetHostName()));
            int i = 0;
            while (i < addresses.Count)
            {
                if (addresses[i].AddressFamily != AddressFamily.InterNetwork || addresses[i] == IPAddress.Loopback)
                {
                    addresses.RemoveAt(i);
                }
                else i++;
            }
            return addresses.ToArray();
        }

        private void AnnounceWorker()
        {
            Thread.CurrentThread.Name = "Announcing on: " + ListenerSocket.LocalEndPoint.ToString();
            byte[] buffer=new byte[8];
            
            EndPoint remoteEndPoint = new IPEndPoint(IPAddress.Any, 0);
            while (ListeningEnabled)
            {
                try
                {
                    AnnounceSocket.ReceiveFrom(buffer, ref remoteEndPoint);
                    if (BitConverter.ToInt64(buffer, 0) == ProtocolIdentifier)
                    {
                        AnnounceSocket.SendTo(buffer, remoteEndPoint);
                    }
                }
                catch
                {

                }
            }
            AnnounceSocket.Dispose();
        }

        void EC_Disconnected(object sender, ExcaliburClient.DisconnectEventArgs e)
        {
            if (ClientDisconnected != null)
            {
                ClientDisconnected(this, new ClientEventArgs(sender as ExcaliburClient));
            }
        }

        private void ClientConnectedCallback(object o)
        {
            if (ClientConnected != null)
            {
                ClientConnected(this, new ClientEventArgs(o as ExcaliburClient));
            }
        }

        public class ClientEventArgs : EventArgs
        {
            public ExcaliburClient Client;
            public ClientEventArgs(ExcaliburClient client)
            {
                Client = client;
            }
        }
        public delegate void ClientEventHandler(object sender, ClientEventArgs e);
        public event ClientEventHandler ClientConnected, ClientDisconnected;
    }

    public class ExcaliburClient
    {
        private const int TIMEOUT = int.MaxValue;
        private const int PACKETSIZE = 4096;
        private const byte FRAMEDELIMITER = 170;
        Socket ClientSocket;
        long ProtocolIdentifier;
        Thread ReceiveThread, SendThread;
        public IPEndPoint RemoteEndPoint { get; private set; }
        SynchronizationContext SyncContext;
        int NextSendID = 0;
        Dictionary<int, Packet> PacketsSending, PacketsReceiving;
        Queue<Packet> PacketsToSend;
        AutoResetEvent SendARE, ConnectARE, NodeARE;
        enum MessageTypes : byte { Header = 254, Data = 1, AcceptPacket = 2, KeepAlive = 3, CancelBySender = 4, CancelByReceiver = 5, AnnounceNode = 6, ReadNode = 7, WriteNode = 8, NodeValue = 9, TextMessage=32, Close = 127, Footer = 255 };
        Timer KeepAliveTimer;
        bool AnnounceInProgress;
        public bool IsConnected { get; private set; }
        public int Timeout
        {
            get
            {
                if (IsConnected)
                    return ClientSocket.ReceiveTimeout;
                else
                    return 0;
            }
            set
            {
                if (IsConnected)
                {
                    ClientSocket.ReceiveTimeout = value;
                    KeepAliveTimer.Change(value / 4, value / 2);
                }
            }
        }
        public int PacketSize { get; set; }
        public ExcaliburClient(IPEndPoint clientEndPoint, long protocolIdentifier)
        {
            ClientSocket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            ProtocolIdentifier = protocolIdentifier;
            RemoteEndPoint = clientEndPoint;
            CommonInit();
            ReceiveThread = new Thread(ConnectAndReceiveWorker);
            ReceiveThread.Start();
        }

        void ConnectAndReceiveWorker()
        {
            try
            {
                ClientSocket.Connect(RemoteEndPoint);
                ClientSocket.Send(BitConverter.GetBytes(ProtocolIdentifier));
                byte[] identifier = new byte[8];
                ClientSocket.Receive(identifier);
                if (BitConverter.ToInt64(identifier, 0) == ProtocolIdentifier)
                {
                    ReceiveWorker();
                }
            }
            catch
            {
                DisconnectType = DisconnectTypes.CannotConnect;
                FreeResources();
            }
        }

        internal ExcaliburClient(Socket clientSocket, long protocolIdentifier, SynchronizationContext sc)
        {
            ProtocolIdentifier = protocolIdentifier;
            RemoteEndPoint = clientSocket.LocalEndPoint as IPEndPoint;
            ClientSocket = clientSocket;
            SyncContext = sc;
            CommonInit();
            ReceiveThread = new Thread(ReceiveWorker);
            ReceiveThread.Start();
        }

        void CommonInit()
        {
            if (SyncContext == null)
            {
                if (SynchronizationContext.Current != null)
                {
                    SyncContext = SynchronizationContext.Current;
                }
                else
                {
                    SyncContext = new SynchronizationContext();
                }
            }
            Nodes = new ObservableCollection<ExcaliburNode>();
            RemoteNodes = new ObservableCollection<ExcaliburNode>();
            IsConnected = true;
            ClientSocket.Blocking = true;
            ClientSocket.NoDelay = false;
            KeepAliveTimer = new Timer(KeepAlive, null, TIMEOUT / 2, TIMEOUT / 2);
            Timeout = TIMEOUT;
            PacketSize = PACKETSIZE;
            DisconnectType = DisconnectTypes.Unknown;
            PacketsSending = new Dictionary<int, Packet>();
            PacketsReceiving = new Dictionary<int, Packet>();
            PacketsToSend = new Queue<Packet>();
            SendARE = new AutoResetEvent(false);
            ConnectARE = new AutoResetEvent(false);
            SendThread = new Thread(SendWorker);
            SendThread.Start();
            AnnounceInProgress = false;
            NodeARE = new AutoResetEvent(false);
            foreach (ExcaliburNode node in Nodes)
            {
                AnnounceNode(node);
            }
        }

        void KeepAlive(object o)
        {
            if (SendARE != null)
                SendARE.Set();
        }

        public class PacketEventArgs : EventArgs
        {
            public Packet P;
            public PacketEventArgs(Packet p)
            {
                P = p;
            }
        }
        public delegate void PacketEventHandler(object sender, PacketEventArgs e);
        public event PacketEventHandler ReceivingStarted, FragmentReceived, ReceivingFinished, ReceivingCancelledBySender, SendCancelledByReceiver, PacketSent;

        struct PacketEventProperties
        {
            public PacketEventTypes PacketEventHandler;
            public PacketEventArgs PacketEventArgs;
            public PacketEventProperties(PacketEventTypes eventHandler, PacketEventArgs eventArgs)
            {
                PacketEventHandler = eventHandler;
                PacketEventArgs = eventArgs;
            }
        }

        enum PacketEventTypes { ReceivingStarted, FragmentReceived, ReceivingFinished, ReceivingCancelledBySender, SendCancelledByReceiver, Disconnected, PacketSent };
        void RaisePacketEventAsync(PacketEventTypes eventType, PacketEventArgs e)
        {
            SyncContext.Post(RaisePacketEventCallback, new PacketEventProperties(eventType, e));
        }

        public enum DisconnectTypes { Direct, Indirect, Unknown, CannotConnect };
        DisconnectTypes DisconnectType;
        public class DisconnectEventArgs : EventArgs
        {
            public DisconnectTypes DisconnectType;
            public DisconnectEventArgs(DisconnectTypes type)
            {
                DisconnectType = type;
            }
        }
        public delegate void DisconnectEventHandler(object sender, DisconnectEventArgs e);
        public event DisconnectEventHandler Disconnected;

        void RaisePacketEventCallback(object o)
        {
            PacketEventProperties PEP = (PacketEventProperties)o;
            PacketEventHandler eventHandler;
            switch (PEP.PacketEventHandler)
            {
                case PacketEventTypes.ReceivingStarted:
                    eventHandler = ReceivingStarted;
                    break;
                case PacketEventTypes.FragmentReceived:
                    eventHandler = FragmentReceived;
                    break;
                case PacketEventTypes.ReceivingFinished:
                    if (ReceivingFinished == null)
                    {
                        PEP.PacketEventArgs.P.Data.Dispose();
                    }
                    eventHandler = ReceivingFinished;
                    break;
                case PacketEventTypes.ReceivingCancelledBySender:
                    eventHandler = ReceivingCancelledBySender;
                    break;
                case PacketEventTypes.SendCancelledByReceiver:
                    eventHandler = SendCancelledByReceiver;
                    break;
                case PacketEventTypes.PacketSent:
                    eventHandler = PacketSent;
                    break;
                default:
                    eventHandler = null;
                    break;
            }
            if (eventHandler != null)
            {
                eventHandler(this, PEP.PacketEventArgs);
            }
            switch (PEP.PacketEventHandler)
            {
                case PacketEventTypes.ReceivingStarted:
                    if (PEP.PacketEventArgs.P.Data == null)
                    {
                        PEP.PacketEventArgs.P.Data = new MemoryStream((int)PEP.PacketEventArgs.P.Length);
                    }
                    Packet P = new Packet();
                    P.InternalID = NextSendID++;
                    P.ID = PEP.PacketEventArgs.P.InternalID;
                    P.Type = Packet.Types.AcceptPacket;
                    PEP.PacketEventArgs.P.State |= Packet.States.Handshake;
                    lock (PacketsToSend)
                    {
                        PacketsToSend.Enqueue(P);
                    }
                    SendARE.Set();
                    break;
            }
        }

        bool DisconnectReported = false;
        void DisconnectCallback(object o)
        {
            if (Disconnected != null && !DisconnectReported)
            {
                DisconnectReported = true;
                Disconnected(this, new DisconnectEventArgs(DisconnectType));
            }
        }

        void ReceiveWorker()
        {
            Thread.CurrentThread.Name = "Receive from " + RemoteEndPoint.ToString();
            byte[] lengthBuffer = new byte[5];
            byte[] buffer = new byte[PacketSize];
            MemoryStream bufferStream = new MemoryStream(buffer);
            BinaryReader bufferReader = new BinaryReader(bufferStream, ASCIIEncoding.ASCII);
            int internalID, length, id;
            NodeChangedData NCD;
            MessageTypes messageType;
            Packet P;
            ConnectARE.Set();
            while (IsConnected)
            {
                try
                {
                    ClientSocket.Read(lengthBuffer, 5);
                }
                catch
                {
                    break;
                }
                if (lengthBuffer[0] != FRAMEDELIMITER)
                    throw new Exception();
                length = BitConverter.ToInt32(lengthBuffer, 1);
                if (length == 0)
                    throw new Exception();
                if (buffer.Length < length)
                {
                    buffer = new byte[length];
                    bufferStream = new MemoryStream(buffer);
                    bufferReader = new BinaryReader(bufferStream, ASCIIEncoding.ASCII);
                }
                else
                {
                    bufferStream.Position = 0;
                }
                try
                {
                    ClientSocket.Read(buffer, length);
                }
                catch
                {
                    break;
                }
                messageType = (MessageTypes)bufferReader.ReadByte();
                internalID = bufferReader.ReadInt32();
                switch (messageType)
                {
                    case MessageTypes.Header:
                        P = new Packet();
                        P.InternalID = internalID;
                        P.ID = bufferReader.ReadInt32();
                        P.Length = bufferReader.ReadInt64();
                        P.State = Packet.States.Header;
                        PacketsReceiving.Add(P.InternalID, P);
                        RaisePacketEventAsync(PacketEventTypes.ReceivingStarted, new PacketEventArgs(P));
                        break;
                    case MessageTypes.Data:
                        P = PacketsReceiving[internalID];
                        P.Data.Write(buffer, (int)bufferStream.Position, length - (int)bufferStream.Position);
                        P.Position = P.Data.Position;
                        if ((P.Settings & Packet.Options.ShowFragments) == Packet.Options.ShowFragments)
                        {
                            RaisePacketEventAsync(PacketEventTypes.FragmentReceived, new PacketEventArgs(P));
                        }
                        break;
                    case MessageTypes.Footer:
                        P = PacketsReceiving[internalID];
                        P.State |= Packet.States.Data | Packet.States.Footer;
                        RaisePacketEventAsync(PacketEventTypes.ReceivingFinished, new PacketEventArgs(P));
                        PacketsReceiving.Remove(internalID);
                        break;
                    case MessageTypes.AcceptPacket:
                        id = bufferReader.ReadInt32();
                        PacketsSending[id].State |= Packet.States.Handshake;
                        PacketsSending[id].Processing = true;
                        SendARE.Set();
                        break;
                    case MessageTypes.CancelBySender:
                        if (PacketsReceiving.ContainsKey(internalID))
                        {
                            P = PacketsReceiving[internalID];
                            P.State |= Packet.States.CancelledBySender;
                            RaisePacketEventAsync(PacketEventTypes.ReceivingCancelledBySender, new PacketEventArgs(P));
                            PacketsReceiving.Remove(internalID);
                        }
                        break;
                    case MessageTypes.CancelByReceiver:
                        id = bufferReader.ReadInt32();
                        if (PacketsSending.ContainsKey(id))
                        {
                            P = PacketsSending[id];
                            P.State |= Packet.States.CancelledByReceiver | Packet.States.StreamComplete;
                            RaisePacketEventAsync(PacketEventTypes.SendCancelledByReceiver, new PacketEventArgs(P));
                        }
                        break;
                    case MessageTypes.AnnounceNode:
                        id = bufferReader.ReadInt32();
                        ExcaliburNode.NodeTypes nodeType = (ExcaliburNode.NodeTypes)bufferReader.ReadByte();
                        ExcaliburNode.AccessModes accessMode = (ExcaliburNode.AccessModes)bufferReader.ReadByte();
                        string name = bufferReader.ReadString();
                        NCD = new NodeChangedData();
                        NCD.ID = id;
                        NCD.Task = NodeChangedData.Tasks.NewNode;
                        NCD.Node = new ExcaliburNode(name, nodeType, accessMode, null, id);
                        AnnounceInProgress = true;
                        SyncContext.Post(NodeChangedCallback, NCD);
                        break;
                    case MessageTypes.ReadNode:
                        id = bufferReader.ReadInt32();
                        SendNodeValue(id);
                        break;
                    case MessageTypes.WriteNode:
                        id = bufferReader.ReadInt32();
                        if (Nodes.Count>id && id>=0)
                        {
                            Nodes[id].ReadValue(bufferReader);
                            RaiseNodeEventHandlerAsync(NodeWrite, new NodeEventArgs(Nodes[id]));
                        }
                        break;
                    case MessageTypes.NodeValue:
                        if (AnnounceInProgress) NodeARE.WaitOne();
                        id = bufferReader.ReadInt32();
                        if (RemoteNodes.Count > id && id >= 0)
                        {
                            RemoteNodes[id].ReadValue(bufferReader);
                            RaiseNodeEventHandlerAsync(NodeRead, new NodeEventArgs(RemoteNodes[id]));
                        }                        
                        break;
                    case MessageTypes.TextMessage:
                        string text = bufferReader.ReadString();
                        SyncContext.Post(MessageReceivedCallback, new object[] { text, internalID });
                        break;
                    case MessageTypes.Close:
                        DisconnectType = DisconnectTypes.Indirect;
                        IsConnected = false;
                        SendARE.Set();
                        break;
                }
            }
            FreeResources();
            bufferStream.Dispose();
            bufferReader.Dispose();            
        }

        public class MessageEventArgs : EventArgs
        {
            public int Id { get; private set; }
            public string Text { get; private set; }
            public MessageEventArgs(string text, int id)
            {
                Id = id;
                Text = text;
            }
        }
        public delegate void MessageEventHandler(object sender, MessageEventArgs e);
        public event MessageEventHandler MessageReceived;
        private void MessageReceivedCallback(object o)
        {
            if (MessageReceived != null)
            {
                object[] args = (object[])o; 
                MessageReceived(this, new MessageEventArgs((string)args[0], (int)args[1]));
            }
        }

        class NodeChangedData
        {
            public enum Tasks { None, NewNode };
            public Tasks Task;
            public int ID;
            public ExcaliburNode Node;
        }

        void NodeChangedCallback(object o)
        {
            NodeChangedData NCD = o as NodeChangedData;
            switch (NCD.Task)
            {
                case NodeChangedData.Tasks.NewNode:
                    if (RemoteNodes.Count > NCD.ID) RemoteNodes[NCD.ID] = NCD.Node;
                    else if (NCD.ID == RemoteNodes.Count) RemoteNodes.Add(NCD.Node);
                    else throw new InvalidOperationException();
                    if(NodeAnnounced!=null) NodeAnnounced(this,new NodeEventArgs(NCD.Node));
                    AnnounceInProgress = false;
                    NodeARE.Set();
                    break;
            }
        }

        private void FreeResources()
        {
            IsConnected = false;
            foreach (Packet p in PacketsReceiving.Values)
            {
                if (p.Data != null)
                {
                    p.Data.Dispose();
                }
            }
            PacketsReceiving.Clear();
            PacketsReceiving = null;            
            if (DisconnectType == DisconnectTypes.Direct)
            {
                ClientSocket.Dispose();
                ClientSocket = null;
            }
            if (SendARE != null) SendARE.Set();            
            SyncContext.Post(DisconnectCallback, null);
            KeepAliveTimer.Dispose();
        }

        public void CancelReceiveOperation(int id)
        {
            if (IsConnected && PacketsReceiving.ContainsKey(id))
            {
                Packet P = new Packet();
                P.ID = id;
                P.Type = Packet.Types.CancelStream;
                P.InternalID = NextSendID++;
                lock (PacketsToSend)
                {
                    PacketsToSend.Enqueue(P);
                }
                SendARE.Set();
            }
        }

        public void CancelSendOperation(int id)
        {
            if (IsConnected && PacketsSending.ContainsKey(id))
            {
                PacketsSending[id].State |= Packet.States.CancelledBySender;
                SendARE.Set();
            }
        }

        public class ClientDisconnectedException : Exception { }
        public void Disconnect()
        {
            if (IsConnected)
            {
                IsConnected = false;
                DisconnectType = DisconnectTypes.Direct;
                SendARE.Set();
            }
        }

        public int Send(int ID, Stream stream, long length)
        {
            if (IsConnected)
            {
                Packet P = new Packet();
                P.InternalID = NextSendID++;
                P.Data = stream;
                P.ID = ID;
                P.Length = length;
                P.Type = Packet.Types.Stream;
                lock (PacketsToSend)
                {
                    PacketsToSend.Enqueue(P);
                }
                SendARE.Set();
                return P.InternalID;
            }
            else
                throw new ClientDisconnectedException();
        }

        public int SendMessage(string text, int id = -1)
        {
            if (IsConnected)
            {
                Packet P = new Packet();
                P.InternalID = NextSendID++;
                P.Text = text;
                P.ID = id;
                P.Type = Packet.Types.Text;
                lock (PacketsToSend)
                {
                    PacketsToSend.Enqueue(P);
                }
                SendARE.Set();
                return P.InternalID;
            }
            else
                throw new ClientDisconnectedException();
        }


        private void SendWorker()
        {
            Thread.CurrentThread.Name = "Send to " + RemoteEndPoint.ToString();
            byte[] buffer = new byte[PacketSize];
            MemoryStream bufferStream = new MemoryStream(buffer);
            BinaryWriter bufferWriter = new BinaryWriter(bufferStream, ASCIIEncoding.ASCII);
            int dataLength, packetsToProcess;
            Packet P;
            Packet[] Packets;
            ConnectARE.WaitOne();
            ConnectARE.Dispose();
            ConnectARE = null;
            while (true)
            {
                SendARE.WaitOne();
                if (buffer.Length != PacketSize)
                {
                    buffer = new byte[PacketSize];
                }

                packetsToProcess = 1;
                while (packetsToProcess > 0)
                {
                    if (!IsConnected)
                    {
                        if (DisconnectType == DisconnectTypes.Direct)
                        {
                            bufferStream.Position = 0L;
                            bufferWriter.Write(FRAMEDELIMITER);
                            bufferWriter.Write(5);
                            bufferWriter.Write((byte)MessageTypes.Close);
                            bufferWriter.Write(-1);
                            try
                            {
                                ClientSocket.Send(buffer, 10, SocketFlags.None);
                            }
                            catch
                            {

                            }
                            ClientSocket.Disconnect(false);
                            ClientSocket.Close();
                        }
                        else
                        {
                            ClientSocket.Dispose();
                            ClientSocket = null;
                        }
                        bufferStream.Dispose();
                        bufferWriter.Close();
                        PacketsToSend.Clear();
                        PacketsSending.Clear();
                        PacketsToSend = null;
                        PacketsSending = null;
                        SendARE.Dispose();
                        SendARE = null;
                        KeepAliveTimer.Dispose();
                        SyncContext.Post(DisconnectCallback, null);
                        return;
                    }

                    lock (PacketsToSend)
                    {
                        while (PacketsToSend.Count > 0)
                        {
                            P = PacketsToSend.Dequeue();
                            PacketsSending.Add(P.InternalID, P);
                        }
                    }

                    Packets = PacketsSending.Values.ToArray<Packet>();
                    packetsToProcess = 0;
                    for (int i = 0; i < Packets.Length; i++)
                    {
                        P = Packets[i];
                        bufferStream.Position = 0L;
                        bufferWriter.Write(FRAMEDELIMITER);
                        bufferStream.Position = 5L;
                        switch (P.Type)
                        {
                            case Packet.Types.Stream:
                                if ((P.State & Packet.States.CancelledBySender) == Packet.States.CancelledBySender)
                                {
                                    bufferWriter.Write((byte)MessageTypes.CancelBySender);
                                    bufferWriter.Write(P.InternalID);
                                    P.Processing = false;
                                    P.State |= Packet.States.StreamComplete;
                                }
                                else
                                {
                                    switch (P.State)
                                    {
                                        case Packet.States.None:
                                            bufferWriter.Write((byte)MessageTypes.Header);
                                            bufferWriter.Write(P.InternalID);
                                            bufferWriter.Write(P.ID);
                                            bufferWriter.Write(P.Length);
                                            P.State = Packet.States.Header;
                                            P.Processing = false;
                                            break;
                                        case Packet.States.SendingData:
                                            bufferWriter.Write((byte)MessageTypes.Data);
                                            bufferWriter.Write(P.InternalID);
                                            if (P.Position != P.Data.Position)
                                            {
                                                P.Data.Position = P.Position;
                                            }
                                            int sentBytes = P.Data.Read(buffer, (int)bufferStream.Position, PacketSize - (int)bufferStream.Position);
                                            P.Position += sentBytes;
                                            bufferStream.Position += sentBytes;
                                            if (P.Position >= P.Length)
                                            {
                                                P.State |= Packet.States.Data;
                                            }
                                            break;
                                        case Packet.States.DataComplete:
                                            bufferWriter.Write((byte)MessageTypes.Footer);
                                            bufferWriter.Write(P.InternalID);
                                            P.Processing = false;
                                            P.State |= Packet.States.Footer;
                                            break;
                                    }
                                }
                                if ((P.State & Packet.States.StreamComplete) == Packet.States.StreamComplete)
                                {
                                    PacketsSending.Remove(P.InternalID);
                                    RaisePacketEventAsync(PacketEventTypes.PacketSent, new PacketEventArgs(P));
                                }
                                break;
                            case Packet.Types.AcceptPacket:
                                bufferWriter.Write((byte)MessageTypes.AcceptPacket);
                                bufferWriter.Write(P.InternalID);
                                bufferWriter.Write(P.ID);
                                P.Processing = false;
                                PacketsSending.Remove(P.InternalID);
                                break;
                            case Packet.Types.CancelStream:
                                bufferWriter.Write((byte)MessageTypes.CancelByReceiver);
                                bufferWriter.Write(P.InternalID);
                                bufferWriter.Write(P.ID);
                                P.Processing = false;
                                PacketsSending.Remove(P.InternalID);
                                break;
                            case Packet.Types.NodeAnnounce:
                                bufferWriter.Write((byte)MessageTypes.AnnounceNode);
                                bufferWriter.Write(P.InternalID);
                                bufferWriter.Write(P.Node.ID);
                                bufferWriter.Write((byte)P.Node.NodeType);
                                bufferWriter.Write((byte)P.Node.AccessMode);
                                bufferWriter.Write(P.Node.Name);
                                P.Processing = false;
                                PacketsSending.Remove(P.InternalID);
                                break;
                            case Packet.Types.NodeRead:
                                bufferWriter.Write((byte)MessageTypes.ReadNode);
                                bufferWriter.Write(P.InternalID);
                                bufferWriter.Write(P.Node.ID);
                                P.Processing = false;
                                PacketsSending.Remove(P.InternalID);
                                break;
                            case Packet.Types.NodeWrite:
                                bufferWriter.Write((byte)MessageTypes.WriteNode);   //0
                                bufferWriter.Write(P.InternalID);                   //1-4
                                bufferWriter.Write(P.Node.ID);                      //5-8
                                bufferWriter.Write(P.Node.GetByteValue());          //9..
                                P.Processing = false;
                                PacketsSending.Remove(P.InternalID);
                                break;
                            case Packet.Types.NodeValue:
                                bufferWriter.Write((byte)MessageTypes.NodeValue);
                                bufferWriter.Write(P.InternalID);
                                bufferWriter.Write(P.Node.ID);
                                bufferWriter.Write(P.Node.GetByteValue());
                                P.Processing = false;
                                PacketsSending.Remove(P.InternalID);
                                break;
                            case Packet.Types.Text:
                                bufferWriter.Write((byte)MessageTypes.TextMessage);
                                bufferWriter.Write(P.ID);
                                bufferWriter.Write(P.Text);
                                P.Processing = false;
                                PacketsSending.Remove(P.InternalID);
                                break;
                        }
                        if (bufferStream.Position != 5L)
                        {
                            dataLength = (int)bufferStream.Position;
                            bufferStream.Position = 1;
                            bufferWriter.Write(dataLength - 5);
                            bufferWriter.Flush();
                            if (IsConnected)
                            {
                                try
                                {
                                    ClientSocket.Send(buffer, dataLength, SocketFlags.None);
                                }
                                catch
                                {
                                    IsConnected = false;
                                    break;
                                }
                            }
                            else break;
                        }
                        if (P.Processing) packetsToProcess++;
                    }
                }

                if (IsConnected)
                {
                    bufferStream.Position = 0L;
                    bufferWriter.Write(FRAMEDELIMITER);
                    bufferWriter.Write(5);
                    bufferWriter.Write((byte)MessageTypes.KeepAlive);
                    bufferWriter.Write(-1);
                    try
                    {
                        ClientSocket.Send(buffer, 10, SocketFlags.None);
                    }
                    catch
                    {
                        IsConnected = false;
                    }
                }
            }
        }

        struct NodeEventProperties
        {
            public NodeEventHandler NodeEventHandler;
            public NodeEventArgs NodeEventArgs;
            public NodeEventProperties(NodeEventHandler eventHandler, NodeEventArgs eventArgs)
            {
                NodeEventHandler = eventHandler;
                NodeEventArgs = eventArgs;
            }
        }
        enum NodeEventTypes { NodeWrite, NodeRead };
        public class NodeEventArgs : EventArgs
        {
            public ExcaliburNode Node { get; private set; }
            public NodeEventArgs(ExcaliburNode node)
            {
                Node = node;
            }
        }
        public delegate void NodeEventHandler(object sender, NodeEventArgs e);
        public event NodeEventHandler NodeRead, NodeWrite, NodeAnnounced;
        private void RaiseNodeEventHandlerAsync(NodeEventHandler eventHandler, NodeEventArgs e)
        {
            SyncContext.Post(RaiseNodeEventHandlerCallback, new NodeEventProperties(eventHandler, e));
        }
        private void RaiseNodeEventHandlerCallback(object o)
        {
            NodeEventProperties NEP = (NodeEventProperties)o;
            
            if (NEP.NodeEventHandler != null) NEP.NodeEventHandler(this, NEP.NodeEventArgs);
        }
        public ObservableCollection<ExcaliburNode> Nodes { get; private set; }
        public ObservableCollection<ExcaliburNode> RemoteNodes { get; private set; }
        private int NextNodeID = 0;
        public void AddNode(ExcaliburNode node)
        {
            Nodes.Add(node);
            node.ValueChanged += node_ValueChanged;
            node.ID = NextNodeID;
            NextNodeID++;
            AnnounceNode(node);
        }

        void node_ValueChanged(object sender, EventArgs e)
        {
            SendNodeValue((sender as ExcaliburNode).ID);
        }

        private void AnnounceNode(ExcaliburNode node)
        {
            if (IsConnected)
            {
                Packet P = new Packet();
                P.InternalID = NextSendID++;
                P.ID = node.ID;
                P.Node = node;
                P.Type = Packet.Types.NodeAnnounce;
                lock (PacketsToSend)
                {
                    PacketsToSend.Enqueue(P);
                }
                SendARE.Set();
                SendNodeValue(node.ID);
            }
        }

        public bool ReadNode(int id)
        {
            if (IsConnected)
            {
                if (RemoteNodes[id].AccessMode.HasFlag(ExcaliburNode.AccessModes.Read))
                {
                    Packet P = new Packet();
                    P.InternalID = NextSendID++;
                    P.ID = id;
                    P.Node = RemoteNodes[id];
                    P.Type = Packet.Types.NodeRead;
                    lock (PacketsToSend)
                    {
                        PacketsToSend.Enqueue(P);
                    }
                    SendARE.Set();
                    return true;
                }
                else return false;
            }
            else
                throw new ClientDisconnectedException();
        }

        public void SendAllNodes()
        {
            for (int i = 0; i < Nodes.Count; i++) SendNodeValue(i);
        }

        public void QueryNodes()
        {
            for (int i = 0; i < RemoteNodes.Count; i++) ReadNode(i);
        }

        public bool SendNodeValue(int id)
        {
            if (IsConnected)
            {
                if (Nodes.Count > id && Nodes[id].AccessMode.HasFlag(ExcaliburNode.AccessModes.Read))
                {
                    Packet P = new Packet();
                    P.InternalID = NextSendID++;
                    P.ID = id;
                    P.Node = Nodes[id];
                    P.Type = Packet.Types.NodeValue;
                    lock (PacketsToSend)
                    {
                        PacketsToSend.Enqueue(P);
                    }
                    SendARE.Set();
                    return true;
                }
                else
                    return false;
            }
            else
                return false;
        }

        public bool WriteNode(int id, object value)
        {
            if (IsConnected)
            {
                if (RemoteNodes[id].AccessMode.HasFlag(ExcaliburNode.AccessModes.Write))
                {
                    Packet P = new Packet();
                    P.InternalID = NextSendID++;
                    P.ID = id;
                    P.Node = RemoteNodes[id];
                    P.Node.Value = value;
                    P.Type = Packet.Types.NodeWrite;
                    lock (PacketsToSend)
                    {
                        PacketsToSend.Enqueue(P);
                    }
                    SendARE.Set();
                    return true;
                }
                else return false;

            }
            else
                throw new ClientDisconnectedException();
        }

        public class Packet
        {
            internal bool Processing = true;
            internal enum Types : byte
            {
                Stream = 0,
                AcceptPacket,
                CancelStream,
                NodeAnnounce,
                NodeWrite,
                NodeRead,
                NodeValue,
                Text = 32
            }
            internal Types Type;
            [Flags]
            internal enum States : byte
            {
                None = 0,
                Header = 1,
                Data = 2,
                Footer = 4,
                Body = 7,
                Handshake = 8,
                SendingData = 9,
                DataComplete = 11,
                StreamComplete = 15,
                CancelledBySender = 16,
                CancelledByReceiver = 32
            };
            internal States State;
            [Flags]
            public enum Options : byte
            {
                None = 0,
                ShowFragments = 1
            }
            public Options Settings;
            public int ID;
            public int InternalID { get; internal set; }
            public Stream Data;
            public string Text;
            public ExcaliburNode Node;
            internal long Position, Length;
        }
    }

    public class ExcaliburNode : INotifyPropertyChanged
    {
        internal int ID;
        public string Name { get; private set; }
        public string Description { get; private set; }
        [Flags]
        public enum AccessModes : byte { None = 0, Read = 1, Write = 2, ReadWrite = 3 }
        public AccessModes AccessMode { get; private set; }
        public enum NodeTypes : byte { Boolean = 0, Byte, Int16, UInt16, Int32, UInt32, Int64, UInt64, Single, Double }
        public NodeTypes NodeType { get; private set; }
        public event EventHandler ValueChanged;
        private object value;
        public object Value 
        {
            get
            {
                return value;
            }
            set
            {
                this.value = value;
                OnPropertyChanged("Value");
                if (ValueChanged != null) ValueChanged(this, new EventArgs());
            }
        }

        public object TryParse(string text)
        {
            bool ok = false;
            object value = null;
            switch (NodeType)
            {
                case NodeTypes.Boolean:
                    Boolean booleanValue;
                    ok = Boolean.TryParse(text, out booleanValue);
                    value = booleanValue;
                    break;
                case NodeTypes.Byte:
                    Byte byteValue;
                    ok = Byte.TryParse(text, out byteValue);
                    value = byteValue;
                    break;
                case NodeTypes.Double:
                    Double doubleValue;
                    ok = Double.TryParse(text, out doubleValue);
                    value = doubleValue;
                    break;
                case NodeTypes.Int16:
                    Int16 int16Value;
                    ok = Int16.TryParse(text, out int16Value);
                    value = int16Value;
                    break;
                case NodeTypes.Int32:
                    Int32 int32Value;
                    ok = Int32.TryParse(text, out int32Value);
                    value = int32Value;
                    break;
                case NodeTypes.Int64:
                    Int64 int64Value;
                    ok = Int64.TryParse(text, out int64Value);
                    value = int64Value;
                    break;
                case NodeTypes.Single:
                    Single singleValue;
                    ok = Single.TryParse(text, out singleValue);
                    value = singleValue;
                    break;
                case NodeTypes.UInt16:
                    UInt16 uInt16Value;
                    ok = UInt16.TryParse(text, out uInt16Value);
                    value = uInt16Value;
                    break;
                case NodeTypes.UInt32:
                    UInt32 uInt32Value;
                    ok = UInt32.TryParse(text, out uInt32Value);
                    value = uInt32Value;
                    break;
                case NodeTypes.UInt64:
                    UInt64 uInt64Value;
                    ok = UInt64.TryParse(text, out uInt64Value);
                    value = uInt64Value;
                    break;
            }
            if (ok) return value;
            else return null;
        }

        private void OnPropertyChanged(string name)
        {
            if (PropertyChanged != null) PropertyChanged(this, new PropertyChangedEventArgs(name));
        }

        public ExcaliburNode(string name, NodeTypes type, AccessModes mode, object value, int id = -1)
        {
            Name = name;
            AccessMode = mode;
            NodeType = type;
            ID = id;
            Value = value;
        }

        public byte[] GetByteValue()
        {
            byte[] data = null;
            switch (NodeType)
            {
                case NodeTypes.Boolean:
                    data = new Byte[] { (Byte)((Boolean)Value ? 1 : 0) };
                    break;
                case NodeTypes.Byte:
                    data = new byte[1] { (Byte)Value };
                    break;
                case NodeTypes.Double:
                    data = BitConverter.GetBytes((Double)Value);
                    break;
                case NodeTypes.Int16:
                    data = BitConverter.GetBytes((Int16)Value);
                    break;
                case NodeTypes.Int32:
                    data = BitConverter.GetBytes((Int32)Value);
                    break;
                case NodeTypes.Int64:
                    data = BitConverter.GetBytes((Int64)Value);
                    break;
                case NodeTypes.Single:
                    data = BitConverter.GetBytes((Single)Value);
                    break;
                case NodeTypes.UInt16:
                    data = BitConverter.GetBytes((UInt16)Value);
                    break;
                case NodeTypes.UInt32:
                    data = BitConverter.GetBytes((UInt32)Value);
                    break;
                case NodeTypes.UInt64:
                    data = BitConverter.GetBytes((UInt64)Value);
                    break;
            }
            return data;
        }

        public void ReadValue(BinaryReader br)
        {
            object value=null;
            switch (NodeType)
            {
                case NodeTypes.Boolean:
                    value = br.ReadBoolean();
                    break;
                case NodeTypes.Byte:
                    value = br.ReadByte();
                    break;
                case NodeTypes.Double:
                    value = br.ReadDouble();
                    break;
                case NodeTypes.Int16:
                    value = br.ReadInt16();
                    break;
                case NodeTypes.Int32:
                    value = br.ReadInt32();
                    break;
                case NodeTypes.Int64:
                    value = br.ReadInt64();
                    break;
                case NodeTypes.Single:
                    value = br.ReadSingle();
                    break;
                case NodeTypes.UInt16:
                    value = br.ReadUInt16();
                    break;
                case NodeTypes.UInt32:
                    value = br.ReadUInt32();
                    break;
                case NodeTypes.UInt64:
                    value = br.ReadUInt64();
                    break;
            }
            if (AccessMode.HasFlag(AccessModes.Read))
                Value = value;
        }

        public event PropertyChangedEventHandler PropertyChanged;
    }

    public class ExcaliburFS
    {
        private enum MessageTypes : int { Unknown = 0, Command = 1, ListResult = 2, Preview = 3 };
        public ExcaliburClient Client { get; private set; }
        public string RootDirectory { get; private set; }
        public ExcaliburFS(ExcaliburClient client, string path)
        {
            Client = client;
            Client.ReceivingFinished += new ExcaliburClient.PacketEventHandler(Client_ReceivingFinished);
            Client.PacketSent += new ExcaliburClient.PacketEventHandler(Client_PacketSent);
            RootDirectory = path;
        }

        void Client_PacketSent(object sender, ExcaliburClient.PacketEventArgs e)
        {
            e.P.Data.Close();
            e.P.Data.Dispose();
        }

        int NextCommandID = 0;

        public int ListDirectory(string path)
        {
            return SendCommand("list|" + path);
        }

        public int PreviewFile(string path)
        {
            return SendCommand("prev|" + path);
        }

        public delegate Stream SteamFromPath(string path);
        public SteamFromPath PreviewFunction;

        private int SendCommand(string command)
        {
            MemoryStream MS = new MemoryStream(UTF8Encoding.UTF8.GetBytes(command));
            MS.Position = 0;
            Client.Send(CreateCommandID(MessageTypes.Command, NextCommandID++), MS, MS.Length);
            NextCommandID &= 0x00FFFFFF;
            return NextCommandID - 1;
        }

        private int CreateCommandID(MessageTypes messageType, int id)
        {
            return ((int)messageType << 24) + id;
        }

        private MessageTypes GetMessageType(int i)
        {
            return (MessageTypes)((i >> 24) & 0x000000FF);
        }

        private int GetMessageID(int i)
        {
            return i & 0x00FFFFFF;
        }

        static readonly char[] CommandDelimiter = new char[] { '|' };
        void Client_ReceivingFinished(object sender, ExcaliburClient.PacketEventArgs e)
        {
            Stream S = e.P.Data;
            int messageID = GetMessageID(e.P.ID);
            switch (GetMessageType(e.P.ID))
            {
                case MessageTypes.Command:
                    e.P.Data.Position = 0L;
                    StreamReader SR = new StreamReader(e.P.Data, UTF8Encoding.UTF8);
                    string[] command = SR.ReadToEnd().Split(CommandDelimiter);
                    SR.Dispose();
                    switch (command[0])
                    {
                        case "list":
                            if (RootDirectory != null && (command[1].StartsWith(RootDirectory) || command[1] == ""))
                            {
                                MemoryStream MS = new MemoryStream();
                                ExcaliburDirectory.FromFileSystem((command[1].StartsWith(RootDirectory) ? command[1] : RootDirectory), false).ToStream(MS);
                                MS.Position = 0;
                                Client.Send(CreateCommandID(MessageTypes.ListResult, messageID), MS, MS.Length);
                            }
                            break;
                        case "prev":
                            if (PreviewFunction != null)
                            {
                                Stream prevStream = PreviewFunction(command[1]);
                                if (prevStream != null)
                                {
                                    prevStream.Position = 0;
                                    Client.Send(CreateCommandID(MessageTypes.Preview, messageID), prevStream, prevStream.Length);
                                }
                            }
                            break;
                    }
                    break;
                case MessageTypes.ListResult:
                    S.Position = 0;
                    if (ListReceived != null)
                    {
                        ListReceived(this, new ExcaliburDirectoryEventArgs(ExcaliburDirectory.FromStream(S)));
                    }
                    break;
                case MessageTypes.Preview:
                    if (PreviewReceived != null)
                    {
                        e.P.Data.Position = 0;
                        PreviewReceived(this, new StreamEventArgs(messageID, e.P.Data));
                    }
                    break;
            }
            e.P.Data.Dispose();
        }

        public class StreamEventArgs : EventArgs
        {
            public int ID;
            public Stream Stream;
            public StreamEventArgs(int id, Stream stream)
            {
                ID = id;
                Stream = stream;
            }
        }
        public delegate void StreamEventHandler(object sender, StreamEventArgs e);
        public event StreamEventHandler PreviewReceived;

        public class ExcaliburDirectoryEventArgs : EventArgs
        {
            public ExcaliburDirectory Directory { get; private set; }
            public ExcaliburDirectoryEventArgs(ExcaliburDirectory directory)
            {
                Directory = directory;
            }
        }
        public delegate void ExcaliburDirectoryEventHandler(object sender, ExcaliburDirectoryEventArgs e);
        public event ExcaliburDirectoryEventHandler ListReceived;


        public class ExcaliburDirectory
        {
            public string Name, Location;
            public DateTimeOffset Creation, LastModification, LastAccess;
            public ExcaliburFile[] Files;
            public ExcaliburDirectory[] Directories;
            public int FileCount, DirectoryCount;
            public ExcaliburDirectory(string name, string location)
            {
                Name = name;
                Location = location;
            }

            public static ExcaliburDirectory FromFileSystem(string path, bool recursive)
            {
                ExcaliburDirectory ED = new ExcaliburDirectory(Path.GetFileName(path), path);
                if (Directory.Exists(path))
                {
                    ED.Creation = Directory.GetCreationTime(path);
                    ED.LastModification = Directory.GetLastWriteTime(path);
                    ED.LastAccess = Directory.GetLastAccessTime(path);
                    string[] dirs = Directory.GetDirectories(path);
                    ED.DirectoryCount = dirs.Length;
                    ED.Directories = new ExcaliburDirectory[dirs.Length];
                    string dir;
                    ExcaliburDirectory sED;
                    for (int i = 0; i < dirs.Length; i++)
                    {
                        dir = dirs[i];
                        if (recursive)
                        {
                            sED = FromFileSystem(dir, recursive);
                        }
                        else
                        {
                            sED = new ExcaliburDirectory(Path.GetFileName(dir), dir);
                            sED.Creation = Directory.GetCreationTime(dir);
                            sED.LastModification = Directory.GetLastWriteTime(dir);
                            sED.LastAccess = Directory.GetLastAccessTime(dir);
                            try
                            {
                                sED.DirectoryCount = Directory.GetDirectories(dir).Length;
                                sED.FileCount = Directory.GetFiles(dir).Length;
                            }
                            catch
                            {

                            }
                        }
                        ED.Directories[i] = sED;
                    }
                    string[] files = Directory.GetFiles(path);
                    ED.FileCount = files.Length;
                    ED.Files = new ExcaliburFile[files.Length];
                    string file;
                    ExcaliburFile EF;
                    FileInfo FI;
                    for (int i = 0; i < files.Length; i++)
                    {
                        file = files[i];
                        FI = new FileInfo(file);
                        EF = new ExcaliburFile(Path.GetFileName(file), FI.Length);
                        EF.Creation = FI.CreationTime;
                        EF.LastModification = FI.LastWriteTime;
                        EF.LastAccess = FI.LastAccessTime;
                        ED.Files[i] = EF;
                    }
                }
                return ED;
            }

            public void ToStream(Stream stream)
            {
                XmlWriter XW = XmlWriter.Create(stream);
                ToXML(XW);
            }

            private void ToXML(XmlWriter XW)
            {
                if ((Directories != null && DirectoryCount != Directories.Length) || (Files != null && FileCount != Files.Length))
                {
                    throw new Exception("Directory or file count is inconsistent with array size.");
                }
                if (string.IsNullOrEmpty(Name) || string.IsNullOrEmpty(Location))
                {
                    throw new Exception("Name and location cannot be empty or null.");
                }
                XW.WriteStartElement("Directory");
                XW.WriteAttributeString("Name", Name);
                XW.WriteAttributeString("Location", Location);
                XW.WriteAttributeString("Creation", Creation.ToFileTime().ToString());
                XW.WriteAttributeString("LastModification", LastModification.ToFileTime().ToString());
                XW.WriteAttributeString("LastAccess", LastAccess.ToFileTime().ToString());
                XW.WriteAttributeString("DirectoryCount", DirectoryCount.ToString());
                XW.WriteAttributeString("FileCount", FileCount.ToString());
                if (DirectoryCount > 0 && Directories != null)
                {
                    XW.WriteStartElement("Directories");
                    foreach (ExcaliburDirectory ED in Directories)
                    {
                        ED.ToXML(XW);
                    }
                    XW.WriteEndElement();
                }
                if (FileCount > 0 && Files != null)
                {
                    XW.WriteStartElement("Files");
                    foreach (ExcaliburFile EF in Files)
                    {
                        XW.WriteStartElement("File");
                        XW.WriteAttributeString("Name", EF.Name);
                        XW.WriteAttributeString("Size", EF.Size.ToString());
                        XW.WriteAttributeString("Creation", EF.Creation.ToFileTime().ToString());
                        XW.WriteAttributeString("LastModification", EF.LastModification.ToFileTime().ToString());
                        XW.WriteAttributeString("LastAccess", EF.LastAccess.ToFileTime().ToString());
                        XW.WriteEndElement();
                    }
                    XW.WriteEndElement();
                }
                XW.WriteEndElement();
                XW.Flush();
            }

            public static ExcaliburDirectory FromStream(Stream stream)
            {
                XmlReader XR = XmlReader.Create(stream);
                XR.ReadToFollowing("Directory");
                return FromXML(XR);
            }

            private static ExcaliburDirectory FromXML(XmlReader XR)
            {
                string name = null, location = null;
                int directories = 0, files = 0;
                DateTimeOffset creation, lastModification, lastAccess;
                creation = lastModification = lastAccess = DateTimeOffset.MinValue;
                if (XR.HasAttributes)
                {
                    while (XR.MoveToNextAttribute())
                    {
                        switch (XR.Name)
                        {
                            case "Name":
                                name = XR.Value;
                                break;
                            case "Location":
                                location = XR.Value;
                                break;
                            case "Creation":
                                creation = DateTimeOffset.FromFileTime(long.Parse(XR.Value));
                                break;
                            case "LastModification":
                                lastModification = DateTimeOffset.FromFileTime(long.Parse(XR.Value));
                                break;
                            case "LastAccess":
                                lastAccess = DateTimeOffset.FromFileTime(long.Parse(XR.Value));
                                break;
                            case "DirectoryCount":
                                directories = int.Parse(XR.Value);
                                break;
                            case "FileCount":
                                files = int.Parse(XR.Value);
                                break;
                        }
                    }
                }
                ExcaliburDirectory ED = null;
                if (name != null && location != null)
                {
                    ED = new ExcaliburDirectory(name, location);
                    ED.Creation = creation;
                    ED.LastModification = lastModification;
                    ED.LastAccess = lastAccess;
                    ED.DirectoryCount = directories;
                    ED.FileCount = files;

                    XR.MoveToElement();
                    if (!XR.IsEmptyElement)
                    {
                        XR.ReadStartElement();
                        while (XR.NodeType != XmlNodeType.EndElement)
                        {
                            switch (XR.Name)
                            {
                                case "Directories":
                                    ED.Directories = new ExcaliburDirectory[directories];
                                    XR.ReadToFollowing("Directory");
                                    for (int i = 0; i < directories; i++)
                                    {
                                        ED.Directories[i] = FromXML(XR);
                                    }
                                    XR.ReadEndElement();
                                    break;
                                case "Files":
                                    ED.Files = new ExcaliburFile[files];
                                    XR.ReadToFollowing("File");
                                    for (int i = 0; i < files; i++)
                                    {
                                        ED.Files[i] = ExcaliburFile.FromXML(XR);
                                    }
                                    XR.ReadEndElement();
                                    break;
                                default:
                                    XR.Skip();
                                    break;
                            }
                        }
                        XR.ReadEndElement();
                    }
                    else
                    {
                        XR.Read();
                    }
                }

                return ED;
            }

            public override string ToString()
            {
                return Name;
            }

            public string Description
            {
                get
                {
                    string text = "";
                    if (DirectoryCount > 0)
                    {
                        if (DirectoryCount == 1)
                        {
                            text += "1 subdirectory, ";
                        }
                        else
                        {
                            text += DirectoryCount.ToString() + " directories, ";
                        }
                    }

                    if (FileCount > 0)
                    {
                        if (FileCount == 1)
                        {
                            text += "1 file, ";
                        }
                        else
                        {
                            text += FileCount.ToString() + " files, ";
                        }
                    }

                    text += "created at " + Creation.ToString("y");
                    return text;
                }
            }
        }

        public class ExcaliburFile
        {
            public string Name;
            public long Size;
            public DateTimeOffset Creation, LastModification, LastAccess;
            public ExcaliburFile(string name, long size)
            {
                Name = name;
                Size = size;
            }

            internal static ExcaliburFile FromXML(XmlReader XR)
            {
                string name = null;
                long size = 0;

                DateTimeOffset creation, lastModification, lastAccess;
                creation = lastModification = lastAccess = DateTimeOffset.MinValue;
                if (XR.HasAttributes)
                {
                    while (XR.MoveToNextAttribute())
                    {
                        switch (XR.Name)
                        {
                            case "Name":
                                name = XR.Value;
                                break;
                            case "Size":
                                size = long.Parse(XR.Value);
                                break;
                            case "Creation":
                                creation = DateTimeOffset.FromFileTime(long.Parse(XR.Value));
                                break;
                            case "LastModification":
                                lastModification = DateTimeOffset.FromFileTime(long.Parse(XR.Value));
                                break;
                            case "LastAccess":
                                lastAccess = DateTimeOffset.FromFileTime(long.Parse(XR.Value));
                                break;
                        }
                    }
                }
                XR.ReadStartElement();
                ExcaliburFile EF = null;
                if (name != null)
                {
                    EF = new ExcaliburFile(name, size);
                    EF.Creation = creation;
                    EF.LastModification = lastModification;
                    EF.LastAccess = lastAccess;
                }
                return EF;
            }

            public override string ToString()
            {
                return Name;
            }

            public string FriendlyModification
            {
                get
                {
                    return LastModification.ToString("y");
                }
            }

            public string FriendlySize
            {
                get
                {
                    string text = "";
                    float size = Size;
                    int i = 0;
                    while (size > 1024f)
                    {
                        size /= 1024f;
                        i++;
                    }
                    text += Math.Round(size, 2).ToString();
                    switch (i)
                    {
                        case 0:
                            text += " B";
                            break;
                        case 1:
                            text += " KB";
                            break;
                        case 2:
                            text += " MB";
                            break;
                        case 3:
                            text += " GB";
                            break;
                        case 4:
                            text += " TB";
                            break;
                        default:
                            text += " * 1024^" + i.ToString() + " B";
                            break;
                    }
                    return text;
                }
            }

            public string Extension
            {
                get
                {
                    return Name.Substring(Name.LastIndexOf('.') + 1).ToLower();
                }
            }

            public string TypeColorFlag
            {
                get
                {
                    switch (Extension)
                    {
                        case "exe":
                        case "dll":
                            return "Red";
                        case "rar":
                        case "zip":
                        case "iso":
                            return "Magenta";
                        case "nfo":
                        case "txt":
                        case "rtf":
                        case "doc":
                        case "xls":
                        case "ppt":
                        case "docx":
                        case "xlsx":
                        case "pptx":
                            return "Blue";
                        case "bmp":
                        case "jpg":
                        case "jpeg":
                        case "gif":
                        case "png":
                        case "tiff":
                            return "Green";
                        case "wmv":
                        case "avi":
                        case "mp4":
                        case "mkv":
                            return "Yellow";
                        case "wav":
                        case "mp3":
                        case "wma":
                        case "flac":
                            return "Orange";
                        default:
                            return "Gray";
                    }
                }
            }
        }
    }

    public static class Extensions
    {
        public static void Read(this Socket socket, byte[] buffer, int count)
        {
            int readedbytes = 0;
            while (readedbytes < count && socket.Connected)
            {
                readedbytes += socket.Receive(buffer, readedbytes, count - readedbytes, SocketFlags.None);
            }
            if (!socket.Connected)
                throw new SocketException();
        }
    }
}
