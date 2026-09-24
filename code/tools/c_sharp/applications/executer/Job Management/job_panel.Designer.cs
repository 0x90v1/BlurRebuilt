namespace Executer
{
	partial class JobPanel
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

			// Remove the hook into m_job.
			_job.JobStatusChangedEvent -= m_job_JobStatusChangedEvent;
			_job.JobMenuDataChangedEvent -= m_job_JobMenuDataChangedEvent;

			base.Dispose(disposing);
		}

		#region Component Designer generated code

		/// <summary> 
		/// Required method for Designer support - do not modify 
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.groupBoxMain = new System.Windows.Forms.GroupBox();
			this.menuSystem = new MenuSystemLibrary.MenuSystem();
			this.groupBox2 = new System.Windows.Forms.GroupBox();
			this.buttonClose = new System.Windows.Forms.Button();
			this.JobStatus = new System.Windows.Forms.Label();
			this.jobStatusLabelTitle = new System.Windows.Forms.Label();
			this.buttonKill = new System.Windows.Forms.Button();
			this.panelMain = new System.Windows.Forms.Panel();
			this.groupBoxMain.SuspendLayout();
			this.groupBox2.SuspendLayout();
			this.panelMain.SuspendLayout();
			this.SuspendLayout();
			// 
			// groupBoxMain
			// 
			this.groupBoxMain.Controls.Add(this.menuSystem);
			this.groupBoxMain.Controls.Add(this.groupBox2);
			this.groupBoxMain.Dock = System.Windows.Forms.DockStyle.Fill;
			this.groupBoxMain.Location = new System.Drawing.Point(0, 0);
			this.groupBoxMain.Name = "groupBoxMain";
			this.groupBoxMain.Size = new System.Drawing.Size(584, 468);
			this.groupBoxMain.TabIndex = 1;
			this.groupBoxMain.TabStop = false;
			this.groupBoxMain.Text = "Feedback";
			// 
			// menuSystem
			// 
			this.menuSystem.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
						| System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.menuSystem.AutoValidate = System.Windows.Forms.AutoValidate.Disable;
			this.menuSystem.BackColor = System.Drawing.SystemColors.Control;
			this.menuSystem.CausesValidation = false;
			this.menuSystem.Location = new System.Drawing.Point(3, 19);
			this.menuSystem.Name = "menuSystem";
			this.menuSystem.Size = new System.Drawing.Size(575, 386);
			this.menuSystem.TabIndex = 0;
			// 
			// groupBox2
			// 
			this.groupBox2.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.groupBox2.Controls.Add(this.buttonClose);
			this.groupBox2.Controls.Add(this.JobStatus);
			this.groupBox2.Controls.Add(this.jobStatusLabelTitle);
			this.groupBox2.Controls.Add(this.buttonKill);
			this.groupBox2.Location = new System.Drawing.Point(3, 411);
			this.groupBox2.Name = "groupBox2";
			this.groupBox2.Size = new System.Drawing.Size(572, 51);
			this.groupBox2.TabIndex = 2;
			this.groupBox2.TabStop = false;
			// 
			// buttonClose
			// 
			this.buttonClose.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonClose.Enabled = false;
			this.buttonClose.Location = new System.Drawing.Point(491, 19);
			this.buttonClose.Name = "buttonClose";
			this.buttonClose.Size = new System.Drawing.Size(75, 23);
			this.buttonClose.TabIndex = 3;
			this.buttonClose.Text = "Close Tab";
			this.buttonClose.UseVisualStyleBackColor = true;
			this.buttonClose.Click += new System.EventHandler(this.buttonClose_Click);
			// 
			// JobStatus
			// 
			this.JobStatus.AutoSize = true;
			this.JobStatus.Location = new System.Drawing.Point(69, 19);
			this.JobStatus.Name = "JobStatus";
			this.JobStatus.Size = new System.Drawing.Size(76, 13);
			this.JobStatus.TabIndex = 2;
			this.JobStatus.Text = "Please Wait ...";
			// 
			// jobStatusLabelTitle
			// 
			this.jobStatusLabelTitle.AutoSize = true;
			this.jobStatusLabelTitle.Location = new System.Drawing.Point(6, 19);
			this.jobStatusLabelTitle.Name = "jobStatusLabelTitle";
			this.jobStatusLabelTitle.Size = new System.Drawing.Size(60, 13);
			this.jobStatusLabelTitle.TabIndex = 1;
			this.jobStatusLabelTitle.Text = "Job Status:";
			// 
			// buttonKill
			// 
			this.buttonKill.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonKill.Enabled = false;
			this.buttonKill.Location = new System.Drawing.Point(410, 19);
			this.buttonKill.Name = "buttonKill";
			this.buttonKill.Size = new System.Drawing.Size(75, 23);
			this.buttonKill.TabIndex = 0;
			this.buttonKill.Text = "Kill Job";
			this.buttonKill.UseVisualStyleBackColor = true;
			this.buttonKill.Click += new System.EventHandler(this.buttonKill_Click);
			// 
			// panelMain
			// 
			this.panelMain.Controls.Add(this.groupBoxMain);
			this.panelMain.Dock = System.Windows.Forms.DockStyle.Fill;
			this.panelMain.Location = new System.Drawing.Point(0, 0);
			this.panelMain.Name = "panelMain";
			this.panelMain.Size = new System.Drawing.Size(584, 468);
			this.panelMain.TabIndex = 2;
			// 
			// JobPanel
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.Controls.Add(this.panelMain);
			this.MinimumSize = new System.Drawing.Size(356, 252);
			this.Name = "JobPanel";
			this.Size = new System.Drawing.Size(584, 468);
			this.groupBoxMain.ResumeLayout(false);
			this.groupBox2.ResumeLayout(false);
			this.groupBox2.PerformLayout();
			this.panelMain.ResumeLayout(false);
			this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.GroupBox groupBoxMain;
		private System.Windows.Forms.GroupBox groupBox2;
		private System.Windows.Forms.Panel panelMain;
		internal System.Windows.Forms.Button buttonKill;
		public MenuSystemLibrary.MenuSystem menuSystem;
		private System.Windows.Forms.Label jobStatusLabelTitle;
		private System.Windows.Forms.Label JobStatus;
		internal System.Windows.Forms.Button buttonClose;
	}
}
