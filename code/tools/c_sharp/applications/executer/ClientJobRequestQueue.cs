using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;
using System.Diagnostics;
using Executer.Farm;
using Executer.JobManagement;

namespace Executer
{
	class ClientJobRequestQueue
	{
		public event EventHandler<NotifyChangedJobStatusEventArgs> NotifyJobStatusChangedEvent;
		
		protected void OnNotifyJobStatusChangedEvent(JobStatus newJobStatus, int jobId, long jobLuid)
		{
			if(NotifyJobStatusChangedEvent != null)
			{
				NotifyJobStatusChangedEvent(this, new NotifyChangedJobStatusEventArgs(newJobStatus, jobId, jobLuid));
			}
		}

		private const int JOB_TIMEOUT = 1000 * 60 * 1;							// Wait one minute before giving up on a job.
		private Thread _jobRequestQueueThread = null;
		private int _lastJobRequestId = -2;										// Rogue value. -1 means 'request denied', otherwise it's a job number assigned by the farm.
		private readonly EventWaitHandle _waitForJobRequestAddedHandle = new EventWaitHandle(false, EventResetMode.AutoReset);
		private readonly EventWaitHandle _waitForJobRequestResponseHandle = new EventWaitHandle(false, EventResetMode.AutoReset);
		private readonly EventWaitHandle _waitForProgramExit = new EventWaitHandle(false, EventResetMode.ManualReset);

		private readonly List<Job> _serverRequestQueue = new List<Job>();

		//TODO: [Obsolete]// Want to use events instead.
		private readonly FarmClient _farmClient;

		public ClientJobRequestQueue(FarmClient farmClient)
		{
			_farmClient = farmClient;

			Initialise();
		}

		public void Initialise()
		{
			JobRequestQueueThreadStart();
		}

		private int GetLastRequestId()
		{
			Debug.Assert(_lastJobRequestId != -2);
			return _lastJobRequestId;
		}

		private bool JobRequestQueueThreadStart()
		{
			// Create a low priority thread that manages the job request queue.
			_jobRequestQueueThread = new Thread(new ThreadStart(JobRequestQueueThread)) { Priority = ThreadPriority.Normal };
			_jobRequestQueueThread.Start();

			return true;
		}

		internal bool JobRequestQueueThreadStop()
		{
			if(_jobRequestQueueThread != null)
			{
				_waitForProgramExit.Set();
//				m_jobQueueThread.Abort();				/// Signalling the above event is enough to abort the thread.
				_jobRequestQueueThread = null;
				return true;
			}
			return false;
		}

		private void JobRequestQueueThread()
		{
			Dictionary<long, List<long>> dependencyMap = new Dictionary<long, List<long>>();

			while(true)
			{
				if(_serverRequestQueue.Count > 0)										// If there are pending jobs in the queue ...
				{
					Job job;

					lock(_serverRequestQueue)
					{
						job = _serverRequestQueue[0];
						_serverRequestQueue.RemoveAt(0);								// We've got the job, so remove it from the queue.
					}

					#region Job dependencies part one

					List<long> jobDependencies = null;									// This is used to pick this job's dependencies from the dependency map if there are any.
					
					if(dependencyMap.ContainsKey(job.LocalJobId))
					{
						jobDependencies = dependencyMap[job.LocalJobId];
						dependencyMap.Remove(job.LocalJobId);					// This dependency list is no longer needed so discard it.
					}

					#endregion Job dependencies part one

					string strFarmCommand = BuildFarmCommand(job, jobDependencies);

					Debug.WriteLine("Sending command to farm server: '" + strFarmCommand + "'");

					// Signal the request BEFORE we actually do it. This prevents the Tcp thread reporting a job has been accepted
					// before the request message has been processed.
					OnNotifyJobStatusChangedEvent(JobStatus.Requested, -1, job.LocalJobId);
					//job.JobStatus = JobStatus.Requested;
					_farmClient.SendTcpString(strFarmCommand);
					
					// Wait for job 
					int result = WaitHandle.WaitAny(new WaitHandle[] { _waitForJobRequestResponseHandle, _waitForProgramExit }, JOB_TIMEOUT, false);
					if(result == 1)				// If program exit is signalled, just return from this thread.
					{
						return;
					}
					else if(result == 0)					// If waitForJobResponseHandle was signalled, i.e. if we have a job response ...
					{
						// Received a signal, i.e. farm responded to job ...
						int remoteJobId = GetLastRequestId();

						Debug.WriteLine("Farm responded to job " + remoteJobId.ToString());

						if(remoteJobId != -1)											// If the job request did not fail ...
						{
							// Job id was assigned.
							OnNotifyJobStatusChangedEvent(JobStatus.Queued, remoteJobId, job.LocalJobId);

							// Before this lock was added, the Tcp stuff was processing the next Tcp message (JobStatusEnum.Created)
							// before this message had been processed, i.e. before the app. knew about the job.
							_waitForJobRequestResponseToBeProcessedHandle.Set();			// The Job has been queued, so it is safe to proceed.

							#region Job dependencies part 2
							// Now add the current job's jobId to it's parent's dependency list. (Before the parent job is requested.)
							if(job.LocalParentJobId != -1)						// As long as the job has a parent! :-)
							{
								if(dependencyMap.ContainsKey(job.LocalParentJobId) == false)
								{
									dependencyMap.Add(job.LocalParentJobId, new List<long>());
								}
								List<long> parentJobDependencies = dependencyMap[job.LocalParentJobId];

								parentJobDependencies.Add(remoteJobId);
							}
							#endregion Job dependencies part 2
						}
						else	// Farm denied the job.
						{
							OnNotifyJobStatusChangedEvent(JobStatus.Failed, -1, job.LocalJobId);

							_waitForJobRequestResponseToBeProcessedHandle.Set();			// The Job has been failed, so it is safe to proceed.
						}
					}
					else	// Job request timed out ...
					{
						Debug.Assert(result == WaitHandle.WaitTimeout);

						OnNotifyJobStatusChangedEvent(JobStatus.Timeout, -1, job.LocalJobId);
					}
				}
				else
				{
					// Wait for a job to be added to the jobqueue or for program exit signal.
					if(WaitHandle.WaitAny(new WaitHandle[] { _waitForJobRequestAddedHandle, _waitForProgramExit }) == 1)
						return;
				}
			}
		}

