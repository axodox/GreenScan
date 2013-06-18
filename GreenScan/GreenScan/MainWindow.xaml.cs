using System.Windows;
using System.Windows.Interop;
using Green.Graphics;
using System.Threading;
using Green.Kinect;
namespace GreenScan
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        KinectManager KM;
        Timer T;
        public MainWindow()
        {
            InitializeComponent();
            KM = new KinectManager();
            DataContext = KM;

            T = new Timer(TCallback, null, 0, 30);
        }

        void TCallback(object o)
        {
            //XC.Draw();
            
        }
    }
}
