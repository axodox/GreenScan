using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Controls;
using System.Reflection;
using System.IO;
using System.Globalization;

namespace Green.Settings
{
    public abstract class Setting : INotifyPropertyChanged
    {
        public static CultureInfo Culture;
        static Setting()
        {
            Culture = new CultureInfo("en-US");
        }

        public string Name { get; protected set; }
        
        [Flags]
        public enum Types : uint { Unknown = 0, Boolean = 1, Enum = 2, SByte = 4, Int16 = 8, Int32 = 16, Int64 = 32, Byte = 64, UInt16 = 128, UInt32 = 256, UInt64 = 512, Single = 1024, Double = 2048, Numeric = 4092, Integer = 1020 };
        public Types Type { get; protected set; }
        
        public T GetValue<T>() where T : struct
        {
            return (this as NumericSetting<T>).Value;
        }

        public void SetValue<T>(T value) where T : struct
        {
            (this as NumericSetting<T>).Value = value;
        }

        public event PropertyChangedEventHandler PropertyChanged;
        public event EventHandler ValueChanged;
        protected void OnPropertyChanged(string name)
        {
            if (PropertyChanged != null) PropertyChanged(this, new PropertyChangedEventArgs(name));
        }
        protected void OnValueChanged()
        {
            OnPropertyChanged("Value");
            OnPropertyChanged("StringValue");
            OnPropertyChanged("HasDefaultValue");
            if (ValueChanged != null) ValueChanged(this, null);
        }

        public abstract string StringValue { get; set; }
        public abstract bool HasDefaultValue { get; }
        public abstract void ResetValue();

        private string friendlyName = null;
        public string FriendlyName
        {
            get
            {
                if (friendlyName == null) return Name;
                else return friendlyName;
            }
            set
            {
                friendlyName = value;
            }
        }
    }

    public abstract class NumericSetting : Setting 
    {
        public int Decimals { get; protected set; }
    }

    public class NumericSetting<T> : NumericSetting where T : struct
    {
        private T value;
        public T Value 
        {
            get { return value; }
            set { 
                this.value = value;
                OnValueChanged();
            }
        }
        public T Maximum { get; private set; }
        public T Minimum { get; private set; }
        public T DefaultValue { get; private set; }        
        
        public NumericSetting(string name, T value, T min, T max, int decimals = 0)
        {
            Types type;
            if (Enum.TryParse<Types>(typeof(T).Name, out type) && Types.Numeric.HasFlag(type))
                Type = type;
            else
                throw new NotSupportedException("The given datatype is not supported!");

            Name = name;
            DefaultValue = Value = value;
            Maximum = max;
            Minimum = min;
            Decimals = decimals;
        }

        public override string StringValue
        {
            get
            {
                return (string)typeof(T).InvokeMember("ToString", BindingFlags.InvokeMethod | BindingFlags.Public | BindingFlags.Instance, null, value, new object[] { "F" + Decimals, Culture });
            }
            set
            {
                try
                {
                    Value = (T)typeof(T).InvokeMember("Parse", BindingFlags.InvokeMethod | BindingFlags.Static | BindingFlags.Public, null, null, new object[] { value, Culture });
                }
                catch { }
            }
        }

        public override bool HasDefaultValue
        {
            get { return DefaultValue.Equals(value); }
        }

        public override void ResetValue()
        {
            Value = DefaultValue;
        }
    }

    public class BooleanSetting : Setting
    {
        private bool value;
        public bool Value
        {
            get { return value; }
            set
            {
                this.value = value;
                OnValueChanged();
            }
        }

        public bool DefaultValue { get; private set; }

        public BooleanSetting(string name, bool value)
        {
            Name = name;
            DefaultValue=Value = value;
        }

        public override string StringValue
        {
            get { return value.ToString(); }
            set
            {
                bool newValue;
                if (Boolean.TryParse(value, out newValue))
                {
                    Value = newValue;
                }
            }
        }

        public override bool HasDefaultValue
        {
            get { return DefaultValue == value; }
        }

        public override void ResetValue()
        {
            Value = DefaultValue;
        }
    }

    public abstract class EnumSetting : Setting
    {
        public string[] StringOptions { get; protected set; }

        private string[] friendlyOptions = null;
        public string[] FriendlyOptions
        {
            get
            {
                if (friendlyOptions == null) return StringOptions;
                else return friendlyOptions;
            }
            set
            {
                if (value.Length == StringOptions.Length)
                    friendlyOptions = value;
            }
        }

