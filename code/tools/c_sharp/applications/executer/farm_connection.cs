using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Net;
using System.Collections;
using communications;
using SuperControl;
using System.Diagnostics;
using System.Threading;
using YesNoMemoryDialog;

namespace Executer
{
	public partial class FarmConnection : Form
	{
		const int IP_COLUMN = 2;
		const int STATUS_COLUMN = 5;

		public delegate bool DelegateJobDataCallback(ServerEntry sv, string strData);

		DelegateJobDataCallback m_jobDataCallback;

		private FarmConnection()
		{
			InitializeComponent();
		}

		public FarmConnection(DelegateJobDataCallback jobDataCallback) : this()
		{
//			this.Visible = true;
//			this.CreateControl();
//			this.Visible = false;
			this.CreateHandle();

			m_jobDataCallback = jobDataCallback;

			this.superListView_ServerList.Clear();			/// Removes items and columns.

			this.superListView_ServerList.Columns.Add("Name", 140);
			this.superListView_ServerList.Columns.Add("Local name", 140);
			this.superListView_ServerList.Columns.Add("IP address", 90);
			this.superListView_ServerList.Columns.Add("TCP Port", 60);
			this.superListView_ServerList.Columns.Add("UDP Port", 60);
			this.superListView_ServerList.Columns.Add("Status", 150);
		}

		/// <summary>
		/// Set the status of each Server entry in the list view to 'PotentiallyUnwanted'.
		/// </summary>
		/// <returns>The number of servers in the list view.</returns>
		public int MarkAllServersAsInactive()
		{
			foreach(ListViewItem lvi in this.superListView_ServerList.Items)
			{
				ServerEntry se = lvi.Tag as ServerEntry;
				se.m_activityStatus = ServerEntry.ActivityStatus.PotentiallyUnwanted;
			}
			return superListView_ServerList.Items.Count;
		}

		/// <summary>
		/// Sets the status of each unwanted server in the list view to 'MarkedForDeletion'.
		/// </summary>
		/// <returns>The number of unwanted servers.</returns>
		public int MarkPotentiallyUnwantedServersForDeletion()
		{
			int retval = 0;

			foreach(ListViewItem lvi in this.superListView_ServerList.Items)
			{
				ServerEntry se = lvi.Tag as ServerEntry;
				if(se.m_activityStatus == ServerEntry.ActivityStatus.PotentiallyUnwanted)
				{
					se.m_activityStatus = ServerEntry.ActivityStatus.MarkedForDeletion;
					retval++;
				}
			}
			return retval;
		}

		public int DeleteUnwantedServers()
		{
			ArrayList deleteList = new ArrayList();

			foreach(ListViewItem lvi in this.superListView_ServerList.Items)
			{
				ServerEntry se = lvi.Tag as ServerEntry;
				if(se.m_activityStatus == ServerEntry.ActivityStatus.MarkedForDeletion)
				{
					deleteList.Add(lvi);
				}
			}

			foreach(ListViewItem lvi in deleteList)
			{
				// TODO: Urgent. Close any connections the tag object has. Release any resources it has.
				ServerEntry se = lvi.Tag as ServerEntry;
				if(se.m_commsUdp != null)
				{
					se.m_commsUdp.Close();
				}
				if(se.m_commsClient != null)
				{
					se.m_commsClient.Close();
				}
				se.JobQueueThreadStop();
				se.m_commsUdp = null;

				superListView_ServerList.Items.Remove(lvi);
			}
			return deleteList.Count;
		}

		bool OnChangeCallback(ListViewItem lvi, object data, ServerEntry.ChangeType changeType, ServerEntry server)
		{
			switch(changeType)
			{
				case ServerEntry.ChangeType.ChangeStatus:
					{
						InvokeSetSubItemText(lvi.SubItems[STATUS_COLUMN], data as string);
						return true;
					}
				case ServerEntry.ChangeType.ChangeIp:
					{
						InvokeSetSubItemText(lvi.SubItems[IP_COLUMN], data as string);
						return true;
					}
				case ServerEntry.ChangeType.ReceivedString:
					{
						return m_jobDataCallback(server, data as string);
					}
				case ServerEntry.ChangeType.Killed:
					{
						return m_jobDataCallback(server, data as string);
					}
				case ServerEntry.ChangeType.Failed:
					{
						return m_jobDataCallback(server, data as string);
					}
				default:
					return false;
			}
		}

		delegate bool DelegateSetSubItemText(ListViewItem.ListViewSubItem lvsi, string strText);

		internal void InvokeSetSubItemText(ListViewItem.ListViewSubItem listViewSubItem, string strText)
		{
			Invoke(new DelegateSetSubItemText(SetSubItemText), new object[] {listViewSubItem, strText });
		}

		bool SetSubItemText(ListViewItem.ListViewSubItem lvsi, string strText)
		{
			lvsi.Text = strText;
			return true;
		}

