using System;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Animation;

namespace Green.Branding
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class AboutBox : Window
    {
        Color[] AnimColors;
        int ColorNum = 0;
        public AboutBox()
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
            (Resources["Swap"] as Storyboard).Begin();
        }

        private void Storyboard_Completed(object sender, EventArgs e)
        {
            StartAnimation();
        }
    }
}
