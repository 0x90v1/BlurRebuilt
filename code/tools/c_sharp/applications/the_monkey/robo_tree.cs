using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;
using System.IO;

namespace RoboPublish
{
	public partial class RoboTree : TreeView
	{
		/// <summary>
		///  
		/// </summary>
		internal enum TreeItemIconIndex
		{
			NoSourceFile = 0,
			Same,
			SourceFileIsNewer,
			SourceFileIsOlder,
			NoDestinationFile,
			Problem,
			FileFolderMismatch,
			Folder,
		}

		public enum Flags
		{
			Checked = 0,
			All,
		}

		public List<Sku> m_skus = null;

		Font m_italicNodeFont;

		public RoboTree()
		{
			InitializeComponent();

			this.DrawMode	= TreeViewDrawMode.OwnerDrawText;
			this.DrawNode	+= new DrawTreeNodeEventHandler(RoboTree_DrawNode);
			this.MouseDown	+= new MouseEventHandler(RoboTree_MouseDown);

			m_italicNodeFont = new Font(this.Font, FontStyle.Italic);

			this.toolStripMenuItemCollapseAll.Click += new EventHandler(toolStripMenuItemCollapseAll_Click);
			this.toolStripMenuItemCollapseChildren.Click += new EventHandler(toolStripMenuItemCollapseChildren_Click);
		}

		void toolStripMenuItemCollapseChildren_Click(object sender, EventArgs e)
		{
			TreeObject to = this.SelectedNode as TreeObject;

			if(to != null)
			{
				foreach(TreeObject toChild in to.Nodes)
				{
					toChild.Collapse(false);
				}
			}
		}

		void toolStripMenuItemCollapseAll_Click(object sender, EventArgs e)
		{
			TreeObject to = this.SelectedNode as TreeObject;
			if(to != null)
				to.Collapse(false);
		}

		//private void CollapseAll(TreeObject to)
		//{
		//    foreach(TreeObject toChild in to.Nodes)
		//    {
		//        toChild.Collapse(;
		//}

		void RoboTree_DrawNode(object sender, DrawTreeNodeEventArgs e)
		{
			RoboTree roboTree = sender as RoboTree;

			TreeObject to = e.Node as TreeObject;
			
			//if(to.Parent == null)
			//{
			//    e.DrawDefault = true;
			//    return;
			//}
			Debug.Assert(e.DrawDefault == false);
			Rectangle bounds = NodeBounds(e.Node);

			if(bounds.Height == 0)
			{
				return;
			}

			// Draw the background and node text for a selected node.
			if((e.State & TreeNodeStates.Selected) != 0)
			{
				// Draw the background of the selected node.
				e.Graphics.FillRectangle(Brushes.PaleGoldenrod, bounds);
			}

			//Font nodeFont = e.Node.NodeFont;								// Retrieve the node font. If the node font has not been set use the TreeView font.
			Font nodeFont = this.Font;								// Retrieve the node font. If the node font has not been set use the TreeView font.
			
			if(nodeFont == null)
				nodeFont = ((TreeView)sender).Font;

			// Draw the icons.
			for(int c = 0; c < to.NumTags; c++)
			{
				TreeTagInfo tti = to.m_listTagInfo[c];

				int imageIndex = (int)tti.m_iconIndex;
				if(this.m_skus[c].m_disabled == false)
				{
					if(tti.m_bhidden == true)
						e.Graphics.DrawImage(imageList.Images[9], bounds.X + c * 16, bounds.Y);
					else
						e.Graphics.DrawImage(imageList.Images[imageIndex], bounds.X + c * 16, bounds.Y);
				}
				else
					e.Graphics.DrawImage(imageList.Images[8], bounds.X + c * 16, bounds.Y);

				Debug.Assert(this.m_skus[c].m_disabled == to.m_listTagInfo[c].m_doNotCopy);
			}

			// Draw the node text.
			
			Brush brush;
			if(to.m_bIsFolder == true)
				brush = Brushes.Black;
			else
				brush = new SolidBrush(to.m_textColour);

			if(e.Node.Text[0] == '?')
				e.Graphics.DrawString(e.Node.Text.Substring(1), m_italicNodeFont, brush, bounds.X + to.NumTags * 16, bounds.Y);
			else
				e.Graphics.DrawString(e.Node.Text, nodeFont, brush, bounds.X + to.NumTags * 16, bounds.Y);

			// If the node has focus, draw the focus rectangle.
			if((e.State & TreeNodeStates.Focused) != 0)
			{
				using(Pen focusPen = new Pen(Color.Black))
				{
					focusPen.DashStyle = System.Drawing.Drawing2D.DashStyle.Dot;
					Rectangle focusBounds = NodeBounds(e.Node);
					focusBounds.Size = new Size(focusBounds.Width - 1, focusBounds.Height - 1);
					e.Graphics.DrawRectangle(focusPen, focusBounds);
				}
			}
		}
		
