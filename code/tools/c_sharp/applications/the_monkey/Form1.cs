using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Global;
using System.Xml;
using System.Diagnostics;

namespace RoboPublish
{
	public partial class Form1 : Form
	{
		private const string APPLICATION_CONFIG_FILE = "ApplicationConfig.xml";

		ConfigManager m_configManager;

		public Form1(string[]args)
		{
			InitializeComponent();

			m_configManager = new ConfigManager(GetApplicationConfigPathFileName());

			if(m_configManager.Initialise() == false)
			{
				throw new ApplicationException("Cannot initialise the ConfigManager.");
			}


			this.Show();

			if(args.Length != 0)
			{
				List<string> listArgs = new List<string>(args.Length);
				foreach(string strTemplateName in args)
				{
					listArgs.Add(strTemplateName);
				}
				OpenPanels(listArgs);
			}
			else
			{
				LoadLastConfiguration();
			}
		}
		
		/// <summary>
		/// This is used to intercept keystrokes on the tree view in
		/// order to e.g. process the 'delete' key.
		/// </summary>
		/// <param name="keyData"></param>
		/// <returns></returns>
		protected override bool ProcessDialogKey(System.Windows.Forms.Keys keyData)
		{
			System.Windows.Forms.Keys key = keyData;

			if(key == System.Windows.Forms.Keys.F5)
			{
				foreach(RoboPanel treePanel in m_treePanels)
				{
					treePanel.RefreshRoboTree();
				}
				return true;
			}
			return base.ProcessDialogKey(keyData);
		}

		string GetApplicationConfigPathFileName()
		{
			int index = Application.ExecutablePath.LastIndexOf('\\');

			string retval = Application.ExecutablePath.Substring(0, index + 1) + APPLICATION_CONFIG_FILE;
			return retval;
		}

		private bool LoadLastConfiguration()
		{
			XmlElement xPanels = m_configManager.m_xConfig.SelectSingleNode("/root/panels") as XmlElement;

			if(xPanels != null)
			{
				XmlNodeList xNodes = xPanels.SelectNodes("panel");

				if(xNodes != null)
				{
					foreach(XmlElement xPanel in xNodes)
					{
						if(XmlHelper.GetKeyBool(xPanel, "do_not_load", false) == false)
						{
							string strFilename = XmlHelper.GetKeyValue(xPanel, "filename");
							if(strFilename != null && strFilename != "")
							{
								this.AddTreePanel(strFilename, xPanel["ConfigureSorceryParams"]);

							}
						}
					}
					LayoutPanels();
				}
			}
			return true;


			//return OpenPanels(projectList);
		}
		
		private bool OpenPanels(List<string>projectList)
		{
			try
			{
				if(projectList != null)
				{
					foreach(string strFilename in projectList)
					{
						// Load it.

						XmlElement xPanel = XmlHelper.FindNodeByKeyValue(m_configManager.xPanels.ChildNodes, "filename", strFilename, false);

						XmlElement xSorceryConfig = null;

						if(xPanel != null)
							xSorceryConfig = xPanel["ConfigureSorceryParams"];


						RoboPanel newTreePanel = this.AddTreePanel(strFilename, xSorceryConfig);


//						this.AddTreePanel(strFilename, true);
					}
					LayoutPanels();
				}

				return true;
			}
			catch(Exception ex)
			{
				MessageBox.Show("Failed to load the last configuration. The system error is:" + ex.Message);
			}
			return false;
		}
		
		List<RoboPanel> m_treePanels = new List<RoboPanel>();


		private RoboPanel AddTreePanel(string strFilename, XmlElement xSorceryConfig)
		{
			try
			{
				RoboPanel newTreePanel = new RoboPanel(strFilename, xSorceryConfig);
				newTreePanel.RoboPanelClosing += new EventHandler(newTreePanel_RoboPanelClosing);

				newTreePanel.toolStrip2.Visible = showAdvancedToolbarToolStripMenuItem.Checked;



				m_treePanels.Add(newTreePanel);
				return newTreePanel;
				//LayoutPanels();
			}
			catch (Exception ex)
			{
				MessageBox.Show(this, "Cannot create a panel from the file '" + strFilename + "'\n" +
					"The system reports: '" + ex.Message + "'",
					"The Monkey",
					MessageBoxButtons.OK, MessageBoxIcon.Error);

				return null;
			}
		}

