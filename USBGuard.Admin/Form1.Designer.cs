namespace USBGuard.Admin
{
    partial class Form1   
    {
        private System.ComponentModel.IContainer components = null;

        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        private void InitializeComponent()
        {
            this.txtAppID = new System.Windows.Forms.TextBox();
            this.txtSerial = new System.Windows.Forms.TextBox();
            this.btnGerar = new System.Windows.Forms.Button();
            this.btnLerSerial = new System.Windows.Forms.Button();
            this.lblAppID = new System.Windows.Forms.Label();
            this.lblSerial = new System.Windows.Forms.Label();
            this.cboUnidades = new System.Windows.Forms.ComboBox();
            this.SuspendLayout();
            // 
            // txtAppID
            // 
            this.txtAppID.Location = new System.Drawing.Point(20, 40);
            this.txtAppID.Name = "txtAppID";
            this.txtAppID.Size = new System.Drawing.Size(240, 20);
            this.txtAppID.TabIndex = 1;
            // 
            // txtSerial
            // 
            this.txtSerial.Location = new System.Drawing.Point(20, 100);
            this.txtSerial.Name = "txtSerial";
            this.txtSerial.ReadOnly = true;
            this.txtSerial.Size = new System.Drawing.Size(240, 20);
            this.txtSerial.TabIndex = 3;
            // 
            // btnGerar
            // 
            this.btnGerar.Location = new System.Drawing.Point(145, 180);
            this.btnGerar.Name = "btnGerar";
            this.btnGerar.Size = new System.Drawing.Size(115, 35);
            this.btnGerar.TabIndex = 5;
            this.btnGerar.Text = "Gerar .AUTH";
            this.btnGerar.Click += new System.EventHandler(this.btnGerar_Click);
            // 
            // btnLerSerial
            // 
            this.btnLerSerial.Location = new System.Drawing.Point(145, 140);
            this.btnLerSerial.Name = "btnLerSerial";
            this.btnLerSerial.Size = new System.Drawing.Size(115, 35);
            this.btnLerSerial.TabIndex = 4;
            this.btnLerSerial.Text = "Ler Serial";
            this.btnLerSerial.Click += new System.EventHandler(this.btnLerSerial_Click);
            // 
            // lblAppID
            // 
            this.lblAppID.Location = new System.Drawing.Point(20, 20);
            this.lblAppID.Name = "lblAppID";
            this.lblAppID.Size = new System.Drawing.Size(200, 20);
            this.lblAppID.TabIndex = 0;
            this.lblAppID.Text = "Identificador da Empresa (AppID):";
            // 
            // lblSerial
            // 
            this.lblSerial.Location = new System.Drawing.Point(20, 80);
            this.lblSerial.Name = "lblSerial";
            this.lblSerial.Size = new System.Drawing.Size(200, 20);
            this.lblSerial.TabIndex = 2;
            this.lblSerial.Text = "Serial do Hardware (USB):";
            // 
            // cboUnidades
            // 
            this.cboUnidades.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cboUnidades.Location = new System.Drawing.Point(20, 140);
            this.cboUnidades.Name = "cboUnidades";
            this.cboUnidades.Size = new System.Drawing.Size(115, 21);
            this.cboUnidades.TabIndex = 6;
            // 
            // Form1
            // 
            this.ClientSize = new System.Drawing.Size(292, 234);
            this.Controls.Add(this.lblAppID);
            this.Controls.Add(this.txtAppID);
            this.Controls.Add(this.lblSerial);
            this.Controls.Add(this.txtSerial);
            this.Controls.Add(this.btnLerSerial);
            this.Controls.Add(this.btnGerar);
            this.Controls.Add(this.cboUnidades);
            this.Name = "Form1";
            this.Text = "USB Guard - Admin";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        private System.Windows.Forms.TextBox txtAppID;
        private System.Windows.Forms.TextBox txtSerial;
        private System.Windows.Forms.Button btnGerar;
        private System.Windows.Forms.Button btnLerSerial;
        private System.Windows.Forms.Label lblAppID;
        private System.Windows.Forms.Label lblSerial;
        private System.Windows.Forms.ComboBox cboUnidades;
    }
}