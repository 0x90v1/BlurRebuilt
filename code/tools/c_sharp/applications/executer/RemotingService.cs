using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading;

namespace Executer
{
	internal class RemotingService
	{
		private readonly object _messageLock = new object();
		private readonly object _queueLock = new object();

		private readonly Queue<string[]> _argsQueue = new Queue<string[]>();
		private static readonly AutoResetEvent QueueStartTrigger = new AutoResetEvent(false);
		private readonly AutoResetEvent _queueTrigger;
		private bool _bIsAborting = false;

		private static readonly object SingletonLock = new object();
		private static RemotingService _singletonInstance = null;

		private RemotingService()
		{
			_queueTrigger = new AutoResetEvent(true);

			Thread argsThread = new Thread(new ThreadStart(QueueThread)) { Name = "Remoting Thread", Priority = ThreadPriority.Normal, IsBackground = true };
			argsThread.Start();
		}

		public static RemotingService GetRemotingService()
		{
			if(_singletonInstance == null)
			{
				lock(SingletonLock)
				{
					if(_singletonInstance == null)
					{
						_singletonInstance = new RemotingService();
					}
				}
			}
			return _singletonInstance;
		}


		private void QueueThread()
		{
			QueueStartTrigger.WaitOne(); // This is set by Executer after the main form has loaded. (By calling 'Start')

			//Debug.Assert(Executer.Program.ExecuterMainForm._DelegateProcessCommandLine != null);

			while(true)
			{
				_queueTrigger.WaitOne();

				lock(_queueLock)
				{
					while(_argsQueue.Count > 0)
					{
						string[] args = _argsQueue.Dequeue();

						// TODO: Remove this dependency.
						Executer.Program.ExecuterMainForm.Invoke(new Executer.ExecuterMainForm.DelegateProcessCommandLine(CommandLineInterface.ProcessCommandLine), new Object[] { Executer.Program.ExecuterMainForm.MainTreeView, args });
					}
				}
			}
		}

		//private static bool DoCommandLine(ExecuterTreeView executerTreeView, String[] args)
		//{
		//    return CommandLineInterface.ProcessCommandLine(executerTreeView, args);
		//}

		public bool EnqueueArgs(string[] args)
		{
			if(_bIsAborting == true)
				return false;

			foreach(string str in args)
			{
				if(str == "-a")
					_bIsAborting = true;
			}

			lock(_messageLock)
			{
				lock(_queueLock)
				{
					_argsQueue.Enqueue(args);
					Debug.WriteLine("queued something." + args.ToString());
				}
				_queueTrigger.Set();
			}
			return true;
		}

		public static void Start()
		{
			QueueStartTrigger.Set();
		}
	}
}