using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using MenuSystemLibrary;
using System.Xml;
using System.Diagnostics;



namespace Executer
{
	public partial class JobTab : Form
	{
		string m_strName;
		MenuSystem m_ms;

		private JobTab()
		{
			InitializeComponent();
		}

		public JobTab(string strName) : this()
		{
			m_strName = strName;
			this.groupBox_name.Text = strName;
			m_ms = new MenuSystem();
			m_ms.Initialise(null, MenuChangedEvent);			/// There is no definition document associated with the menus here.
		}

		/// <summary>
		/// This is called when the menu_system changes value through user action, e.g. a slider is moved.
		/// </summary>
		/// <param name="xMenu">This is the new value of the entire menu_system control.</param>
		void MenuChangedEvent(XmlElement xMenu)
		{
			Debug.WriteLine("MenuChangedEvent in JobTab has been called.");
		}

		/// <summary>
		/// Accepts a menu element and displays it in the menu_system. The menu template is disregarded.
		/// </summary>
		/// <param name="xData">The Xml data to display.</param>
		/// <returns>Success or failure.</returns>
		public bool SetMenu(XmlElement xData)
		{
			return m_ms.ForceMenu(xData);
		}


//		public delegate bool DelegateRequestJobCallback(JobStatus status, int jobId, object userData, ServerEntry se);

		public bool JobCallback(ServerEntry.Job.JobStatus status, int jobId, object userData, ServerEntry se)
		{
			switch(status)
			{
				case ServerEntry.Job.JobStatus.Uninitialised:
					{
						break;
					}
				case ServerEntry.Job.JobStatus.Queued:
					{
						break;
					}
				case ServerEntry.Job.JobStatus.Created:
					{
						break;
					}
				case ServerEntry.Job.JobStatus.Failed:
					{
						break;
					}
				case ServerEntry.Job.JobStatus.Timeout:
					{
						break;
					}
				default:
					{
						break;
					}
			}
			return false;
		}

	}
}