		// Returns the bounds of the specified node, including the region occupied by the node label and any node tag displayed.
		private Rectangle NodeBounds(TreeNode node)
		{
			// Set the return value to the normal node bounds.
			Rectangle bounds = node.Bounds;

			TreeObject to = node as TreeObject;
			
			// Retrieve a Graphics object from the TreeView handle and use it to calculate the display width of the tag.
			int iconWidth = to.NumTags * 16;

			bounds = new Rectangle(bounds.X, bounds.Y, bounds.Width + iconWidth, bounds.Height);

			return bounds;
		}

		void RoboTree_MouseDown(object sender, MouseEventArgs e)
		{
			TreeNode clickedNode = this.GetNodeAt(e.X, e.Y);
			if(clickedNode != null)
			{
				if(NodeBounds(clickedNode).Contains(e.X, e.Y))
				{
					this.SelectedNode = clickedNode;
				}
			}
		}
/*
		internal TreeNode GetTreeNodeFromPath(string strPath)
		{
			string[] split = strPath.Split(new Char[] { '\\' });

			TreeNodeCollection tnc = this.Nodes;
			TreeNode tn = null;

			foreach(string s in split)
			{
				tn = FindTreeNodeByText(tnc, s);

				if(tn == null)
					return null;
				else
					tnc = tn.Nodes;
			}
			return tn;
		}
*/
		public TreeObject FindTreeNodeByText(TreeNodeCollection tnc, string strText, string strPathName, bool bMergeFolders)
		{
			string strLowerText = strText.ToLower();

			foreach(TreeObject to in tnc)
			{
				bool bIsDirectory = true;
				for(int c = 0; c < to.NumTags; c++)
				{
					if(to.m_listTagInfo[c].m_fsiSource != null && (to.m_listTagInfo[c].m_fsiSource.Attributes & FileAttributes.Directory) != FileAttributes.Directory)
					bIsDirectory = false;
				}


				if(to.Text.ToLower() == strLowerText && (to.m_strPathName == strPathName || (bIsDirectory == true && bMergeFolders == true)))
					return to;
			}
			return null;
		}

		internal delegate bool TreeNodeActionDelegate(TreeObject to, object args);		// Functions called recursively match this delegate.
		private Flags m_flags;
		private object m_args;
		public int m_numProcessedFiles;

		internal bool Recurse(TreeObject to, TreeNodeActionDelegate DoAction, Flags flags, object args)
		{
			m_flags = flags;
			m_args = args;

			m_numProcessedFiles = 0;

			try
			{
				return Recurse(to, DoAction);
			}
			catch(Exception ex)
			{
				MessageBox.Show(null, "Exception:\n" + ex.Message + "\n", "Unhandled exception", MessageBoxButtons.OK, MessageBoxIcon.Error);
				return false;
			}
		}

		private bool Recurse(TreeObject to, TreeNodeActionDelegate DoAction)
		{
			int count = to.Nodes.Count;
			for(int i = count - 1; i >= 0; i--)		// Count backwards since a node may delete itself and screw up a forward counter.
			{
				TreeNode tnChild = to.Nodes[i];
				if(Recurse(tnChild as TreeObject, DoAction) == false)
					return false;
			}

			if((m_flags == Flags.Checked && to.Checked) || (m_flags == Flags.All))
			{
				m_numProcessedFiles++;

				if(DoAction(to, m_args) == false)
					return false;
			}

			return true;
		}

		ToolTip tt = new ToolTip();

		TreeNode oldTreeNode = null;
		int oldIconIndex = -1;
		bool bLastDrewPath = true;

