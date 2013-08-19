using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Net.NetworkInformation;
using System.Management;
using System.Security.Cryptography;
using System.Net;
using ExcaliburSecurity.Properties;
using System.Windows.Forms;

namespace ExcaliburSecurity
{
    public class SecurityClient
    {
        public bool HasPermissionToRun
        {
            get
            {
                return IsActivated;
            }
        }
        private string SecurityServer;
        private string HardwareHash;
        public int GracePeriod { get; private set; }
        public SecurityClient(string securityServer = "http://delta.inflab.bme.hu/~axodox/kinectactivation.php", int reactivationPeriod = 90)
        {
            Modules = new Dictionary<string, string>();
            SecurityServer = securityServer;
            HardwareHash = GetHardwareHash();
            GracePeriod = reactivationPeriod;
            CheckActivation();
            if (!HasPermissionToRun)
            {
                Application.Run(new ActivationForm(this));
            }
        }

        public bool IsComputerChanged { get; private set; }
        public bool IsActivated { get; private set; }
        public bool IsCheating { get; private set; }
        public bool IsFirstStart { get; private set; }
        private void CheckActivation()
        {
            IsFirstStart = string.IsNullOrEmpty(Settings.Default.HardwareHash);
            if (IsFirstStart) Settings.Default.FirstStart = DateTime.Now;
            IsComputerChanged = Settings.Default.HardwareHash != HardwareHash;
            IsActivated = Hash(Settings.Default.Serial + '+' + HardwareHash) == Settings.Default.ActivationCode;
            if (IsActivated && (DateTime.Now - Settings.Default.FirstStart).TotalDays > GracePeriod)
            {
                Reactivate();
            }
            if (IsActivated && Settings.Default.LastStart > DateTime.Now)
            {
                Reactivate();
                IsCheating = !IsActivated;
            }
            if (!IsCheating) Settings.Default.LastStart = DateTime.Now;
            InitModules();
            Settings.Default.HardwareHash = HardwareHash;
            Settings.Default.Save();
        }

        private void Reactivate()
        {
            string serial = Settings.Default.Serial;
            ResetActivation();
            CheckActivation();
            Activate(serial);
        }

        private void InitModules()
        {
            Modules.Clear();
            string[] mcodes = Settings.Default.ModuleCodes.Split('|');
            string[] items;
            for (int i = 1; i < mcodes.Length; i++)
            {
                items = mcodes[i].Split(':');
                Modules.Add(items[0], items[1]);
            }
        }

        private Dictionary<string, string> Modules;
        public bool IsModuleActivated(string name)
        {
            string fname=name.ToUpper();
            if (Modules.ContainsKey(fname))
            {
                string h = Hash(Settings.Default.Serial + "+" + HardwareHash + "+" + fname);
                return h == Modules[fname];
            }
            else return false;
        }

        public static void ResetActivation()
        {
            Settings.Default.Reset();
            Settings.Default.Save();
        }

        public void Exit()
        {
            if (!IsCheating) Settings.Default.LastStart = DateTime.Now;
            Settings.Default.Save();
        }

        public bool Activate(string serial)
        {
            try
            {
                using (WebClient WC = new WebClient())
                {
                    string text = WC.DownloadString(SecurityServer + "?serial=" + serial + "&computer=" + HardwareHash);
                    if (text.StartsWith("Error"))
                    {
                        MessageBox.Show(text);
                        return false;
                    }
                    string[] codes = text.Split('|');
                    if (codes[0] == Hash(serial + '+' + HardwareHash))
                    {
                        Settings.Default.ActivationCode = codes[0];
                        Settings.Default.ModuleCodes = text;
                        InitModules();
                        Settings.Default.Serial = serial;
                        Settings.Default.HardwareHash = HardwareHash;
                        Settings.Default.Save();
                        IsActivated = true;
                    }
                }
                return IsActivated;
            }
            catch
            {
                return false;
            }
        }

        public string GetHardwareHash()
        {
            List<object[]> hardwareInfo=new List<object[]>();
            hardwareInfo.Add(GetManagementInfo("Win32_Processor", new string[] { "Name", "ProcessorId" }));
            hardwareInfo.Add(GetManagementInfo("Win32_VideoController", new string[] { "Name"}));
            hardwareInfo.Add(GetManagementInfo("Win32_PhysicalMemory", new string[] { "SerialNumber", "PartNumber" }));
            string hardwareHash = "";

            foreach (object[] hardwareGroup in hardwareInfo)
            {
                if (hardwareGroup == null) continue;
                foreach (object[] hardware in hardwareGroup)
                {
                    if (hardware == null) continue;
                    foreach (object property in hardware)
                    {
                        if (property != null && property is string) hardwareHash += (string)property;
                    }
                }
            }
            return Hash(hardwareHash);
        }

        private string Hash(string text)
        {
            byte[] hash;
            using (SHA1 encoder = SHA1.Create())
            {
                hash = encoder.ComputeHash(UTF8Encoding.UTF8.GetBytes(text));
            }
            string hashString = "";
            for (int i = 0; i < hash.Length; i++)
                hashString += hash[i].ToString("X2");
            return hashString;
        }

        private object[] GetManagementInfo(string key, string[] properties)
        {
            ManagementObjectSearcher MOS = new ManagementObjectSearcher("SELECT * FROM " + key);
            ManagementObjectCollection MOC = MOS.Get();            
            object[][] results = new object[MOC.Count][];
            int i = 0;
            foreach (ManagementObject MO in MOS.Get())
            {
                results[i] = new object[properties.Length];
                for (int j = 0; j < properties.Length; j++)
                {
                    results[i][j]
                        = MO.Properties[properties[j]].Value;
                }
                i++;
                MO.Dispose();
            }
            MOS.Dispose();
            MOC.Dispose();
            return results;
        }

        private ulong[] GetHardwareID()
        {
            NetworkInterface[] NIs = NetworkInterface.GetAllNetworkInterfaces();
            List<ulong> IDs = new List<ulong>(NIs.Length);
            byte[] buffer = new byte[8];
            byte[] phy;
            ulong id;
            for (int i = 0; i < NIs.Length; i++)
            {
                phy = NIs[i].GetPhysicalAddress().GetAddressBytes();
                if (phy.Length > 0 && NIs[i].OperationalStatus == OperationalStatus.Up)
                {
                    for (int j = 0; j < phy.Length; j++) buffer[j] = phy[j];
                    for (int j = phy.Length; j < 8; j++) buffer[j] = 0;
                    id = BitConverter.ToUInt64(buffer, 0);
                    for (int j = 0; j < IDs.Count; j++)
                    {
                        while (IDs.Count > j && IDs[j] == id)
                        {
                            IDs.RemoveAt(j);
                        }
                    }
                    IDs.Add(id);
                }
            }
            return IDs.ToArray();
        }
    }
}
