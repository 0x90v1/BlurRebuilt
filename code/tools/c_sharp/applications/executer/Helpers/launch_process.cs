using System;
using System.Diagnostics;
using System.IO;

namespace Executer.Helpers
{
	class LaunchProcess
	{
		#region Do this to launch the job directly ...




		static public Process LaunchJob(string strCommandLine, string strWorkingDirectory, bool bHideWindowFromUser, bool bStartMinimised, EventHandler completedEvent)
		{
			// Seperate command line and arguments ...
			string strCommand, strArguments;
			int argsIndex = 0;

			if(strCommandLine.StartsWith("\""))
			{
				argsIndex = strCommandLine.IndexOf("\"", 1);
			}
			else
			{
				argsIndex = strCommandLine.IndexOf(" ", 1);
			}

			if(argsIndex != -1)
			{
				strCommand = strCommandLine.Substring(0, argsIndex);
				strArguments = strCommandLine.Substring(argsIndex + 1);
			}
			else
			{
				strCommand = strCommandLine;
				strArguments = "";
			}

			try
			{
				return LaunchProcess.LaunchJob(strCommand, strArguments, strWorkingDirectory, bHideWindowFromUser, bStartMinimised, completedEvent);
			}
			catch(Exception ex)
			{
				return null;
			}
		}









		static public Process LaunchJob(string strCommand, string strClArguments, string strWorkingDirectory, bool bHideWindowFromUser, bool bStartMinimised, EventHandler completedEvent)
		{
			try
			{
				Process p = new Process();

				p.StartInfo = new ProcessStartInfo(strCommand, strClArguments);

				if(strWorkingDirectory != null)
				{
					p.StartInfo.WorkingDirectory = strWorkingDirectory;
				}
				else
				{
					p.StartInfo.WorkingDirectory = Path.GetDirectoryName(strCommand);
				}

				// Allow the window to be hidden so that it run's without being seen
				if(bHideWindowFromUser)
				{
					p.StartInfo.WindowStyle = ProcessWindowStyle.Hidden;
				}
				else if(bStartMinimised == true)
				{
					p.StartInfo.WindowStyle = ProcessWindowStyle.Minimized;
				}

				if(completedEvent != null)
					p.Exited += completedEvent;

				p.EnableRaisingEvents = true;
				p.Start();
				return p;
			}
			catch(Exception ex)
			{
				throw (ex);
			}
		}

		#endregion Do this to launch the job directly ...
	}
}