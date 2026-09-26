using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Windows.Forms;
using System.Threading;
using System.Runtime.Remoting;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting.Channels.Ipc;
using System.Reflection;
using message_handler;
using MiniDump;

namespace Executer
{
	internal enum StateMachine
	{
		Starting = 0,
		GetBusyState,
		Finished,
		TryGetPrimaryInstanceMutex,
		GotPrimaryInstance,
		TryGetSecondaryInstance,
		IsSecondary,
		RemoteObjectNotListening,
		RemoteObjectNotPresent,
		MustGetPrimaryInstanceMutex
	} ;

	internal enum StateMachineValue
	{
		Nothing = 0,
		PrimaryInstance,
		BailOut,
		FailedToGetSecondInstance,
		FailedToTakeOverPrimaryInstance,
		Finished,
		VersionMismatch
	}


	internal static class Program
	{
		private static StateMachine machineState;
		private static StateMachineValue stateMachineValue;
		private static string[] _args;

		private const int GET_BUSY_MUTEX__TIMEOUT			= 1000*60*3;
		public const int REMOTING_SERVER_RETRY_TIMEOUT		= 50;			// Units of 1 second.
		private const int GET_SECONDARY_MUTEX__TIMEOUT		= 1000*60*2;
		private const int GET_PRIMARY_MUTEX__TIMEOUT		= 1000*30;

		public static JobManager s_jobManager;
		public static FarmClientManager s_farmClientManager; // TODO: This is still instantiated in the main form. Bad.

		public static JobViewer s_jobViewer;

		public static bool m_sSilentCommandLine = false;

		/// <summary>
		/// s_ExitCode is set to 1 if a program update was requested.
		/// </summary>
		public static int s_ExitCode = 0;

		public static bool s_bDebugModeEnabled = false;
		public static bool s_bAdvancedModeEnabled = false;
		public static bool s_bHideLocalFarmWhenLaunching = false;
		public static bool s_bValidateMenuTemplates = false;

		/// <summary>
		/// A static instance of the main Executer form.
		/// </summary>
		public static ExecuterMainForm ExecuterMainForm { get; private set; }

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		private static int Main(string[] args)
		{
			_args = args;
			bool bPauseStartup = false;

			// Look for the silent flag ...
			// TODO: Look for other global flags.
			foreach(string strArg in args)
			{
				if(strArg == "-pause")
					bPauseStartup = true;

				if(strArg == "-silent")
				{
					m_sSilentCommandLine = true;
					break;
				}
			}

			if(bPauseStartup == true)
				MessageBox.Show(null, "Paused", "Executer", MessageBoxButtons.OK, MessageBoxIcon.Stop);


			if(Program.m_sSilentCommandLine == true)
			{
				MessageHandler.MemoryRedirect = MessageHandler.MessageRedirect.ToLogfile;
				MessageHandler.DialogRedirect = MessageHandler.MessageRedirect.ToLogfile;
			}

			machineState = StateMachine.Starting;

			while(machineState != StateMachine.Finished)
			{
				RunStateMachine();
			}

			switch(stateMachineValue)
			{
					//case StateMachineValue.Nothing:
					//	mutexBusyInstance.ReleaseMutex();
					//    break;
				case StateMachineValue.PrimaryInstance:
					bool bRemotingServerStarted = RemoteObject.SetupRemotingServer();
					RemotingService.GetRemotingService().EnqueueArgs(args); // These will be first in the queue. Guaranteed by the use of mutexBusyInstance.

					mutexBusyInstance.ReleaseMutex();
					if(bRemotingServerStarted == true)
					{
						RunNewExecuter(_args);
						RemoteObject.StopRemotingServer();
					}
					mutexPrimaryInstance.ReleaseMutex();
					break;
					//case StateMachineValue.BailOut:
					//	mutexBusyInstance.ReleaseMutex();
					//    break;
					//case StateMachineValue.FailedToGetSecondInstance:
					//	mutexBusyInstance.ReleaseMutex();
					//    break;
					//case StateMachineValue.FailedToTakeOverPrimaryInstance:
					//	mutexBusyInstance.ReleaseMutex();
					//    break;
					//case StateMachineValue.VersionMismatch:
					//	mutexBusyInstance.ReleaseMutex();
					//	break;
				default:
					mutexBusyInstance.ReleaseMutex();
					break;
			}
			return s_ExitCode;


			//catch(Exception ex)
			//        {
			//            Console.WriteLine(ex);
			//            MessageBox.Show(null, "Exception. System reportsy:\n'" + ex.Message + "'", "Executer", MessageBoxButtons.OK, MessageBoxIcon.Error);
			//            //string dumpFile =
			//            //    System.IO.Path.Combine(System.Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments), "dump.dmp");
			//            //MiniDumper.Write(dumpFile);
			//        }
			//        return s_ExitCode;
		}


