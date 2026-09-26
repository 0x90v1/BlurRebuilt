using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using MenuSystemLibrary;
using Global;

namespace Executer
{
	public partial class OptionsManager : Form
	{
//		XmlDocument m_xMenuDocument;

		XmlElement m_xValues;

		private OptionsManager()
		{
			InitializeComponent();
		}

		public OptionsManager(XmlElement xOptions, XmlElement xValues) : this()
		{
			m_xValues = xValues;

			this.m_menuSystem.Initialise(xOptions);
			this.m_menuSystem.MenuChangedEvent += new EventHandler<MenuSystemChangedEventArgs>(m_menuSystem_MenuChangedEvent);

			XmlElement xFirstDuplicate = this.m_menuSystem.CheckContainerNamesAreUnique();
			if(xFirstDuplicate != null)
			{
				string strError = "The name '" + XmlHelper.GetKeyValue(xFirstDuplicate, "name") + "' is multiply defined.";

				throw new ApplicationException(strError);
			}

			XmlElement xRootContainer = xValues.SelectSingleNode("container") as XmlElement;

			if(xRootContainer == null)
			{
				xRootContainer = xValues.OwnerDocument.CreateElement("container");
				xValues.AppendChild(xRootContainer);
			}

			this.m_menuSystem.SetMenu(xRootContainer, true);
		}

		void m_menuSystem_MenuChangedEvent(object sender, MenuSystemChangedEventArgs e)
		{
			m_xValues.RemoveAll();

			XmlElement xImportedChild = m_xValues.OwnerDocument.ImportNode(e.XOwnerDocument.DocumentElement, true) as XmlElement;
			m_xValues.AppendChild(xImportedChild);
		}

		internal bool GetContainerBoolByName(string strContainerName, bool defaultValue)
		{
			XmlElement xElement = XmlHelper.FindNodeByKeyValue(m_xValues.ChildNodes, "name", strContainerName, true) as XmlElement;

			if(xElement != null)
			{
				return XmlHelper.GetKeyBool(xElement, "value", defaultValue);
			}
			return defaultValue;
		}

		internal string GetContainerStringByName(string strContainerName)
		{
			XmlElement xElement = XmlHelper.FindNodeByKeyValue(m_xValues.ChildNodes, "name", strContainerName, true) as XmlElement;

			if(xElement != null)
			{
				return XmlHelper.GetKeyValue(xElement, "value");

			}
			return null;
		}

		private void OptionsManager_FormClosing(object sender, FormClosingEventArgs e)
		{
			if(e.CloseReason == CloseReason.UserClosing)
			{
				this.Visible = false;
				e.Cancel = true;
			}
		}

		private void buttonOk_Click(object sender, EventArgs e)
		{
			this.Visible = false;
		}
	}
}