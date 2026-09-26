using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using comms_tcp;
using System.Net;
using System.Diagnostics;
using System.Collections;
using System.Threading;
using System.Security.Principal;
using System.Text.RegularExpressions;
using communications_udp;

namespace Executer
{
	// formerly known as ServerEntry.
	public class FarmClient
	{
		public event EventHandler<ChangedStatusEventArgs> StatusChangedEvent;
		public event EventHandler<ChangedConnectionStateEventArgs> ConnectionStateChangedEvent;
		public event EventHandler<ReceivedDataEventArgs> ReceivedDataEvent;
		public event EventHandler<NotifyChangedJobStatusEventArgs> NotifyJobStatusChangedEvent;
		public event EventHandler<NotifyReceivedJobDataEventArgs> NotifyReceivedJobDataEvent;

		/// Public
		public string StrName { get { return m_strName; } }
		public string StrLocalName { get { return m_strLocalName; } }
		public int TcpPort { get { return m_tcpPort; } }
		public int UdpPort { get { return m_udpPort; } }
		public bool Connected { get { return m_connectionStatus == ClientConnectionStatus.Connected; } }

		/// <summary>
		/// If the client status changes, generate an OnChange event.
		/// </summary>
		public ClientStatus CurrentStatus
		{
			get
			{
				return m_clientStatus;
			}
			set
			{
				ClientStatus oldClientStatus = m_clientStatus;
				m_clientStatus = value;
				this.OnStatusChange(new ChangedStatusEventArgs(oldClientStatus, m_clientStatus));
			}
		}

		protected void OnStatusChange(ChangedStatusEventArgs changedStatusEventArgs)
		{
			if(StatusChangedEvent != null)
			{
				StatusChangedEvent(this, changedStatusEventArgs);
			}
		}

		/// <summary>
		/// If the connection status changes, generate an OnChange event.
		/// </summary>
		public ClientConnectionStatus ConnectionStatus
		{
			get
			{
				return m_connectionStatus;
			}
			set
			{
				ClientConnectionStatus oldStatus = m_connectionStatus;
				m_connectionStatus = value;
				if(ConnectionStateChangedEvent != null)
				{
					ConnectionStateChangedEvent(this, new ChangedConnectionStateEventArgs(oldStatus, m_connectionStatus));
				}
			}
		}
		public IPAddress RemoteIp { get { return m_commsClient == null ? null : m_commsClient.GetIpAddress; } }
		public readonly bool m_bKillOnExit;

		/// Internal
		internal IPEndPoint RemoteIpEndPoint { get { return m_remoteEp as IPEndPoint; } }

		/// Private
		private ClientConnectionStatus m_connectionStatus = ClientConnectionStatus.None;
		private ClientStatus m_clientStatus = ClientStatus.None;
		private static string SERVER_REQUEST_STRING = "Server, Where Are You!";
		private CommunicationsUdp m_commsUdp;
		private CommsClient m_commsClient = null;
		private System.Threading.Timer m_udpTimer;								// Timer used to watch for a Udp response and re-issue Udp request if no response arrives.
		private System.Threading.Timer m_launchTimer;							// Timer used to prevent too many farms being launched.
		private System.Threading.Timer m_keepAlive;								// Used to keep a Tcp connection alive.
		private const int m_keepAliveDelay = 10 * 60 * 1000;					// 10 minutes.

		private string m_strName;
		private string m_strLocalName;
		private readonly int m_tcpPort;
		private readonly int m_udpPort;
		private IPEndPoint m_udpIpEp;
		private EndPoint m_remoteEp = null;
		private readonly IPAddress m_localIpAddress = cGlobal.GetLocalIpAddress();
		private readonly ClientJobRequestQueue m_clientJobRequestQueue;

		/// <summary>
		/// This is the command required to start up the server that this client wants to connect to.
		/// If the client cannot connect then this command is given to the system before a reconnect is tried.
		/// </summary>
		readonly string m_strStartupCommand;
		bool m_bInitialised = false;


		IPAddress m_ipAddr;

