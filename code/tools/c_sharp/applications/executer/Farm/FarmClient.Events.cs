using System;
using System.Collections.Generic;
using System.Text;
namespace Executer
{
	public enum ClientStatus
	{
		None = 0,
		Created,
		Active,
		PotentiallyUnwanted,
		MarkedForDeletion,
		Deleting,
		Destroyed,
		ChangedIp
	}

	public class ChangedStatusEventArgs : EventArgs
	{
		public ClientStatus OldStatus { get; private set; }
		public ClientStatus NewStatus { get; private set; }


		public ChangedStatusEventArgs(ClientStatus oldStatus, ClientStatus newStatus)
		{
			OldStatus = oldStatus;
			NewStatus = newStatus;
		}
	}

	public enum ClientConnectionStatus
	{
		None = 0,
		SearchingForServer,
		FoundServer,
		Connecting,
		Connected,
		Disconnected,
	}

	public class ChangedConnectionStateEventArgs : EventArgs
	{
		public ClientConnectionStatus OldStatus { get; private set; }
		public ClientConnectionStatus NewStatus { get; private set; }

		public ChangedConnectionStateEventArgs(ClientConnectionStatus oldStatus, ClientConnectionStatus newStatus)
		{
			OldStatus = oldStatus;
			NewStatus = newStatus;
		}
	}

	public enum DataType
	{
		Udp = 0,
		Tcp
	}

	public class ReceivedDataEventArgs : EventArgs
	{
		public string StrData { get; private set; }
		internal DataType DataType { get; private set; }

		public ReceivedDataEventArgs(string strData, DataType dataType)
		{
			StrData = strData;
			DataType = dataType;
		}
	}

	public enum JobStatus
	{
		None = 0,

		Requested,
		Queued,
		Failed,
		Timeout,
		Created,
		Killed,
		Abandoned,
		Finished,
		Cancelled,
	}


	public class NotifyChangedJobStatusEventArgs : EventArgs
	{
		public JobStatus JobStatus { get; private set; }
		public int JobId { get; private set; }
		public long JobLuid { get; private set; }

		public NotifyChangedJobStatusEventArgs(JobStatus jobStatus, int jobId, long jobLuid)
		{
			JobStatus = jobStatus;
			JobId = jobId;
			JobLuid = jobLuid;
		}
	}

	public class NotifyReceivedJobDataEventArgs : EventArgs
	{
		public int JobId { get; private set; }
		public string StrJobData { get; private set; }

		public NotifyReceivedJobDataEventArgs(int jobId, string strJobData)
		{
			JobId = jobId;
			StrJobData = strJobData;
		}
	}
}
