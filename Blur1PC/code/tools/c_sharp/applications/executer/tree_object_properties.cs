using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;
using Executer.TreeObjects;
using message_handler;

namespace Executer
{
	public partial class TreeObjectProperties : Form
	{
		private TreeObjectProperties()
		{
			InitializeComponent();
		}

		TreeProjectObject m_tpo;
		//public TreeObjectProperties(TreeObject to)
		public TreeObjectProperties(TreeProjectObject tpo)
			: this()
		{
			m_tpo = tpo;

			this.textBoxProjectName.Text = tpo.StrProjectDocumentFilename;
			this.textBoxTemplateName.Text = tpo._strTemplateDocumentFilename;
			this.textBoxTemporaryFolder.Text = tpo.StrFarmTempFolder;
			foreach(string s in tpo.RequiredServerList.Keys)
			{
				this.richTextBox1.AppendText(s + "\n");
			}
		}

		private void button1_Click(object sender, EventArgs e)
		{
			this.Close();
		}

		private void buttonOpenProject_Click(object sender, EventArgs e)
		{
			LaunchJob(this.textBoxProjectName.Text);
		}

		private void buttonOpenTemplate_Click(object sender, EventArgs e)
		{
			Form f = new XmlViewer(m_tpo.XMenuTemplateDocument);

			f.Show(this);

			//LaunchJob("IExplore "+ this.textBoxTemplateName.Text);
		}

		private void buttonOpenTemporaryFolder_Click(object sender, EventArgs e)
		{
			LaunchJob(this.textBoxTemporaryFolder.Text);
		}

		public bool LaunchJob(string strCommand)
		{
			try
			{
				Process p = new Process();

				p.StartInfo = new ProcessStartInfo(strCommand);

				return p.Start();
			}
			catch(Exception ex)
			{
				MessageHandler.Show(this, "Cannot open the specified file. System reports:\n" + ex.Message, "Executer", MessageBoxButtons.OK);
				return false;
				//throw (ex);
			}
		}
	}
}