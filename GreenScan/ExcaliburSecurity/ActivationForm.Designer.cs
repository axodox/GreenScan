namespace ExcaliburSecurity
{
    partial class ActivationForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ActivationForm));
            this.BActivate = new System.Windows.Forms.Button();
            this.TBSerial = new System.Windows.Forms.TextBox();
            this.LMessage = new System.Windows.Forms.Label();
            this.LHelp = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // BActivate
            // 
            resources.ApplyResources(this.BActivate, "BActivate");
            this.BActivate.Name = "BActivate";
            this.BActivate.UseVisualStyleBackColor = true;
            // 
            // TBSerial
            // 
            resources.ApplyResources(this.TBSerial, "TBSerial");
            this.TBSerial.Name = "TBSerial";
            // 
            // LMessage
            // 
            resources.ApplyResources(this.LMessage, "LMessage");
            this.LMessage.Name = "LMessage";
            // 
            // LHelp
            // 
            resources.ApplyResources(this.LHelp, "LHelp");
            this.LHelp.Name = "LHelp";
            // 
            // ActivationForm
            // 
            resources.ApplyResources(this, "$this");
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.LHelp);
            this.Controls.Add(this.LMessage);
            this.Controls.Add(this.TBSerial);
            this.Controls.Add(this.BActivate);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "ActivationForm";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TextBox TBSerial;
        private System.Windows.Forms.Button BActivate;
        private System.Windows.Forms.Label LMessage;
        private System.Windows.Forms.Label LHelp;


    }
}