		private static Mutex mutexBusyInstance;
		private static Mutex mutexPrimaryInstance;
		private static Mutex mutexSecondaryInstance;

		private static void RunStateMachine()
		{
			switch(machineState)
			{
				case StateMachine.Starting:
					ChangeState(StateMachine.GetBusyState, StateMachineValue.Nothing);
					break;


				case StateMachine.GetBusyState:
					// Acquire 'busy' mutex.
					mutexBusyInstance = new Mutex(false, "Executer busy mutex");
					{
// TODO: RetryTime
						if(mutexBusyInstance.WaitOne(GET_BUSY_MUTEX__TIMEOUT) == false)
						{
							MessageHandler.Show(null, "Cannot acquire busy status. Bailing out", "Executer - ERROR", MessageBoxButtons.OK, MessageBoxIcon.Error);

							ChangeState(StateMachine.Finished, StateMachineValue.BailOut);
						}
						ChangeState(StateMachine.TryGetPrimaryInstanceMutex, StateMachineValue.Nothing);
					}
					break;


				case StateMachine.TryGetPrimaryInstanceMutex:
					mutexPrimaryInstance = new Mutex(false, "Executer primary mutex");

					if(mutexPrimaryInstance.WaitOne(0) == true) // If we can own the mutex then we are prime ...
					{
						ChangeState(StateMachine.GotPrimaryInstance, StateMachineValue.Nothing);
					}
					else
					{
						ChangeState(StateMachine.TryGetSecondaryInstance, StateMachineValue.Nothing);
					}
					break;


				case StateMachine.GotPrimaryInstance:
					ChangeState(StateMachine.Finished, StateMachineValue.PrimaryInstance);
					break;

				case StateMachine.TryGetSecondaryInstance:
					// Get secondary mutex or quit.
					mutexSecondaryInstance = new Mutex(false, "Executer secondary mutex");
					// TODO:RetryTime
					if(mutexSecondaryInstance.WaitOne(GET_SECONDARY_MUTEX__TIMEOUT) == false) // If timeout ... (i.e. a secondary instance of Executer is already running and does not relinquish the mutex, 
					{
						// Or too many instances are running and the mutex doesn't have time to get to this instance.
						Debug.WriteLine("  Secondary mutex timeout. Bailing out.");
						MessageHandler.Show(null, "Secondary mutex timeout. Bailing out.", "Executer - ERROR", MessageBoxButtons.OK, MessageBoxIcon.Error);

						ChangeState(StateMachine.Finished, StateMachineValue.FailedToGetSecondInstance);
					}
					else
					{
						ChangeState(StateMachine.IsSecondary, StateMachineValue.Nothing);
					}
					break;


				case StateMachine.IsSecondary:
					// IsSecondary.
					//Send a message.
					// Note: No messages are sent over the network until a method is called on the proxy. 
					RemoteObject remObject = (RemoteObject)Activator.GetObject(typeof(RemoteObject), "ipc://Server/RemotingServer_Fist_zz9zza");

					if((!(_args.Length == 1 && _args[0] == "-a")) && RemoteObject.GetBuildTime() != remObject.InstanceGetBuildTime())
					{
						MessageHandler.Show(null, "You cannot launch Executer when a different version of Executer is already running.\n" + "You must close the existing version of Executer to launch this version.", "Executer", MessageBoxButtons.OK, MessageBoxIcon.Error);
						ChangeState(StateMachine.Finished, StateMachineValue.VersionMismatch);
					}
					else
					{
						try
						{
							if(remObject.ReplyMessage(_args) == false) // If primary instance is present but not listening ...
							{
								ChangeState(StateMachine.RemoteObjectNotListening, StateMachineValue.Nothing);
							}
							else
							{
								mutexSecondaryInstance.ReleaseMutex();
								ChangeState(StateMachine.Finished, StateMachineValue.Finished);
							}
						}
						catch(Exception)
						{
							MessageHandler.Show(null, "Primary is dead or dying or has not yet started up its Remoting Server.", "Executer - ERROR", MessageBoxButtons.OK, MessageBoxIcon.Error);

							ChangeState(StateMachine.RemoteObjectNotPresent, StateMachineValue.Nothing);
						}
					}
					break;


				case StateMachine.RemoteObjectNotListening:
					// IsSecondary.
					// Primary is shutting down, or has by now shut down.
					// Remember we have mutexBusyInstance; Nothing can grab mutexPrimaryInstance yet. (Except this thread.)
					ChangeState(StateMachine.MustGetPrimaryInstanceMutex, StateMachineValue.Nothing);
					break;


				case StateMachine.RemoteObjectNotPresent:
					// IsSecondary.
					// Primary is gone or starting up.
					// Remember we have mutexBusyInstance; Nothing can grab mutexPrimaryInstance yet. (Except this thread.)
					ChangeState(StateMachine.MustGetPrimaryInstanceMutex, StateMachineValue.Nothing);
					break;


				case StateMachine.MustGetPrimaryInstanceMutex:
					// IsSecondary.
					// TODO: RetryTime
					if(mutexPrimaryInstance.WaitOne(GET_PRIMARY_MUTEX__TIMEOUT) == true) // Try and become the primary ...
					{
						// IsPrimary.
						// IsSecondary.
						// IsBusy.
						// It is safe to release the mutexSecondaryInstance here because we have mutexBusyInstance.
						mutexSecondaryInstance.ReleaseMutex();
						ChangeState(StateMachine.Finished, StateMachineValue.PrimaryInstance);
					}
					else
					{
						// IsSecondary.
						// IsBusy.
						MessageHandler.Show(null, "Failed to become primary instance. Bailing out.", "Executer - ERROR", MessageBoxButtons.OK, MessageBoxIcon.Error);

						// It is safe to release the mutexSecondaryInstance here because we have mutexBusyInstance.
						mutexSecondaryInstance.ReleaseMutex();
						ChangeState(StateMachine.Finished, StateMachineValue.FailedToTakeOverPrimaryInstance);
					}
					break;


				case StateMachine.Finished:
					break;

				default:
					throw (new ApplicationException("Bad state."));
			}
		}


