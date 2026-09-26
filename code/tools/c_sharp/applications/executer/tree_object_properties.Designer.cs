namespace Executer
{
	partial class TreeObjectProperties
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
			this.buttonClose = new System.Windows.Forms.Button();
			this.groupBoxMain = new System.Windows.Forms.GroupBox();
			this.textBoxProjectName = new System.Windows.Forms.TextBox();
			this.labelProjectName = new System.Windows.Forms.Label();
			this.buttonOpenProject = new System.Windows.Forms.Button();
			this.labelTemplateName = new System.Windows.Forms.Label();
			this.buttonOpenTemplate = new System.Windows.Forms.Button();
			this.textBoxTemplateName = new System.Windows.Forms.TextBox();
			this.groupBoxServers = new System.Windows.Forms.GroupBox();
			this.richTextBox1 = new System.Windows.Forms.RichTextBox();
			this.buttonOpenTemporaryFolder = new System.Windows.Forms.Button();
			this.textBoxTemporaryFolder = new System.Windows.Forms.TextBox();
			this.labelTemporaryFolder = new System.Windows.Forms.Label();
			this.groupBoxMain.SuspendLayout();
			this.groupBoxServers.SuspendLayout();
			this.SuspendLayout();
			// 
			// buttonClose
			// 
			this.buttonClose.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonClose.Location = new System.Drawing.Point(397, 233);
			this.buttonClose.Name = "buttonClose";
			this.buttonClose.Size = new System.Drawing.Size(75, 23);
			this.buttonClose.TabIndex = 0;
			this.buttonClose.Text = "Close";
			this.buttonClose.UseVisualStyleBackColor = true;
			this.buttonClose.Click += new System.EventHandler(this.button1_Click);
			// 
			// groupBoxMain
			// 
			this.groupBoxMain.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
						| System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.groupBoxMain.Controls.Add(this.buttonOpenTemporaryFolder);
			this.groupBoxMain.Controls.Add(this.textBoxTemporaryFolder);
			this.groupBoxMain.Controls.Add(this.labelTemporaryFolder);
			this.groupBoxMain.Controls.Add(this.groupBoxServers);
			this.groupBoxMain.Controls.Add(this.buttonOpenTemplate);
			this.groupBoxMain.Controls.Add(this.textBoxTemplateName);
			this.groupBoxMain.Controls.Add(this.labelTemplateName);
			this.groupBoxMain.Controls.Add(this.buttonOpenProject);
			this.groupBoxMain.Controls.Add(this.labelProjectName);
			this.groupBoxMain.Controls.Add(this.textBoxProjectName);
			this.groupBoxMain.Location = new System.Drawing.Point(12, 12);
			this.groupBoxMain.Name = "groupBoxMain";
			this.groupBoxMain.Size = new System.Drawing.Size(460, 215);
			this.groupBoxMain.TabIndex = 1;
			this.groupBoxMain.TabStop = false;
			this.groupBoxMain.Text = "Things";
			// 
			// textBoxProjectName
			// 
			this.textBoxProjectName.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.textBoxProjectName.Location = new System.Drawing.Point(92, 13);
			this.textBoxProjectName.Name = "textBoxProjectName";
			this.textBoxProjectName.ReadOnly = true;
			this.textBoxProjectName.Size = new System.Drawing.Size(281, 20);
			this.textBoxProjectName.TabIndex = 0;
			// 
			// labelProjectName
			// 
			this.labelProjectName.AutoSize = true;
			this.labelProjectName.Location = new System.Drawing.Point(6, 16);
			this.labelProjectName.Name = "labelProjectName";
			this.labelProjectName.Size = new System.Drawing.Size(69, 13);
			this.labelProjectName.TabIndex = 1;
			this.labelProjectName.Text = "Project name";
			// 
			// buttonOpenProject
			// 
			this.buttonOpenProject.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonOpenProject.Location = new System.Drawing.Point(379, 11);
			this.buttonOpenProject.Name = "buttonOpenProject";
			this.buttonOpenProject.Size = new System.Drawing.Size(75, 23);
			this.buttonOpenProject.TabIndex = 2;
			this.buttonOpenProject.Text = "Open";
			this.buttonOpenProject.UseVisualStyleBackColor = true;
			this.buttonOpenProject.Click += new System.EventHandler(this.buttonOpenProject_Click);
			// 
			// labelTemplateName
			// 
			this.labelTemplateName.AutoSize = true;
			this.labelTemplateName.Location = new System.Drawing.Point(6, 43);
			this.labelTemplateName.Name = "labelTemplateName";
			this.labelTemplateName.Size = new System.Drawing.Size(80, 13);
			this.labelTemplateName.TabIndex = 3;
			this.labelTemplateName.Text = "Template name";
			// 
			// buttonOpenTemplate
			// 
			this.buttonOpenTemplate.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonOpenTemplate.Location = new System.Drawing.Point(379, 38);
			this.buttonOpenTemplate.Name = "buttonOpenTemplate";
			this.buttonOpenTemplate.Size = new System.Drawing.Size(75, 23);
			this.buttonOpenTemplate.TabIndex = 5;
			this.buttonOpenTemplate.Text = "Open";
			this.buttonOpenTemplate.UseVisualStyleBackColor = true;
			this.buttonOpenTemplate.Click += new System.EventHandler(this.buttonOpenTemplate_Click);
			// 
			// textBoxTemplateName
			// 
			this.textBoxTemplateName.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.textBoxTemplateName.Location = new System.Drawing.Point(92, 40);
			this.textBoxTemplateName.Name = "textBoxTemplateName";
			this.textBoxTemplateName.ReadOnly = true;
			this.textBoxTemplateName.Size = new System.Drawing.Size(281, 20);
			this.textBoxTemplateName.TabIndex = 4;
			// 
			// groupBoxServers
			// 
			this.groupBoxServers.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
						| System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.groupBoxServers.Controls.Add(this.richTextBox1);
			this.groupBoxServers.Location = new System.Drawing.Point(9, 93);
			this.groupBoxServers.Name = "groupBoxServers";
			this.groupBoxServers.Size = new System.Drawing.Size(445, 116);
			this.groupBoxServers.TabIndex = 6;
			this.groupBoxServers.TabStop = false;
			this.groupBoxServers.Text = "Servers required by the template";
			// 
			// richTextBox1
			// 
			this.richTextBox1.Dock = System.Windows.Forms.DockStyle.Fill;
			this.richTextBox1.HideSelection = false;
			this.richTextBox1.Location = new System.Drawing.Point(3, 16);
			this.richTextBox1.Name = "richTextBox1";
			this.richTextBox1.ReadOnly = true;
			this.richTextBox1.Size = new System.Drawing.Size(439, 97);
			this.richTextBox1.TabIndex = 0;
			this.richTextBox1.Text = "";
			// 
			// buttonOpenTemporaryFolder
			// 
			this.buttonOpenTemporaryFolder.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonOpenTemporaryFolder.Location = new System.Drawing.Point(379, 64);
			this.buttonOpenTemporaryFolder.Name = "buttonOpenTemporaryFolder";
			this.buttonOpenTemporaryFolder.Size = new System.Drawing.Size(75, 23);
			this.buttonOpenTemporaryFolder.TabIndex = 9;
			this.buttonOpenTemporaryFolder.Text = "Open";
			this.buttonOpenTemporaryFolder.UseVisualStyleBackColor = true;
			this.buttonOpenTemporaryFolder.Click += new System.EventHandler(this.buttonOpenTemporaryFolder_Click);
			// 
			// textBoxTemporaryFolder
			// 
			this.textBoxTemporaryFolder.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.textBoxTemporaryFolder.Location = new System.Drawing.Point(92, 66);
			this.textBoxTemporaryFolder.Name = "textBoxTemporaryFolder";
			this.textBoxTemporaryFolder.ReadOnly = true;
			this.textBoxTemporaryFolder.Size = new System.Drawing.Size(281, 20);
			this.textBoxTemporaryFolder.TabIndex = 8;
			// 
			// labelTemporaryFolder
			// 
			this.labelTemporaryFolder.AutoSize = true;
			this.labelTemporaryFolder.Location = new System.Drawing.Point(6, 69);
			this.labelTemporaryFolder.Name = "labelTemporaryFolder";
			this.labelTemporaryFolder.Size = new System.Drawing.Size(86, 13);
			this.labelTemporaryFolder.TabIndex = 7;
			this.labelTemporaryFolder.Text = "Temporary folder";
			// 
			// treeObjectProperties
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.AutoScroll = true;
			this.ClientSize = new System.Drawing.Size(484, 268);
			this.Controls.Add(this.groupBoxMain);
			this.Controls.Add(this.buttonClose);
			this.MinimumSize = new System.Drawing.Size(492, 302);
			this.Name = "treeObjectProperties";
			this.Text = "Executer project properties";
			this.groupBoxMain.ResumeLayout(false);
			this.groupBoxMain.PerformLayout();
			this.groupBoxServers.ResumeLayout(false);
			this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.Button buttonClose;
		private System.Windows.Forms.GroupBox groupBoxMain;
		private System.Windows.Forms.Label labelProjectName;
		private System.Windows.Forms.TextBox textBoxProjectName;
		private System.Windows.Forms.Label labelTemplateName;
		private System.Windows.Forms.Button buttonOpenProject;
		private System.Windows.Forms.Button buttonOpenTemplate;
		private System.Windows.Forms.TextBox textBoxTemplateName;
		private System.Windows.Forms.GroupBox groupBoxServers;
		private System.Windows.Forms.RichTextBox richTextBox1;
		private System.Windows.Forms.Button buttonOpenTemporaryFolder;
		private System.Windows.Forms.TextBox textBoxTemporaryFolder;
		private System.Windows.Forms.Label labelTemporaryFolder;
	}
}