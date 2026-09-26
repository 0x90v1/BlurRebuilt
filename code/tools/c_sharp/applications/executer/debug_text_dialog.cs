using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;
using Executer.Helpers;
using message_handler;
using System.Xml;

namespace Executer
{
	public partial class debug_text_dialog : Form
	{
		private debug_text_dialog()
		{
			InitializeComponent();
		}

		public debug_text_dialog(string strFilename, string strCommand, string strArguments)
		{
			InitializeComponent();

			m_strFilename = strFilename;
			m_strCommand = strCommand;
			m_strArguments = strArguments;

			this.textBox1.Text = m_strCommand;
			this.textBox2.Text = m_strFilename;
			this.richTextBox1.AppendText(m_strArguments);
		}

		static string m_strFilename = null;
		string m_strCommand = null;
		string m_strArguments = null;

		private void button_Save_Click(object sender, EventArgs e)
		{
			try
			{
				string strFilename = m_strFilename;
				this.textBox2.Text = m_strFilename;

				SaveTempFile(strFilename);
			}
			catch(Exception ex)
			{
				MessageHandler.Show(this, "Cannot save the file to '" + m_strFilename + "'\nThe system reports: " + ex.Message);
			}
		}

		private void SaveTempFile(string strFilename)
		{
			using(StreamWriter sw = new StreamWriter(strFilename, false))
			{
				sw.Write(m_strArguments);
			}
		}


		private void button_SaveAs_Click(object sender, EventArgs e)
		{
			string strNewFilename = GetFilename(m_strFilename);
			if(strNewFilename != null)
			{
				m_strFilename = strNewFilename;
				button_Save_Click(sender, e);
			}
		}

		private void button_Copy_Click(object sender, EventArgs e)
		{
			Clipboard.SetText(m_strArguments);
		}

		private string GetFilename(string strFilename)
		{
			SaveFileDialog sfd = new SaveFileDialog();
			string strOldFilename = strFilename;
			sfd.CheckFileExists = false;
			sfd.InitialDirectory = strFilename;

			sfd.Filter = "xml files (*.xml)|*.xml|All files (*.*)|*.*";
			sfd.FilterIndex = 1;
			sfd.RestoreDirectory = true;

			if(sfd.ShowDialog() == DialogResult.OK)
			{
				return sfd.FileName;
			}
			return null;
		}

		private void buttonView_Click(object sender, EventArgs e)
		{

		//static public Process LaunchJob(string strCommand, string strClArguments, string strWorkingDirectory, bool bHideWindowFromUser, bool bStartMinimised)

			try
			{
				SaveTempFile(m_strFilename+".xml");
			}
			catch(Exception ex)
			{
				MessageHandler.Show(this, "Cannot save the file to '" + m_strFilename + "'\nThe system reports: " + ex.Message);
			}

			try
			{
				LaunchProcess.LaunchJob("iexplore", m_strFilename + ".xml", null, false, false, null);
			}
			catch(Exception ex)
			{
				MessageHandler.Show(this, "Cannot launch ie with '" + m_strFilename+".xml" + "'\nThe system reports: " + ex.Message);
			}

			
		}
	}
}