		private static void ChangeState(StateMachine stateMachine, StateMachineValue stateValue)
		{
			machineState = stateMachine;
			stateMachineValue = stateValue;
		}

		private static bool bTest = false;

		private static void RunNewExecuter(string[] args)
		{
			Debug.Assert(bTest == false);
			bTest = true;

			try
			{
				Application.EnableVisualStyles();
				Application.SetCompatibleTextRenderingDefault(false);

				s_farmClientManager = new FarmClientManager(true); // s_jobManager requires s_farmClientManager.
				s_jobManager = new JobManager( /*true*/);
				// This is the glue between everything, so we need to set all that up ...

				ExecuterMainForm = new ExecuterMainForm( /*args*/);

				ExecuterMainForm.Load += new EventHandler(s_executerMainForm_Load);

				ExecuterMainForm.m_args = args;

				s_jobViewer = new JobViewer(); // Create this before processing any command line arguments.
				ExecuterMainForm.Initialise(args);
				if(ExecuterMainForm.IsDisposed == false)
					s_jobViewer.Owner = ExecuterMainForm;

				if(ExecuterMainForm.m_bApplicationHasStartedSuccesfully)
				{
					if(Program.m_sSilentCommandLine == true)
					{
						MessageHandler.MemoryRedirect = MessageHandler.MessageRedirect.None;
						MessageHandler.DialogRedirect = MessageHandler.MessageRedirect.None;
					}

					Application.Run(ExecuterMainForm);
				}
				//RemoteObject.StopRemotingServer();
			}
			catch(Exception ex)
			{
				MessageHandler.Show(null, "Cannot launch Executer (In Program.RunNewExecuter). System reports:'"+ex.Message+"'", "Executer - ERROR", MessageBoxButtons.OK, MessageBoxIcon.Error);
			}
		}

		/// <summary>
		/// This is called when the main form loads and is used to signal to the RemoteObject that the arguments queue can start up.
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private static void s_executerMainForm_Load(object sender, EventArgs e)
		{
			RemoteObject.Start();
		}

		private static void ShowError(IWin32Window owner, string strError)
		{
			MessageHandler.Show(owner, strError, "Executer - ERROR", MessageBoxButtons.OK, MessageBoxIcon.Error);
		}


		public static string AssemblyTitle
		{
			get
			{
				// Get all Title attributes on this assembly
				object[] attributes = Assembly.GetExecutingAssembly().GetCustomAttributes(typeof(AssemblyTitleAttribute), false);
				// If there is at least one Title attribute
				if(attributes.Length > 0)
				{
					// Select the first one
					AssemblyTitleAttribute titleAttribute = (AssemblyTitleAttribute)attributes[0];
					// If it is not an empty string, return it
					if(titleAttribute.Title != "")
						return titleAttribute.Title;
				}
				// If there was no Title attribute, or if the Title attribute was the empty string, return the .exe name
				return System.IO.Path.GetFileNameWithoutExtension(Assembly.GetExecutingAssembly().CodeBase);
			}
		}
	}
}