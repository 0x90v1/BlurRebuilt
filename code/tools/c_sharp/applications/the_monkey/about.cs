using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace RoboPublish
{
	public partial class About : Form
	{
		public About()
		{
			InitializeComponent();
			this.labelBuildDate.Text = BuildTime.Date;
			this.labelBuildTime.Text = BuildTime.Time;
		}
	}
}