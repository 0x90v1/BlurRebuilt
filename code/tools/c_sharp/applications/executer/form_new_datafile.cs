using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Diagnostics;
using message_handler;

namespace Executer
{
	public partial class Form_NewDatafile : Form
	{
		string strFileThatCanBeSafelyOverwritten = null;

		static string s_strTemplateFolder=null;
		static string s_strProjectFolder=null;

		public Form_NewDatafile()
		{
			InitializeComponent();

			this.Text = Program.AssemblyTitle;
		}

		public string DataFile
		{
			get
			{
				return this.textBox_dataFile.Text;
			}
			set
			{
				this.textBox_dataFile.Text = value;
			}
		}


		public string TemplateFile
		{
			get
			{
				return this.textBox_templateFile.Text;
			}
			set
			{
				this.textBox_templateFile.Text = value;
			}
		}

		public bool CreateDefaultTree
		{
			get { return this.checkBox_createDefaultTree.Checked; }
			set { this.checkBox_createDefaultTree.Checked = value; }
		}


		private void button_browseDatafile_Click(object sender, EventArgs e)
		{
			DataFile = GetSaveFilename(DataFile, true);
		}

		private void button_browseTemplate_Click(object sender, EventArgs e)
		{
			TemplateFile = GetOpenFilename(TemplateFile, true);
		}

		private string GetSaveFilename(string strFilename, bool bOverwritePrompt)
		{
			SaveFileDialog sfd = new SaveFileDialog();
			string strOldFilename = strFilename;

			sfd.OverwritePrompt = bOverwritePrompt;

			if(strFilename == null || strFilename.Length == 0)
				sfd.InitialDirectory = s_strProjectFolder;
			else
				sfd.InitialDirectory = strFilename;

            sfd.Filter = "Executer Project Files (*.xpr;*.xml)|*.xpr;*.xml|All Files (*.*)|*.*";
			sfd.FilterIndex = 1;
			sfd.RestoreDirectory = true;

			if (sfd.ShowDialog() == DialogResult.OK)
			{
				s_strProjectFolder = sfd.FileName;
				this.strFileThatCanBeSafelyOverwritten = sfd.FileName;		// Either the file does not exist or the user has confirmed
				return sfd.FileName;										// that it is okay to overwrite it.
			}
			this.strFileThatCanBeSafelyOverwritten = null;
			return strOldFilename;
		}

		private string GetOpenFilename(string strFilename, bool bCheckFileExists)
		{
			OpenFileDialog ofd = new OpenFileDialog();
			string strOldFilename = strFilename;

			ofd.CheckFileExists = bCheckFileExists;

			if(strFilename == null || strFilename.Length == 0)
				ofd.InitialDirectory = s_strTemplateFolder;
			else
				ofd.InitialDirectory = strFilename;

            ofd.Filter = "Executer Template Files (*.xtm;*.xml)|*.xtm;*.xml|All Files (*.*)|*.*";
			ofd.FilterIndex = 1;
			ofd.RestoreDirectory = true;

			if (ofd.ShowDialog() == DialogResult.OK)
			{
				s_strTemplateFolder = ofd.FileName;
				return ofd.FileName;
			}
			return strOldFilename;
		}


		private void buttonCancel_Click(object sender, EventArgs e)
		{
			this.DialogResult = DialogResult.Cancel;
		}

		private void buttonOk_Click(object sender, EventArgs e)
		{
			if ((File.Exists(this.textBox_dataFile.Text) == true) && (strFileThatCanBeSafelyOverwritten != this.textBox_dataFile.Text))
			{
				DialogResult dr = MessageHandler.Show(this, "The project filename already exists. Do you wish to overwrite it?", "Executer.", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
				if(dr != DialogResult.Yes)
				{
					//s_strProjectFolder = Path.GetDirectoryName(this.textBox_dataFile.Text);
					//s_strTemplateFolder = Path.GetDirectoryName(this.textBox_templateFile.Text);

					return;
				}
			}

			if (File.Exists(this.textBox_templateFile.Text) == false)
			{
				MessageHandler.Show(this, "The template file you have selected does not exist.", "Executer", MessageBoxButtons.OK, MessageBoxIcon.Error);
			}
			else
				this.DialogResult = DialogResult.OK;
		}

	}
}