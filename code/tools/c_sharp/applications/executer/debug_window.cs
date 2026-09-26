using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace Executer
{
	// TODO: Make a library of this and use comms to report messages. TCP or UDP etc.
	// TODO: Write a listener that is generic and can listen on ip / port for messages and can filter.

	public partial class DebugWindow : Form
	{
		static DebugWindow m_debugWindow = null;

		public static void WriteToDebugWindow(string strMessage)
		{
			if(m_debugWindow == null)
				m_debugWindow = new DebugWindow();

			m_debugWindow.DebugOutput(strMessage);
		}

		public static bool ShowDebugWindow(bool bShow)
		{
			if(m_debugWindow == null)
				m_debugWindow = new DebugWindow();

			m_debugWindow.Visible = bShow;
			if(bShow == true)
				m_debugWindow.BringToFront();

			return true;
		}


		private DebugWindow()
		{
			InitializeComponent();

			this.Visible = true;
			this.CreateControl();
			this.Visible = false;
//			this.Show();
		}

		//delegate void DebugWindowDelegate(string strText);

		private bool DebugOutput(string strMessage)
		{
			if(InvokeRequired)
				//this.BeginInvoke(new DebugWindowDelegate(this.AppendText), new object[] { strMessage });
				this.BeginInvoke((MethodInvoker)delegate() { this.AppendText(strMessage); });
			else
				this.AppendText(strMessage);

			return true;
		}


		void AppendText(string strText)
		{
			this.richTextBox1.AppendText(strText + "\n");
		}


		private void buttonCopy_Click(object sender, EventArgs e)
		{
			Clipboard.SetDataObject(this.richTextBox1.Text);
		}

		private void buttonClear_Click(object sender, EventArgs e)
		{
			this.richTextBox1.Clear();
		}

		private void DebugWindow_FormClosing(object sender, FormClosingEventArgs e)
		{
			if(e.CloseReason == CloseReason.UserClosing)
			{
				this.Visible = false;
				e.Cancel = true;
			}
		}

		private void DebugWindow_Load(object sender, EventArgs e)
		{

		}
	}
}