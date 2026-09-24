using System.Windows.Forms;
namespace Executer
{
	partial class ExecuterTreeView
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
			this.SuspendLayout();
			// 
			// ExecuterTreeView
			// 
			this.LineColor = System.Drawing.Color.Black;
			this.AfterCheck += new System.Windows.Forms.TreeViewEventHandler(this.ExecuterTreeView_AfterCheck);
			this.AfterCollapse += new System.Windows.Forms.TreeViewEventHandler(this.ExecuterTreeView_AfterCollapseOrExpand);
			this.AfterLabelEdit += new System.Windows.Forms.NodeLabelEditEventHandler(this.TreeObjectManager_AfterLabelEdit);
			this.DragDrop += new System.Windows.Forms.DragEventHandler(this.TreeObjectManager_DragDrop);
			this.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.TreeObjectManager_AfterSelect);
			this.DragEnter += new System.Windows.Forms.DragEventHandler(this.TreeObjectManager_DragEnter);
			this.KeyUp += new System.Windows.Forms.KeyEventHandler(this.TreeObjectManager_KeyUp);
			this.NodeMouseClick += new System.Windows.Forms.TreeNodeMouseClickEventHandler(this.TreeObjectManager_NodeMouseClick);
			this.BeforeLabelEdit += new System.Windows.Forms.NodeLabelEditEventHandler(this.TreeObjectManager_BeforeLabelEdit);
			this.AfterExpand += new System.Windows.Forms.TreeViewEventHandler(this.ExecuterTreeView_AfterCollapseOrExpand);
			this.ItemDrag += new System.Windows.Forms.ItemDragEventHandler(this.TreeObjectManager_ItemDrag);
			this.DragOver += new System.Windows.Forms.DragEventHandler(this.TreeObjectManager_DragOver);
			this.ResumeLayout(false);

		}


		#endregion

	}
}
