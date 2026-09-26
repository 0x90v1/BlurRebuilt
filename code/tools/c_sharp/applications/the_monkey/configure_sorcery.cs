using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Global;
using System.IO;

namespace RoboPublish
{
	public partial class ConfigureSorcery : Form
	{

		public class ConfigureSorceryParams
		{
			public ConfigureSorceryParams()
			{
				m_strSorceryDatabase = "";
				m_sorceryQueryItems = new List<string>();
				m_bRadioButtonMatches = false;
				m_bStripExtensions = true;
			}

			public string m_strSorceryDatabase;
			public List<string> m_sorceryQueryItems;

			public bool m_bRadioButtonMatches = false;
			public bool m_bStripExtensions = false;
			public string m_StrSorceryQuery
			{
				get
				{
					string strResult = "";
					foreach(string strQuery in m_sorceryQueryItems)
					{
						if(strResult != "")
							strResult += " ";
						strResult += strQuery;
					}
					return strResult;
				}
			}

			internal void SetDefaults()
			{
				this.m_strSorceryDatabase = @"z:\shitstorm\sorcery2";

				this.m_sorceryQueryItems.Clear();

				this.m_sorceryQueryItems.Add("GameModel.Filename");
				this.m_sorceryQueryItems.Add("GameAnimSkin.Filename");
				this.m_sorceryQueryItems.Add("GameAnimSkin.LOD2Filename");
				this.m_sorceryQueryItems.Add("GameAnimSkin.LOD3Filename");

				this.m_sorceryQueryItems.Add("GameAnimSkeleton.Filename");
				this.m_sorceryQueryItems.Add("GameAnimSkeleton.LOD2Filename");
				this.m_sorceryQueryItems.Add("GameAnimSkeleton.LOD3Filename");

				this.m_sorceryQueryItems.Add("GameTexture.Filename");
				this.m_sorceryQueryItems.Add("GameAnimCycle.Filename");

				this.m_sorceryQueryItems.Add("AudioObject.ObjectFilename");

				this.m_sorceryQueryItems.Add("RagdollModels.CollideModelFileName");
				this.m_sorceryQueryItems.Add("RagdollModels.GenerationModelFileName");
				this.m_sorceryQueryItems.Add("RagdollModels.MomentsDataFileName");

			}
		}


		ConfigureSorceryParams m_csp;

		public ConfigureSorcery(ConfigureSorceryParams csp)
		{
			InitializeComponent();
			////////////////
			m_csp = csp;
		}

		private void buttonCancel_Click(object sender, EventArgs e)
		{
			DialogResult = DialogResult.Cancel;
			this.Close();
		}



		private void buttonBrowseFile_Click(object sender, EventArgs e)
		{
			OpenFileDialog ofd = new OpenFileDialog();
			ofd.CheckFileExists = false;

			this.textBoxSorceryDatabase.Text = this.textBoxSorceryDatabase.Text.Replace('/', '\\');
			ofd.FileName = this.textBoxSorceryDatabase.Text;

			ofd.Filter = "All files (*.*)|*.*|Xml files (*.xml)|*.xml|Gay files(*.gay)|*.gay";

			if(File.Exists(this.textBoxSorceryDatabase.Text) == false)
			{
				ofd.FileName = "";
				string strDirectory = null;

				try
				{
					strDirectory = Path.GetDirectoryName(this.textBoxSorceryDatabase.Text);
				}
				catch { }

				ofd.InitialDirectory = strDirectory;
			}

			DialogResult dr = ofd.ShowDialog(this);

			if(dr == DialogResult.OK)
			{
				this.textBoxSorceryDatabase.Text = ofd.FileName;
				this.textBoxSorceryDatabase.Select(this.textBoxSorceryDatabase.Text.Length, 0);
			}
		}

		private void buttonOk_Click(object sender, EventArgs e)
		{
			m_csp.m_sorceryQueryItems.Clear();

			foreach(ListViewItem lvi in listViewSorceryQuery.Items)
			{
				m_csp.m_sorceryQueryItems.Add(lvi.Text);
			}

			m_csp.m_strSorceryDatabase = textBoxSorceryDatabase.Text;

			m_csp.m_bRadioButtonMatches = radioButtonMatches.Checked;
			//m_csp.m_bRadioButtonMatches = !radioButtonMismatches.Checked;
			m_csp.m_bStripExtensions = checkBoxStripExtensions.Checked;

			this.DialogResult = DialogResult.OK;
			this.Close();
		}






		private void ConfigureSorcery_Load(object sender, EventArgs e)
		{
			SetValues();

		}

		private void SetValues()
		{
			textBoxSorceryDatabase.Text = m_csp.m_strSorceryDatabase;

			listViewSorceryQuery.Items.Clear();

			foreach(string str in m_csp.m_sorceryQueryItems)
			{
				listViewSorceryQuery.Items.Add(str);
			}

			radioButtonMatches.Checked = m_csp.m_bRadioButtonMatches;
			radioButtonMismatches.Checked = !m_csp.m_bRadioButtonMatches;

			checkBoxStripExtensions.Checked = m_csp.m_bStripExtensions;
		}

		private void buttonAddQuery_Click(object sender, EventArgs e)
		{
			string strQuery = textBoxSorcefyQueryLine.Text.Trim();

			if(strQuery != "")
			{
				listViewSorceryQuery.Items.Add(strQuery);
			}
		}

		private void buttonRemoveQuery_Click(object sender, EventArgs e)
		{

			if(listViewSorceryQuery.SelectedItems.Count > 0)
			{
				List<ListViewItem> itemsToRemove = new List<ListViewItem>(listViewSorceryQuery.Items.Count);

				foreach(ListViewItem lvi in listViewSorceryQuery.SelectedItems)
				{
					itemsToRemove.Add(lvi);
				}

				foreach(ListViewItem lvi in itemsToRemove)
				{
					listViewSorceryQuery.Items.Remove(lvi);

				}
			}
		}

		private void radioButtonMatches_CheckedChanged(object sender, EventArgs e)
		{
			//m_csp.m_bRadioButtonMatches = radioButtonMatches.Checked;
		}

		private void radioButtonMismatches_CheckedChanged(object sender, EventArgs e)
		{
			//m_csp.m_bRadioButtonMatches = !radioButtonMismatches.Checked;
		}

		private void checkBoxStripExtensions_CheckedChanged(object sender, EventArgs e)
		{
			//m_csp.m_bStripExtensions = checkBoxStripExtensions.Checked;
		}

		private void buttonDefault_Click(object sender, EventArgs e)
		{
			this.m_csp.SetDefaults();
			SetValues();
		}


	}
}