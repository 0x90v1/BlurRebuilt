using System;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Diagnostics;
using System.Media;
using Executer.Farm;
using Global;
using SuperCollections;

namespace Executer
{
	public class JobManager
	{
		//private DoubleKeyDictionary<long, string, Job> m_jobMap = new DoubleKeyDictionary<long, string, Job>();

		class JobMap : DoubleKeyDictionary<long, string, Job>
		{
			
		}

		private JobMap m_jobMap = new JobMap();

		public JobManager()
		{
			Program.s_farmClientManager.FarmClientCreatedEvent += s_farmClientManager_FarmClientCreatedEvent;
			Program.s_farmClientManager.FarmClientRemovedEvent += s_farmClientManager_FarmClientRemovedEvent;
		}

		void s_farmClientManager_FarmClientCreatedEvent(object sender, FarmClientEventArgs e)
		{
			// TODO: Looks untidy.
			e.FarmClient.StatusChangedEvent				+= FarmClient_StatusChangedEvent;
			e.FarmClient.ConnectionStateChangedEvent	+= FarmClient_ConnectionStateChangedEvent;

			e.FarmClient.NotifyJobStatusChangedEvent	+= FarmClientJobStatusChanged;
			e.FarmClient.NotifyReceivedJobDataEvent		+= FarmClientReceivedJobData;
		}

		void s_farmClientManager_FarmClientRemovedEvent(object sender, FarmClientEventArgs e)
		{
			if(Program.s_farmClientManager.m_farmClientViewer != null)
				Program.s_farmClientManager.m_farmClientViewer.RemoveClient(e.FarmClient);

			Debug.WriteLine("Farm client removed.");
		}

		void FarmClient_ConnectionStateChangedEvent(object sender, ChangedConnectionStateEventArgs e)
		{
			if(Program.s_farmClientManager.m_farmClientViewer != null)
				Program.s_farmClientManager.m_farmClientViewer.UpdateConnectionStatus(sender as FarmClient, e.NewStatus.ToString());
		}

		void FarmClient_StatusChangedEvent(object sender, ChangedStatusEventArgs e)
		{
			if(Program.s_farmClientManager.m_farmClientViewer != null)
				Program.s_farmClientManager.m_farmClientViewer.UpdateClientStatus(sender as FarmClient, e.NewStatus.ToString());
		}

		/// <summary>
		/// This is the start of events when the tree manager requests a job. All server job requests start here.
		/// </summary>
		/// <param name="job">This describes the job that needs to be run.</param>
		/// <returns>true if the server manager queues the job, otherwise false.</returns>
		internal bool RequestJob(Job job)
		{
			job.OwnerTreeUserObject.AddJobDataEntry(job);

			if(Executer_MainForm.s_treeObjectViewer.CurrentlyViewedTreeObject == job.OwnerTreeUserObject)
				Executer_MainForm.s_treeObjectViewer.UpdateView();
			m_jobMap.AddKey1(job.LocalJobId, job);

			return Program.s_farmClientManager.RequestJob(job);
		}
		
		internal void RemoveJob(Job job)
		{
			TreeUserObject treeUserObject = job.OwnerTreeUserObject;

			treeUserObject.RemoveJobDataEntry(job);

			if(job.StrJobId != null)
				this.m_jobMap.RemoveKey2(job.StrJobId);
			else
				this.m_jobMap.RemoveKey1(job.LocalJobId);
		}

		LocallyUniqueIdMaker m_jobLuidMaker = new LocallyUniqueIdMaker();

		object m_changedJobStatusCallbackLock = new object();

