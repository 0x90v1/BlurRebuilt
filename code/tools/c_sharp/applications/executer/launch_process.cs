using System;
using System.Diagnostics;
using System.IO;

namespace Executer
{
	// TODO: Move this to a new namespace and library.
	class launch_process
	{
		#region Do this to launch the job directly ...

		static public Process LaunchJob(string strCommand, string strClArguments, string strWorkingDirectory, bool bHideWindowFromUser, bool bStartMinimised)
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