		public bool AddServer(string strName, string strLocalName, int tcpPort, int udpPort, string strLocation, string strStartupCommand, bool bKillOnExit)
		{
			/// Ignore this server if it is already in the list.
			int keyIndex = superListView_ServerList.Items.IndexOfKey(strLocalName);

			if(keyIndex == -1)
			{
				ListViewItem lvi = new ListViewItem(strName);
				lvi.SubItems.Add(strLocalName);
				lvi.SubItems.Add("Unknown");
				lvi.SubItems.Add(tcpPort.ToString());
				lvi.SubItems.Add(udpPort.ToString());
				lvi.SubItems.Add("Disconnected");
				lvi.Name = strLocalName;

				IPAddress ipAddr = null;

				if(strLocation == "local")
				{
					ipAddr = cGlobal.GetLocalIpAddress();
				}
				else if(strLocation == "unknown")
				{
					ipAddr = null;
				}
				else
				{
					try
					{
						ipAddr = IPAddress.Parse(strLocation);
					}
					catch
					{
						YesNoMemory.Show(this, "corrupt ip address", "You have a corrupt ip address entry in your server list.\nValid entries are 'local', 'unknown' or an ip address in dotted quad notation.", Program.AssemblyTitle, MessageBoxButtons.OK, MessageBoxIcon.Error);
					}
				}

				superListView_ServerList.Items.Add(lvi);
				ServerEntry sv = new ServerEntry(strName, strLocalName, tcpPort, udpPort, OnChangeCallback, lvi, ipAddr, strStartupCommand, bKillOnExit);
				lvi.Tag = sv;
				
				this.superListView_ServerList.AddItemControl(lvi.SubItems[1], SuperListViewItemControl.ControlType.Button, new SuperListView.DelegateManageControl(PopulateSuperListView_Button), null);
			}
			else
			{
				ListViewItem lvi = superListView_ServerList.Items[keyIndex];
				ServerEntry sv = lvi.Tag as ServerEntry;
				sv.m_activityStatus = ServerEntry.ActivityStatus.Active;
//				InvokeSetSubItemText(lvi.SubItems[STATUS_COLUMN], "Searching again ...");
//				sv.m_commsUdp.SendDataString("Server, Where Are You!", sv.m_udpIpEp);		This is bollocks since a local connection has no udp object.
			}
			return true;
		}

		public bool Display(IWin32Window owner)
		{
			this.Visible = true;
			this.BringToFront();

			return true;
		}

		bool PopulateSuperListView_Button(object myObject, Control control, ListViewItem.ListViewSubItem lvsi, bool set)
		{
			return true;
		}

		private void FarmConnection_FormClosing(object sender, FormClosingEventArgs e)
		{
			if(e.CloseReason == CloseReason.UserClosing)
			{
				this.Visible = false;
				e.Cancel = true;
			}
		}

		public void KillServerThreads()
		{
			bool bKilledSomething = false;

			foreach(ListViewItem lvi in this.superListView_ServerList.Items)
			{
				ServerEntry se = lvi.Tag as ServerEntry;

				if(se.m_bKillOnExit == true)
				{
					if(se.m_connectionStatus == ServerEntry.ConnectionStatus.Connected)
					{
						se.SendTcpString("SHUTDOWN");
						bKilledSomething = true;
					}
				}

				se.JobQueueThreadStop();
			}
			if(bKilledSomething == true)
			{
				System.Threading.Thread.Sleep(6000);
			}

		}

/*
		public bool RequestJob(string strServerLocalName, string strJob, string strJobDescription, object userData, JobManager.Job.DelegateRequestJobCallback jobCallback, int tempId, int tempParentId)
		{
			/// Find the correct server. Add the job into the queue on that server.
			ServerEntry sv = FindServerByName(strServerLocalName);

			if(sv != null)
			{
				if(sv.m_connectionStatus == ServerEntry.ConnectionStatus.Connected)
				{
					JobManager.Job newJob = new JobManager.Job(strServerLocalName, strJob, strJobDescription, userData, jobCallback, tempId, tempParentId);

					sv.AddJob(newJob);
					return true;
				}
				else
				{
					MessageBox.Show(this, "You are not currently connected to the server you have chosen.\nCancelling all jobs in current request.", "Executer", MessageBoxButtons.OK, MessageBoxIcon.Stop);
					return false;
				}
			}
			else
			{
				MessageBox.Show(this, "The server you requested is not available.\nCancelling all jobs in current request.", "Executer", MessageBoxButtons.OK, MessageBoxIcon.Stop);
				return false;
			}
		}
*/
		
//		public bool RequestJob(string strServerLocalName, string strJob, string strJobDescription, object userData, JobManager.Job.DelegateRequestJobCallback jobCallback, int tempId, int tempParentId)
		public bool RequestJob(JobManager.Job job)
		{
			/// Find the correct server. Add the job into the queue on that server.
			ServerEntry sv = FindServerByName(job.m_strServerLocalName);

			if(sv != null)
			{
				if(sv.m_connectionStatus == ServerEntry.ConnectionStatus.Connected)
				{
					sv.AddJob(job);
					return true;
				}
				else
				{
					MessageBox.Show(this, "You are not currently connected to the server you have chosen.\nCancelling all jobs in current request.", "Executer", MessageBoxButtons.OK, MessageBoxIcon.Stop);
					return false;
				}
			}
			else
			{
				MessageBox.Show(this, "The server you requested is not available.\nCancelling all jobs in current request.", "Executer", MessageBoxButtons.OK, MessageBoxIcon.Stop);
				return false;
			}
		}

		internal ServerEntry FindServerByName(string strServerLocalName)
		{
			int keyIndex = superListView_ServerList.Items.IndexOfKey(strServerLocalName);
			if(keyIndex != -1)
			{
				ListViewItem lvi = superListView_ServerList.Items[keyIndex];
				ServerEntry sv = lvi.Tag as ServerEntry;
				return sv;
			}
			return null;
		}


		private void superListView_ServerList_SelectedIndexChanged(object sender, EventArgs e)
		{
			SuperListView slv = sender as SuperListView;

			if(slv.SelectedItems.Count != 0)
			{
				ListViewItem lvi = slv.SelectedItems[0];
				ServerEntry sv = lvi.Tag as ServerEntry;
				this.panel_serverFeedback.Controls.Clear();
				this.panel_serverFeedback.Controls.Add(sv.m_richTextBox);
			}
		}

	}
}
