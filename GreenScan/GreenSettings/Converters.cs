using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace Green.Settings.UI
{
    [ValueConversion(typeof(Setting), typeof(UIElement))]
    public class SettingToUIConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            if (value is Setting && (value as Setting).IsHidden) 
                return null;
            if (value is NumericSetting)
                return new NumericControl();
            if (value is EnumSetting)
                return new EnumControl();
            if (value is BooleanSetting)
                return new BooleanControl();
            if (value is MatrixSetting)
                return new MatrixControl();
            if (value is PathSetting)
                return new PathControl();
            if (value is StringSetting)
                return new StringControl();
            return null;
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    [ValueConversion(typeof(SettingGroup), typeof(UIElement))]
    public class SettingGroupToUIConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            if (value is SettingGroup)
            {
                if ((value as SettingGroup).IsHidden) return null;
                return new SettingGroupControl();
            }
            return null;
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    [ValueConversion(typeof(Boolean), typeof(FontWeight))]
    public class HasDefaultValueToFontWeightConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            if (value is Boolean)
                return ((bool)value ? FontWeights.Normal : FontWeights.Bold);
            else
                return null;
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}