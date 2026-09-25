using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;
using Executer.Helpers;
using Executer.Interfaces;
using message_handler;
using CustomMessageBox;
using System.Drawing;

namespace Executer
{
	public abstract class TreeBaseObject : TreeNode, ITreeNodeContainer
	{
		//public static Color R_READONLY_COLOUR = System.Drawing.SystemColors.Highlight;
		//public static Color R_RESTRICTED_COLOR = System.Drawing.SystemColors.HighlightText;
		//public static Color R_PRIMARY_RESTRICTED_COLOR = System.Drawing.SystemColors.MenuHighlight;
		public static Color R_READONLY_COLOUR = System.Drawing.Color.DarkBlue;
		public static Color R_RESTRICTED_COLOR = System.Drawing.Color.Red;
		public static Color R_PRIMARY_RESTRICTED_COLOR = System.Drawing.Color.Purple;
		//public static Color R_WRITABLE_COLOUR = System.Drawing.Color.Green;
		public static Color R_WRITABLE_COLOUR = System.Drawing.Color.Black;

		private bool m_bBackupExpandedState = false;

		public TreeBaseObject()
		{
		}

		public TreeBaseObject(string strName)
			: base(strName)
		{
		}

		static LocallyUniqueIdMaker s_luidMaker = new LocallyUniqueIdMaker();

		internal readonly long m_luid = s_luidMaker.GetNextLuid();

		internal void AddWithIcons(TreeNode treeChild, string strImageKey, string strSelectedImageKey)
		{
			this.Nodes.Add(treeChild);

			if(treeChild.IsExpanded == true)
				this.Expand();
			
			try
			{
				treeChild.ImageKey = strImageKey;
				treeChild.SelectedImageKey = strSelectedImageKey;
			}
			catch
			{
			}
		}

		public abstract string TypeName { get; }

		public void StoreExpandedState()
		{
			this.m_bBackupExpandedState = this.IsExpanded;

			foreach(TreeBaseObject tbo in this.Nodes)
			{
				if(tbo != null)
				{
					tbo.StoreExpandedState();
				}
			}
		}

		public void RestoreExpandedState()
		{
			if (m_bBackupExpandedState != this.IsExpanded)
			{
				if (m_bBackupExpandedState == true)
					this.Expand();
				else
					this.Collapse();
			}

			foreach(TreeBaseObject tbo in this.Nodes)
			{
				if(tbo != null)
				{
					tbo.RestoreExpandedState();
				}
			}
		}



		#region ITreeNodeContainer Members

		public TreeNodeCollection ChildNodes
		{
			get { return this.Nodes; }
		}

		public ITreeNodeContainer NextNodeContainer
		{
			get { return this.NextNode as ITreeNodeContainer; }
		}

		public ITreeNodeContainer ParentContainer
		{
			get
			{
				if(this.Parent != null)
					return this.Parent as ITreeNodeContainer;
				if(this.TreeView != null)
				{
					if(this.TreeView.Nodes.Contains(this))
						return this.TreeView as ITreeNodeContainer;
				}
				return null;
			}
		}

		#endregion
	}
}
