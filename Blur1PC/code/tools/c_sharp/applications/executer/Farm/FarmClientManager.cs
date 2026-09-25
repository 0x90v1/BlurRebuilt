using System;
using System.Collections.Generic;
using System.Text;
using Executer.Farm;
using Executer.JobManagement;
using SuperCollections;
using Global;
using System.Xml;
using comms_tcp;
using System.Net;
using System.Windows.Forms;
using message_handler;
using System.Diagnostics;
using super_collections;

namespace Executer
{
	public enum FarmClientStatus
	{
		Added = 0,
		Removed
	}

	public class FarmClientEventArgs : EventArgs
	{
		readonly FarmClient m_farmClient;

		readonly FarmClientStatus m_farmClientStatus;

		public FarmClient FarmClient
		{
			get { return m_farmClient; }
		}

		public FarmClientStatus FarmClientStatus
		{
		  get { return m_farmClientStatus; }  
		}

		public FarmClientEventArgs(FarmClient farmClient, FarmClientStatus status)
		{
			m_farmClient = farmClient;

			m_farmClientStatus = status;
		}
	}

	public class FarmClientManager
	{
#pragma Not ideal. This should be private.
		internal FarmClientViewer m_farmClientViewer = null;

		public event EventHandler<FarmClientEventArgs> FarmClientCreatedEvent;
		public event EventHandler<FarmClientEventArgs> FarmClientRemovedEvent;

		protected void OnFarmClientCreated(FarmClient newFarmClient)
		{
			if(FarmClientCreatedEvent != null)
			{
				FarmClientCreatedEvent(this, new FarmClientEventArgs(newFarmClient, FarmClientStatus.Added));
			}
		}

		protected void OnFarmClientRemoved(FarmClient farmClient)
		{
			if(FarmClientRemovedEvent != null)
			{
				FarmClientRemovedEvent(this, new FarmClientEventArgs(farmClient, FarmClientStatus.Removed));
			}
		}

		/// <summary>
		/// This is a dictionary of farm FarmClients that the FarmClientManager manages.
		/// </summary>
		internal EventDictionary<string, FarmClient> m_farmClients;

		/// <summary>
		/// This creates a FarmClientManager and creates an empty dictionary of clients.
		/// </summary>
		public FarmClientManager(bool bCreateClientViewer)
		{
			m_farmClients = new EventDictionary<string, FarmClient>();

			m_farmClients.DictionaryChangedEvent += new EventHandler<EventDictionary<string, FarmClient>.DictionaryChangedEventArgs>(m_clients_OnChange);

			if(bCreateClientViewer == true)
			{
				m_farmClientViewer = new FarmClientViewer();
			}
		}

		/// <summary>
		/// This gets called if anything gets added to or removed from m_farmClients. It is called before the addition or removal.
		/// </summary>
		/// <param name="ct">Describes the operation that is about to take place.</param>
		/// <param name="strName">The key of the dictionary entry being operated on. (Where applicable)</param>
		/// <param name="se">The value of the dictionary entry being operated on. (Where applicable)</param>
		void m_clients_OnChange(object sender, EventDictionary<string, FarmClient>.DictionaryChangedEventArgs e)
		{
			FarmClient farmClient = e.TValue as FarmClient;

			Debug.Assert(farmClient != null);
			switch (e.ChangeType)
			{
				case DictionaryChangeType.Add:
					this.OnFarmClientCreated(farmClient);
					farmClient.CurrentStatus = ClientStatus.Created;
					
					break;
				case DictionaryChangeType.Clear:
					throw new ApplicationException("m_farmClients Clear not supported.");
//					break;
				case DictionaryChangeType.Remove:
					this.OnFarmClientRemoved(farmClient);
					farmClient.CurrentStatus = ClientStatus.Destroyed;
					break;
				default:
					break;
			}
		}

		// Public methods.