        public string FriendlyValue
        {
            get
            {
                for (int i = 0; i < StringOptions.Length; i++)
                {
                    if (StringOptions[i] == StringValue)
                    {
                        return FriendlyOptions[i];
                    }
                }
                return null;
            }
            set
            {
                for (int i = 0; i < FriendlyOptions.Length; i++)
                {
                    if (FriendlyOptions[i] == value)
                    {
                        StringValue = StringOptions[i];
                        break;
                    }
                }
            }
        }
    }

    public class EnumSetting<T> : EnumSetting where T : struct
    {
        private T value;
        public T Value 
        {
            get { return value; }
            set { 
                this.value = value;
                OnValueChanged();
            }
        }
        public T DefaultValue { get; private set; }

        public override string StringValue
        {
            get { return value.ToString(); }
            set
            {
                T newValue;
                if(Enum.TryParse<T>(value, out newValue))
                {
                    Value = newValue;
                }
            }
        }

        public EnumSetting(string name, T value)
        {
            if (!typeof(T).IsEnum) throw new NotSupportedException("The given datatype is not an enumeration!");
            Type = Types.Enum;
            Name = name;
            DefaultValue = Value = value;
            StringOptions = Enum.GetNames(typeof(T));
        }

        public override bool HasDefaultValue
        {
            get { return DefaultValue.Equals(value); }
        }

        public override void ResetValue()
        {
            Value = DefaultValue;
        }
    }

    public class SettingGroup : INotifyPropertyChanged
    {
        private bool hasDefaultValues;
        public bool HasDefaultValues 
        {
            get { return hasDefaultValues; }
            private set
            {
                hasDefaultValues = value;
                OnPropertyChanged("HasDefaultValues");
            }
        }
        public string Name { get; private set; }
        public ObservableCollection<Setting> Settings { get; private set; }
        public SettingGroup(string name)
        {
            Name = name;
            Settings = new ObservableCollection<Setting>();
            Settings.CollectionChanged += Settings_CollectionChanged;
            HasDefaultValues = true;
        }

        void Settings_CollectionChanged(object sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
        {
            foreach (Setting s in e.NewItems)
            {
                s.PropertyChanged += Setting_PropertyChanged;
                if (!s.HasDefaultValue) HasDefaultValues = false;
            }
        }

        void Setting_PropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            bool hdv = true;
            foreach (Setting s in Settings)
            {
                if (!s.HasDefaultValue)
                {
                    hdv = false;
                    break;
                }
            }
            HasDefaultValues = hdv;
        }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string name)
        {
            if (PropertyChanged != null) PropertyChanged(this, new PropertyChangedEventArgs(name));
        }

        private string friendlyName = null;
        public string FriendlyName
        {
            get
            {
                if (friendlyName == null) return Name;
                else return friendlyName;
            }
            set
            {
                friendlyName = value;
            }
        }
    }

    public class SettingManager
    {
        public ObservableCollection<SettingGroup> SettingGroups { get; private set; }
        public SettingManager()
        {
            SettingGroups = new ObservableCollection<SettingGroup>();
        }

        public bool Save(string path)
        {
            try
            {
                using (StreamWriter SW = new StreamWriter(path))
                {
                    foreach (SettingGroup SG in SettingGroups)
                    {
                        SW.WriteLine('[' + SG.Name + ']');
                        foreach (Setting S in SG.Settings)
                        {
                            SW.WriteLine(S.Name + '=' + S.StringValue);
                        }
                        SW.WriteLine();
                    }
                    SW.Close();
                }
                return true;
            }
            catch 
            {
                return false;
            }
        }

        public bool Load(string path)
        {
            try
            {
                Dictionary<string, Setting> settings = new Dictionary<string, Setting>();
                foreach (SettingGroup SG in SettingGroups)
                    foreach (Setting S in SG.Settings)
                    {
                        settings.Add(SG.Name + '.' + S.Name, S);
                    }

                string group = "", setting, value;
                string[] items;
                using (StreamReader SR = new StreamReader(path))
                {
                    while (!SR.EndOfStream)
                    {       
                        string line = SR.ReadLine();
                        if (line.Length == 0) continue;
                        if (line[0] == '[')
                        {
                            group = line.Substring(1, line.Length - 2);
                        }
                        else
                        {
                            items = line.Split('=');
                            setting = group+'.'+items[0];
                            value = items[1];
                            if (settings.ContainsKey(setting))
                            {
                                settings[setting].StringValue = value;
                            }
                        }
                    }
                    SR.Close();
                }
                return true;
            }
            catch
            {
                return false;
            }
        }
    }
}
