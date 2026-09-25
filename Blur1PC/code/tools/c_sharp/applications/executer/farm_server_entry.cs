using System;
using System.Collections.Generic;
using System.Text;
using System.Net;

namespace Executer
{
	class FarmServerEntry
	{
		ServerEntry m_serverEntry;
		ServerJobRequestQueue m_serverJobRequestQueue;

		FarmServerEntry(string strName, string strLocalName, int tcpPort, int udpPort, ServerEntry.OnChangeCallbackDelegate callback, object userData, IPAddress ipAddr)
		{
			m_serverEntry = new ServerEntry(strName, strLocalName, tcpPort, udpPort, callback, userData, ipAddr);

			m_serverJobRequestQueue = new ServerJobRequestQueue();
		}




	}
}