		/// <summary>
		/// This takes parameters that describe a farm server. If the client list contains a client that matches the parameters
		/// then that client is marked as ServerStatusEnum.Active, otherwise a client is created from the parameters and added to the
		/// client list. This new client is then similarly marked as active.
		/// </summary>
		/// <param name="xServer">An XmlElement describing a server.</param>
		/// <param name="userData">A user defined object that is attached to every onChangeCallback from this server.</param>
		/// <returns>true.</returns>
		private bool AddClient(XmlElement xServer, object userData)
		{
			string strName = XmlHelper.GetKeyValue(xServer, "name");								// name
			string strLocalName = XmlHelper.GetKeyValue(xServer, "local_name");						// local_name
			int tcpPort = -1;	
			int.TryParse(XmlHelper.GetKeyValue(xServer, "tcp_port"), out tcpPort);					// tcp_port
			int udpPort = -1;
			int.TryParse(XmlHelper.GetKeyValue(xServer, "udp_port"), out udpPort);					// udp_port
			string strLocation = XmlHelper.GetKeyValue(xServer, "location");						// location

			string strStartupCommand = XmlHelper.GetKeyValue(xServer, "startup_command");
			bool bKillOnExit = XmlHelper.GetKeyBool(xServer, "kill_on_exit");

			// Ignore this server if it is already in the list, otherwise create a new one.
			// TODO: Filter on server ports rather than server name.
			if (m_farmClients.ContainsKey(strLocalName) == false)
			{
				IPAddress ipAddr = null;

				if(strLocation == "local")
				{
//					ipAddr = cGlobal.GetLocalIpAddress();
					ipAddr = IPAddress.Loopback;
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
						throw new ApplicationException("You have a corrupt ip address entry in your server list. Valid entries are 'local', 'unknown' or an ip address in dotted quad notation.");
					}
				}
				//StatusChangedEvent
				FarmClient farmClient = new FarmClient(strName, strLocalName, tcpPort, udpPort, /*OnChangeCallback,*/ ipAddr, strStartupCommand, bKillOnExit);

				//farmClient.StatusChangedEvent += new EventHandler<ChangedStatusEventArgs>(m_farmClientViewer.FarmClientStatusChanged);
				//farmClient.ConnectionStateChangedEvent += new EventHandler<ChangedConnectionStateEventArgs>(m_farmClientViewer.FarmClientConnectionStatusChanged);

				//// TODO: Can these be put into 'Job'?????
				//farmClient.NotifyJobStatusChangedEvent += new EventHandler<NotifyChangedJobStatusEventArgs>(Program.s_jobManager.FarmClientJobStatusChanged);
				//farmClient.NotifyReceivedJobDataEvent += new EventHandler<NotifyReceivedJobDataEventArgs>(Program.s_jobManager.FarmClientReceivedJobData);

					//		Program.s_jobManager.ReceivedString(data as string, farmClient, jobId);


				m_farmClients.Add(strLocalName, farmClient);
				farmClient.Initialise();
			}

			m_farmClients[strLocalName].CurrentStatus = ClientStatus.Active;
			
			return true;
		}

		/// <summary>
		/// Looks for a farm client with the given local_name.
		/// </summary>
		/// <param name="strClientLocalName">The name to look for.</param>
		/// <returns>Either a reference to the FarmClient or null if the client is not found.</returns>
		public FarmClient FindClientByLocalName(string strClientLocalName)
		{
			if(m_farmClients.ContainsKey(strClientLocalName))
			{
				return m_farmClients[strClientLocalName];
			}
			else
			{
				return null;
			}
		}

		/// <summary>
		/// Set the status of each FarmClient in the dictionary to 'PotentiallyUnwanted'.
		/// </summary>
		/// <returns>The number of clients in the dictionary.</returns>
		private int MarkAllClientsAsPotentiallyUnwanted()
		{
			foreach (FarmClient farmClient in m_farmClients.Values)
			{
				farmClient.CurrentStatus = ClientStatus.PotentiallyUnwanted;
			}
			return m_farmClients.Count;
		}

		/// <summary>
		/// Sets the status of each unwanted server in the dictionary to 'MarkedForDeletion'.
		/// </summary>
		/// <returns>The number of unwanted servers.</returns>
		private int MarkPotentiallyUnwantedServersForDeletion()
		{
			int retval = 0;

			foreach (FarmClient se in m_farmClients.Values)
			{
				if(se.CurrentStatus == ClientStatus.PotentiallyUnwanted)
				{
					se.CurrentStatus = ClientStatus.MarkedForDeletion;
					retval++;
				}
			}
			return retval;
		}

