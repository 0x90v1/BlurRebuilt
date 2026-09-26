namespace Executer
{
	partial class Form_NewDatafile
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
			this.folderBrowserDialog1 = new System.Windows.Forms.FolderBrowserDialog();
			this.groupBox1 = new System.Windows.Forms.GroupBox();
			this.button_browseDatafile = new System.Windows.Forms.Button();
			this.textBox_dataFile = new System.Windows.Forms.TextBox();
			this.textBox_templateFile = new System.Windows.Forms.TextBox();
			this.groupBox2 = new System.Windows.Forms.GroupBox();
			this.button_browseTemplate = new System.Windows.Forms.Button();
			this.groupBox3 = new System.Windows.Forms.GroupBox();
			this.buttonCancel = new System.Windows.Forms.Button();
			this.buttonOk = new System.Windows.Forms.Button();
			this.checkBox_createDefaultTree = new System.Windows.Forms.CheckBox();
			this.groupBox1.SuspendLayout();
			this.groupBox2.SuspendLayout();
			this.groupBox3.SuspendLayout();
			this.SuspendLayout();
			// 
			// groupBox1
			// 
			this.groupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.groupBox1.Controls.Add(this.textBox_dataFile);
			this.groupBox1.Controls.Add(this.button_browseDatafile);
			this.groupBox1.Location = new System.Drawing.Point(12, 74);
			this.groupBox1.Name = "groupBox1";
			this.groupBox1.Size = new System.Drawing.Size(473, 49);
			this.groupBox1.TabIndex = 0;
			this.groupBox1.TabStop = false;
			this.groupBox1.Text = "Project filename";
			// 
			// button_browseDatafile
			// 
			this.button_browseDatafile.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.button_browseDatafile.Location = new System.Drawing.Point(392, 17);
			this.button_browseDatafile.Name = "button_browseDatafile";
			this.button_browseDatafile.Size = new System.Drawing.Size(75, 23);
			this.button_browseDatafile.TabIndex = 3;
			this.button_browseDatafile.Text = "Browse";
			this.button_browseDatafile.UseVisualStyleBackColor = true;
			this.button_browseDatafile.Click += new System.EventHandler(this.button_browseDatafile_Click);
			// 
			// textBox_dataFile
			// 
			this.textBox_dataFile.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.textBox_dataFile.Location = new System.Drawing.Point(6, 19);
			this.textBox_dataFile.Name = "textBox_dataFile";
			this.textBox_dataFile.Size = new System.Drawing.Size(380, 20);
			this.textBox_dataFile.TabIndex = 2;
			// 
			// textBox_templateFile
			// 
			this.textBox_templateFile.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.textBox_templateFile.Location = new System.Drawing.Point(6, 19);
			this.textBox_templateFile.Name = "textBox_templateFile";
			this.textBox_templateFile.Size = new System.Drawing.Size(380, 20);
			this.textBox_templateFile.TabIndex = 0;
			// 
			// groupBox2
			// 
			this.groupBox2.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.groupBox2.Controls.Add(this.textBox_templateFile);
			this.groupBox2.Controls.Add(this.button_browseTemplate);
			this.groupBox2.Location = new System.Drawing.Point(12, 19);
			this.groupBox2.Name = "groupBox2";
			this.groupBox2.Size = new System.Drawing.Size(473, 49);
			this.groupBox2.TabIndex = 0;
			this.groupBox2.TabStop = false;
			this.groupBox2.Text = "Template filename";
			// 
			// button_browseTemplate
			// 
			this.button_browseTemplate.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.button_browseTemplate.Location = new System.Drawing.Point(392, 17);
			this.button_browseTemplate.Name = "button_browseTemplate";
			this.button_browseTemplate.Size = new System.Drawing.Size(75, 23);
			this.button_browseTemplate.TabIndex = 1;
			this.button_browseTemplate.Text = "Browse";
			this.button_browseTemplate.UseVisualStyleBackColor = true;
			this.button_browseTemplate.Click += new System.EventHandler(this.button_browseTemplate_Click);
			// 
			// groupBox3
			// 
			this.groupBox3.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
						| System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.groupBox3.Controls.Add(this.groupBox2);
			this.groupBox3.Controls.Add(this.groupBox1);
			this.groupBox3.Controls.Add(this.checkBox_createDefaultTree);
			this.groupBox3.Controls.Add(this.buttonOk);
			this.groupBox3.Controls.Add(this.buttonCancel);
			this.groupBox3.Location = new System.Drawing.Point(12, 12);
			this.groupBox3.Name = "groupBox3";
			this.groupBox3.Size = new System.Drawing.Size(491, 158);
			this.groupBox3.TabIndex = 0;
			this.groupBox3.TabStop = false;
			this.groupBox3.Text = "New project";
			// 
			// buttonCancel
			// 
			this.buttonCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.buttonCancel.Location = new System.Drawing.Point(404, 129);
			this.buttonCancel.Name = "buttonCancel";
			this.buttonCancel.Size = new System.Drawing.Size(75, 23);
			this.buttonCancel.TabIndex = 6;
			this.buttonCancel.Text = "Cancel";
			this.buttonCancel.UseVisualStyleBackColor = true;
			this.buttonCancel.Click += new System.EventHandler(this.buttonCancel_Click);
			// 
			// buttonOk
			// 
			this.buttonOk.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonOk.Location = new System.Drawing.Point(326, 129);
			this.buttonOk.Name = "buttonOk";
			this.buttonOk.Size = new System.Drawing.Size(75, 23);
			this.buttonOk.TabIndex = 5;
			this.buttonOk.Text = "OK";
			this.buttonOk.UseVisualStyleBackColor = true;
			this.buttonOk.Click += new System.EventHandler(this.buttonOk_Click);
			// 
			// checkBox_createDefaultTree
			// 
			this.checkBox_createDefaultTree.AutoSize = true;
			this.checkBox_createDefaultTree.Location = new System.Drawing.Point(18, 133);
			this.checkBox_createDefaultTree.Name = "checkBox_createDefaultTree";
			this.checkBox_createDefaultTree.Size = new System.Drawing.Size(197, 17);
			this.checkBox_createDefaultTree.TabIndex = 4;
			this.checkBox_createDefaultTree.Text = "Create default tree if one is available";
			this.checkBox_createDefaultTree.UseVisualStyleBackColor = true;
			// 
			// Form_NewDatafile
			// 
			this.AcceptButton = this.buttonOk;
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.CancelButton = this.buttonCancel;
			this.ClientSize = new System.Drawing.Size(506, 180);
			this.Controls.Add(this.groupBox3);
			this.MaximizeBox = false;
			this.MaximumSize = new System.Drawing.Size(1280, 214);
			this.MinimizeBox = false;
			this.MinimumSize = new System.Drawing.Size(407, 214);
			this.Name = "Form_NewDatafile";
			this.Text = "Shitstorm";
			this.groupBox1.ResumeLayout(false);
			this.groupBox1.PerformLayout();
			this.groupBox2.ResumeLayout(false);
			this.groupBox2.PerformLayout();
			this.groupBox3.ResumeLayout(false);
			this.groupBox3.PerformLayout();
			this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.FolderBrowserDialog folderBrowserDialog1;
		private System.Windows.Forms.GroupBox groupBox1;
		private System.Windows.Forms.Button button_browseDatafile;
		private System.Windows.Forms.TextBox textBox_dataFile;
		private System.Windows.Forms.GroupBox groupBox2;
		private System.Windows.Forms.Button button_browseTemplate;
		private System.Windows.Forms.TextBox textBox_templateFile;
		private System.Windows.Forms.GroupBox groupBox3;
		private System.Windows.Forms.CheckBox checkBox_createDefaultTree;
		private System.Windows.Forms.Button buttonCancel;
		private System.Windows.Forms.Button buttonOk;
	}
}