		public FarmClient(string strName, string strLocalName, int tcpPort, int udpPort, IPAddress ipAddr, string strStartupCommand, bool bKillOnExit)
		{
			m_strName = strName;
			m_strLocalName = strLocalName;
			m_tcpPort = tcpPort;
			m_udpPort = udpPort;

			m_ipAddr = ipAddr;

			m_strStartupCommand = strStartupCommand;
			m_bKillOnExit = bKillOnExit;

			m_clientJobRequestQueue = new ClientJobRequestQueue(this);
			m_clientJobRequestQueue.NotifyJobStatusChangedEvent += new EventHandler<NotifyChangedJobStatusEventArgs>(ChangedJobStatus);
			m_clientJobRequestQueue.Initialise();
		}

		void ChangedJobStatus(object sender, NotifyChangedJobStatusEventArgs e)
		{
			this.OnJobStatusChange(e.JobStatus, e.JobId, e.JobLuid);
		}

		public void Initialise()
		{
			if(m_bInitialised == false)
			{

				if(m_ipAddr != null)															// If we know where to connect to ...
				{
					ConnectionStatus = ClientConnectionStatus.Connecting;						// This fires an ConnectionStateChangedEvent.
					IPEndPoint remoteEp = new IPEndPoint(m_ipAddr, m_tcpPort);
					m_commsClient = new CommsClient(remoteEp, true, new CommsProtocolJan());
					m_commsClient.ConnectionStateChangedEvent += new ConnectionStateChangedDelegate(m_commsClient_ConnectionChangedEvent);
					m_commsClient.DataReceivedEvent += new DataCallbackDelegate(m_commsClient_DataCallbackEvent);

					m_commsClient.Start();

					this.CurrentStatus = ClientStatus.ChangedIp;
				}
				else																		// Otherwise use Udp to find our server ...
				{
					m_udpIpEp = new IPEndPoint(IPAddress.Broadcast, m_udpPort);
					m_commsUdp = new CommunicationsUdp(m_udpPort);
					m_commsUdp.GotUdpDataEvent += new GotUdpDataDelegate(m_commsUdp_GotUdpDataEvent);
					m_commsUdp.Start();
					ConnectionStatus = ClientConnectionStatus.SearchingForServer;				// This fires a ConnectionStateChangedEvent.
					m_udpTimer = new System.Threading.Timer(UdpTimerCallback, null, 15000, System.Threading.Timeout.Infinite);
					//m_OnChangeCallback("Unknown", Event.ChangedServerStatus, ServerStatusEnum.ChangedIp, this, -1, -1);
					this.CurrentStatus = ClientStatus.ChangedIp;
					
					BroadcastFarmSearch();
				}

				m_keepAlive = new System.Threading.Timer(KeepAliveCallback, null, m_keepAliveDelay, m_keepAliveDelay);

				m_launchTimer = new System.Threading.Timer(LaunchTimerCallback, null, Timeout.Infinite, Timeout.Infinite);

				m_bInitialised = true;
			}
			else
				throw new ApplicationException("Do not call FarmClient.Initialise twice.");
		}