		/// <summary>
		/// Removes every server that is marked for deletion.
		/// </summary>
		/// <returns>The number of deleted servers.</returns>
		public int DeleteUnwantedServers(bool bKillThem)
		{
			List<string> deleteList = new List<string>();
			
			foreach (string strServerName in m_farmClients.Keys)
			{
				FarmClient se = m_farmClients[strServerName];
				if(se.CurrentStatus == ClientStatus.MarkedForDeletion)
				{
					deleteList.Add(strServerName);
				}
			}

			foreach (string strServerName in deleteList)
			{
				FarmClient farmClient = m_farmClients[strServerName];
				farmClient.CurrentStatus = ClientStatus.Deleting;
				if(farmClient.m_bKillOnExit == true && bKillThem == true)
				{
					if(farmClient.Connected == true)
					{
						farmClient.SendTcpString("SHUTDOWN");
					}
				}
				farmClient.Shutdown();
				m_farmClients.Remove(strServerName);
			}
			return deleteList.Count;
		}



		/// <summary>
		/// Shutdown all the servers in the m_servers list.
		/// This implementation is chosen so that the events declaring the
		/// stages of deletion all get fired off.
		/// </summary>
		public void ShutdownServers(bool bKillThem)
		{
			MarkAllClientsAsPotentiallyUnwanted();
			MarkPotentiallyUnwantedServersForDeletion();
			DeleteUnwantedServers(bKillThem);
		}

		public bool RequestJob(Job job)
		{
			// Find the correct server. Add the job into the queue on that server.
			if(m_farmClients.ContainsKey(job.StrServerName))

			{
				FarmClient farmClient = m_farmClients[job.StrServerName];

				if(farmClient.Connected == true)
				{
					return farmClient.RequestJob(job);
				}
				else
				{
					throw new ApplicationException("You are not currently connected to the server you have chosen.");
				}
			}
			else
			{
				throw new ApplicationException("The server you requested is not available.");
			}
		}

		public bool SendMessage(string strServerName, string strMessage)
		{
			FarmClient server = FindClientByLocalName(strServerName);

			if(server != null)
			{
				return server.SendTcpString(strMessage);
			}
			else
				return false;
		}

		bool m_bReportedServerDuplicationProblem = false;

		/// <summary>
		/// Iterate through every server in every project in the tree view.
		/// Present each server to the server manager in such a way that it
		/// 1) Keeps existing servers if they are still in use.
		/// 2) Creates new servers if need be.
		/// 3) Removes servers that are no longer needed.
		/// </summary>
		/// <returns>true</returns>
		public bool PopulateFarmConnection(List<XmlElement> xAllServers)
		{
			MarkAllClientsAsPotentiallyUnwanted();

			foreach(XmlElement xServers in xAllServers)
			{
				List<int> tcpPortList = new List<int>();

				// TODO: Use xPath to demystify this.
				foreach(XmlNode xNode in xServers)
				{
					if(xNode.NodeType == XmlNodeType.Element)
					{
						XmlElement xServer = xNode as XmlElement;
						if(xServer.Name == "server_definition")
						{
							string strServerName = XmlHelper.GetKeyValue(xServer, "name");

							if(strServerName.ToLower().StartsWith("[debug]") && (Program.s_bDebugModeEnabled == false))
							{
							}
							else
							{
								int tcpPort = XmlHelper.GetKeyInt(xServer, "tcp_port", -1);

								if(tcpPortList.Contains(tcpPort) == false)
								{
									tcpPortList.Add(tcpPort);

									try
									{
										AddClient(xServer, null);
									}
									catch(Exception ex)
									{
										MessageHandler.ShowMemory(null, "Cannot add server", "Cannot add server.",
										"Cannot add the following server:\n" + xServer.OuterXml + "\nSystem reports: '" + ex.Message + "'",
										"Executer", MessageBoxButtons.OK, MessageBoxIcon.Error);
									}
								}
								else
								{
									if(m_bReportedServerDuplicationProblem == false)
										MessageHandler.ShowMemory(null, "Duplicated server tcp port", "Warn on duplicated server TCP port?", "You have one or more server definitions that define a different server name on the same TCP port.\nPlease review all templates in use by all projects that are currently loaded.", "Executer", MessageBoxButtons.OK, MessageBoxIcon.Warning);
									m_bReportedServerDuplicationProblem = true;
								}
							}
						}
					}
				}
			}

			MarkPotentiallyUnwantedServersForDeletion();
			DeleteUnwantedServers(false);

			return true;
		}

		internal void ShowServerViewer()
		{
			this.m_farmClientViewer.Show();
			this.m_farmClientViewer.BringToFront();

		}
	}
}
