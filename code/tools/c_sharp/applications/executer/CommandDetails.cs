using System;
using System.Collections.Generic;
using System.Text;
using Executer.TreeObjects;

namespace Executer
{
	/// <summary>
	/// This is intermediate data that is needed to construct a job request.
	/// </summary>
	public class CommandDetails
	{
		/// <summary>
		/// This is intermediate data that is needed to construct a job request.
		/// </summary>
		/// <param name="strServer">The name of the server on which the task is to run.</param>
		/// <param name="bRecurse"></param>
		/// <param name="tuo"></param>
		/// <param name="strTaskName"></param>
		public CommandDetails(string strServer, bool bRecurse, TreeUserObject tuo, string strTaskName)
		{
			m_strServerName = strServer;
			m_bRecurse = bRecurse;
			m_treeUserObject = tuo;
			m_strTaskName = strTaskName;
		}

		public string m_strServerName = null;
		public bool m_bRecurse = false;
		public TreeUserObject m_treeUserObject = null;
		public string m_strTaskName = null;
	}
}
