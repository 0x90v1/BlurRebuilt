namespace RoboPublish
{
	partial class ConfigureSorcery
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
			this.panelMain = new System.Windows.Forms.Panel();
			this.groupBoxMain = new System.Windows.Forms.GroupBox();
			this.checkBoxStripExtensions = new System.Windows.Forms.CheckBox();
			this.buttonRemoveQuery = new System.Windows.Forms.Button();
			this.buttonAddQuery = new System.Windows.Forms.Button();
			this.textBoxSorcefyQueryLine = new System.Windows.Forms.TextBox();
			this.listViewSorceryQuery = new System.Windows.Forms.ListView();
			this.textBoxSorceryDatabase = new System.Windows.Forms.TextBox();
			this.buttonBrowseFile = new System.Windows.Forms.Button();
			this.groupBoxMatches = new System.Windows.Forms.GroupBox();
			this.radioButtonMismatches = new System.Windows.Forms.RadioButton();
			this.radioButtonMatches = new System.Windows.Forms.RadioButton();
			this.buttonCancel = new System.Windows.Forms.Button();
			this.buttonOk = new System.Windows.Forms.Button();
			this.buttonDefault = new System.Windows.Forms.Button();
			this.labelFileToReadIn = new System.Windows.Forms.Label();
			this.labelSorceryQuery = new System.Windows.Forms.Label();
			this.panelMain.SuspendLayout();
			this.groupBoxMain.SuspendLayout();
			this.groupBoxMatches.SuspendLayout();
			this.SuspendLayout();
			// 
			// panelMain
			// 
			this.panelMain.Controls.Add(this.groupBoxMain);
			this.panelMain.Dock = System.Windows.Forms.DockStyle.Fill;
			this.panelMain.Location = new System.Drawing.Point(0, 0);
			this.panelMain.Name = "panelMain";
			this.panelMain.Size = new System.Drawing.Size(606, 399);
			this.panelMain.TabIndex = 0;
			// 
			// groupBoxMain
			// 
			this.groupBoxMain.Controls.Add(this.checkBoxStripExtensions);
			this.groupBoxMain.Controls.Add(this.buttonRemoveQuery);
			this.groupBoxMain.Controls.Add(this.buttonAddQuery);
			this.groupBoxMain.Controls.Add(this.textBoxSorcefyQueryLine);
			this.groupBoxMain.Controls.Add(this.listViewSorceryQuery);
			this.groupBoxMain.Controls.Add(this.textBoxSorceryDatabase);
			this.groupBoxMain.Controls.Add(this.buttonBrowseFile);
			this.groupBoxMain.Controls.Add(this.groupBoxMatches);
			this.groupBoxMain.Controls.Add(this.buttonCancel);
			this.groupBoxMain.Controls.Add(this.buttonOk);
			this.groupBoxMain.Controls.Add(this.buttonDefault);
			this.groupBoxMain.Controls.Add(this.labelFileToReadIn);
			this.groupBoxMain.Controls.Add(this.labelSorceryQuery);
			this.groupBoxMain.Dock = System.Windows.Forms.DockStyle.Fill;
			this.groupBoxMain.Location = new System.Drawing.Point(0, 0);
			this.groupBoxMain.Name = "groupBoxMain";
			this.groupBoxMain.Size = new System.Drawing.Size(606, 399);
			this.groupBoxMain.TabIndex = 0;
			this.groupBoxMain.TabStop = false;
			// 
			// checkBoxStripExtensions
			// 
			this.checkBoxStripExtensions.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.checkBoxStripExtensions.AutoSize = true;
			this.checkBoxStripExtensions.Location = new System.Drawing.Point(12, 339);
			this.checkBoxStripExtensions.Name = "checkBoxStripExtensions";
			this.checkBoxStripExtensions.Size = new System.Drawing.Size(211, 17);
			this.checkBoxStripExtensions.TabIndex = 18;
			this.checkBoxStripExtensions.Text = "Strip file extensions before comparisons";
			this.checkBoxStripExtensions.UseVisualStyleBackColor = true;
			this.checkBoxStripExtensions.CheckedChanged += new System.EventHandler(this.checkBoxStripExtensions_CheckedChanged);
			// 
			// buttonRemoveQuery
			// 
			this.buttonRemoveQuery.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.buttonRemoveQuery.Location = new System.Drawing.Point(12, 222);
			this.buttonRemoveQuery.Name = "buttonRemoveQuery";
			this.buttonRemoveQuery.Size = new System.Drawing.Size(148, 23);
			this.buttonRemoveQuery.TabIndex = 17;
			this.buttonRemoveQuery.Text = "Remove Selected Queries";
			this.buttonRemoveQuery.UseVisualStyleBackColor = true;
			this.buttonRemoveQuery.Click += new System.EventHandler(this.buttonRemoveQuery_Click);
			// 
			// buttonAddQuery
			// 
			this.buttonAddQuery.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonAddQuery.Location = new System.Drawing.Point(523, 59);
			this.buttonAddQuery.Name = "buttonAddQuery";
			this.buttonAddQuery.Size = new System.Drawing.Size(71, 23);
			this.buttonAddQuery.TabIndex = 16;
			this.buttonAddQuery.Text = "Add Query";
			this.buttonAddQuery.UseVisualStyleBackColor = true;
			this.buttonAddQuery.Click += new System.EventHandler(this.buttonAddQuery_Click);
			// 
			// textBoxSorcefyQueryLine
			// 
			this.textBoxSorcefyQueryLine.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.textBoxSorcefyQueryLine.Location = new System.Drawing.Point(12, 62);
			this.textBoxSorcefyQueryLine.Name = "textBoxSorcefyQueryLine";
			this.textBoxSorcefyQueryLine.Size = new System.Drawing.Size(505, 20);
			this.textBoxSorcefyQueryLine.TabIndex = 15;
			// 
			// listViewSorceryQuery
			// 
			this.listViewSorceryQuery.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
						| System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.listViewSorceryQuery.Location = new System.Drawing.Point(12, 88);
			this.listViewSorceryQuery.Name = "listViewSorceryQuery";
			this.listViewSorceryQuery.Size = new System.Drawing.Size(581, 127);
			this.listViewSorceryQuery.TabIndex = 14;
			this.listViewSorceryQuery.UseCompatibleStateImageBehavior = false;
			this.listViewSorceryQuery.View = System.Windows.Forms.View.List;
			// 
			// textBoxSorceryDatabase
			// 
			this.textBoxSorceryDatabase.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.textBoxSorceryDatabase.Location = new System.Drawing.Point(105, 19);
			this.textBoxSorceryDatabase.Name = "textBoxSorceryDatabase";
			this.textBoxSorceryDatabase.Size = new System.Drawing.Size(456, 20);
			this.textBoxSorceryDatabase.TabIndex = 13;
			// 
			// buttonBrowseFile
			// 
			this.buttonBrowseFile.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonBrowseFile.Location = new System.Drawing.Point(567, 17);
			this.buttonBrowseFile.Name = "buttonBrowseFile";
			this.buttonBrowseFile.Size = new System.Drawing.Size(26, 23);
			this.buttonBrowseFile.TabIndex = 12;
			this.buttonBrowseFile.Text = "...";
			this.buttonBrowseFile.UseVisualStyleBackColor = true;
			this.buttonBrowseFile.Click += new System.EventHandler(this.buttonBrowseFile_Click);
			// 
			// groupBoxMatches
			// 
			this.groupBoxMatches.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.groupBoxMatches.Controls.Add(this.radioButtonMismatches);
			this.groupBoxMatches.Controls.Add(this.radioButtonMatches);
			this.groupBoxMatches.Location = new System.Drawing.Point(12, 251);
			this.groupBoxMatches.Name = "groupBoxMatches";
			this.groupBoxMatches.Size = new System.Drawing.Size(114, 82);
			this.groupBoxMatches.TabIndex = 11;
			this.groupBoxMatches.TabStop = false;
			this.groupBoxMatches.Text = "Files to check";
			// 
			// radioButtonMismatches
			// 
			this.radioButtonMismatches.AutoSize = true;
			this.radioButtonMismatches.Checked = true;
			this.radioButtonMismatches.Location = new System.Drawing.Point(9, 54);
			this.radioButtonMismatches.Name = "radioButtonMismatches";
			this.radioButtonMismatches.Size = new System.Drawing.Size(81, 17);
			this.radioButtonMismatches.TabIndex = 7;
			this.radioButtonMismatches.TabStop = true;
			this.radioButtonMismatches.Text = "Mismatches";
			this.radioButtonMismatches.UseVisualStyleBackColor = true;
			this.radioButtonMismatches.CheckedChanged += new System.EventHandler(this.radioButtonMismatches_CheckedChanged);
			// 
			// radioButtonMatches
			// 
			this.radioButtonMatches.AutoSize = true;
			this.radioButtonMatches.Location = new System.Drawing.Point(9, 19);
			this.radioButtonMatches.Name = "radioButtonMatches";
			this.radioButtonMatches.Size = new System.Drawing.Size(66, 17);
			this.radioButtonMatches.TabIndex = 6;
			this.radioButtonMatches.TabStop = true;
			this.radioButtonMatches.Text = "Matches";
			this.radioButtonMatches.UseVisualStyleBackColor = true;
			this.radioButtonMatches.CheckedChanged += new System.EventHandler(this.radioButtonMatches_CheckedChanged);
			// 
			// buttonCancel
			// 
			this.buttonCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonCancel.Location = new System.Drawing.Point(519, 364);
			this.buttonCancel.Name = "buttonCancel";
			this.buttonCancel.Size = new System.Drawing.Size(75, 23);
			this.buttonCancel.TabIndex = 10;
			this.buttonCancel.Text = "Cancel";
			this.buttonCancel.UseVisualStyleBackColor = true;
			this.buttonCancel.Click += new System.EventHandler(this.buttonCancel_Click);
			// 
			// buttonOk
			// 
			this.buttonOk.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonOk.Location = new System.Drawing.Point(438, 364);
			this.buttonOk.Name = "buttonOk";
			this.buttonOk.Size = new System.Drawing.Size(75, 23);
			this.buttonOk.TabIndex = 9;
			this.buttonOk.Text = "OK";
			this.buttonOk.UseVisualStyleBackColor = true;
			this.buttonOk.Click += new System.EventHandler(this.buttonOk_Click);
			// 
			// buttonDefault
			// 
			this.buttonDefault.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonDefault.Location = new System.Drawing.Point(357, 364);
			this.buttonDefault.Name = "buttonDefault";
			this.buttonDefault.Size = new System.Drawing.Size(75, 23);
			this.buttonDefault.TabIndex = 8;
			this.buttonDefault.Text = "Default";
			this.buttonDefault.UseVisualStyleBackColor = true;
			this.buttonDefault.Click += new System.EventHandler(this.buttonDefault_Click);
			// 
			// labelFileToReadIn
			// 
			this.labelFileToReadIn.AutoSize = true;
			this.labelFileToReadIn.Location = new System.Drawing.Point(9, 22);
			this.labelFileToReadIn.Name = "labelFileToReadIn";
			this.labelFileToReadIn.Size = new System.Drawing.Size(90, 13);
			this.labelFileToReadIn.TabIndex = 1;
			this.labelFileToReadIn.Text = "Sorcery database";
			// 
			// labelSorceryQuery
			// 
			this.labelSorceryQuery.AutoSize = true;
			this.labelSorceryQuery.Location = new System.Drawing.Point(12, 46);
			this.labelSorceryQuery.Name = "labelSorceryQuery";
			this.labelSorceryQuery.Size = new System.Drawing.Size(74, 13);
			this.labelSorceryQuery.TabIndex = 0;
			this.labelSorceryQuery.Text = "Sorcery Query";
			// 
			// ConfigureSorcery
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(606, 399);
			this.Controls.Add(this.panelMain);
			this.MinimumSize = new System.Drawing.Size(264, 359);
			this.Name = "ConfigureSorcery";
			this.Load += new System.EventHandler(this.ConfigureSorcery_Load);
			this.panelMain.ResumeLayout(false);
			this.groupBoxMain.ResumeLayout(false);
			this.groupBoxMain.PerformLayout();
			this.groupBoxMatches.ResumeLayout(false);
			this.groupBoxMatches.PerformLayout();
			this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.Panel panelMain;
		private System.Windows.Forms.GroupBox groupBoxMain;
		private System.Windows.Forms.Label labelFileToReadIn;
		private System.Windows.Forms.Label labelSorceryQuery;
		private System.Windows.Forms.Button buttonCancel;
		private System.Windows.Forms.Button buttonOk;
		private System.Windows.Forms.Button buttonDefault;
		private System.Windows.Forms.RadioButton radioButtonMismatches;
		private System.Windows.Forms.RadioButton radioButtonMatches;
		private System.Windows.Forms.Button buttonBrowseFile;
		private System.Windows.Forms.GroupBox groupBoxMatches;
		private System.Windows.Forms.ListView listViewSorceryQuery;
		private System.Windows.Forms.TextBox textBoxSorceryDatabase;
		private System.Windows.Forms.TextBox textBoxSorcefyQueryLine;
		private System.Windows.Forms.Button buttonRemoveQuery;
		private System.Windows.Forms.Button buttonAddQuery;
		private System.Windows.Forms.CheckBox checkBoxStripExtensions;
	}
}