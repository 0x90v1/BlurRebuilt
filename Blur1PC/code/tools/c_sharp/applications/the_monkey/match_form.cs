using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace RoboPublish
{
	public partial class MatchForm : Form
	{
		public MatchForm()
		{
			InitializeComponent();
		}

		private void MatchForm_FormClosing(object sender, FormClosingEventArgs e)
		{
			if(e.CloseReason == CloseReason.UserClosing)
			{
				this.Visible = false;
				e.Cancel = true;
			}
		}
	}
}