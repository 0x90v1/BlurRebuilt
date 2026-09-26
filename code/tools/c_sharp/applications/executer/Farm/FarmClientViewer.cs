using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;
using Executer.Farm;

namespace Executer
{
	public partial class FarmClientViewer : Form
	{
		public FarmClientViewer()
		{
			InitializeComponent();

			// TODO: BUG.
			this.Visible = true;
			this.CreateControl();
			this.Visible = false;
		}

		bool bLoaded = false;

		internal bool UpdateClientStatus(FarmClient farmClient, string strChange)
		{
			if(InvokeRequired == true)
			{
				bool bRetVal = false;
				// ATTENTION Invoke -> BeginInvoke to fix deadlock preventing connections to farm during command line processing.
				//this.Invoke((MethodInvoker)delegate() { bRetVal = UpdateClientStatus(farmClient, strChange); });
				this.BeginInvoke((MethodInvoker)delegate() { bRetVal = UpdateClientStatus(farmClient, strChange); });
				return bRetVal;
			}

			ListViewItem lvi = PrepareListViewItem(farmClient, strChange);

			lvi.SubItems[5].Text = strChange;

			return true;
		}


		internal bool UpdateConnectionStatus(FarmClient farmClient, string strChange)
		{
			if(InvokeRequired == true)
			{
				bool bRetVal = false;
				// ATTENTION Invoke -> BeginInvoke to fix deadlock with test of 2 identical solutions loaded.
				this.BeginInvoke((MethodInvoker)delegate() { bRetVal = UpdateConnectionStatus(farmClient, strChange); });
				return bRetVal;
			}

			ListViewItem lvi = PrepareListViewItem(farmClient, strChange);

			lvi.SubItems[6].Text = strChange;

			return true;
		}

		private ListViewItem PrepareListViewItem(FarmClient farmClient, string strChange)
		{
			Debug.Assert(bLoaded == true);

			string strOutput = "Server name '" + farmClient.StrLocalName + "' Message '" + strChange + "'\n";

			ListViewItem lvi = this.superListView.Items[farmClient.StrName];

			if(lvi == null)
			{
				lvi = new ListViewItem(farmClient.StrName);
				lvi.Name = farmClient.StrName;
				for(int c = 0; c < 6; c++)
				{
					lvi.SubItems.Add("EMPTY");
				}
				this.superListView.Items.Add(lvi);
			}

			lvi.SubItems[1].Text = farmClient.StrLocalName;
			lvi.SubItems[2].Text = farmClient.RemoteIp == null ? "Unknown" : farmClient.RemoteIp.ToString();
			lvi.SubItems[3].Text = farmClient.TcpPort.ToString();
			lvi.SubItems[4].Text = farmClient.UdpPort.ToString();
			return lvi;
		}

		private void ServerViewer_FormClosing(object sender, FormClosingEventArgs e)
		{
			if(e.CloseReason == CloseReason.UserClosing)
			{
				this.Visible = false;
				e.Cancel = true;
			}
		}

		private void ServerViewer_Load(object sender, EventArgs e)
		{
			this.superListView.Columns.Add("Name", 200);
			this.superListView.Columns.Add("Local name", 120);
			this.superListView.Columns.Add("IP Address", 120);
			this.superListView.Columns.Add("TCP Port", 120);
			this.superListView.Columns.Add("UDP Port", 120);
			this.superListView.Columns.Add("Status", 90);
			this.superListView.Columns.Add("Connection Status", 120);

			bLoaded = true;
		}

		internal void RemoveClient(FarmClient farmClient)
		{
			ListViewItem lvi = this.superListView.Items[farmClient.StrName];

			Debug.Assert(lvi != null);

			if(lvi != null)
			{
				lvi.Remove();
			}
		}
	}
}