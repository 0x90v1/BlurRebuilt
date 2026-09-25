namespace Executer
{
	partial class JobViewer
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
			this.panelMain = new System.Windows.Forms.Panel();
			this.buttonLocate = new System.Windows.Forms.Button();
			this.buttonKill = new System.Windows.Forms.Button();
			this.buttonClose = new System.Windows.Forms.Button();
			this.buttonClear = new System.Windows.Forms.Button();
			this.buttonCleanup = new System.Windows.Forms.Button();
			this.superListView = new SuperControl.SuperListView();
			this.panelMain.SuspendLayout();
			this.SuspendLayout();
			// 
			// panelMain
			// 
			this.panelMain.Controls.Add(this.buttonLocate);
			this.panelMain.Controls.Add(this.buttonKill);
			this.panelMain.Controls.Add(this.buttonClose);
			this.panelMain.Controls.Add(this.buttonClear);
			this.panelMain.Controls.Add(this.buttonCleanup);
			this.panelMain.Controls.Add(this.superListView);
			this.panelMain.Dock = System.Windows.Forms.DockStyle.Fill;
			this.panelMain.Location = new System.Drawing.Point(0, 0);
			this.panelMain.Name = "panelMain";
			this.panelMain.Size = new System.Drawing.Size(551, 382);
			this.panelMain.TabIndex = 1;
			// 
			// buttonLocate
			// 
			this.buttonLocate.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.buttonLocate.Location = new System.Drawing.Point(257, 347);
			this.buttonLocate.Name = "buttonLocate";
			this.buttonLocate.Size = new System.Drawing.Size(75, 23);
			this.buttonLocate.TabIndex = 5;
			this.buttonLocate.Text = "Locate";
			this.buttonLocate.UseVisualStyleBackColor = true;
			this.buttonLocate.Click += new System.EventHandler(this.buttonLocateSelected_Click);
			// 
			// buttonKill
			// 
			this.buttonKill.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.buttonKill.Location = new System.Drawing.Point(175, 347);
			this.buttonKill.Name = "buttonKill";
			this.buttonKill.Size = new System.Drawing.Size(75, 23);
			this.buttonKill.TabIndex = 4;
			this.buttonKill.Text = "Kill";
			this.buttonKill.UseVisualStyleBackColor = true;
			this.buttonKill.Click += new System.EventHandler(this.buttonKill_Click);
			// 
			// buttonClose
			// 
			this.buttonClose.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonClose.Location = new System.Drawing.Point(464, 347);
			this.buttonClose.Name = "buttonClose";
			this.buttonClose.Size = new System.Drawing.Size(75, 23);
			this.buttonClose.TabIndex = 3;
			this.buttonClose.Text = "Close";
			this.buttonClose.UseVisualStyleBackColor = true;
			this.buttonClose.Click += new System.EventHandler(this.buttonClose_Click);
			// 
			// buttonClear
			// 
			this.buttonClear.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.buttonClear.Location = new System.Drawing.Point(94, 347);
			this.buttonClear.Name = "buttonClear";
			this.buttonClear.Size = new System.Drawing.Size(75, 23);
			this.buttonClear.TabIndex = 2;
			this.buttonClear.Text = "Clear All";
			this.buttonClear.UseVisualStyleBackColor = true;
			this.buttonClear.Click += new System.EventHandler(this.buttonClear_Click);
			// 
			// buttonCleanup
			// 
			this.buttonCleanup.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.buttonCleanup.Location = new System.Drawing.Point(13, 347);
			this.buttonCleanup.Name = "buttonCleanup";
			this.buttonCleanup.Size = new System.Drawing.Size(75, 23);
			this.buttonCleanup.TabIndex = 1;
			this.buttonCleanup.Text = "Cleanup";
			this.buttonCleanup.UseVisualStyleBackColor = true;
			this.buttonCleanup.Click += new System.EventHandler(this.buttonCleanup_Click);
			// 
			// superListView
			// 
			this.superListView.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
						| System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.superListView.Location = new System.Drawing.Point(12, 27);
			this.superListView.Name = "superListView";
			this.superListView.Size = new System.Drawing.Size(527, 314);
			this.superListView.TabIndex = 0;
			this.superListView.UseCompatibleStateImageBehavior = false;
			// 
			// JobViewer
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.AutoScroll = true;
			this.ClientSize = new System.Drawing.Size(551, 382);
			this.Controls.Add(this.panelMain);
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.MinimumSize = new System.Drawing.Size(466, 180);
			this.Name = "JobViewer";
			this.ShowInTaskbar = false;
			this.Text = "Executer - Job Viewer";
			this.VisibleChanged += new System.EventHandler(this.JobViewer_VisibleChanged);
			this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.JobViewer_FormClosing);
			this.panelMain.ResumeLayout(false);
			this.ResumeLayout(false);

		}

		#endregion

		private SuperControl.SuperListView superListView;
		private System.Windows.Forms.Panel panelMain;
		private System.Windows.Forms.Button buttonClose;
		private System.Windows.Forms.Button buttonClear;
		private System.Windows.Forms.Button buttonCleanup;
		private System.Windows.Forms.Button buttonLocate;
		private System.Windows.Forms.Button buttonKill;
	}
}