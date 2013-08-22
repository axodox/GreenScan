using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.IO;
using System.Net;
using System.Reflection;
using System.Security.Cryptography;
using System.Threading;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Threading;
using System.Linq;
using System.ComponentModel;
using System.Diagnostics;

namespace AxodoxUpdater
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        Color[] AnimColors;
        int ColorNum = 0;
        public MainWindow()
        {
            InitializeComponent();
            AnimColors = new Color[] { 
                Colors.White,
                Color.FromRgb(255, 221, 0),
                Color.FromRgb(255, 162, 0),
                Color.FromRgb(238, 19, 23), 
                Color.FromRgb(242, 7, 117), 
                Color.FromRgb(240, 15, 236),
                Color.FromRgb(154, 15, 240),
                Color.FromRgb(21, 38, 228),
                Color.FromRgb(0, 149, 249),
                Color.FromRgb(0, 249, 187),
                Color.FromRgb(37, 239, 15),
                Color.FromRgb(168, 239, 15),
                Color.FromRgb(255, 221, 0)
            };
            StartAnimation();

            ExitTimer = new DispatcherTimer();
            ExitTimer.Tick += ExitTimer_Tick;
            ExitTimer.Interval = new TimeSpan(0, 0, 1);

            StringDictionary settings = LoadSettings("Updater.ini");
            Server = settings["Server"];
            Application = settings["Application"];
            PostSetupAction = settings["PostSetupAction"];            

            if (Server == null || Application == null || PostSetupAction == null)
            {
                ToErrorState("Updater.ini missing or corrupt!");
                return;
            }
            Success = false;
            WC = new WebClient();
            WC.DownloadStringCompleted += WC_DownloadStringCompleted;
            UpdateState = UpdateStates.Connecting;
            SC = SynchronizationContext.Current;
            Root = Assembly.GetExecutingAssembly().Location;
            Root = Root.Remove(Root.LastIndexOf('\\'));
            rootLength = Root.Length;
            WC.DownloadStringAsync(new Uri(Server + "update.php?app=" + Application, UriKind.Absolute));
            WC.DownloadFileCompleted += WC_DownloadFileCompleted;
            WC.DownloadProgressChanged += WC_DownloadProgressChanged;
            
        }

        Color GetColor(int index)
        {
            return AnimColors[index % AnimColors.Length];
        }

        void StartAnimation()
        {
            BBack.Background = new SolidColorBrush(GetColor(ColorNum));
            RFront.Fill = new SolidColorBrush(GetColor(ColorNum + 1));
            ColorNum++;
            if (ColorNum == AnimColors.Length) ColorNum = 0;
            (Resources["Swap"] as Storyboard).Begin();
        }

        private void Storyboard_Completed(object sender, EventArgs e)
        {
            StartAnimation();
        }

        enum UpdateStates { Connecting, ComparingFiles, FetchingFiles, InstallingUpdates, Done }
        string[] ProtectedFiles = new string[] { "\\AxodoxUpdater.exe", "\\Updater.ini"};
        string[] ProtectedFolders = new string[] { "" };
        UpdateStates UpdateState;
        string Server;
        string Application;
        string DownloadRoot;
        string PostSetupAction;
        WebClient WC;
        SynchronizationContext SC;
        string Root;
        int rootLength;
        DispatcherTimer ExitTimer;
        bool Success;

        void ExitTimer_Tick(object sender, EventArgs e)
        {
            if (WC != null) WC.Dispose();
            ExitTimer.Stop();
            if (Success)
            {
                try
                {
                    Process.Start(PostSetupAction);
                }
                catch { }
            }
            Close();
        }

        void ToErrorState(string errorText = "The connection to the update servers has been lost.")
        {
            PBProgress.Value = 0;
            PBProgress.IsIndeterminate = false;
            PBProgress.Visibility = System.Windows.Visibility.Collapsed;
            TBProgress.Text = errorText;
            ExitTimer.Start();
        }

        void WC_DownloadStringCompleted(object sender, DownloadStringCompletedEventArgs e)
        {
            if (e.Error != null)
            {
                ToErrorState();
                return;
            }
            switch (UpdateState)
            {
                case UpdateStates.Connecting:
                    try
                    {
                        ProcessReleaseManifest(e.Result);
                        TBProgress.Text = string.Format("Updating to {0}, released at: {1}...", UpdateName, DeployDate);
                        UpdateState = UpdateStates.ComparingFiles;
                        Thread T = new Thread(BuildDownloadList);
                        T.Start();
                    }
                    catch { ToErrorState("No updates could be found this time."); }
                    break;
            }
        }

        Dictionary<string, bool> LocalDirectories, LocalFiles;
        long AllDownloadSize, DownloadedSize;
        void BuildDownloadList()
        {
            AllDownloadSize = 0;
            string path;
            
            Queue<string> dirsToProcess = new Queue<string>();
            dirsToProcess.Enqueue(Root);
            LocalDirectories = new Dictionary<string, bool>();
            LocalFiles = new Dictionary<string, bool>();
            while (dirsToProcess.Count > 0)
            {
                path = dirsToProcess.Dequeue();
                LocalDirectories.Add(path.Substring(rootLength), false);
                string[] dirs = Directory.GetDirectories(path);
                foreach (string d in dirs) dirsToProcess.Enqueue(d);
                string[] files = Directory.GetFiles(path);
                foreach (string f in files) LocalFiles.Add(f.Substring(rootLength), false);
            }

            SHA1 sha1 = SHA1.Create();
            bool fileOk;
            string relPath;
            DownloadTasks = new Queue<DownloadTask>();
            foreach (UpdateDirectory d in Directories)
            {
                if (Directory.Exists(Root + d)) LocalDirectories[d.Name] = true;
                else Directory.CreateDirectory(Root + d);
                foreach (UpdateFile f in d.Files)
                {
                    fileOk = false;
                    relPath = d.Name + "\\" + f.Name;
                    path = Root + relPath;
                    if (File.Exists(path))
                    {
                        try
                        {
                            using (FileStream FS = new FileStream(path, FileMode.Open, FileAccess.Read))
                            {
                                byte[] hash = sha1.ComputeHash(FS);
                                string hashString = "";
                                for (int i = 0; i < hash.Length; i++)
                                    hashString += hash[i].ToString("X2");
                                if (hashString.ToLower() == f.Checksum)
                                {
                                    fileOk = true;
                                    LocalFiles[d.Name + '\\' + f.Name] = true;
                                }
                            }
                        }
                        catch { }
                    }

                    if (!fileOk)
                    {
                        DownloadTask DT = new DownloadTask(
                            Path.GetTempFileName(),
                            DownloadRoot + relPath.Replace('\\', '/'),
                            path,
                            relPath,
                            f.Size);
                        DownloadTasks.Enqueue(DT);
                        AllDownloadSize += f.Size;
                    }
                }
            }            
            SC.Post(BuildDownloadListCallback, null);
        }

        Queue<DownloadTask> DownloadTasks;
        List<DownloadTask> CompletedDownloads;        
        class DownloadTask
        {
            public string Source { get; set; }
            public string TempFile { get; set; }
            public string Name { get; set; }
            public string Target { get; set; }
            public long Size { get; set; }
            public DownloadTask(string tempFile, string source, string target, string name, long size)
            {
                TempFile = tempFile;
                Source = source;
                Target = target;
                Name = name;
                Size = size;
            }
        }

        void BuildDownloadListCallback(object o)
        {
            UpdateState = UpdateStates.FetchingFiles;
            CompletedDownloads = new List<DownloadTask>();
            FileNum = 1;
            PBProgress.IsIndeterminate = false;
            PBProgress.Value = 0;
            DownloadedSize = 0;
            FileCount = DownloadTasks.Count;
            FetchFiles();
        }

        int FileNum, FileCount;
        void FetchFiles()
        {
            if (DownloadTasks.Count > 0)
            {
                DownloadTask DT = DownloadTasks.Dequeue();
                TBProgress.Text = "Downloading file " + FileNum + "/" + FileCount + ": " + DT.Name + "...";
                WC.DownloadFileAsync(
                    new Uri(DT.Source, UriKind.Absolute),
                    DT.TempFile, DT);
            }
            else
            {
                UpdateState = UpdateStates.InstallingUpdates;
                TBProgress.Text = "Installing update...";
                PBProgress.IsIndeterminate = true;
                new Thread(InstallUpdate).Start();    
            }
        }

        void InstallUpdate()
        {
            foreach (KeyValuePair<string, bool> file in LocalFiles)
            {
                try
                {
                    if (!file.Value && !ProtectedFiles.Contains(file.Key)) File.Delete(Root+file.Key);
                }
                catch { }
            }

            foreach (KeyValuePair<string, bool> dir in LocalDirectories)
            {
                try
                {
                    if (!dir.Value && !ProtectedFolders.Contains(dir.Key)) Directory.Delete(Root + dir.Key, true);
                }
                catch { }
            }

            foreach (DownloadTask f in CompletedDownloads)
            {
                try
                {
                    File.Move(f.TempFile, f.Target);
                }
                catch { }
            }
            SC.Post(InstallUpdateCallback,null);
        }

        void InstallUpdateCallback(object o)
        {
            PBProgress.Value = 100;
            PBProgress.IsIndeterminate = false;
            TBProgress.Text = "Done.";
            Success = true;
            ExitTimer.Start();
        }

        void WC_DownloadProgressChanged(object sender, DownloadProgressChangedEventArgs e)
        {
            if (UpdateState == UpdateStates.FetchingFiles)
            {
                DownloadTask DT = (DownloadTask)e.UserState;
                PBProgress.Value = (int)((DownloadedSize * 100 + DT.Size * e.ProgressPercentage) / AllDownloadSize);
                TBProgress.Text = "Downloading file " + FileNum + "/" + FileCount + ": " + DT.Name + "... " + e.ProgressPercentage + "%";
            }
        }

        void WC_DownloadFileCompleted(object sender, AsyncCompletedEventArgs e)
        {
            if (e.Error != null)
            {
                ToErrorState();
                return;
            }
            if (UpdateState == UpdateStates.FetchingFiles)
            {
                DownloadTask DT = (DownloadTask)e.UserState;
                DownloadedSize += DT.Size;
                PBProgress.Value = (int)(DownloadedSize * 100 / AllDownloadSize);
                CompletedDownloads.Add(DT);
                FileNum++;
                FetchFiles();
            }
        }

        class UpdateFile
        {
            public string Name { get; set; }
            public string Checksum { get; set; }
            public long Size { get; set; }
            public UpdateFile(string name, string checksum, long size)
            {
                Name = name;
                Checksum = checksum;
                Size = size;
            }
            public override string ToString()
            {
                return Name;
            }
        }
        class UpdateDirectory
        {
            public string Name { get; set; }
            public List<UpdateFile> Files { get; set; }
            public UpdateDirectory(string name)
            {
                Name = name;
                Files = new List<UpdateFile>();
            }
            public override string ToString()
            {
                return Name;
            }
        }
        List<UpdateDirectory> Directories;
        string UpdateName;
        string DeployDate;

        void ProcessReleaseManifest(string text)
        {
            int start = text.IndexOf("<body>") + 6;
            int end = text.LastIndexOf("</body>");
            string[] lines = text.Substring(start, end - start).Split(new string[] { "<br/>" }, StringSplitOptions.RemoveEmptyEntries);
            string[] valueSeparator=new string[] {": "};
            Directories = new List<UpdateDirectory>();
            UpdateDirectory dir = null;
            bool listingFiles = false;
            int rootLength = 0;
            foreach (string line in lines)
            {
                string[] items = line.Split(valueSeparator, StringSplitOptions.RemoveEmptyEntries);
                switch (items[0])
                {
                    case "Last update":
                        UpdateName = items[1];
                        break;
                    case "Deployed at":
                        DeployDate = items[1];
                        break;
                    case "Root":
                        DownloadRoot = Server + items[1];
                        break;
                    case "Listing files:":
                        listingFiles = true;
                        break;
                    case "Directory":
                        if (rootLength == 0)
                        {
                            rootLength = items[1].Length;
                        }
                        dir = new UpdateDirectory(items[1].Substring(rootLength));
                        Directories.Add(dir);
                        break;
                    case "End of listing files.":
                        listingFiles = false;
                        break;
                    default:
                        if (listingFiles)
                        {
                            string[] fileData=items[1].Split('|');
                            dir.Files.Add(new UpdateFile(items[0],fileData[0],long.Parse(fileData[1])));
                        }
                        break;
                }
            }
        }
        
        StringDictionary LoadSettings(string file)
        {
            StringDictionary settings = new StringDictionary();
            try
            {
                string[] lines = File.ReadAllLines(file);
                foreach (string line in lines)
                {
                    try
                    {
                        string[] items = line.Split('=');
                        settings.Add(items[0], items[1]);
                    }
                    catch { }
                }
            }
            catch { }
            return settings;
        }
    }
}
