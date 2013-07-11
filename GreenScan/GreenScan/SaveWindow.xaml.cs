using Green.Graphics;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace GreenScan
{
    /// <summary>
    /// Interaction logic for SaveWindow.xaml
    /// </summary>
    public partial class SaveWindow : Window
    {
        GraphicsCanvas Canvas;
        String Filename;
        GraphicsCanvas.SaveFormats Format;
        BackgroundWorker Worker; 
        public SaveWindow(GraphicsCanvas canvas, string filename, GraphicsCanvas.SaveFormats format)
        {
            InitializeComponent();
            Canvas = canvas;
            Filename = filename;
            Format = format;
            Worker = new BackgroundWorker();
            Worker.DoWork += Worker_DoWork;
            Worker.RunWorkerCompleted += Worker_RunWorkerCompleted;
            Worker.RunWorkerAsync();
        }

        void Worker_DoWork(object sender, DoWorkEventArgs e)
        {
            e.Result = Canvas.SaveModel(Filename, Format);
        }

        void Worker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            Worker.Dispose();
            DialogResult = (bool)e.Result;
            Close();
        }
    }
}
