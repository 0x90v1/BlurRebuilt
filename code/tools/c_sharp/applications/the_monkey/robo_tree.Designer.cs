namespace RoboPublish
{
	partial class RoboTree
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
			this.components = new System.ComponentModel.Container();
			System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(RoboTree));
			this.imageList = new System.Windows.Forms.ImageList(this.components);
			this.contextMenuStrip = new System.Windows.Forms.ContextMenuStrip(this.components);
			this.toolStripMenuItemCollapseAll = new System.Windows.Forms.ToolStripMenuItem();
			this.toolStripMenuItemCollapseChildren = new System.Windows.Forms.ToolStripMenuItem();
			this.contextMenuStrip.SuspendLayout();
			this.SuspendLayout();
			// 
			// imageList
			// 
			this.imageList.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("imageList.ImageStream")));
			this.imageList.TransparentColor = System.Drawing.Color.Transparent;
			this.imageList.Images.SetKeyName(0, "");
			this.imageList.Images.SetKeyName(1, "");
			this.imageList.Images.SetKeyName(2, "");
			this.imageList.Images.SetKeyName(3, "");
			this.imageList.Images.SetKeyName(4, "");
			this.imageList.Images.SetKeyName(5, "exclamation.bmp");
			this.imageList.Images.SetKeyName(6, "error.bmp");
			this.imageList.Images.SetKeyName(7, "folder.bmp");
			this.imageList.Images.SetKeyName(8, "FilteredOut.bmp");
			this.imageList.Images.SetKeyName(9, "filtered_out.bmp");
			// 
			// contextMenuStrip
			// 
			this.contextMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripMenuItemCollapseAll,
            this.toolStripMenuItemCollapseChildren});
			this.contextMenuStrip.Name = "contextMenuStrip";
			this.contextMenuStrip.ShowImageMargin = false;
			this.contextMenuStrip.Size = new System.Drawing.Size(141, 48);
			// 
			// toolStripMenuItemCollapseAll
			// 
			this.toolStripMenuItemCollapseAll.Name = "toolStripMenuItemCollapseAll";
			this.toolStripMenuItemCollapseAll.Size = new System.Drawing.Size(140, 22);
			this.toolStripMenuItemCollapseAll.Text = "Collapse all";
			this.toolStripMenuItemCollapseAll.ToolTipText = "Collapse all child tree nodes.";
			// 
			// toolStripMenuItemCollapseChildren
			// 
			this.toolStripMenuItemCollapseChildren.Name = "toolStripMenuItemCollapseChildren";
			this.toolStripMenuItemCollapseChildren.Size = new System.Drawing.Size(140, 22);
			this.toolStripMenuItemCollapseChildren.Text = "Collapse children";
			// 
			// RoboTree
			// 
			this.CheckBoxes = true;
			this.ContextMenuStrip = this.contextMenuStrip;
			this.HideSelection = false;
			this.LineColor = System.Drawing.Color.Black;
			this.MouseMove += new System.Windows.Forms.MouseEventHandler(this.RoboTree_MouseMove);
			this.MouseLeave += new System.EventHandler(this.RoboTree_MouseLeave);
			this.contextMenuStrip.ResumeLayout(false);
			this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.ImageList imageList;
		private System.Windows.Forms.ContextMenuStrip contextMenuStrip;
		private System.Windows.Forms.ToolStripMenuItem toolStripMenuItemCollapseAll;
		private System.Windows.Forms.ToolStripMenuItem toolStripMenuItemCollapseChildren;
	}
}
