using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Collections;
using System.Diagnostics;
using Executer.Interfaces;
using Executer.TreeObjects;


namespace Executer
{
	public partial class PromptSave : Form
	{
		private PromptSave()
		{
			InitializeComponent();

			this.listView1.Items.Clear();
		}

		readonly bool m_bAllowCancelButton;


		public PromptSave(ISaveable saveableObject, bool bAllowCancelButton, bool bOnlyListDirtyObjects) : this()
		{
			List<ISaveable> objectsToSave = Saveable.GetSaveableObjectList(saveableObject, bOnlyListDirtyObjects);

			foreach(ISaveable objectToSave in objectsToSave)
			{
				if((objectToSave.IsDirty == true) || (bOnlyListDirtyObjects == false))
					AddItem(objectToSave);
			}

			m_bAllowCancelButton = bAllowCancelButton;
		}

		public bool IsEmpty { get { return this.listView1.Items.Count == 0; } }

		private void PromptSave_Load(object sender, EventArgs e)
		{
			this.buttonCancel.Enabled = m_bAllowCancelButton;
		}

		private void AddAll(List<ISaveable> objectsToSave)
		{
			foreach(ISaveable saveableObject in objectsToSave)
			{
				ISaveable iSaveable = (ISaveable)saveableObject;
				if(iSaveable.IsDirty == true)
					AddItem(saveableObject as ISaveable);
			}
		}

		public int NumEntries { get { return this.listView1.Items.Count; } }

		private bool AddItem(ISaveable iSaveable)
		{
			string saveName = iSaveable.SaveName;

			if(saveName == null || saveName == "")
			{
				TreeBaseObject tbo = iSaveable as TreeBaseObject;
				if(tbo != null)
				{
					saveName = "Unnamed " + tbo.TypeName;
				}
			}

			ListViewItem lvi = this.listView1.Items.Add(saveName);
			
			lvi.Tag = iSaveable;
			lvi.Checked = true;
			numCheckedItems++;
			return true;
		}

		private void buttonAll_Click(object sender, EventArgs e)
		{
			foreach(ListViewItem lvi in this.listView1.Items)
			{
				ISaveable saveableObject = lvi.Tag as ISaveable;

				if(saveableObject != null)
				{
					if(Saveable.Save(saveableObject, false) == SaveableResult.Cancelled)
					{
						this.DialogResult = DialogResult.Cancel;
						return;
					}
				}
			}
			this.DialogResult = DialogResult.OK;
		}

		private void buttonSelected_Click(object sender, EventArgs e)
		{
			List<ListViewItem> removeList = new List<ListViewItem>(listView1.Items.Count);
			foreach(ListViewItem lvi in this.listView1.Items)
			{
				if(lvi.Checked == true)
				{
					ISaveable saveableObject = lvi.Tag as ISaveable;
					if(saveableObject != null)
					{
						if(Saveable.Save(saveableObject, false) == SaveableResult.Success)
						{
							numCheckedItems--;
							if(numCheckedItems == 0)
								buttonSelected.Enabled = false;
							removeList.Add(lvi);
						}
					}
				}
			}

			foreach(ListViewItem lviToRemove in removeList)
			{
				listView1.Items.Remove(lviToRemove);
			}
			if(listView1.Items.Count == 0)		// Don't bail out unless there is nothing left to save.
				this.DialogResult = DialogResult.OK;
		}

		private void buttonNo_Click(object sender, EventArgs e)
		{
			this.DialogResult = DialogResult.No;
		}

		private void buttonCancel_Click(object sender, EventArgs e)
		{
			this.DialogResult = DialogResult.Cancel;
		}

		int numCheckedItems = 0;

		private void listView1_ItemChecked(object sender, ItemCheckedEventArgs e)
		{
			if(e.Item.Checked == true)
			{
				numCheckedItems++;
				buttonSelected.Enabled = true;
			}
			else
			{
				numCheckedItems--;
				if(numCheckedItems == 0)
					buttonSelected.Enabled = false;
			}

		}
	}
}