		void newTreePanel_RoboPanelClosing(object sender, EventArgs e)
		{
			RoboPanel panelFrom = sender as RoboPanel;

			this.m_treePanels.Remove(panelFrom);
			LayoutPanels();
		}




		private void LayoutPanels()
		{
			this.panelPanels.Controls.Clear();

			for(int c = 0; c < m_treePanels.Count; c++)
			{
				RoboPanel treePanel = m_treePanels[c];

				if(c != m_treePanels.Count - 1)
				{
					Splitter splitter = new System.Windows.Forms.Splitter();
					splitter.Size = new System.Drawing.Size(3, 721);
					splitter.TabStop = false;
					this.panelPanels.Controls.Add(splitter);
					treePanel.Dock = DockStyle.Left;
					this.panelPanels.Controls.Add(treePanel);
				}
				else
				{
					treePanel.Dock = DockStyle.Fill;
					this.panelPanels.Controls.Add(treePanel);
					treePanel.BringToFront();
				}
			}
		}

		bool OpenXmlConfigFile(bool bAppend)
		{
			OpenFileDialog ofn = new OpenFileDialog();

			ofn.ReadOnlyChecked = true;

			if(ofn.ShowDialog(this) == DialogResult.OK)
			{
				try
				{
					if(bAppend == false)
					{
						m_treePanels.Clear();
					}

					RoboPanel newTreePanel = this.AddTreePanel(ofn.FileName, null);
					m_treePanels.Add(newTreePanel);

					return true;
				}
				catch(ApplicationException ex)
				{
					MessageBox.Show(this, ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
				}
				catch(Exception ex)
				{
					MessageBox.Show(this, ex.Message + "\nStack trace: " + ex.StackTrace + "\n\nAborting.", "Unhandled & Unexpected Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
				}
			}
			return false;
		}

		private void openToolStripMenuItem_Click(object sender, EventArgs e)
		{
			if(m_treePanels.Count > 0)
			{
				DialogResult dr = MessageBox.Show(this, "Are you sure you wish to close the existing panel"+((m_treePanels.Count == 1) ? "" : "s")+"?",
				Application.ProductName, MessageBoxButtons.YesNo);

				if(dr != DialogResult.Yes)
					return;
			}

			OpenXmlConfigFile(false);
			LayoutPanels();
		}

		private void appendToolStripMenuItem_Click(object sender, EventArgs e)
		{
			OpenXmlConfigFile(true);
			LayoutPanels();
		}

		private void Form1_FormClosing(object sender, FormClosingEventArgs e)
		{
			XmlElement xPanels = m_configManager.xPanels;

			Debug.Assert(xPanels != null);

			if(xPanels != null)
			{
				//xPanels.RemoveAll();

				foreach(XmlElement xPanel in xPanels)
				{
					if(xPanel != null)
						XmlHelper.AddAttribute(xPanel, "do_not_load", "true");
				}


				foreach(RoboPanel roboPanel in m_treePanels)
				{
					XmlElement xPanel = XmlHelper.FindNodeByKeyValue(m_configManager.xPanels.ChildNodes, "filename", roboPanel.m_strFilename, false);

					XmlHelper.AddAttribute(xPanel, "do_not_load", "false");

					if(xPanel != null)
						m_configManager.xPanels.RemoveChild(xPanel);

					m_configManager.AddPanel(roboPanel);
				}
				m_configManager.SaveConfig();
			}
		}

		private void exitToolStripMenuItem_Click(object sender, EventArgs e)
		{

			this.Close();
		}

		private void helpToolStripMenuItem1_Click(object sender, EventArgs e)
		{
			Help h = new Help();
			h.ShowDialog(this);
		}

		private void aboutToolStripMenuItem_Click(object sender, EventArgs e)
		{
			About a = new About();
			a.ShowDialog();
		}

		private void showAdvancedToolbarToolStripMenuItem_Click(object sender, EventArgs e)
		{
			foreach(RoboPanel panel in m_treePanels)
			{
				panel.toolStrip2.Visible = showAdvancedToolbarToolStripMenuItem.Checked;
			}
		}
	}
}