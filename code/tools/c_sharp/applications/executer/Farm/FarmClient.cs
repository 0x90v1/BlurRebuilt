using System;
using comms_tcp;
using System.Net;
using System.Diagnostics;
using System.Threading;
using communications_udp;
using Executer.Helpers;
using Executer.JobManagement;

namespace Executer.Farm
{
	public class FarmClient
	{
		public event EventHandler<ChangedStatusEventArgs> StatusChangedEvent;
		public event EventHandler<ChangedConnectionStateEventArgs> ConnectionStateChangedEvent;
		public event EventHandler<ReceivedDataEventArgs> ReceivedDataEvent;
		public event EventHandler<NotifyChangedJobStatusEventArgs> NotifyJobStatusChangedEvent;
		public event EventHandler<NotifyReceivedJobDataEventArgs> NotifyReceivedJobDataEvent;

		/// Public
		public string StrName { get; private set; }

		public string StrLocalName { get; private set; }
		public int TcpPort { get; private set; }
		public int UdpPort { get; private set; }
		public bool Connected { get { return _connectionStatus == ClientConnectionStatus.Connected; } }

		/// <summary>
		/// If the client status changes, generate an OnChange event.
		/// </summary>
		public ClientStatus CurrentStatus
		{
			get
			{
				return _clientStatus;
			}
			set
			{
				ClientStatus oldClientStatus = _clientStatus;
				_clientStatus = value;
				this.OnStatusChange(new ChangedStatusEventArgs(oldClientStatus, _clientStatus));
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
				return _connectionStatus;
			}
			set
			{
				ClientConnectionStatus oldStatus = _connectionStatus;
				_connectionStatus = value;
				if(ConnectionStateChangedEvent != null)
				{
					ConnectionStateChangedEvent(this, new ChangedConnectionStateEventArgs(oldStatus, _connectionStatus));
				}
			}
		}
		public IPAddress RemoteIp { get { return _commsClient == null ? null : _commsClient.GetIpAddress; } }
		public readonly bool m_bKillOnExit;

		/// Internal
		internal IPEndPoint RemoteIpEndPoint { get { return _remoteEp as IPEndPoint; } }

		/// Private
		private ClientConnectionStatus _connectionStatus = ClientConnectionStatus.None;
		private ClientStatus _clientStatus = ClientStatus.None;
		private const string SERVER_REQUEST_STRING = "Server, Where Are You!";
		private CommunicationsUdp _commsUdp;
		private CommsClient _commsClient = null;
		private System.Threading.Timer _udpTimer;								// Timer used to watch for a Udp response and re-issue Udp request if no response arrives.
		private System.Threading.Timer _launchTimer;							// Timer used to prevent too many farms being launched.
		private System.Threading.Timer _keepAlive;								// Used to keep a Tcp connection alive.
		private const int _keepAliveDelay = 10 * 60 * 1000;					// 10 minutes.

		private IPEndPoint _udpIpEp;
		private EndPoint _remoteEp = null;
		private readonly IPAddress _localIpAddress = cGlobal.GetLocalIpAddress();
		private readonly ClientJobRequestQueue _clientJobRequestQueue;

		/// <summary>
		/// This is the command required to start up the server that this client wants to connect to.
		/// If the client cannot connect then this command is given to the system before a reconnect is tried.
		/// </summary>
		readonly string _strStartupCommand;
		bool _bInitialised = false;


		IPAddress _ipAddr;

		public FarmClient(string strName, string strLocalName, int tcpPort, int udpPort, IPAddress ipAddr, string strStartupCommand, bool bKillOnExit)
		{
			StrName = strName;
			StrLocalName = strLocalName;
			TcpPort = tcpPort;
			UdpPort = udpPort;

			_ipAddr = ipAddr;

			_strStartupCommand = strStartupCommand;
			m_bKillOnExit = bKillOnExit;

			_clientJobRequestQueue = new ClientJobRequestQueue(this);
			_clientJobRequestQueue.NotifyJobStatusChangedEvent += new EventHandler<NotifyChangedJobStatusEventArgs>(ChangedJobStatus);
			_clientJobRequestQueue.Initialise();
		}

		void ChangedJobStatus(object sender, NotifyChangedJobStatusEventArgs e)
		{
			this.OnJobStatusChange(e.JobStatus, e.JobId, e.JobLuid);
		}