		/// This is where everything gets glued together ...
		internal void FarmClientJobStatusChanged(object sender, NotifyChangedJobStatusEventArgs e)
		{
			FarmClient client = sender as FarmClient;
			Debug.Assert(client != null);

			lock(m_changedJobStatusCallbackLock)
			{
				Job job = null;

				string strJobId = null;

				// A job id of -1 means that the farm has not yet assigned an id to the job, so we should look it up using the local id.
				if(e.JobId != -1)
				{
					strJobId = GetServerIdString(client, e.JobId);

					if(e.JobStatus == JobStatus.Queued)
					{
						// TODO: The following test is flawed since a restarted farm starts giving job numbers out from index 0
						// and Executer may not (or no longer) have any jobs with job number 0, therefore a farm restart will be missed.
						if(m_jobMap.ContainsKey2(strJobId))					// If this key already exists then something bad has happened with the server.
						{
							// Re-key all job entries relating to this server.
							// Set their status to 'Abandoned'
							job = m_jobMap.FindValueByKey1(e.JobLuid);
							
							List<string> keys = new List<string>();
							foreach(string strKey in m_jobMap.key2_dict.Keys)
							{
								if(strKey.StartsWith(client.StrLocalName))
								{
									keys.Add(strKey);
								}
							}

							foreach(string strKey in keys)
							{
								Job abandonedJob = m_jobMap.FindValueByKey2(strKey);
								m_jobMap.RemoveKey2(strKey);
								string strUniqueKey = "Abandoned_" + m_jobLuidMaker.GetNextLuid().ToString();
								m_jobMap.AddKey2(strUniqueKey, abandonedJob);
								abandonedJob.JobStatus = JobStatus.Abandoned;
								try
								{
									abandonedJob.StrJobId = strUniqueKey;	// This will throw an ApplicationException since StrJobId already has a value.
								}											// Ignore the exception - the assignment will still be made.
								catch { }
							}

							// The dialog must not halt this thread, hence BeginInvoke.
							Program.S_executerMainForm.BeginInvoke((MethodInvoker)delegate()
							{
								Program.S_executerMainForm.MessageBoxError("The server '" + client.StrLocalName + "' has been restarted.\nConnections to all jobs associated with that server have therefore been abandoned.\nYou can view the last known status of each job by clicking on it's job panel.");
							});
						}
						job = m_jobMap.FindValueByKey1(e.JobLuid);
					}
					else
					{
						// Note, we may be getting a message for a job Executer knows nothing about. That's how the farm works.
						if(m_jobMap.TryValueFindByKey2(strJobId, out job) == false)
						{
							//Debug.Assert(false);
							m_jobMap.TryValueFindByKey1(e.JobLuid, out job);
						}
					}
				}
				else
					m_jobMap.TryValueFindByKey1(e.JobLuid, out job);

				
				if(job != null)
				{
					// If the job in question is currently viewed, update the view. (Via a delegate).

					JobStatus oldJobStatus = job.JobStatus;
					job.JobStatus = e.JobStatus;
					switch(e.JobStatus)
					{
						case JobStatus.None:
							throw new ApplicationException("Logical error in ChangedJobStatusCallback. jobStatus has default value.");
						//						break;
						case JobStatus.Requested:
							break;
						case JobStatus.Queued:
							m_jobMap.AddKey2(strJobId, job);									// The job now exists on the server. Add it to the map.
							job.StrJobId = strJobId;
							job.JobId = e.JobId;
							break;
						case JobStatus.Failed:

							string strFailedWav = XmlHelper.GetKeyValue(job.OwnerTreeUserObject.XTypeTemplate, "error_wav");
							if(MyPlaySound(strFailedWav) == false)
								SystemSounds.Asterisk.Play();
							
							// The following line causes a crash for many people ...
							//MessageHandler.ShowMemory(null, "Job failed", "Suppress notification when a job fails?", "The job '" + jobEntry.m_serverRequest.m_strJobName + "' failed.", "Executer", MessageBoxButtons.OK, MessageBoxIcon.Error);
							Debug.WriteLine("The farm server actively refused to process this job, or it started it and it then failed.");
//							File.Delete(jobEntry.m_strTempArgsFile);
							break;
						case JobStatus.Timeout:
							Debug.WriteLine("The farm server failed to respond to this job within the timeout period.");
							break;
						case JobStatus.Created:
							break;
						case JobStatus.Killed:
							//File.Delete(jobEntry.m_strTempArgsFile);
							break;
						case JobStatus.Abandoned:
							Debug.WriteLine("Job '" + job.StrTaskDisplayName + "' on server '" + client.StrLocalName + "' ID:" + e.JobId.ToString() + " abandoned.");
							break;
						case JobStatus.Finished:

							string strCompletedWav = XmlHelper.GetKeyValue(job.OwnerTreeUserObject.XTypeTemplate, "complete_wav");
							if(MyPlaySound(strCompletedWav) == false)
								SystemSounds.Beep.Play();
//							global.SmartNotify.Add("hello", Executer.Executer_MainForm.ActiveForm);
							//File.Delete(jobEntry.m_strTempArgsFile);
							break;
						case JobStatus.Cancelled:
							//File.Delete(jobEntry.m_strTempArgsFile);
							break;
						default:
							break;
					}
					// Do this after the switch statement, so that m_jobMap is current.

					Program.s_jobViewer.BeginInvoke(new JobViewer.DelegateUpdateJobStatus(Program.s_jobViewer.UpdateJobStatus), new object[] { job });
				}
			}
		}

		private bool MyPlaySound(string strFailedWav)
		{
			try
			{
				System.Media.SoundPlayer myPlayer = new System.Media.SoundPlayer();
				myPlayer.SoundLocation = strFailedWav;
				myPlayer.Play();
				return true;
			}
			catch
			{
				return false;
			}
		}

		private string GetServerIdString(FarmClient fc, int jobId)
		{
			return fc.StrLocalName + " " + jobId;
		}

		internal void KillJob(Job job)
		{
			Program.s_farmClientManager.SendMessage(job.StrServerName, "KILL " + job.JobId.ToString());
		}

		internal void FarmClientReceivedJobData(object sender, NotifyReceivedJobDataEventArgs e)
		{
			FarmClient farmClient = sender as FarmClient;
			Job job = null;
			string strJobId = GetServerIdString(farmClient, e.JobId);
			m_jobMap.TryValueFindByKey2(strJobId, out job);
			if(job != null)
			{
				job.ReceivedMenuData(e.StrJobData);
			}
		}

		delegate bool InvokeRedrawJobDelegate(Job job);

		internal static void ShowJobViewer()
		{
			Program.s_jobViewer.SetDesiredVisibility(true);
			//Program.s_jobViewer.Visible = true;
			//Program.s_jobViewer.bWantsToBeVisible = true;
			Program.s_jobViewer.BringToFront();
		}
	}
}
