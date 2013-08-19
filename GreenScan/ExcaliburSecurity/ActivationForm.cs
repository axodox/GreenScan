using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace ExcaliburSecurity
{
    public partial class ActivationForm : Form
    {
        private SecurityClient SC;
        public ActivationForm(SecurityClient securityClient)
        {
            InitializeComponent();
            SC = securityClient;
            BActivate.Click += BActivate_Click;
        }

        void BActivate_Click(object sender, EventArgs e)
        {
            if (SC.Activate(TBSerial.Text))
            {
                Close();
            }
            else
            {
                MessageBox.Show(LocalizedResources.ActivationUnsuccessful);
            }
        }
    }
}