		/// <summary>
		/// Called when a TCP connection is established.
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		void m_commsClient_ConnectionChangedEvent(object sender, TcpConnectionCallbackEventArgs e)
		{
			CommsListener commsBase = e.CommsListener as CommsListener;

			if(commsBase.Connected == true)
			{
				ConnectionStatus = ClientConnectionStatus.Connected;						// This fires a ConnectionStateChangedEvent.

				try
				{
					string strLogonName = System.Security.Principal.WindowsIdentity.GetCurrent().Name.ToString();
					SendTcpString("EXECUTER " + strLogonName);
				}
				catch
				{
					SendTcpString("EXECUTER " + "Unknown user");
				}
			}
			else
			{
				if(this.m_clientStatus != ClientStatus.Deleting)
				{
					ConnectionStatus = ClientConnectionStatus.Connecting;					// This fires a ConnectionStateChangedEvent.
					//
					if((m_strStartupCommand != null) && m_bWaitingForFarm == false)
					{
						m_launchTimer.Change(1000 * 10, Timeout.Infinite);

						m_bWaitingForFarm = true;

						if(m_farmProcess == null || m_farmProcess.HasExited == true)
						{
							// Seperate command line and arguments ...
							string strCommand, strArguments;
							int argsIndex = 0;

							if(m_strStartupCommand.StartsWith("\""))
							{
								argsIndex = m_strStartupCommand.IndexOf("\"", 1);
							}
							else
							{
								argsIndex = m_strStartupCommand.IndexOf(" ", 1);
							}

							if(argsIndex != -1)
							{
								strCommand = m_strStartupCommand.Substring(0, argsIndex);
								strArguments = m_strStartupCommand.Substring(argsIndex + 1);
							}
							else
							{
								strCommand = m_strStartupCommand;
								strArguments = "";
							}

							try
							{
								m_farmProcess = launch_process.LaunchJob(strCommand, strArguments, null, Program.s_bHideLocalFarmWhenLaunching, true);
							}
							catch
							{
							}
						}
					}
				}
			}

			return;
		}

		/// <summary>
		/// Called when we get UDP data on the port for this server.
		/// We must decide whether to connect to the originating farm server.
		/// </summary>
		/// <param name="e.RemoteEndpoint">Where the data came from.</param>
		/// <param name="e.RawData">The data that we received.</param>
		/// <returns></returns>
		void m_commsUdp_GotUdpDataEvent(object sender, GotUdpDataEventArgs e)
		{
			Debug.Assert(sender is CommunicationsUdp);

			IPEndPoint tempIpEp = e.RemoteEndpoint as IPEndPoint;
			m_remoteEp = e.RemoteEndpoint;

			string strData = e.DataAsString;

			// Farmservers will BROADCAST a response to a 'Where are you' request so we must filter out any that we're not interested in.
			// This is achieved by ignoring anything unless we're expecting it.
			if(ConnectionStatus == ClientConnectionStatus.SearchingForServer)
			{
				if(strData == "Client, I'm Over Here!")
				{
					ConnectionStatus = ClientConnectionStatus.Connecting;						// This fires a ConnectionStateChangedEvent.
					//m_OnChangeCallback(RemoteIpEndPoint.Address.ToString(), Event.ChangedServerStatus, ServerStatusEnum.ChangedIp, this, -1, -1);
					this.CurrentStatus = ClientStatus.ChangedIp;
					IPEndPoint ipep = new IPEndPoint(RemoteIpEndPoint.Address, m_tcpPort);
					m_commsClient = new CommsClient(ipep, true, new CommsProtocolJan());
					m_commsClient.ConnectionStateChangedEvent += new ConnectionStateChangedDelegate(m_commsClient_ConnectionChangedEvent);
					m_commsClient.DataReceivedEvent += new DataCallbackDelegate(m_commsClient_DataCallbackEvent);
					m_commsClient.Start();
				}
			}
			if(strData != SERVER_REQUEST_STRING)			// Okay, this is a bit hacky. It is to make sure the string didn't originate from this app.
			{
				this.OnReceivedData(strData, DataType.Udp);
			}
		}

		protected void OnReceivedData(string strData, DataType dataType)
		{
			if(ReceivedDataEvent != null)
			{
				ReceivedDataEvent(this, new ReceivedDataEventArgs(strData, dataType));
			}
		}

		bool m_bWaitingForFarm = false;

		private void LaunchTimerCallback(object state)
		{
			m_bWaitingForFarm = false;
		}

		internal bool SendTcpString(string strData)
		{
			// TODO: Modify SendTcpString so it passes through the originating jobId and jobLuid
			//			m_OnChangeCallback(m_userData, strData, Event.SentString, this, -1, -1);
			if(m_commsClient != null)
			{
				//m_OnChangeCallback(strData, Event.SentData, SentDataEnum.SentString, this, -1, -1);
				return m_commsClient.SendData(strData);
			}
			else
				return false;
		}

