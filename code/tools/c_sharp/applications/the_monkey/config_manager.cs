using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;
using System.Windows.Forms;
using System.Diagnostics;
using Global;

namespace RoboPublish
{
	class ConfigManager
	{
		readonly string m_strFilename;

		public XmlDocument m_xConfig;

		public ConfigManager(string strFilename)
		{
			m_strFilename = strFilename;
		}

		public bool Initialise()
		{
			m_xConfig = LoadOrCreateConfigFile(m_strFilename);

			if(m_xConfig != null)
				return true;
			else
				return false;
		}

		public XmlDocument LoadOrCreateConfigFile(string strFilename)
		{
			m_xConfig = new XmlDocument();

			try
			{
				m_xConfig.Load(strFilename);
			}
			catch(Exception ex)
			{
				if(ex.GetType() == Type.GetType("System.IO.FileNotFoundException"))
				{
				}
				else
				{
					MessageBox.Show("Cannot load your existing config. file - Creating a new one.\nSystem report: " + ex.Message);
				}
				try
				{
					m_xConfig.LoadXml(	"<root>" +
											"<options/>" +
											"<panels/>" +
										"</root>"
									);

					m_xConfig.Save(strFilename);
				}
				catch(Exception ex2)
				{
					MessageBox.Show("Cannot create the config. file. System report:\n" + ex2.Message);
					return null;
				}
			}

			return m_xConfig;
		}

		public bool SaveConfig()
		{
			try
			{
				m_xConfig.Save(m_strFilename);
				return true;
			}
			catch(Exception ex)
			{
				MessageBox.Show("Cannot save the configuration file. System Error:\n" + ex.Message);
				return false;
			}
		}

		public XmlElement AddPanel(RoboPanel panel)
		{
			XmlElement xPanels = m_xConfig.SelectSingleNode("/root/panels") as XmlElement;
 
			if(xPanels != null)
			{
				XmlElement xNewPanel = m_xConfig.CreateElement("panel");
				XmlHelper.AddAttribute(xNewPanel, "filename", panel.m_strFilename);
				xPanels.AppendChild(xNewPanel);

				XmlElement xSorceryConfig = m_xConfig.ImportNode(panel.Serialize(), true) as XmlElement;
				xNewPanel.AppendChild(xSorceryConfig);

				return xNewPanel;
			}
			
			Debug.Assert(false);
			return null;
		}

		//public bool RemovePanel(string strFilename)
		//{
		//    XmlElement xPanels = m_xConfig.SelectSingleNode("/root/panels") as XmlElement;
 
		//    if(xPanels != null)
		//    {
		//        XmlElement xOldPanel = XmlHelper.FindChildNode(xPanels, "panel", "filename", strFilename);
		//        if(xOldPanel != null)
		//        {
		//            xPanels.RemoveChild(xOldPanel);
		//            return true;
		//        }
		//    }
		//    Debug.Assert(false);
		//    return false;
		//}

		//internal bool RemoveAllPanels()
		//{
		//    XmlElement xPanels = m_xConfig.SelectSingleNode("/root/panels") as XmlElement;

		//    if(xPanels != null)
		//    {
		//        xPanels.RemoveAll();
		//        return true;
		//    }
		//    Debug.Assert(false);
		//    return false;
		//}

		//public List<string> GetPanelFilenames()
		//{
		//    XmlElement xPanels = m_xConfig.SelectSingleNode("/root/panels") as XmlElement;

		//    if(xPanels != null)
		//    {
		//        XmlNodeList xNodes = xPanels.SelectNodes("panel");
		//        if(xNodes != null)
		//        {
		//            List<string> listRetval = new List<string>(xNodes.Count);

		//            foreach(XmlElement xPanel in xNodes)
		//            {
		//                string strFilename = XmlHelper.GetKeyValue(xPanel, "filename");
		//                if(strFilename != null && strFilename != "")
		//                {
		//                    listRetval.Add(strFilename);
		//                }
		//            }
		//            return listRetval;
		//        }
		//    }
		//    return null;
		//}









		public XmlElement xPanels {get {return m_xConfig.SelectSingleNode("/root/panels") as XmlElement;}}
	}
}