		private void RoboTree_MouseMove(object sender, MouseEventArgs e)
		{
			Debug.Assert(this.ShowNodeToolTips == false);
			Point p = this.PointToClient(MousePosition);
			TreeObject to = GetNodeAt(p) as TreeObject;
			
			if(to != null && to.Parent != null)				/// Wrt tn.Parent. The root node should have no tooltip.
			{
				Rectangle rect = to.Bounds;

				//if(p.X < rect.X + 3 * 16 && p.X > rect.X)
				if(p.X < rect.X + this.m_skus.Count * 16 && p.X > rect.X)
				{
					int iconIndex = (p.X - rect.X) / 16;
					Point q = new Point(p.X, p.Y + 22);
					if(to != oldTreeNode || iconIndex != oldIconIndex || bLastDrewPath == true)
					{
						bLastDrewPath = false;
						//TreeObject to = to as TreeObject;
						string strText = "Platform:\t'" + m_skus[iconIndex].strSkuName + "'\n";

						TreeItemIconIndex imageIndex = to.m_listTagInfo[iconIndex].m_iconIndex;

						switch(imageIndex)
						{
							case TreeItemIconIndex.NoSourceFile:
								strText += "No source file.";
								break;
							case TreeItemIconIndex.Same:
								strText += "Type:\t\tFile.\nFrom:\t\t'" + to.m_listTagInfo[iconIndex].m_fsiSource.FullName + "'\tTimestamp: "+ to.m_listTagInfo[iconIndex].m_fsiSource.LastWriteTime.ToString()+"'\n" +
									"To:\t\t'" + to.m_listTagInfo[iconIndex].m_fsiDest.FullName + "'\tTimestamp: " + to.m_listTagInfo[iconIndex].m_fsiDest.LastWriteTime.ToString() + "'\n" +
									"Source and destination file match.";
								break;
							case TreeItemIconIndex.SourceFileIsNewer:
								strText += "Type:\t\tFile.\nFrom:\t\t'" + to.m_listTagInfo[iconIndex].m_fsiSource.FullName + "'\tTimestamp: " + to.m_listTagInfo[iconIndex].m_fsiSource.LastWriteTime.ToString() + "'\n" +
									"To:\t\t'" + to.m_listTagInfo[iconIndex].m_fsiDest.FullName + "'\tTimestamp: " + to.m_listTagInfo[iconIndex].m_fsiDest.LastWriteTime.ToString() + "'\n" +
									"Source file is newer.";
								break;
							case TreeItemIconIndex.SourceFileIsOlder:
								strText += "Type:\t\tFile.\nFrom:\t\t'" + to.m_listTagInfo[iconIndex].m_fsiSource.FullName + "'\tTimestamp: " + to.m_listTagInfo[iconIndex].m_fsiSource.LastWriteTime.ToString() + "'\n" +
									"To:\t\t'" + to.m_listTagInfo[iconIndex].m_fsiDest.FullName + "'\tTimestamp: " + to.m_listTagInfo[iconIndex].m_fsiDest.LastWriteTime.ToString() + "'\n" +
									"Source file is older.";
								break;
							case TreeItemIconIndex.NoDestinationFile:
								strText += "Type:\t\tFile.\nFrom:\t\t'" + to.m_listTagInfo[iconIndex].m_fsiSource.FullName + "'\tTimestamp: " + to.m_listTagInfo[iconIndex].m_fsiSource.LastWriteTime.ToString() + "'\n" +
									"To:\t\t'" + to.m_listTagInfo[iconIndex].m_fsiDest.FullName + "'\n" +
									"Destination file does not exist.";
								break;
							case TreeItemIconIndex.Problem:
								strText += "*THIS IS A PROBLEM*";
								break;
							case TreeItemIconIndex.FileFolderMismatch:
								strText += "* YOU HAVE A FILE / FOLDER MISMATCH*";
								break;
							case TreeItemIconIndex.Folder:
								strText += "Type:\t\tFolder.\nFrom:\t\t'" + to.m_listTagInfo[iconIndex].m_fsiSource.FullName + "'\n" +
									"To:\t\t'" + to.m_listTagInfo[iconIndex].m_fsiDest.FullName + "'\n\n"
									;
								break;
							default:
								break;
						}
						tt.Show(strText, this.Parent, q);
					}

					oldIconIndex = iconIndex;
				}
				else
				{
					if(p.X > rect.X)
					{
						if(to.m_bIsFolder == false && (oldTreeNode != to || bLastDrewPath == false))
						{
							bLastDrewPath = true;
							tt.Show(to.m_strPathName, this.Parent, new Point(p.X, p.Y + 22));
						}
						else if(to.m_bIsFolder == true)
							tt.Hide(this.Parent);
					}
					else
						tt.Hide(this.Parent);

					//to = null;
					//tt.Hide(this.Parent);
				}
			}
			else
				tt.Hide(this.Parent);

			oldTreeNode = to;
		}

		private void RoboTree_MouseLeave(object sender, EventArgs e)
		{
			oldTreeNode = null;

			tt.Hide(this.Parent);
		}



	}
}
