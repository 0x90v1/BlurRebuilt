namespace Executer
{
	partial class FarmConnection
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
			System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(FarmConnection));
			this.panelMain = new System.Windows.Forms.Panel();
			this.panel_serverFeedback = new System.Windows.Forms.Panel();
			this.groupBox_Servers = new System.Windows.Forms.GroupBox();
			this.superListView_ServerList = new SuperControl.SuperListView();
			this.groupBox1 = new System.Windows.Forms.GroupBox();
			this.panelMain.SuspendLayout();
			this.groupBox_Servers.SuspendLayout();
			this.groupBox1.SuspendLayout();
			this.SuspendLayout();
			// 
			// panelMain
			// 
			this.panelMain.Controls.Add(this.groupBox1);
			this.panelMain.Controls.Add(this.groupBox_Servers);
			this.panelMain.Dock = System.Windows.Forms.DockStyle.Fill;
			this.panelMain.Location = new System.Drawing.Point(0, 0);
			this.panelMain.Name = "panelMain";
			this.panelMain.Size = new System.Drawing.Size(620, 357);
			this.panelMain.TabIndex = 0;
			// 
			// panel_serverFeedback
			// 
			this.panel_serverFeedback.Dock = System.Windows.Forms.DockStyle.Fill;
			this.panel_serverFeedback.Location = new System.Drawing.Point(3, 16);
			this.panel_serverFeedback.Name = "panel_serverFeedback";
			this.panel_serverFeedback.Size = new System.Drawing.Size(605, 113);
			this.panel_serverFeedback.TabIndex = 3;
			// 
			// groupBox_Servers
			// 
			this.groupBox_Servers.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.groupBox_Servers.Controls.Add(this.superListView_ServerList);
			this.groupBox_Servers.Location = new System.Drawing.Point(3, 12);
			this.groupBox_Servers.MinimumSize = new System.Drawing.Size(198, 19);
			this.groupBox_Servers.Name = "groupBox_Servers";
			this.groupBox_Servers.Size = new System.Drawing.Size(614, 207);
			this.groupBox_Servers.TabIndex = 0;
			this.groupBox_Servers.TabStop = false;
			this.groupBox_Servers.Text = "Servers";
			// 
			// superListView_ServerList
			// 
			this.superListView_ServerList.Dock = System.Windows.Forms.DockStyle.Fill;
			this.superListView_ServerList.FullRowSelect = true;
			this.superListView_ServerList.Location = new System.Drawing.Point(3, 16);
			this.superListView_ServerList.Name = "superListView_ServerList";
			this.superListView_ServerList.Size = new System.Drawing.Size(608, 188);
			this.superListView_ServerList.TabIndex = 0;
			this.superListView_ServerList.UseCompatibleStateImageBehavior = false;
			this.superListView_ServerList.View = System.Windows.Forms.View.Details;
			this.superListView_ServerList.SelectedIndexChanged += new System.EventHandler(this.superListView_ServerList_SelectedIndexChanged);
			// 
			// groupBox1
			// 
			this.groupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
						| System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.groupBox1.Controls.Add(this.panel_serverFeedback);
			this.groupBox1.Location = new System.Drawing.Point(6, 222);
			this.groupBox1.Name = "groupBox1";
			this.groupBox1.Size = new System.Drawing.Size(611, 132);
			this.groupBox1.TabIndex = 4;
			this.groupBox1.TabStop = false;
			this.groupBox1.Text = "Server conversation";
			// 
			// FarmConnection
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(620, 357);
			this.Controls.Add(this.panelMain);
			this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
			this.MinimizeBox = false;
			this.MinimumSize = new System.Drawing.Size(442, 314);
			this.Name = "FarmConnection";
			this.ShowInTaskbar = false;
			this.Text = "farm_connection";
			this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.FarmConnection_FormClosing);
			this.panelMain.ResumeLayout(false);
			this.groupBox_Servers.ResumeLayout(false);
			this.groupBox1.ResumeLayout(false);
			this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.Panel panelMain;
		private System.Windows.Forms.GroupBox groupBox_Servers;
		private SuperControl.SuperListView superListView_ServerList;
		private System.Windows.Forms.Panel panel_serverFeedback;
		private System.Windows.Forms.GroupBox groupBox1;
	}
}