using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Xml;

namespace Executer
{
	public partial class XmlViewer : Form
	{
		protected XmlViewer()
		{
			InitializeComponent();
		}

		string m_strFileName;
		XmlDocument _xDocument;

		public XmlViewer(XmlDocument xDocument)
		{
			InitializeComponent();

			_xDocument = new XmlDocument();
			_xDocument.AppendChild(_xDocument.ImportNode(xDocument.DocumentElement, true));
		}

		private void button_SaveAs_Click(object sender, EventArgs e)
		{
			string strNewFilename = GetFilename(m_strFileName);
			if(strNewFilename != null)
			{
				m_strFileName = strNewFilename;

				Save(strNewFilename);
			}
		}

		private void Save(string strFilename)
		{
			try
			{
				_xDocument.Save(strFilename);
			}
			catch(Exception ex)
			{
				MessageBox.Show(this, "Exception text:\n'" + ex.Message + "'", "Exception", MessageBoxButtons.OK, MessageBoxIcon.Error);
			}
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

		private void XmlViewer_Load(object sender, EventArgs e)
		{
			string sPath;
			sPath = Path.GetTempPath();
			m_strFileName = sPath + "Temp_ExpandedExecuterTemplate.xml";

			try
			{
				Save(m_strFileName);

				this.webBrowser1.Url = new Uri(m_strFileName);
			}
			catch
			{


			}
		}
	}
}
