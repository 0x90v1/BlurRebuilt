using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;
using Executer.Interfaces;
using Executer.TreeObjects;
using message_handler;

namespace Executer
{
	public partial class SearchDialog : Form
	{
		private static ITreeNodeContainer searchAnchor = null;

		public SearchDialog()
		{
			InitializeComponent();
		}

		private void SearchDialog_Load(object sender, EventArgs e)
		{
			comboBoxMatchNodeName.Enabled = checkBoxMatchNodeName.Checked;
			comboBoxMatchContainerName.Enabled = checkBoxMatchContainerName.Checked;
			comboBoxMatchContainerValue.Enabled = checkBoxMatchContainerValue.Checked;
			comboBoxMatchTypeName.Enabled = checkBoxMatchTypeName.Checked;
			comboBoxMatchInternalName.Enabled = checkBoxMatchInternalName.Checked;

			//comboBoxLookIn.SelectedValue = comboBoxLookIn.Items[1];
			comboBoxLookIn.SelectedIndex = 1;
		}

		private void SearchDialog_FormClosing(object sender, FormClosingEventArgs e)
		{
			if(e.CloseReason == CloseReason.UserClosing)
			{
				this.Visible = false;
				e.Cancel = true;
			}
		}

		private void checkBoxMatchNodeName_CheckedChanged(object sender, EventArgs e)
		{
			CheckBox checkBox = sender as CheckBox;

			if(checkBox != null)
			{
				comboBoxMatchNodeName.Enabled = checkBox.Checked;
			}
		}

		private void checkBoxMatchContainerName_CheckedChanged(object sender, EventArgs e)
		{
			CheckBox checkBox = sender as CheckBox;

			if(checkBox != null)
			{
				comboBoxMatchContainerName.Enabled = checkBox.Checked;
			}
		}

		private void checkBoxMatchContainerValue_CheckedChanged(object sender, EventArgs e)
		{
			CheckBox checkBox = sender as CheckBox;

			if(checkBox != null)
			{
				comboBoxMatchContainerValue.Enabled = checkBox.Checked;
			}
		}

		private void checkBoxMatchTypeName_CheckedChanged(object sender, EventArgs e)
		{
			CheckBox checkBox = sender as CheckBox;

			if(checkBox != null)
			{
				comboBoxMatchTypeName.Enabled = checkBox.Checked;
			}
		}

		private void checkBoxMatchInternalName_CheckedChanged(object sender, EventArgs e)
		{
			CheckBox checkBox = sender as CheckBox;

			if(checkBox != null)
			{
				comboBoxMatchInternalName.Enabled = checkBox.Checked;
			}
		}

		private void buttonFindNext_Click(object sender, EventArgs e)
		{
			// Aargh.

			SearchDetails searchDetails = GetSearchDetailsFromUI();
			if(searchDetails != null)
			{
				ITreeNodeContainer tn = searchDetails.GetNextTreeNode();

				if(tn != null && tn is TreeNode)
					Program.ExecuterMainForm.MainTreeView.SelectedNode = tn as TreeNode;
			}
		}

		private SearchDetails GetSearchDetailsFromUI()
		{
			string strLookIn = comboBoxLookIn.Text;

			string strNodeName = checkBoxMatchNodeName.Checked == true ? comboBoxMatchNodeName.Text : null;
			string strContainerName = checkBoxMatchContainerName.Checked == true ? comboBoxMatchContainerName.Text : null;
			string strContainerValue = checkBoxMatchContainerValue.Checked == true ? comboBoxMatchContainerValue.Text : null;
			string strTypeName = checkBoxMatchTypeName.Checked == true ? comboBoxMatchTypeName.Text : null;
			string strInternalName = checkBoxMatchInternalName.Checked == true ? comboBoxMatchInternalName.Text : null;

			//bool bNodeNameMatchRequired = true;
			//bool bContainerNameMatchRequired = true;
			//bool bContainerValueMatchRequired = true;
			//bool bTypeNameMatchRequired = true;
			//bool bInternalNameMatchRequired = true;

			//if(buttonMatchNodeName.Text == "REQUIRED") bNodeNameMatchRequired = true;
			//else bNodeNameMatchRequired = false;
			//if(buttonMatchContainerName.Text == "REQUIRED") bContainerNameMatchRequired = true;
			//else bContainerNameMatchRequired = false;
			//if(buttonMatchContainerValue.Text == "REQUIRED") bContainerValueMatchRequired = true;
			//else bContainerValueMatchRequired = false;
			//if(buttonMatchTypeName.Text == "REQUIRED") bTypeNameMatchRequired = true;
			//else bTypeNameMatchRequired = false;
			//if(buttonMatchInternalName.Text == "REQUIRED") bInternalNameMatchRequired = true;
			//else bInternalNameMatchRequired = false;

			ITreeNodeContainer currentNode = Program.ExecuterMainForm.MainTreeView.SelectedNode as ITreeNodeContainer;

			ITreeNodeContainer anchorNode;

			if(comboBoxLookIn.Text == "Anchor children")
			{
				anchorNode = searchAnchor;
			}
			else if(comboBoxLookIn.Text == "Current Project")
			{
				anchorNode = SearchUpforTypeOf(currentNode, typeof(TreeProjectObject));
				if(anchorNode == null)
				{
					MessageHandler.Show(this, "The current tree selection is not part of a Project.");
					return null;
				}
			}
			else if(comboBoxLookIn.Text == "Current Solution")
			{
				anchorNode = SearchUpforTypeOf(currentNode, typeof(TreeSolutionObject));
				if(anchorNode == null)
				{
					MessageHandler.Show(this, "The current tree selection is not part of a Solution.");
					return null;
				}
			}
			else if(comboBoxLookIn.Text == "All Solutions")
			{
				anchorNode = Program.ExecuterMainForm.MainTreeView;
				if(anchorNode == null)
				{
					MessageHandler.Show(this, "The current tree selection is not valid.");
					return null;
				}
			}
			else
			{
				MessageHandler.Show(this, "The current 'Look in' value is not valid.");
				return null;
			}

			if(currentNode == null)
			{
				currentNode = anchorNode;
			}

			return new SearchDetails
				(
				strNodeName,
				strContainerName,
				strContainerValue,
				strTypeName,
				strInternalName,
				//bNodeNameMatchRequired,
				//bContainerNameMatchRequired,
				//bContainerValueMatchRequired,
				//bTypeNameMatchRequired,
				//bInternalNameMatchRequired,
				anchorNode,
				currentNode,
				currentNode
				);
		}

