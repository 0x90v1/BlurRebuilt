using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace RoboPublish
{
	public partial class DeleteListViewer : Form
	{
		public DeleteListViewer()
		{
			InitializeComponent();
		}

		public bool Clear()
		{
			this.listView1.Items.Clear();
			return true;
		}

		public ListViewItem AddItem(FileSystemInfo fsi, bool bChecked)
		{
			ListViewItem lvi = this.listView1.Items.Add(fsi.FullName);
			lvi.Tag = fsi;
			lvi.Checked = bChecked;
			if((fsi.Attributes & FileAttributes.Directory) == FileAttributes.Directory)
				lvi.ForeColor = Color.Blue;
			return lvi;
		}

		private void buttonSelectAll_Click(object sender, EventArgs e)
		{
			foreach(ListViewItem lvi in listView1.Items)
			{
				lvi.Checked = true;
			}
		}

		private void buttonToggleSelection_Click(object sender, EventArgs e)
		{
			foreach(ListViewItem lvi in listView1.Items)
			{
				lvi.Checked = !lvi.Checked;
			}
		}

		bool m_bDeletedSomething = false;

		private void buttonDelete_Click(object sender, EventArgs e)
		{
			try
			{
				DialogResult dr = MessageBox.Show(this, "Are you sure you wish to delete the selected items?", "Monkey - Confirm delete", MessageBoxButtons.YesNo, MessageBoxIcon.Question);

				if(dr == DialogResult.Yes)
				{
					List<ListViewItem> deleteList = new List<ListViewItem>();

					foreach(ListViewItem lvi in listView1.Items)
					{
						if(lvi.Checked == true)
						{
							FileSystemInfo fsi = lvi.Tag as FileSystemInfo;

							if((fsi.Attributes & FileAttributes.Directory) == FileAttributes.Directory)
							{
								try
								{
									//Directory.Delete(fsi.FullName, true);
									Directory.Delete(fsi.FullName);
									deleteList.Add(lvi);
								}
								catch { }
							}
							else
							{
								File.Delete(fsi.FullName);
								deleteList.Add(lvi);
							}
						}
					}

					foreach(ListViewItem lvi in deleteList)
					{
						listView1.Items.Remove(lvi);
						m_bDeletedSomething = true;
					}
				}
			}
			catch(Exception ex)
			{
				MessageBox.Show(this, "Something went wrong. Please mail 'tools'. System reports:\n" + ex.Message, "Naughty Monkey", MessageBoxButtons.OK, MessageBoxIcon.Error);
			}
		}

		private void buttonClose_Click(object sender, EventArgs e)
		{
			if(m_bDeletedSomething == true)
				this.DialogResult = DialogResult.Yes;
			else
				this.DialogResult = DialogResult.No;
		}

		private void DeleteListViewer_FormClosing(object sender, FormClosingEventArgs e)
		{
			if(m_bDeletedSomething == true)
				this.DialogResult = DialogResult.Yes;
			else
				this.DialogResult = DialogResult.No;
		}
	}
}