using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Excalibur;
using Green.Settings;

namespace Green.Remote
{
    public class RemoteServer
    {
        Dictionary<string, Setting> Settings;
        ExcaliburServer Server;

        public RemoteServer(SettingManager manager)
        {
            Settings = manager.GetSettingsDictionary();
            foreach (KeyValuePair<string, Setting> setting in Settings)
            {
                setting.Value.ValueChanged += Setting_ValueChanged;
            }
            Server = new ExcaliburServer(5656, 1, true);
            Server.ClientConnected += Server_ClientConnected;
        }

        void Setting_ValueChanged(object sender, EventArgs e)
        {
            Setting setting = sender as Setting;
            string text = "s/" + setting.Group.Name + "." + setting.Name + "=" + setting.StringValue;
            foreach (ExcaliburClient client in Server.Clients)
            {
                client.SendMessage(text);
            }
        }

        void Server_ClientConnected(object sender, ExcaliburServer.ClientEventArgs e)
        {
            e.Client.MessageReceived += Client_MessageReceived;
            foreach(KeyValuePair<string, Setting> setting in Settings)
            {
                e.Client.SendMessage("s/" + setting.Key + "=" + setting.Value.StringValue);
            }
        }

        void Client_MessageReceived(object sender, ExcaliburClient.MessageEventArgs e)
        {
            if (e.Text.StartsWith("s/"))
            {
                int equpos = e.Text.IndexOf('=');
                string key = e.Text.Substring(2, equpos - 2);
                if (Settings.ContainsKey(key))
                {
                    Settings[key].StringValue = e.Text.Substring(equpos + 1);
                }
            }
        }
    }
}
