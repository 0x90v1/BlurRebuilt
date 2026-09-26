using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using MenuSystemLibrary;
using System.Xml;
using System.Diagnostics;
using System.IO;
	


namespace Executer
{
	public partial class JobPanel : UserControl
	{
		private JobPanel()
		{
			InitializeComponent();
		}

		readonly TreeUserObjectViewer m_owner;

		internal readonly Job _job;
		internal readonly string _strDescription = null;

		public JobPanel(TreeUserObjectViewer owner, Job job, string strDescription) : this()
		{
			_strDescription = strDescription;
			this.groupBoxMain.Text = strDescription;
			this.Name = "Need to find a name.";
			
			m_owner = owner;
			_job = job;

			this.menuSystem.Initialise(null);			// There is no definition document associated with the menus here.

			this.menuSystem.MenuChangedEvent += new EventHandler<MenuSystemChangedEventArgs>(menuSystem_MenuChangedEvent);

			SetMenu(_job.XJobMenu);
			UpdateJobStatus();

			_job.JobStatusChangedEvent += new EventHandler<JobStatusChangedEventArgs>(m_job_JobStatusChangedEvent);
			_job.JobMenuDataChangedEvent += new EventHandler<JobMenuDataChangedEventArgs>(m_job_JobMenuDataChangedEvent);
		}

		void menuSystem_MenuChangedEvent(object sender, MenuSystemChangedEventArgs e)
		{
			//m_job.XJobMenu = e.XOwnerDocument;
		}

		void m_job_JobStatusChangedEvent(object sender, JobStatusChangedEventArgs e)
		{
			Debug.WriteLine("XXX CHANGED to "+e.NewJobStatus.ToString()+" XXX");
			UpdateJobStatus();
		}

		void m_job_JobMenuDataChangedEvent(object sender, JobMenuDataChangedEventArgs e)
		{
			this.SetMenu(e.NewMenuData);
		}

		void t_NodeChanged(object sender, XmlNodeChangedEventArgs e)
		{
			throw new NotImplementedException();
		}

		public void SetStatusText(string strJobMessage)
		{
			this.JobStatus.Text = strJobMessage;
		}

		/// <summary>
		/// Sets the panel text and button status according to the job's current status.
		/// </summary>
		/// <returns></returns>
		void UpdateJobStatus()
		{
			if(InvokeRequired == true)
			{
				this.BeginInvoke((MethodInvoker)delegate() { this.UpdateJobStatus(); });
				return;
			}
			switch(_job.JobStatus)
			{
				case Executer.JobStatus.None:
					break;
				case Executer.JobStatus.Requested:
					this.SetStatusText(_job.JobStatus.ToString());
					this.buttonKill.Enabled = false;
					this.buttonClose.Enabled = false;
					break;
				case Executer.JobStatus.Queued:
					this.SetStatusText(_job.JobStatus.ToString());
					this.buttonKill.Enabled = true;
					this.buttonClose.Enabled = false;
					break;
				case Executer.JobStatus.Failed:
					this.SetStatusText(_job.JobStatus.ToString());
					this.buttonKill.Enabled = false;
					this.buttonClose.Enabled = true;
					break;
				case Executer.JobStatus.Timeout:
					this.SetStatusText("Request timeout.");
					this.buttonKill.Enabled = false;
					this.buttonClose.Enabled = true;
					break;
				case Executer.JobStatus.Created:
					this.SetStatusText(_job.JobStatus.ToString());
					this.buttonKill.Enabled = true;
					this.buttonClose.Enabled = false;
					break;
				case Executer.JobStatus.Killed:
					this.SetStatusText(_job.JobStatus.ToString());
					this.buttonKill.Enabled = false;
					this.buttonClose.Enabled = true;
					break;
				case Executer.JobStatus.Abandoned:
					this.SetStatusText(_job.JobStatus.ToString());
					this.buttonKill.Enabled = false;
					this.buttonClose.Enabled = true;
					break;
				case Executer.JobStatus.Finished:
					this.SetStatusText(_job.JobStatus.ToString());
					this.buttonKill.Enabled = false;
					this.buttonClose.Enabled = true;
					break;
				case Executer.JobStatus.Cancelled:
					this.SetStatusText(_job.JobStatus.ToString());
					this.buttonKill.Enabled = false;
					this.buttonClose.Enabled = true;
					break;
				default:
					break;
			}
			return;
		}

		private void buttonKill_Click(object sender, EventArgs e)
		{
			Debug.Assert(_job.JobId != -1);					// This button should not be enabled before the job has a job id.
			Program.s_jobManager.KillJob(_job);
		}

		private void buttonClose_Click(object sender, EventArgs e)
		{
			m_owner.RemovePanel(this);
		}

		/// <summary>
		/// Set the job panel menu system to display the given XmlDocument.
		/// </summary>
		/// <param name="xmlDocument"></param>
		private void SetMenu(XmlDocument xmlDocument)
		{
			XmlElement xRootElement = xmlDocument.DocumentElement;
			if(xRootElement != null)
			{
				if(xRootElement.Name == "values")
					this.menuSystem.ForceMenu(xRootElement.SelectSingleNode("container") as XmlElement);
				else
					this.menuSystem.SetSingleContainer(xRootElement);
			}
		}
	}
}

