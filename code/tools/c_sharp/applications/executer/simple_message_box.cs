using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace Executer
{
	public partial class SimpleMessageBox : Form
	{
		private SimpleMessageBox()
		{
			InitializeComponent();
		}

		public SimpleMessageBox(string strMessage, string strCaption) : this()
		{
			this.label1.Text = strMessage;
			this.Text = strCaption;
		}

		public string LabelText
		{
			get
			{
				return this.label1.Text;
			}
			set
			{
				this.label1.Text = value;
			}
		}
	}
}