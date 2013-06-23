using System;
using System.ComponentModel;
using System.Windows.Controls;

namespace Green.Settings.UI
{
    public class FloatBox : TextBox, INotifyPropertyChanged
    {
        int oldCaretPos;
        string oldText;
        bool skipNext;

        public FloatBox()
            : base()
        {
            value = float.NaN;
        }

        protected override void OnKeyDown(System.Windows.Input.KeyEventArgs e)
        {
            oldCaretPos = CaretIndex;
            oldText = Text;            
            base.OnKeyDown(e);
        }

        protected override void OnTextChanged(TextChangedEventArgs e)
        {
            if (skipNext) return; 
            float val;
            if (Text == "" || Text == "-" || float.TryParse(Text, out val))
            {
                oldText = Text;
                oldCaretPos = CaretIndex;
            }
            else
            {
                skipNext = true;
                Text = oldText;
                CaretIndex = oldCaretPos;
                skipNext = false;
            }
            base.OnTextChanged(e);

            if (float.TryParse(Text, out val))
            {
                value = val;
            }
            else
            {
                value = float.NaN;
            }
            OnValueChanged();
        }

        private float value;
        public float Value
        {
            get
            {
                return value;
            }
            set
            {
                Text = value.ToString();
                this.value = value;

            }
        }

        void OnValueChanged()
        {
            if (PropertyChanged != null)
                PropertyChanged(this, new PropertyChangedEventArgs("Value"));

            if (ValueChanged != null)
                ValueChanged(this, new EventArgs());
        }

        public event EventHandler ValueChanged;
        public event PropertyChangedEventHandler PropertyChanged;
    }
}