		internal void Shutdown()
		{
			if(m_bInitialised == true)
			{
				if(m_commsUdp != null)
				{
					lock(m_commsUdp)
					{
						m_commsUdp.Close();
						m_commsUdp = null;
					}
				}
				if(m_commsClient != null)
				{
					m_commsClient.Close();
					m_commsClient = null;
					//GC.Collect();					// XXX Debug
				}
				m_clientJobRequestQueue.JobRequestQueueThreadStop();
			}
		}

		private bool PokeUdpTimer()
		{
			return m_udpTimer.Change(15000, System.Threading.Timeout.Infinite);
		}

		private void UdpTimerCallback(object state)
		{
			if(m_commsClient == null)					// If there's no TCP connection underway, then the UDP search got lost.
			{
				if(m_commsUdp != null)					// It will be null if it has been shut down.
				{
					BroadcastFarmSearch();

					ConnectionStatus = ClientConnectionStatus.SearchingForServer;

					PokeUdpTimer();						// Timer is one shot, so start it again.
				}
			}
			else
			{
				// Timer is a one shot, so do nothing here.
			}
		}

		private void KeepAliveCallback(object state)
		{
			if(m_commsClient != null) 
			{
				if(m_commsClient.Connected == true)
				{
					Debug.WriteLine("Keepalive sent to :" + this.m_strLocalName);
					m_commsClient.SendData("IDLE");
				}
			}
			else
			{
				Debug.WriteLine("Keepalive not sent to :" + this.m_strLocalName);
			}


		}

		private bool BroadcastFarmSearch()
		{
			lock(m_commsUdp)
			{
				if(m_commsUdp != null)
				{
					return this.m_commsUdp.SendDataString(SERVER_REQUEST_STRING, this.m_udpIpEp);
				}
				else
					return false;
			}
		}

		/// <summary>
		/// This is the process used to launch the local farm.
		/// </summary>
		Process m_farmProcess = null;

		/// <summary>
		/// Called when a TCP connection is established.
		/// </summary>
		/// <param name="cl">This contains the socket we've connected with and various helper functions.</param>
		/// <returns></returns>
		private bool TcpConnectionCallback(object sender, TcpConnectionCallbackEventArgs e)
		{
			throw new ApplicationException("Not implemented.");
		}

		/// <summary>
		/// This is used as a lock to prevent nested calls into TcpDataCallback.
		/// </summary>
		EventWaitHandle ewh = new EventWaitHandle(true, EventResetMode.AutoReset);

		System.Object m_tcpDataCallbackLock = new object();

