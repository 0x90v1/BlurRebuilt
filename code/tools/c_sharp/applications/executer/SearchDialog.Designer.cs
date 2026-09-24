namespace Executer
{
	partial class SearchDialog
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
			this.checkBoxMatchTypeName = new System.Windows.Forms.CheckBox();
			this.checkBoxMatchInternalName = new System.Windows.Forms.CheckBox();
			this.checkBoxMatchNodeName = new System.Windows.Forms.CheckBox();
			this.checkBoxMatchContainerName = new System.Windows.Forms.CheckBox();
			this.comboBoxMatchTypeName = new System.Windows.Forms.ComboBox();
			this.comboBoxMatchInternalName = new System.Windows.Forms.ComboBox();
			this.comboBoxMatchNodeName = new System.Windows.Forms.ComboBox();
			this.comboBoxMatchContainerName = new System.Windows.Forms.ComboBox();
			this.groupBox1 = new System.Windows.Forms.GroupBox();
			this.listViewResults = new System.Windows.Forms.ListView();
			this.groupBox2 = new System.Windows.Forms.GroupBox();
			this.groupBox3 = new System.Windows.Forms.GroupBox();
			this.checkBoxAutoClear = new System.Windows.Forms.CheckBox();
			this.buttonFindAll = new System.Windows.Forms.Button();
			this.buttonSelectInTree = new System.Windows.Forms.Button();
			this.buttonClearResults = new System.Windows.Forms.Button();
			this.buttonDropSearchAnchor = new System.Windows.Forms.Button();
			this.buttonFindNext = new System.Windows.Forms.Button();
			this.labelLookIn = new System.Windows.Forms.Label();
			this.comboBoxLookIn = new System.Windows.Forms.ComboBox();
			this.comboBoxMatchContainerValue = new System.Windows.Forms.ComboBox();
			this.checkBoxMatchContainerValue = new System.Windows.Forms.CheckBox();
			this.groupBox1.SuspendLayout();
			this.groupBox2.SuspendLayout();
			this.groupBox3.SuspendLayout();
			this.SuspendLayout();
			// 
			// checkBoxMatchTypeName
			// 
			this.checkBoxMatchTypeName.AutoSize = true;
			this.checkBoxMatchTypeName.Location = new System.Drawing.Point(96, 115);
			this.checkBoxMatchTypeName.Name = "checkBoxMatchTypeName";
			this.checkBoxMatchTypeName.RightToLeft = System.Windows.Forms.RightToLeft.Yes;
			this.checkBoxMatchTypeName.Size = new System.Drawing.Size(79, 17);
			this.checkBoxMatchTypeName.TabIndex = 0;
			this.checkBoxMatchTypeName.Text = "Type name";
			this.checkBoxMatchTypeName.UseVisualStyleBackColor = true;
			this.checkBoxMatchTypeName.CheckedChanged += new System.EventHandler(this.checkBoxMatchTypeName_CheckedChanged);
			// 
			// checkBoxMatchInternalName
			// 
			this.checkBoxMatchInternalName.AutoSize = true;
			this.checkBoxMatchInternalName.Location = new System.Drawing.Point(85, 138);
			this.checkBoxMatchInternalName.Name = "checkBoxMatchInternalName";
			this.checkBoxMatchInternalName.RightToLeft = System.Windows.Forms.RightToLeft.Yes;
			this.checkBoxMatchInternalName.Size = new System.Drawing.Size(90, 17);
			this.checkBoxMatchInternalName.TabIndex = 1;
			this.checkBoxMatchInternalName.Text = "Internal name";
			this.checkBoxMatchInternalName.UseVisualStyleBackColor = true;
			this.checkBoxMatchInternalName.CheckedChanged += new System.EventHandler(this.checkBoxMatchInternalName_CheckedChanged);
			// 
			// checkBoxMatchNodeName
			// 
			this.checkBoxMatchNodeName.AutoSize = true;
			this.checkBoxMatchNodeName.Location = new System.Drawing.Point(63, 42);
			this.checkBoxMatchNodeName.Name = "checkBoxMatchNodeName";
			this.checkBoxMatchNodeName.RightToLeft = System.Windows.Forms.RightToLeft.Yes;
			this.checkBoxMatchNodeName.Size = new System.Drawing.Size(112, 17);
			this.checkBoxMatchNodeName.TabIndex = 2;
			this.checkBoxMatchNodeName.Text = "Match node name";
			this.checkBoxMatchNodeName.UseVisualStyleBackColor = true;
			this.checkBoxMatchNodeName.CheckedChanged += new System.EventHandler(this.checkBoxMatchNodeName_CheckedChanged);
			// 
			// checkBoxMatchContainerName
			// 
			this.checkBoxMatchContainerName.AutoSize = true;
			this.checkBoxMatchContainerName.Location = new System.Drawing.Point(43, 67);
			this.checkBoxMatchContainerName.Name = "checkBoxMatchContainerName";
			this.checkBoxMatchContainerName.RightToLeft = System.Windows.Forms.RightToLeft.Yes;
			this.checkBoxMatchContainerName.Size = new System.Drawing.Size(132, 17);
			this.checkBoxMatchContainerName.TabIndex = 3;
			this.checkBoxMatchContainerName.Text = "Match container name";
			this.checkBoxMatchContainerName.UseVisualStyleBackColor = true;
			this.checkBoxMatchContainerName.CheckedChanged += new System.EventHandler(this.checkBoxMatchContainerName_CheckedChanged);
			// 
			// comboBoxMatchTypeName
			// 
			this.comboBoxMatchTypeName.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.comboBoxMatchTypeName.FormattingEnabled = true;
			this.comboBoxMatchTypeName.Location = new System.Drawing.Point(181, 113);
			this.comboBoxMatchTypeName.Name = "comboBoxMatchTypeName";
			this.comboBoxMatchTypeName.Size = new System.Drawing.Size(485, 21);
			this.comboBoxMatchTypeName.TabIndex = 4;
			this.comboBoxMatchTypeName.Leave += new System.EventHandler(this.comboBox_All_Leave);
			// 
			// comboBoxMatchInternalName
			// 
			this.comboBoxMatchInternalName.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.comboBoxMatchInternalName.FormattingEnabled = true;
			this.comboBoxMatchInternalName.Location = new System.Drawing.Point(181, 136);
			this.comboBoxMatchInternalName.Name = "comboBoxMatchInternalName";
			this.comboBoxMatchInternalName.Size = new System.Drawing.Size(485, 21);
			this.comboBoxMatchInternalName.TabIndex = 5;
			this.comboBoxMatchInternalName.Leave += new System.EventHandler(this.comboBox_All_Leave);
			// 
			// comboBoxMatchNodeName
			// 
			this.comboBoxMatchNodeName.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.comboBoxMatchNodeName.FormattingEnabled = true;
			this.comboBoxMatchNodeName.Location = new System.Drawing.Point(181, 40);
			this.comboBoxMatchNodeName.Name = "comboBoxMatchNodeName";
			this.comboBoxMatchNodeName.Size = new System.Drawing.Size(485, 21);
			this.comboBoxMatchNodeName.TabIndex = 6;
			this.comboBoxMatchNodeName.Leave += new System.EventHandler(this.comboBox_All_Leave);
			// 
			// comboBoxMatchContainerName
			// 
			this.comboBoxMatchContainerName.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.comboBoxMatchContainerName.FormattingEnabled = true;
			this.comboBoxMatchContainerName.Location = new System.Drawing.Point(181, 63);
			this.comboBoxMatchContainerName.Name = "comboBoxMatchContainerName";
			this.comboBoxMatchContainerName.Size = new System.Drawing.Size(485, 21);
			this.comboBoxMatchContainerName.TabIndex = 7;
			this.comboBoxMatchContainerName.Leave += new System.EventHandler(this.comboBox_All_Leave);
			// 
			// groupBox1
			// 
			this.groupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
						| System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.groupBox1.Controls.Add(this.listViewResults);
			this.groupBox1.Location = new System.Drawing.Point(6, 192);
			this.groupBox1.Name = "groupBox1";
			this.groupBox1.Size = new System.Drawing.Size(659, 293);
			this.groupBox1.TabIndex = 8;
			this.groupBox1.TabStop = false;
			this.groupBox1.Text = "Results";
			// 
			// listViewResults
			// 
			this.listViewResults.Dock = System.Windows.Forms.DockStyle.Fill;
			this.listViewResults.Location = new System.Drawing.Point(3, 16);
			this.listViewResults.MultiSelect = false;
			this.listViewResults.Name = "listViewResults";
			this.listViewResults.Size = new System.Drawing.Size(653, 274);
			this.listViewResults.TabIndex = 0;
			this.listViewResults.UseCompatibleStateImageBehavior = false;
			this.listViewResults.View = System.Windows.Forms.View.List;
			this.listViewResults.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.listViewResults_MouseDoubleClick);
			// 
			// groupBox2
			// 
			this.groupBox2.Controls.Add(this.groupBox3);
			this.groupBox2.Controls.Add(this.buttonSelectInTree);
			this.groupBox2.Controls.Add(this.buttonClearResults);
			this.groupBox2.Controls.Add(this.buttonDropSearchAnchor);
			this.groupBox2.Controls.Add(this.buttonFindNext);
			this.groupBox2.Controls.Add(this.labelLookIn);
			this.groupBox2.Controls.Add(this.comboBoxLookIn);
			this.groupBox2.Controls.Add(this.comboBoxMatchContainerValue);
			this.groupBox2.Controls.Add(this.checkBoxMatchContainerValue);
			this.groupBox2.Controls.Add(this.checkBoxMatchTypeName);
			this.groupBox2.Controls.Add(this.groupBox1);
			this.groupBox2.Controls.Add(this.checkBoxMatchInternalName);
			this.groupBox2.Controls.Add(this.comboBoxMatchContainerName);
			this.groupBox2.Controls.Add(this.checkBoxMatchNodeName);
			this.groupBox2.Controls.Add(this.comboBoxMatchNodeName);
			this.groupBox2.Controls.Add(this.checkBoxMatchContainerName);
			this.groupBox2.Controls.Add(this.comboBoxMatchInternalName);
			this.groupBox2.Controls.Add(this.comboBoxMatchTypeName);
			this.groupBox2.Dock = System.Windows.Forms.DockStyle.Fill;
			this.groupBox2.Location = new System.Drawing.Point(0, 0);
			this.groupBox2.Name = "groupBox2";
			this.groupBox2.Size = new System.Drawing.Size(677, 520);
			this.groupBox2.TabIndex = 9;
			this.groupBox2.TabStop = false;
			// 
			// groupBox3
			// 
			this.groupBox3.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.groupBox3.Controls.Add(this.checkBoxAutoClear);
			this.groupBox3.Controls.Add(this.buttonFindAll);
			this.groupBox3.Location = new System.Drawing.Point(505, 487);
			this.groupBox3.Name = "groupBox3";
			this.groupBox3.Size = new System.Drawing.Size(160, 28);
			this.groupBox3.TabIndex = 24;
			this.groupBox3.TabStop = false;
			// 
			// checkBoxAutoClear
			// 
			this.checkBoxAutoClear.AutoSize = true;
			this.checkBoxAutoClear.Checked = true;
			this.checkBoxAutoClear.CheckState = System.Windows.Forms.CheckState.Checked;
			this.checkBoxAutoClear.Location = new System.Drawing.Point(6, 10);
			this.checkBoxAutoClear.Name = "checkBoxAutoClear";
			this.checkBoxAutoClear.Size = new System.Drawing.Size(75, 17);
			this.checkBoxAutoClear.TabIndex = 10;
			this.checkBoxAutoClear.Text = "Auto Clear";
			this.checkBoxAutoClear.UseVisualStyleBackColor = true;
			this.checkBoxAutoClear.CheckedChanged += new System.EventHandler(this.checkBoxAutoClear_CheckedChanged);
			// 
			// buttonFindAll
			// 
			this.buttonFindAll.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonFindAll.Location = new System.Drawing.Point(84, 5);
			this.buttonFindAll.Name = "buttonFindAll";
			this.buttonFindAll.Size = new System.Drawing.Size(75, 23);
			this.buttonFindAll.TabIndex = 9;
			this.buttonFindAll.Text = "&Find All";
			this.buttonFindAll.UseVisualStyleBackColor = true;
			this.buttonFindAll.Click += new System.EventHandler(this.buttonFindAll_Click);
			// 
			// buttonSelectInTree
			// 
			this.buttonSelectInTree.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonSelectInTree.Enabled = false;
			this.buttonSelectInTree.Location = new System.Drawing.Point(323, 493);
			this.buttonSelectInTree.Name = "buttonSelectInTree";
			this.buttonSelectInTree.Size = new System.Drawing.Size(95, 23);
			this.buttonSelectInTree.TabIndex = 23;
			this.buttonSelectInTree.Text = "&Select All in Tree";
			this.buttonSelectInTree.UseVisualStyleBackColor = true;
			this.buttonSelectInTree.Click += new System.EventHandler(this.buttonSelectInTree_Click);
			// 
			// buttonClearResults
			// 
			this.buttonClearResults.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonClearResults.Location = new System.Drawing.Point(424, 493);
			this.buttonClearResults.Name = "buttonClearResults";
			this.buttonClearResults.Size = new System.Drawing.Size(75, 23);
			this.buttonClearResults.TabIndex = 22;
			this.buttonClearResults.Text = "&Clear";
			this.buttonClearResults.UseVisualStyleBackColor = true;
			this.buttonClearResults.Click += new System.EventHandler(this.buttonClearResults_Click);
			// 
			// buttonDropSearchAnchor
			// 
			this.buttonDropSearchAnchor.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonDropSearchAnchor.Location = new System.Drawing.Point(471, 163);
			this.buttonDropSearchAnchor.Name = "buttonDropSearchAnchor";
			this.buttonDropSearchAnchor.Size = new System.Drawing.Size(115, 23);
			this.buttonDropSearchAnchor.TabIndex = 21;
			this.buttonDropSearchAnchor.Text = "&Drop Search Anchor";
			this.buttonDropSearchAnchor.UseVisualStyleBackColor = true;
			this.buttonDropSearchAnchor.Click += new System.EventHandler(this.buttonDropSearchAnchor_Click);
			// 
			// buttonFindNext
			// 
			this.buttonFindNext.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonFindNext.Location = new System.Drawing.Point(592, 163);
			this.buttonFindNext.Name = "buttonFindNext";
			this.buttonFindNext.Size = new System.Drawing.Size(75, 23);
			this.buttonFindNext.TabIndex = 20;
			this.buttonFindNext.Text = "Find &Next";
			this.buttonFindNext.UseVisualStyleBackColor = true;
			this.buttonFindNext.Click += new System.EventHandler(this.buttonFindNext_Click);
			// 
			// labelLookIn
			// 
			this.labelLookIn.AutoSize = true;
			this.labelLookIn.Location = new System.Drawing.Point(12, 16);
			this.labelLookIn.Name = "labelLookIn";
			this.labelLookIn.Size = new System.Drawing.Size(42, 13);
			this.labelLookIn.TabIndex = 17;
			this.labelLookIn.Text = "Look in";
			// 
			// comboBoxLookIn
			// 
			this.comboBoxLookIn.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.comboBoxLookIn.FormattingEnabled = true;
			this.comboBoxLookIn.Items.AddRange(new object[] {
            "Anchor children",
            "Current Project",
            "Current Solution",
            "All Solutions"});
			this.comboBoxLookIn.Location = new System.Drawing.Point(60, 13);
			this.comboBoxLookIn.Name = "comboBoxLookIn";
			this.comboBoxLookIn.Size = new System.Drawing.Size(211, 21);
			this.comboBoxLookIn.TabIndex = 15;
			this.comboBoxLookIn.Leave += new System.EventHandler(this.comboBox_All_Leave);
			// 
			// comboBoxMatchContainerValue
			// 
			this.comboBoxMatchContainerValue.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.comboBoxMatchContainerValue.FormattingEnabled = true;
			this.comboBoxMatchContainerValue.Items.AddRange(new object[] {
            "Regular Expressions",
            "Wildcards"});
			this.comboBoxMatchContainerValue.Location = new System.Drawing.Point(181, 86);
			this.comboBoxMatchContainerValue.Name = "comboBoxMatchContainerValue";
			this.comboBoxMatchContainerValue.Size = new System.Drawing.Size(485, 21);
			this.comboBoxMatchContainerValue.TabIndex = 11;
			this.comboBoxMatchContainerValue.Leave += new System.EventHandler(this.comboBox_All_Leave);
			// 
			// checkBoxMatchContainerValue
			// 
			this.checkBoxMatchContainerValue.AutoSize = true;
			this.checkBoxMatchContainerValue.Location = new System.Drawing.Point(43, 90);
			this.checkBoxMatchContainerValue.Name = "checkBoxMatchContainerValue";
			this.checkBoxMatchContainerValue.RightToLeft = System.Windows.Forms.RightToLeft.Yes;
			this.checkBoxMatchContainerValue.Size = new System.Drawing.Size(132, 17);
			this.checkBoxMatchContainerValue.TabIndex = 10;
			this.checkBoxMatchContainerValue.Text = "Match container value";
			this.checkBoxMatchContainerValue.UseVisualStyleBackColor = true;
			this.checkBoxMatchContainerValue.CheckedChanged += new System.EventHandler(this.checkBoxMatchContainerValue_CheckedChanged);
			// 
			// SearchDialog
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(677, 520);
			this.Controls.Add(this.groupBox2);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.SizableToolWindow;
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.MinimumSize = new System.Drawing.Size(371, 346);
			this.Name = "SearchDialog";
			this.ShowIcon = false;
			this.ShowInTaskbar = false;
			this.Text = "Executer Search Dialog";
			this.Load += new System.EventHandler(this.SearchDialog_Load);
			this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.SearchDialog_FormClosing);
			this.groupBox1.ResumeLayout(false);
			this.groupBox2.ResumeLayout(false);
			this.groupBox2.PerformLayout();
			this.groupBox3.ResumeLayout(false);
			this.groupBox3.PerformLayout();
			this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.CheckBox checkBoxMatchTypeName;
		private System.Windows.Forms.CheckBox checkBoxMatchInternalName;
		private System.Windows.Forms.CheckBox checkBoxMatchNodeName;
		private System.Windows.Forms.CheckBox checkBoxMatchContainerName;
		private System.Windows.Forms.ComboBox comboBoxMatchTypeName;
		private System.Windows.Forms.ComboBox comboBoxMatchInternalName;
		private System.Windows.Forms.ComboBox comboBoxMatchNodeName;
		private System.Windows.Forms.ComboBox comboBoxMatchContainerName;
		private System.Windows.Forms.GroupBox groupBox1;
		private System.Windows.Forms.ListView listViewResults;
		private System.Windows.Forms.GroupBox groupBox2;
		private System.Windows.Forms.Button buttonFindAll;
		private System.Windows.Forms.ComboBox comboBoxMatchContainerValue;
		private System.Windows.Forms.CheckBox checkBoxMatchContainerValue;
		private System.Windows.Forms.Label labelLookIn;
		private System.Windows.Forms.Button buttonFindNext;
		private System.Windows.Forms.ComboBox comboBoxLookIn;
		private System.Windows.Forms.Button buttonDropSearchAnchor;
		private System.Windows.Forms.Button buttonClearResults;
		public System.Windows.Forms.Button buttonSelectInTree;
		private System.Windows.Forms.GroupBox groupBox3;
		private System.Windows.Forms.CheckBox checkBoxAutoClear;

	}
}