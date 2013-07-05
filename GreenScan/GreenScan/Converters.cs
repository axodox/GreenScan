using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Data;
using Green.Kinect;
using System.Windows.Controls;
using Green.Settings;

namespace Green.Scan
{
    [ValueConversion(typeof(KinectManager), typeof(MenuItem[]))]
    class KinectManagerToDeviceListConverter:IValueConverter
    {
        public static EnumSetting<KinectManager.Modes> ModeSetting;
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            if (!(value is KinectManager)) return null;
            KinectManager KM = (KinectManager)value;
            int count = KM.DeviceCount;
            if (count == 0) return null;
            MenuItem[] items = new MenuItem[count];
            for (int i = 0; i < count; i++)
            {
                items[i] = new MenuItem() { Header = "Device " + i, Tag = i };
                items[i].Click += KinectManagerToDeviceListConverter_Click;
            }
            return items;
        }

        void KinectManagerToDeviceListConverter_Click(object sender, System.Windows.RoutedEventArgs e)
        {
            KinectManager manager = (KinectManager)(sender as MenuItem).DataContext;
            int index = (int)(sender as MenuItem).Tag;
            manager.OpenKinect(index);
            manager.StartKinect(ModeSetting.Value);
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
