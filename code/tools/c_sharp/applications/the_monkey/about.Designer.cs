namespace RoboPublish
{
	partial class About
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
			if(disposing && (components != null))
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
			this.panel1 = new System.Windows.Forms.Panel();
			this.groupBox1 = new System.Windows.Forms.GroupBox();
			this.labelBuildDate = new System.Windows.Forms.Label();
			this.labelBuildTime = new System.Windows.Forms.Label();
			this.labelBuilt = new System.Windows.Forms.Label();
			this.pictureBox1 = new System.Windows.Forms.PictureBox();
			this.panel1.SuspendLayout();
			this.groupBox1.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
			this.SuspendLayout();
			// 
			// panel1
			// 
			this.panel1.Controls.Add(this.groupBox1);
			this.panel1.Controls.Add(this.pictureBox1);
			this.panel1.Dock = System.Windows.Forms.DockStyle.Fill;
			this.panel1.Location = new System.Drawing.Point(0, 0);
			this.panel1.Name = "panel1";
			this.panel1.Size = new System.Drawing.Size(372, 256);
			this.panel1.TabIndex = 0;
			// 
			// groupBox1
			// 
			this.groupBox1.Controls.Add(this.labelBuildDate);
			this.groupBox1.Controls.Add(this.labelBuildTime);
			this.groupBox1.Controls.Add(this.labelBuilt);
			this.groupBox1.Location = new System.Drawing.Point(171, 3);
			this.groupBox1.Name = "groupBox1";
			this.groupBox1.Size = new System.Drawing.Size(197, 74);
			this.groupBox1.TabIndex = 4;
			this.groupBox1.TabStop = false;
			// 
			// labelBuildDate
			// 
			this.labelBuildDate.AutoSize = true;
			this.labelBuildDate.Location = new System.Drawing.Point(5, 31);
			this.labelBuildDate.Name = "labelBuildDate";
			this.labelBuildDate.Size = new System.Drawing.Size(35, 13);
			this.labelBuildDate.TabIndex = 2;
			this.labelBuildDate.Text = "label1";
			// 
			// labelBuildTime
			// 
			this.labelBuildTime.AutoSize = true;
			this.labelBuildTime.Location = new System.Drawing.Point(5, 44);
			this.labelBuildTime.Name = "labelBuildTime";
			this.labelBuildTime.Size = new System.Drawing.Size(35, 13);
			this.labelBuildTime.TabIndex = 3;
			this.labelBuildTime.Text = "label1";
			// 
			// labelBuilt
			// 
			this.labelBuilt.AutoSize = true;
			this.labelBuilt.Location = new System.Drawing.Point(5, 18);
			this.labelBuilt.Name = "labelBuilt";
			this.labelBuilt.Size = new System.Drawing.Size(30, 13);
			this.labelBuilt.TabIndex = 1;
			this.labelBuilt.Text = "Built:";
			// 
			// pictureBox1
			// 
			this.pictureBox1.Image = global::RoboPublish.Properties.Resources.MonkeyMagic;
			this.pictureBox1.Location = new System.Drawing.Point(3, 3);
			this.pictureBox1.Name = "pictureBox1";
			this.pictureBox1.Size = new System.Drawing.Size(167, 250);
			this.pictureBox1.SizeMode = System.Windows.Forms.PictureBoxSizeMode.StretchImage;
			this.pictureBox1.TabIndex = 0;
			this.pictureBox1.TabStop = false;
			// 
			// About
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(372, 256);
			this.Controls.Add(this.panel1);
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "About";
			this.ShowInTaskbar = false;
			this.Text = "Monkey About";
			this.panel1.ResumeLayout(false);
			this.groupBox1.ResumeLayout(false);
			this.groupBox1.PerformLayout();
			((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
			this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.Panel panel1;
		private System.Windows.Forms.PictureBox pictureBox1;
		private System.Windows.Forms.Label labelBuilt;
		private System.Windows.Forms.Label labelBuildDate;
		private System.Windows.Forms.GroupBox groupBox1;
		private System.Windows.Forms.Label labelBuildTime;

	}
}