		private static ITreeNodeContainer SearchUpforTypeOf(ITreeNodeContainer currentNode, Type type)
		{
			while((currentNode != null) && (currentNode.GetType() != type))
				currentNode = currentNode.ParentContainer;
			return currentNode;
		}


		private void buttonDropSearchAnchor_Click(object sender, EventArgs e)
		{
			searchAnchor = Program.ExecuterMainForm.MainTreeView.SelectedNode as ITreeNodeContainer;
		}

		private void checkBoxAutoClear_CheckedChanged(object sender, EventArgs e)
		{

		}

		private void buttonFindAll_Click(object sender, EventArgs e)
		{
			if(checkBoxAutoClear.Checked == true)
			{
				this.listViewResults.Items.Clear();
			}

			SearchDetails searchDetails = GetSearchDetailsFromUI();
			if(searchDetails != null)
			{
				//ITreeNodeContainer tn = searchDetails.GetNextTreeNode();

				while(searchDetails.GetNextTreeNode() != null)
				{
					TreeUserObject tuo = searchDetails.CurrentNode as TreeUserObject;
					if(tuo != null)
					{
						if(ListViewContains(tuo) == false)
						{
							ListViewItem lvi = new ListViewItem(tuo.FullPath);
							lvi.Tag = tuo;
							this.listViewResults.Items.Add(lvi);
						}
					}
					if(searchDetails.EndNode == searchDetails.CurrentNode)
						return;
				}
			}
		}

		private void buttonClearResults_Click(object sender, EventArgs e)
		{
			this.listViewResults.Items.Clear();

		}

		private void buttonSelectInTree_Click(object sender, EventArgs e)
		{
			Debug.Assert(Program.ExecuterMainForm.MainTreeView.CheckBoxes == true);

			foreach(ListViewItem lvi in listViewResults.Items)
			{
				TreeNode tn = lvi.Tag as TreeNode;

				if(tn != null)
					tn.Checked = true;
			}
		}

		private bool ListViewContains(TreeUserObject tuo)
		{
			foreach(ListViewItem lvi in listViewResults.Items)
			{
				if(lvi.Tag == tuo)
					return true;
			}
			return false;
		}

		private void listViewResults_MouseDoubleClick(object sender, MouseEventArgs e)
		{
			if(sender is ListView)
			{
				ListView lv = sender as ListView;
				ListViewItem lvi = lv.SelectedItems[0];

				Program.ExecuterMainForm.MainTreeView.SelectedNode = lvi.Tag as TreeNode;
			}
		}

		private void buttonAndOr_Click(object sender, EventArgs e)
		{
			Button b = sender as Button;
			b.Text = b.Text == "REQUIRED" ? "OPTIONAL" : "REQUIRED";
		}

		private void comboBox_All_Leave(object sender, EventArgs e)
		{
			ToolStripComboBox cb = sender as ToolStripComboBox;

			if(cb != null)
			{
				if(cb.Items.Contains(cb.Text) == false)
				{
					cb.Items.Add(cb.Text);
				}
			}
		}

		#region Nested type: SearchDetails

		#endregion
	}
}

#if false
	//ExecuterTreeUtilities.GetNextTreeNode()
		void test()
		{
			// TODO: Use this to complete the 'find' stuff.
			List<TreeUserObject> results = ExecuterTreeUtilities.GetAllUserObjects(Program.S_executerMainForm.m_executerTreeView.Nodes);

			string strPathQuery = "values//container[@name='" + strValue1 + "' and @value='" + strValue2 + "']";

			TreeUserObject tuo = null;

			foreach(TreeUserObject treeUserObject in results)
			{
				if(treeUserObject.XTreeNodeMenuData.SelectSingleNode(strPathQuery) != null)
				{
					tuo = treeUserObject;
					break;
				}
			}
		}

#endif