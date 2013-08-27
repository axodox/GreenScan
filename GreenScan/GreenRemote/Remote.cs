using System;
using System.Collections.Generic;
using Excalibur;
using Green.Settings;
using System.Net;
using System.Windows.Input;
using System.Windows;

namespace Green.Remoting
{
    public class RemoteEventArgs : EventArgs
    {
        public IPEndPoint RemoteEndPoint;
        public RemoteEventArgs(IPEndPoint remoteEndPoint)
        {
            RemoteEndPoint = remoteEndPoint;
        }
    }

    public class RemotingServer : IDisposable
    {
        public int RemoteCount { get; private set; }
        public bool IsEnabled { get; set; }

        public delegate void RemoteEventHandler(object sender, RemoteEventArgs e);
        public event RemoteEventHandler RemoteConnected, RemoteDisconnected;

        Dictionary<string, Setting> Settings;
        Dictionary<string, RoutedUICommand> Commands;
        ExcaliburServer Server;
        IInputElement Target;

        public bool ExecuteCommand(string name, string argument = "")
        {
            if (Commands.ContainsKey(name))
            {
                RoutedUICommand command = Commands[name];
                bool ok = command.CanExecute(argument, Target);
                if (ok) command.Execute(argument, Target);
                return ok;
            }
            else 
                return false;
        }

        public void SetOption(string name, string value)
        {
            if (Settings.ContainsKey(name))
            {
                Settings[name].StringValue = value;
            }
        }

        public string GetOption(string name)
        {
            if (Settings.ContainsKey(name))
            {
                return Settings[name].StringValue;
            }
            else return null;
        }

        public void StoreOption(string name)
        {
            if (Settings.ContainsKey(name))
            {
                Settings[name].StoreValue();
            }
        }

        public void RestoreOption(string name)
        {
            if (Settings.ContainsKey(name))
            {
                Settings[name].RestoreValue();
            }
        }

        public RemotingServer(SettingManager manager, RoutedUICommand[] commands, IInputElement target, int port, long protocolId)
        {
            RemoteCount = 0;
            Target = target;
            Settings = manager.GetSettingsDictionary();
            foreach (KeyValuePair<string, Setting> setting in Settings)
            {
                setting.Value.ValueChanged += Setting_ValueChanged;
            }
            Commands = new Dictionary<string, RoutedUICommand>();
            foreach(RoutedUICommand command in commands)
            {
                Commands.Add(command.Name, command);
            }
            Server = new ExcaliburServer(port, protocolId, true);
            Server.ClientConnected += Server_ClientConnected;
            Server.ClientDisconnected += Server_ClientDisconnected;
        }

        void Server_ClientDisconnected(object sender, ExcaliburServer.ClientEventArgs e)
        {
            RemoteCount--;
            if (RemoteDisconnected != null) RemoteDisconnected(this, new RemoteEventArgs(e.Client.RemoteEndPoint));
        }

        void Setting_ValueChanged(object sender, EventArgs e)
        {
            Setting setting = sender as Setting;
            string text = "s!" + setting.Group.Name + "." + setting.Name + "=" + setting.StringValue;
            Server.SendMessageToAll(text);
        }

        void Server_ClientConnected(object sender, ExcaliburServer.ClientEventArgs e)
        {
            RemoteCount++;
            if (RemoteConnected != null) RemoteConnected(this, new RemoteEventArgs(e.Client.RemoteEndPoint));
            e.Client.MessageReceived += Client_MessageReceived;
            foreach(KeyValuePair<string, Setting> setting in Settings)
            {
                e.Client.SendMessage("s!" + setting.Key + "=" + setting.Value.StringValue);
            }
            foreach (KeyValuePair<string, RoutedUICommand> command in Commands)
            {
                e.Client.SendMessage("c!" + command.Key + "=" + command.Value.Text);
            }
        }

        void Client_MessageReceived(object sender, ExcaliburClient.MessageEventArgs e)
        {
            if (e.Text.StartsWith("s/") || e.Text.StartsWith("c/"))
            {
                int equpos = e.Text.IndexOf('=');
                string key = e.Text.Substring(2, equpos - 2);
                string value = e.Text.Substring(equpos + 1);

                switch (e.Text[0])
                {
                    case 's':
                        if (IsEnabled) SetOption(key, value);
                        break;
                    case 'c':
                        if (IsEnabled)
                        {
                            bool ok = ExecuteCommand(key, value);
                            string answer = "c?" + (ok ? "succeded" : "failed");
                            Server.SendMessageToAll(answer, e.Id);
                        }
                        else
                            Server.SendMessageToAll("c?failed", e.Id);
                        break;
                }
            }
        }

        bool IsDisposed = false;
        public void Dispose()
        {
            if (!IsDisposed)
            {
                IsDisposed = true;
                Server.StopListening();
                Server.DisconnectAll();                
            }
        }
    }
}
