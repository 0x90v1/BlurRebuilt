using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using System.Runtime.Remoting.Channels.Ipc;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting;
using System.Diagnostics;
using message_handler;
using System.Threading;

namespace Executer
{
	internal class RemoteObject : MarshalByRefObject
	{
		/// <summary>
		/// A remote object is created in the primary instance of Executer when a second instance of Executer tries to communicate with the primary.
		/// </summary>
		private RemoteObject()
		{
			Console.WriteLine("Remote object activated");
		}


		//////////////////////////////////////////////////////////////////////////////
		///return message reply
		public bool ReplyMessage(String[] args)
		{
			bool retval = RemotingService.GetRemotingService().EnqueueArgs(args);
			//if(retval == false)
			//    StopRemotingServer();
			return retval;
		}

		public string InstanceGetBuildTime()
		{
			return GetBuildTime();
		}

		public static string GetBuildTime()
		{
			return BuildTime.Date + "\n" + BuildTime.Time;
		}

		#region Remoting

		private static IpcChannel _channel;

		/// <summary>
		/// Set up a server to which a second instance of this application can attach.
		/// </summary>
		/// <returns></returns>
		public static bool SetupRemotingServer()
		{
			// TODO: RetryTime
			int retryCount = Program.REMOTING_SERVER_RETRY_TIMEOUT;

			do
			{
				try
				{
					Debug.Assert(_channel == null);
					_channel = new IpcChannel("Server"); // Select channel on which to communicate.
				}
				catch(Exception)
				{
					if(retryCount-- == 0)
						return false;
					else
					{
						System.Threading.Thread.Sleep(1000*1);
					}
					MessageBox.Show("Fail");
				}
			} while(_channel == null);

			ChannelServices.RegisterChannel(_channel, false); // Register channel.
			// Register remote object.
			try
			{
				//RemotingConfiguration.RegisterWellKnownServiceType(typeof(RemoteObject), "RemotingServer_Fist_zz9zza", WellKnownObjectMode.Singleton);
				RemotingConfiguration.RegisterWellKnownServiceType(typeof(RemoteObject), "RemotingServer_Fist_zz9zza", WellKnownObjectMode.SingleCall);
			}
			catch(Exception ex)
			{
				MessageHandler.Show(null, "Error:\n" + ex.Message);
				return false;
			}
			return true;
		}

		public static void StopRemotingServer()
		{
			_channel.StopListening(null);
			ChannelServices.UnregisterChannel(_channel);
			_channel = null;
			return;
		}

		#endregion Remoting

		internal static void Start()
		{
			RemotingService.Start();
		}
	}
}