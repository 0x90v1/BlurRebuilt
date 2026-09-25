namespace Executer
{
	partial class PromptSave
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
			System.Windows.Forms.ListViewItem listViewItem1 = new System.Windows.Forms.ListViewItem("This");
			System.Windows.Forms.ListViewItem listViewItem2 = new System.Windows.Forms.ListViewItem("is");
			System.Windows.Forms.ListViewItem listViewItem3 = new System.Windows.Forms.ListViewItem("a");
			System.Windows.Forms.ListViewItem listViewItem4 = new System.Windows.Forms.ListViewItem("test");
			this.buttonSelected = new System.Windows.Forms.Button();
			this.buttonNo = new System.Windows.Forms.Button();
			this.buttonCancel = new System.Windows.Forms.Button();
			this.label1 = new System.Windows.Forms.Label();
			this.listView1 = new System.Windows.Forms.ListView();
			this.buttonAll = new System.Windows.Forms.Button();
			this.SuspendLayout();
			// 
			// buttonSelected
			// 
			this.buttonSelected.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonSelected.AutoEllipsis = true;
			this.buttonSelected.Location = new System.Drawing.Point(174, 239);
			this.buttonSelected.Name = "buttonSelected";
			this.buttonSelected.Size = new System.Drawing.Size(75, 23);
			this.buttonSelected.TabIndex = 0;
			this.buttonSelected.Text = "Selected";
			this.buttonSelected.UseVisualStyleBackColor = true;
			this.buttonSelected.Click += new System.EventHandler(this.buttonSelected_Click);
			// 
			// buttonNo
			// 
			this.buttonNo.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonNo.Location = new System.Drawing.Point(255, 239);
			this.buttonNo.Name = "buttonNo";
			this.buttonNo.Size = new System.Drawing.Size(75, 23);
			this.buttonNo.TabIndex = 1;
			this.buttonNo.Text = "None";
			this.buttonNo.UseVisualStyleBackColor = true;
			this.buttonNo.Click += new System.EventHandler(this.buttonNo_Click);
			// 
			// buttonCancel
			// 
			this.buttonCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonCancel.Location = new System.Drawing.Point(336, 239);
			this.buttonCancel.Name = "buttonCancel";
			this.buttonCancel.Size = new System.Drawing.Size(75, 23);
			this.buttonCancel.TabIndex = 2;
			this.buttonCancel.Text = "Cancel";
			this.buttonCancel.UseVisualStyleBackColor = true;
			this.buttonCancel.Click += new System.EventHandler(this.buttonCancel_Click);
			// 
			// label1
			// 
			this.label1.AutoSize = true;
			this.label1.Location = new System.Drawing.Point(12, 9);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(183, 13);
			this.label1.TabIndex = 3;
			this.label1.Text = "Save changes to the following items?";
			// 
			// listView1
			// 
			this.listView1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
						| System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.listView1.CheckBoxes = true;
			this.listView1.FullRowSelect = true;
			this.listView1.GridLines = true;
			listViewItem1.StateImageIndex = 0;
			listViewItem2.StateImageIndex = 0;
			listViewItem3.StateImageIndex = 0;
			listViewItem4.StateImageIndex = 0;
			this.listView1.Items.AddRange(new System.Windows.Forms.ListViewItem[] {
            listViewItem1,
            listViewItem2,
            listViewItem3,
            listViewItem4});
			this.listView1.Location = new System.Drawing.Point(15, 25);
			this.listView1.Name = "listView1";
			this.listView1.Size = new System.Drawing.Size(396, 201);
			this.listView1.TabIndex = 5;
			this.listView1.UseCompatibleStateImageBehavior = false;
			this.listView1.View = System.Windows.Forms.View.List;
			this.listView1.ItemChecked += new System.Windows.Forms.ItemCheckedEventHandler(this.listView1_ItemChecked);
			// 
			// buttonAll
			// 
			this.buttonAll.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonAll.Location = new System.Drawing.Point(93, 239);
			this.buttonAll.Name = "buttonAll";
			this.buttonAll.Size = new System.Drawing.Size(75, 23);
			this.buttonAll.TabIndex = 6;
			this.buttonAll.Text = "All";
			this.buttonAll.UseVisualStyleBackColor = true;
			this.buttonAll.Click += new System.EventHandler(this.buttonAll_Click);
			// 
			// PromptSave
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(426, 276);
			this.Controls.Add(this.buttonAll);
			this.Controls.Add(this.listView1);
			this.Controls.Add(this.label1);
			this.Controls.Add(this.buttonCancel);
			this.Controls.Add(this.buttonNo);
			this.Controls.Add(this.buttonSelected);
			this.MinimumSize = new System.Drawing.Size(434, 180);
			this.Name = "PromptSave";
			this.Text = "Executer";
			this.Load += new System.EventHandler(this.PromptSave_Load);
			this.ResumeLayout(false);
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.Button buttonSelected;
		private System.Windows.Forms.Button buttonNo;
		private System.Windows.Forms.Button buttonCancel;
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.ListView listView1;
		private System.Windows.Forms.Button buttonAll;
	}
}