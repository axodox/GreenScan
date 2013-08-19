using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Data;
using Green.Kinect;
using System.Windows.Controls;
using Green.Settings;
using System.Windows;

namespace Green.Scan
{
    [ValueConversion(typeof(KinectManager), typeof(MenuItem[]))]
    class KinectManagerToDeviceListConverter:IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            if (!(value is KinectManager)) return null;
            KinectManager KM = (KinectManager)value;
            int count = KM.DeviceCount;
            if (count == 0) return null;
            MenuItem[] items = new MenuItem[count];
            for (int i = 0; i < count; i++)
            {
                items[i] = new MenuItem() { Header = "Device " + i };
                items[i].Command = GreenScanCommands.Start;
                items[i].CommandParameter = i.ToString();
            }
            return items;
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    [ValueConversion(typeof(bool), typeof(bool))]
    class BooleanInverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            return !(bool)value;
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            return !(bool)value;
        }
    }

    [ValueConversion(typeof(bool), typeof(Visibility))]
    class BooleanToVisibilityConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            return ((bool)value ? Visibility.Visible : Visibility.Collapsed);
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