		public void Initialise()
		{
			if(_bInitialised == false)
			{

				if(_ipAddr != null)															// If we know where to connect to ...
				{
					ConnectionStatus = ClientConnectionStatus.Connecting;						// This fires an ConnectionStateChangedEvent.
					IPEndPoint remoteEp = new IPEndPoint(_ipAddr, TcpPort);
					_commsClient = new CommsClient(remoteEp, true, new CommsProtocolJan());
					_commsClient.ConnectionStateChangedEvent += new ConnectionStateChangedDelegate(m_commsClient_ConnectionChangedEvent);
					_commsClient.DataReceivedEvent += new DataCallbackDelegate(m_commsClient_DataCallbackEvent);

					_commsClient.Start();

					this.CurrentStatus = ClientStatus.ChangedIp;
				}
				else																		// Otherwise use Udp to find our server ...
				{
					_udpIpEp = new IPEndPoint(IPAddress.Broadcast, UdpPort);
					_commsUdp = new CommunicationsUdp(UdpPort);
					_commsUdp.GotUdpDataEvent += new GotUdpDataDelegate(m_commsUdp_GotUdpDataEvent);
					_commsUdp.Start();
					ConnectionStatus = ClientConnectionStatus.SearchingForServer;				// This fires a ConnectionStateChangedEvent.
					_udpTimer = new System.Threading.Timer(UdpTimerCallback, null, 15000, System.Threading.Timeout.Infinite);
					//m_OnChangeCallback("Unknown", Event.ChangedServerStatus, ServerStatusEnum.ChangedIp, this, -1, -1);
					this.CurrentStatus = ClientStatus.ChangedIp;

					BroadcastFarmSearch();
				}

				_keepAlive = new System.Threading.Timer(KeepAliveCallback, null, _keepAliveDelay, _keepAliveDelay);

				_launchTimer = new System.Threading.Timer(LaunchTimerCallback, null, Timeout.Infinite, Timeout.Infinite);

				_bInitialised = true;
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
				if(this._clientStatus != ClientStatus.Deleting)
				{
					ConnectionStatus = ClientConnectionStatus.Connecting;					// This fires a ConnectionStateChangedEvent.
					//
					if((_strStartupCommand != null) && m_bWaitingForFarm == false)
					{
						_launchTimer.Change(1000 * 10, Timeout.Infinite);

						m_bWaitingForFarm = true;

						if(m_farmProcess == null || m_farmProcess.HasExited == true)
						{
							// Seperate command line and arguments ...
							string strCommand, strArguments;
							int argsIndex = 0;

							if(_strStartupCommand.StartsWith("\""))
							{
								argsIndex = _strStartupCommand.IndexOf("\"", 1);
							}
							else
							{
								argsIndex = _strStartupCommand.IndexOf(" ", 1);
							}

							if(argsIndex != -1)
							{
								strCommand = _strStartupCommand.Substring(0, argsIndex);
								strArguments = _strStartupCommand.Substring(argsIndex + 1);
							}
							else
							{
								strCommand = _strStartupCommand;
								strArguments = "";
							}

							try
							{
								m_farmProcess = LaunchProcess.LaunchJob(strCommand, strArguments, null, Program.s_bHideLocalFarmWhenLaunching, true, null);
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
			_remoteEp = e.RemoteEndpoint;

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
					IPEndPoint ipep = new IPEndPoint(RemoteIpEndPoint.Address, TcpPort);
					_commsClient = new CommsClient(ipep, true, new CommsProtocolJan());
					_commsClient.ConnectionStateChangedEvent += new ConnectionStateChangedDelegate(m_commsClient_ConnectionChangedEvent);
					_commsClient.DataReceivedEvent += new DataCallbackDelegate(m_commsClient_DataCallbackEvent);
					_commsClient.Start();
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
			if(_commsClient != null)
			{
				//m_OnChangeCallback(strData, Event.SentData, SentDataEnum.SentString, this, -1, -1);
				return _commsClient.SendData(strData);
			}
			else
				return false;
		}

		internal void Shutdown()
		{
			if(_bInitialised == true)
			{
				if(_commsUdp != null)
				{
					lock(_commsUdp)
					{
						_commsUdp.Close();
						_commsUdp = null;
					}
				}
				if(_commsClient != null)
				{
					_commsClient.Close();
					_commsClient = null;
					//GC.Collect();					// XXX Debug
				}
				_clientJobRequestQueue.JobRequestQueueThreadStop();
			}
		}

		private bool PokeUdpTimer()
		{
			return _udpTimer.Change(15000, System.Threading.Timeout.Infinite);
		}

		private void UdpTimerCallback(object state)
		{
			if(_commsClient == null)					// If there's no TCP connection underway, then the UDP search got lost.
			{
				if(_commsUdp != null)					// It will be null if it has been shut down.
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
			if(_commsClient != null)
			{
				if(_commsClient.Connected == true)
				{
					Debug.WriteLine("Keepalive sent to :" + this.StrLocalName);
					_commsClient.SendData("IDLE");
				}
			}
			else
			{
				Debug.WriteLine("Keepalive not sent to :" + this.StrLocalName);
			}


		}

		private bool BroadcastFarmSearch()
		{
			lock(_commsUdp)
			{
				if(_commsUdp != null)
				{
					return this._commsUdp.SendDataString(SERVER_REQUEST_STRING, this._udpIpEp);
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
		/// <param name="sender"></param>
		/// <param name="e"></param>
		/// <returns></returns>
		private bool TcpConnectionCallback(object sender, TcpConnectionCallbackEventArgs e)
		{
			throw new ApplicationException("Not implemented.");
		}

		/// <summary>
		/// This is used as a lock to prevent nested calls into TcpDataCallback.
		/// </summary>
		readonly EventWaitHandle _ewh = new EventWaitHandle(true, EventResetMode.AutoReset);

		/// <summary>
		/// This is called when data comes in on the current connection or when the connection status changes.
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		void m_commsClient_DataCallbackEvent(object sender, TcpDataCallbackEventArgs e)
		{
			_ewh.WaitOne();

			//Debug.WriteLine(data as string);
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
						_clientJobRequestQueue.JobRequestResponseReceived(jobNumber);	// Signal to the job request queue that a job has been received by the farm server.
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
						_clientJobRequestQueue.JobRequestResponseReceived(-1);			// Signal to the job request queue that a job has been denied by the farm server.
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
				_ewh.Set();
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
			return _clientJobRequestQueue.RequestJob(job);
		}

		internal int GetNumberOfQueuedJobs()
		{
			return _clientJobRequestQueue.GetNumberOfQueuedJobs();
		}
	}
}