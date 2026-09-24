using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using SuperControl;
using message_handler;
using System.Diagnostics;

namespace Executer
{
	public partial class JobViewer : Form
	{
		public delegate bool DelegateUpdateJobStatus(Job job);

		Dictionary<Job, ListViewItem> m_jobDataEntries;

		public JobViewer()
		{
			InitializeComponent();
			this.Visible = true;
			this.CreateControl();
			this.Visible = false;
			this._bWantsToBeVisible = false;
			this.superListView.View = View.Details;

			this.superListView.Columns.Add("Description", 200);
			this.superListView.Columns.Add("Job Id", 140);
			this.superListView.Columns.Add("Status", 200);

			m_jobDataEntries = new Dictionary<Job, ListViewItem>();
		}

		public bool UpdateJobStatus(Job job)
		{
			ListViewItem lvi;
			superListView.BeginUpdate();
			
			m_jobDataEntries.TryGetValue(job, out lvi);
			
			if(lvi == null)
			{
				lvi = new ListViewItem(job.StrFarmComment);

				m_jobDataEntries.Add(job, lvi);

				lvi.SubItems.Add("Waiting for jobId");
				lvi.SubItems.Add(job.JobStatus.ToString());
				superListView.Items.Add(lvi);
				lvi.Tag = job;
			}

			if(job.StrJobId != null)
			{
				lvi.SubItems[1].Text = job.StrJobId;
			}
			lvi.SubItems[2].Text = job.JobStatus.ToString();

			lvi.EnsureVisible();
			superListView.EndUpdate();

			return true;
		}

		private void buttonClose_Click(object sender, EventArgs e)
		{
			this._bWantsToBeVisible = false;
			this.Visible = false;
		}

		private void JobViewer_FormClosing(object sender, FormClosingEventArgs e)
		{
			if(e.CloseReason == CloseReason.UserClosing)
			{
				this._bWantsToBeVisible = false;
				this.Visible = false;
				e.Cancel = true;
			}
		}

		private void buttonClear_Click(object sender, EventArgs e)
		{
			m_jobDataEntries.Clear();
			superListView.Items.Clear();
		}

		private void buttonCleanup_Click(object sender, EventArgs e)
		{
			List<KeyValuePair<Job, ListViewItem>> deleteList = new List<KeyValuePair<Job, ListViewItem>>(m_jobDataEntries.Count);
			
			foreach(KeyValuePair<Job, ListViewItem> kvp in m_jobDataEntries)
			{
				Job job = kvp.Key;

				if(
					//jobEntry.JobStatus == FarmClient.JobStatusEnum.Requested ||
					//jobEntry.JobStatus == FarmClient.JobStatusEnum.Queued ||
					job.JobStatus == JobStatus.Failed ||
					job.JobStatus == JobStatus.Timeout ||
					//jobEntry.JobStatus == FarmClient.JobStatusEnum.Created ||
					job.JobStatus == JobStatus.Killed ||
					job.JobStatus == JobStatus.Abandoned ||
					job.JobStatus == JobStatus.Finished ||
					job.JobStatus == JobStatus.Cancelled
				)
				{
					deleteList.Add(kvp);
					superListView.Items.Remove(kvp.Value);
				}
			}

			foreach(KeyValuePair<Job, ListViewItem> kvp in deleteList)
			{
				m_jobDataEntries.Remove(kvp.Key);
			}
		}


		private void buttonLocateSelected_Click(object sender, EventArgs e)
		{
			try
			{
				if(superListView.SelectedItems.Count > 0)
				{
					Job job = superListView.SelectedItems[0].Tag as Job;

					Debug.Assert(job != null);

					Program.S_executerMainForm.m_executerTreeView.SelectedNode = job.OwnerTreeUserObject;

					((Executer_MainForm.s_treeObjectViewer.Controls[0]) as TreeUserObjectViewer).ViewJob(job);
				}
			}
			catch { }

		}

		private void buttonKill_Click(object sender, EventArgs e)
		{
			if(superListView.SelectedItems.Count > 0)
			{
				foreach(ListViewItem lvi in superListView.SelectedItems)
				{
					Job job = lvi.Tag as Job;

					Debug.Assert(job != null);

					Program.s_jobManager.KillJob(job);
				}
			}
		}



		private bool _bWantsToBeVisible = false;

		internal void SetDesiredVisibility(bool bDesiredVisibility)
		{
			_bWantsToBeVisible = bDesiredVisibility;
			SetDesiredVisibility();
		}

		internal void SetDesiredVisibility()
		{
			this.Visible = _bWantsToBeVisible;
		}

		private void JobViewer_VisibleChanged(object sender, EventArgs e)
		{
			if(this.Visible == true)
				SetDesiredVisibility();
		}


	}
}