		/// <summary>
		/// Given a job and a list of dependent job id's, this
		/// creates the job request string that will be sent to the farm.
		/// </summary>
		/// <param name="job">The job request being made.</param>
		/// <param name="jobDependencies">A list of job id's that this job is dependent on.</param>
		/// <returns></returns>
		private static string BuildFarmCommand(Job job, List<long> jobDependencies)
		{
			string strFarmCommand = "JOB";

			strFarmCommand += " " + job.StrJobAndArguments.Length.ToString();
			strFarmCommand += " " + job.StrJobAndArguments;
			string strComment = job.StrFarmComment;

			if(strComment == null)
			{
				strComment = "No comment has been specified for this job. Please attend your template.";
			}
			strFarmCommand += " " + strComment.Length.ToString();
			strFarmCommand += " " + strComment;
			strFarmCommand += " " + "1";						// Flags

			if(jobDependencies != null)
			{
				string strJobDependencies = "";										// This is used to build the job dependency string that is passed to the farm.

				foreach(int jobId in jobDependencies)
				{
					if(strJobDependencies != "")
						strJobDependencies += " ";
					strJobDependencies += jobId.ToString();
				}

				strFarmCommand += " " + jobDependencies.Count.ToString();
				strFarmCommand += " " + strJobDependencies;
			}
			else
			{
				strFarmCommand += " 0";
			}

			string strSource = job.StrOwnerProductName;

			if(strSource == null)
			{
				strSource = "unknown";
			}

			strFarmCommand += " " + strSource.Length.ToString();
			strFarmCommand += " " + strSource;

			return strFarmCommand;
		}

		public bool RequestJob(Job newJob)
		{
			lock(_serverRequestQueue)
			{
				_serverRequestQueue.Add(newJob);
				_waitForJobRequestAddedHandle.Set();									// Allow the jobQueueThread to start processing the next job. (If it isn't already)
			}
			return true;
		}

		internal int GetNumberOfQueuedJobs()
		{
			return _serverRequestQueue.Count;
		}

		private readonly EventWaitHandle _waitForJobRequestResponseToBeProcessedHandle = new EventWaitHandle(false, EventResetMode.AutoReset);

		internal void JobRequestResponseReceived(int jobNumber)
		{
			_lastJobRequestId = jobNumber;
			_waitForJobRequestResponseHandle.Set();									// Allow the jobQueueThread to resume processing it's current job.

			// Wait for the job response to be processed.
			// This function wakes up the JobRequestQueueThread which in turn generates a callback to deal with the new job id.
			// We do not wish to return until the thread has completed that callback, otherwise further messages may be processed
			// out of order.
			_waitForJobRequestResponseToBeProcessedHandle.WaitOne();
//			bool result = waitForJobRequestResponseToBeProcessedHandle.WaitOne(10, false);
//			Debug.Assert(result == true);
		}
	}
}
