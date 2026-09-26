namespace Executer
{
	partial class JobTab
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
			this.menuSystem1 = new MenuSystemLibrary.MenuSystem();
			this.groupBox_name = new System.Windows.Forms.GroupBox();
			this.groupBox2 = new System.Windows.Forms.GroupBox();
			this.groupBox3 = new System.Windows.Forms.GroupBox();
			this.button1 = new System.Windows.Forms.Button();
			this.panel1.SuspendLayout();
			this.groupBox_name.SuspendLayout();
			this.groupBox2.SuspendLayout();
			this.groupBox3.SuspendLayout();
			this.SuspendLayout();
			// 
			// panel1
			// 
			this.panel1.Controls.Add(this.groupBox_name);
			this.panel1.Dock = System.Windows.Forms.DockStyle.Fill;
			this.panel1.Location = new System.Drawing.Point(0, 0);
			this.panel1.Name = "panel1";
			this.panel1.Size = new System.Drawing.Size(537, 453);
			this.panel1.TabIndex = 0;
			// 
			// menuSystem1
			// 
			this.menuSystem1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
						| System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.menuSystem1.BackColor = System.Drawing.SystemColors.Control;
			this.menuSystem1.Location = new System.Drawing.Point(6, 19);
			this.menuSystem1.Name = "menuSystem1";
			this.menuSystem1.Size = new System.Drawing.Size(507, 255);
			this.menuSystem1.TabIndex = 0;
			// 
			// groupBox_name
			// 
			this.groupBox_name.Controls.Add(this.groupBox3);
			this.groupBox_name.Controls.Add(this.groupBox2);
			this.groupBox_name.Dock = System.Windows.Forms.DockStyle.Fill;
			this.groupBox_name.Location = new System.Drawing.Point(0, 0);
			this.groupBox_name.Name = "groupBox_name";
			this.groupBox_name.Size = new System.Drawing.Size(537, 453);
			this.groupBox_name.TabIndex = 1;
			this.groupBox_name.TabStop = false;
			this.groupBox_name.Text = "Job";
			// 
			// groupBox2
			// 
			this.groupBox2.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
						| System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.groupBox2.Controls.Add(this.menuSystem1);
			this.groupBox2.Location = new System.Drawing.Point(6, 161);
			this.groupBox2.Name = "groupBox2";
			this.groupBox2.Size = new System.Drawing.Size(519, 280);
			this.groupBox2.TabIndex = 1;
			this.groupBox2.TabStop = false;
			this.groupBox2.Text = "Feedback";
			// 
			// groupBox3
			// 
			this.groupBox3.Controls.Add(this.button1);
			this.groupBox3.Location = new System.Drawing.Point(12, 19);
			this.groupBox3.Name = "groupBox3";
			this.groupBox3.Size = new System.Drawing.Size(513, 136);
			this.groupBox3.TabIndex = 2;
			this.groupBox3.TabStop = false;
			this.groupBox3.Text = "Stuff";
			// 
			// button1
			// 
			this.button1.Location = new System.Drawing.Point(6, 19);
			this.button1.Name = "button1";
			this.button1.Size = new System.Drawing.Size(75, 23);
			this.button1.TabIndex = 0;
			this.button1.Text = "Thing";
			this.button1.UseVisualStyleBackColor = true;
			// 
			// JobTab
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(537, 453);
			this.Controls.Add(this.panel1);
			this.Name = "JobTab";
			this.Text = "job_tab";
			this.panel1.ResumeLayout(false);
			this.groupBox_name.ResumeLayout(false);
			this.groupBox2.ResumeLayout(false);
			this.groupBox3.ResumeLayout(false);
			this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.Panel panel1;
		private MenuSystemLibrary.MenuSystem menuSystem1;
		private System.Windows.Forms.GroupBox groupBox_name;
		private System.Windows.Forms.GroupBox groupBox2;
		private System.Windows.Forms.GroupBox groupBox3;
		private System.Windows.Forms.Button button1;
	}
}