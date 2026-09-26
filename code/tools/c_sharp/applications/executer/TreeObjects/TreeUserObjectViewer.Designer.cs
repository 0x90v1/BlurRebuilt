namespace Executer.TreeObjects
{
	partial class TreeUserObjectViewer
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

		#region Component Designer generated code

		/// <summary> 
		/// Required method for Designer support - do not modify 
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.panelMain = new System.Windows.Forms.Panel();
			this.tabControl_Main = new System.Windows.Forms.TabControl();
			this.panelMain.SuspendLayout();
			this.SuspendLayout();
			// 
			// panelMain
			// 
			this.panelMain.Controls.Add(this.tabControl_Main);
			this.panelMain.Dock = System.Windows.Forms.DockStyle.Fill;
			this.panelMain.Location = new System.Drawing.Point(0, 0);
			this.panelMain.Name = "panelMain";
			this.panelMain.Size = new System.Drawing.Size(692, 438);
			this.panelMain.TabIndex = 0;
			// 
			// tabControl_Main
			// 
			this.tabControl_Main.Dock = System.Windows.Forms.DockStyle.Fill;
			this.tabControl_Main.Location = new System.Drawing.Point(0, 0);
			this.tabControl_Main.Name = "tabControl_Main";
			this.tabControl_Main.SelectedIndex = 0;
			this.tabControl_Main.Size = new System.Drawing.Size(692, 438);
			this.tabControl_Main.TabIndex = 0;
			this.tabControl_Main.Selected += new System.Windows.Forms.TabControlEventHandler(this.tabControl_Selected);
			// 
			// TreeUserObjectViewer
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.Controls.Add(this.panelMain);
			this.Name = "TreeUserObjectViewer";
			this.Size = new System.Drawing.Size(692, 438);
			this.Load += new System.EventHandler(this.TreeUserObjectViewer_Load);
			this.panelMain.ResumeLayout(false);
			this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.Panel panelMain;
		private System.Windows.Forms.TabControl tabControl_Main;
	}
}