		/// <summary>
		/// This is called when data comes in on the current connection or when the connection status changes.
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		void m_commsClient_DataCallbackEvent(object sender, TcpDataCallbackEventArgs e)
		{
			ewh.WaitOne();
			
//			Debug.WriteLine(data as string);
			try
			{
				string strData = e.Data as string;

				string[] tokens = new string[3];

				int part1 = -1, part2 = -1;

				part1 = strData.IndexOfAny(new char[] { ' ', '\n', '\t', '\r' }, 0);
				if(part1 == -1)
				{
					part1 = strData.Length;
					tokens[0] = strData;
				}
				else
				{
					tokens[0] = strData.Substring(0, part1);
					part2 = strData.IndexOfAny(new char[] { ' ', '\n', '\t', '\r' }, part1 + 1);
					if(part2 == -1)
					{
						tokens[1] = strData.Substring(part1 + 1);
					}
					else
					{
						tokens[1] = strData.Substring(part1 + 1, part2 - part1 - 1);
						tokens[2] = strData.Substring(part2 + 1);
					}
				}

				if(tokens[0] == "ACCEPTED")
				{
					int jobNumber = TryParseInt(tokens[1], -2);
					if(jobNumber != -2)
					{
						m_clientJobRequestQueue.JobRequestResponseReceived(jobNumber);	// Signal to the job request queue that a job has been received by the farm server.
					}																	// This blocks until the job request queue has processed the job acceptance.
					else
					{
						throw new ApplicationException("Job ACCEPTED - missing jobNumber.");
					}
				}
				else if(tokens[0] == "FAILED")
				{
					if(tokens[1] == null)
					{
						m_clientJobRequestQueue.JobRequestResponseReceived(-1);			// Signal to the job request queue that a job has been denied by the farm server.
					}
					else
					{
						int jobNumber = TryParseInt(tokens[1], -2);
						if(jobNumber != -2)
						{
							this.OnJobStatusChange(JobStatus.Failed, jobNumber, -1);
							return;
						}
						else
						{
							throw new ApplicationException("Job FAILED - bad jobNumber.");
						}
					}
				}
				else if(tokens[0] == "STARTED")
				{
					int jobNumber = TryParseInt(tokens[1], -2);
					if(jobNumber != -2)
					{
						this.OnJobStatusChange(JobStatus.Created, jobNumber, -1);

						return;
					}
					else
					{
						throw new ApplicationException("Job STARTED - missing jobNumber.");
					}
				}
				else if(tokens[0] == "COMPLETED")
				{
					int jobNumber = TryParseInt(tokens[1], -2);
					if(jobNumber != -2)
					{
						this.OnJobStatusChange(JobStatus.Finished, jobNumber, -1);
						return;
					}
					else
					{
						throw new ApplicationException("Job COMPLETED - missing jobNumber.");
					}
				}
				else if(tokens[0] == "BADJOB")
				{
					int jobNumber = TryParseInt(tokens[1], -2);
					if(jobNumber != -2)
					{
						this.OnJobStatusChange(JobStatus.Abandoned, jobNumber, -1);

						return;
					}
					else
					{
						throw new ApplicationException("Job BADJOB - missing jobNumber.");
					}
				}
				else if(tokens[0] == "CANCELLED")
				{
					int jobNumber = TryParseInt(tokens[1], -2);
					if(jobNumber != -2)
					{
						this.OnJobStatusChange(JobStatus.Killed, jobNumber, -1);

						return;
					}
					else
					{
						throw new ApplicationException("Job CANCELLED - missing jobNumber.");
					}
				}
				else if(tokens[0] == "JOBSTATUS")
				{
					// TODO: This will become interesting when we want Executer to reconnect to existing jobs.
					//return true;
					return;
				}
				else if(tokens[0] == "EXECUTER")
				{
					// Here is some data for a specified job.
					try
					{
						int jobNumber = TryParseInt(tokens[1], -2);
						if(jobNumber != -2)
						{
							// EXECUTER		<JobNumber>	<string>
							// tokens[0]	tokens[1]	tokens[2]
							this.OnReceivedJobData(jobNumber, tokens[2] as string);

							return;
						}
						else
						{
							throw new ApplicationException("Job EXECUTER - missing jobNumber.");
						}
					}
					catch(Exception)
					{
					}
				}
				else
				{
					Debug.WriteLine("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
					Debug.WriteLine("Unrecognised data on Tcp channel in server '" + this.StrLocalName + "':");
					Debug.WriteLine(strData);
					Debug.WriteLine("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
				}
				return;
			}
			finally
			{
				ewh.Set();
			}
		}

		protected void OnReceivedJobData(int jobId, string strJobData)
		{
			if(NotifyReceivedJobDataEvent != null)
			{
				NotifyReceivedJobDataEvent(this, new NotifyReceivedJobDataEventArgs(jobId, strJobData));
			}
		}

		private void OnJobStatusChange(JobStatus jobStatus, int jobId, long jobLuid)
		{
			if(NotifyJobStatusChangedEvent != null)
			{
				NotifyJobStatusChangedEvent(this, new NotifyChangedJobStatusEventArgs(jobStatus, jobId, jobLuid));
			}
		}

		private int TryParseInt(string strToken, int defaultValue)
		{
			int jobNumber;

			if(int.TryParse(strToken, out jobNumber) == false)
			{
				jobNumber = defaultValue;
			}
			return jobNumber;
		}


		internal bool RequestJob(Job job)
		{
			return m_clientJobRequestQueue.RequestJob(job);
		}

		internal int GetNumberOfQueuedJobs()
		{
			return m_clientJobRequestQueue.GetNumberOfQueuedJobs();
		}
	}
}
