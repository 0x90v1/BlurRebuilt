using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using Global;
using System.Diagnostics;
using System.IO;
using System.Threading;
using System.Text.RegularExpressions;

namespace RoboPublish
{
	public partial class RoboPanel : UserControl
	{
		public readonly string m_strFilename;
		int m_numberOfSkus;

		//private List<string> m_listExcludeFromSourceFilespec;
		//private List<string> m_listDoNotDeleteFilespec;
		private Regex m_regexDoNotDeleteFilespec;
		private bool m_bDeleteSourceFilesAfterCopy;
		//private string m_strExcludeFromSourceFilespec;
		private Regex m_regexExcludeFromSourceFilespec;

		List<Sku> m_skus;

		Dictionary<string, string> m_typeNameLookup;

		bool m_bCopyOlderFiles;
		bool m_bCopySameFiles;
		bool m_bCopyNewerFiles;


		public event EventHandler RoboPanelClosing;

		protected void OnRoboPanelClosing(EventArgs e)
		{
			if(RoboPanelClosing != null)
			{
				RoboPanelClosing(this, e);
			}
		}


		//public enum MessageFromChild
		//{
		//    CloseMe = 0
		//}


//		public delegate bool DelegateMessageFromChild(RoboPanel me, MessageFromChild mfc);

//		DelegateMessageFromChild m_dMfc;

		public ConfigureSorcery.ConfigureSorceryParams m_csp = new ConfigureSorcery.ConfigureSorceryParams();

		XmlElement xConfigPanel;

		System.Xml.Serialization.XmlSerializer configSerialiser;

		public XmlElement Serialize()
		{
			StringBuilder sb = new StringBuilder();
			StringWriter sw = new StringWriter(sb);
			configSerialiser.Serialize(sw, m_csp);

			XmlDocument xdoc = new XmlDocument();
			xdoc.LoadXml(sb.ToString());
			return xdoc["ConfigureSorceryParams"];
		}

		MatchForm m_matchForm;


		public RoboPanel(string strFilename, XmlElement xSorceryConfig)
		{
			InitializeComponent();

			m_matchForm = new MatchForm();



			//m_xPanel = xPanel;

			configSerialiser = new System.Xml.Serialization.XmlSerializer(m_csp.GetType());

			// xPanel will be null if there is no configuration data for this panel.

			if(xSorceryConfig != null)
			{
				m_csp = (ConfigureSorcery.ConfigureSorceryParams)configSerialiser.Deserialize(new StringReader(xSorceryConfig.OuterXml));
			}
			else
			{
				m_csp.SetDefaults();

				//m_csp.m_sorceryQueryItems.Add("Particle_Effect.diffuseTexture");		// Default for Ken?
			}
			this.panel_Feedback.Dock = DockStyle.Fill;

			this.panelParseProgress.Dock = DockStyle.Fill;

			m_strFilename = strFilename;
//			m_strFilename = XmlHelper.GetKeyValue(xPanel, "filename");

///111
			//this.panelParseProgress.BringToFront();
			//this.panelParseProgress.Show();
			////this.CreateControl();
			//this.Show();
			//this.Update();
			//Initialise();
		}

//		Thread initialiseThread;
		int m_templateVersion;
		string m_strApplicationInfoFile;
		string m_strTag;

		private void Initialise()
		{
			XmlDocument xDoc = new XmlDocument();

			xDoc.Load(m_strFilename);				/// May throw an exception.

			XmlElement xRoot = xDoc.SelectSingleNode("/root") as XmlElement;
			XmlElement xPanel = xDoc.SelectSingleNode("/root/panel") as XmlElement;
			XmlNodeList xSkus = xDoc.SelectNodes("/root/panel/sku");
			XmlNodeList xFileTypesList = xDoc.SelectNodes("/root/panel/file_types/file_type");

			if(xRoot == null) throw new ApplicationException("The document '" + m_strFilename + "' does not contain the element '/root'");
			if(xPanel == null) throw new ApplicationException("The document '" + m_strFilename + "' does not contain the element '/root/panel'");
			if(xSkus == null) throw new ApplicationException("The document '" + m_strFilename + "' does not contain any sku' elements in '/root/panel/'");
			if(xSkus.Count == 0) throw new ApplicationException("The document '" + m_strFilename + "' does not contain any sku' elements in '/root/panel/'");

			m_templateVersion = (int)(XmlHelper.GetKeyFloat(xRoot, "template_version", 0) * 100);
			m_strApplicationInfoFile = XmlHelper.GetKeyValue(xRoot, "application_info_file");
			m_strTag = XmlHelper.GetKeyValue(xRoot, "tag");
			if(Tag == null) Tag = "default";


			string strDoNotDeleteFilespec = XmlHelper.GetKeyValue(xPanel, "do_not_delete_filespec");
			m_bDeleteSourceFilesAfterCopy = XmlHelper.GetKeyBool(xPanel, "delete_source_after_copy");
			string strExcludeFromSourceFilespec = XmlHelper.GetKeyValue(xPanel, "exclude_from_source_filespec");
			string strButtonName = XmlHelper.GetKeyValue(xPanel, "button_name");
			if(strButtonName != null)
				this.buttonPublish.Text = strButtonName;

			//m_listExcludeFromSourceFilespec = GetQuotedStrings(m_strExcludeFromSourceFilespec, true);
			m_regexExcludeFromSourceFilespec = new Regex(strExcludeFromSourceFilespec, RegexOptions.IgnoreCase);
			//m_listDoNotDeleteFilespec = GetQuotedStrings(strDoNotDeleteFilespec, true);
			m_regexDoNotDeleteFilespec = new Regex(strDoNotDeleteFilespec, RegexOptions.IgnoreCase);

			m_bCopyOlderFiles = XmlHelper.GetKeyBool(xPanel, "copy_older_files", false);
			m_bCopySameFiles = XmlHelper.GetKeyBool(xPanel, "copy_same_files", false);
			m_bCopyNewerFiles = XmlHelper.GetKeyBool(xPanel, "copy_newer_files", false);
			
			m_numberOfSkus = 0;

			m_typeNameLookup = new Dictionary<string, string>(xFileTypesList.Count);

			foreach(XmlElement xFileType in xFileTypesList)
			{
				string strType = XmlHelper.GetKeyValue(xFileType, "type").ToLower();
				string strName = XmlHelper.GetKeyValue(xFileType, "friendly_name");

				m_typeNameLookup.Add(strType, strName);
			}

			m_skus = new List<Sku>();

			foreach(XmlElement xSku in xSkus)
			{
				string strSkuName = XmlHelper.GetKeyValue(xSku, "name");

				Sku sku = new Sku();

				{
					XmlElement xPaths = xSku["paths"];
					foreach(XmlElement xPath in xPaths)
					{
						if(xPath != null && xPath.Name == "path")
						{
							try
							{
								sku.AddPath(xPath);
							}
							catch (Exception ex)
							{
								MessageBox.Show(this, "Cannot parse a path entry. System reports:\n'" + ex.Message + "'", "Monkey Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
							}
						}
					}

					m_skus.Add(sku);

					XmlElement xExtensions = xSku["extensions"];

					if(xExtensions != null)
					{
						foreach(XmlElement xExtension in xExtensions)
						{
							string strExt = XmlHelper.GetKeyValue(xExtension, "ext").ToLower();
							if(strExt[0] != '.')
								strExt = "." + strExt;

							string strType = XmlHelper.GetKeyValue(xExtension, "type");

							sku.strExtensionsLookup.Add(strExt, strType);
						}
					}

					ToolStripMenuItem tdi = toolStripDropDownButtonFilterSKU.DropDownItems.Add(strSkuName) as ToolStripMenuItem;
					tdi.Click += new EventHandler(RoboPanel_Click);
					tdi.CheckOnClick = true;
					tdi.Checked = true;
					tdi.Tag = sku;
					m_numberOfSkus++;
				}

				sku.strSkuName = strSkuName;

				//Debug.WriteLine("Adding: Source '" + strSource + "', Dest '" + strDest + "'");

				this.roboTree.m_skus = m_skus;
			}

			string strPanelName = XmlHelper.GetKeyValue(xPanel, "name");
			this.labelName.Text = strPanelName;

			VersionCheck();

			BuildTree();
		}

		bool VersionCheck()
		{
			if(m_strApplicationInfoFile != null)
			{
				try
				{
					XmlDocument xApplicationInfoDocument;
					try
					{
						xApplicationInfoDocument = new XmlDocument();
						xApplicationInfoDocument.Load(m_strApplicationInfoFile);
					}
					catch(Exception ex)
					{
						MessageBox.Show(this, "You cannot publish any data because the remote versioning information is not available. System reports:\n'" + ex.Message + "'", "Monkey Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
						return false;
					}

					string strLookup = "/application_info/applications/application[@name='" + Application.ProductName + "']/tag[@name='art_monkey_template']/flags";///@name="+Application.ProductName+"/"+m_strTag;
					XmlElement xTag = xApplicationInfoDocument.SelectSingleNode(strLookup) as XmlElement;

					if(xTag == null)
					{
						strLookup = "/application_info/applications/application[@name='" + Application.ProductName + "']/tag/[@name='default'/flags";
						xTag = xApplicationInfoDocument.SelectSingleNode(strLookup) as XmlElement;
					}

					if(xTag != null)
					{
						if(XmlHelper.GetKeyBool(xTag, "suspend_publishing", false) == true)
						{
							string strReason = XmlHelper.GetKeyValue(xTag, "reason_for_suspension");
							MessageBox.Show(this, "You cannot publish any data because your administrator has suspended publishing rights.\n"+
								(strReason == null || strReason.Length == 0 ? "No reason is given." : "The reason given is\n'"+strReason+"'"), "Monkey Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
							return false;
						}

						int requiredTemplateVersion = (int)(XmlHelper.GetKeyFloat(xTag, "min_version", 0) * 100);

						if(requiredTemplateVersion > m_templateVersion)
						{
							MessageBox.Show(this, "You cannot publish any data because your Monkey template is out of date.\nPlease update your tools.", "Monkey Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
							return false;
						}
						else
							return true;
					}

					return true;
				}
				catch(Exception ex)
				{
					MessageBox.Show(this, "Cannot perform template version check. System reports:\n'" + ex.Message + "'", "Monkey Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
					return false;
				}
			}
			return true;
		}
		




		void RoboPanel_Click(object sender, EventArgs e)
		{
			ToolStripMenuItem tdi = sender as ToolStripMenuItem;

			Sku pairEntry = tdi.Tag as Sku;
			pairEntry.m_disabled = !tdi.Checked;
			TreeObject toRoot = this.roboTree.Nodes[0] as TreeObject;
			roboTree.Recurse(toRoot, new RoboTree.TreeNodeActionDelegate(SetTtiEnabledState), RoboTree.Flags.All, null);
			this.roboTree.Invalidate();
		}

		bool BuildTree()
		{
			this.roboTree.Nodes.Clear();							// Start fresh.

			TreeObject toRoot = new TreeObject(m_numberOfSkus, "root", "ignored", Color.Black);
			this.roboTree.Nodes.Add(toRoot);

			roboTree.SelectedNode = toRoot;
			roboTree.Focus();
			RefreshRoboTree();

			return true;
		}

		int m_FileCountSoFar = 0;


		string m_strCurrentSkuPathName;
		//public string m_strSourcePath;
		//public string m_strDestPath;

		Regex m_regexCurrentSkuDirectory;
		Regex m_regexCurrentSkuFile;
		//private string m_strFolderMask;
		//private string m_strFileMask;

		int m_skuCurrentSkuPathCount;
		bool m_bCurrentSkuMergeFolders;

		int m_currentSkuIndex;

		Color m_currentSkuPathColour;

		Regex m_regexCurrentSkuDestPathPatternToReplace;
		string m_strCurrentSkuDestPathPatternReplacement;

		string m_strCurrentSkuDestPathBase;

		private bool PopulateTree(int skuIndex)
		{
			int count = 0;

			m_currentSkuIndex = skuIndex;

			foreach(SkuPath skuPath in m_skus[skuIndex].m_listSkuPaths)
			{
				m_strCurrentSkuPathName		= skuPath.m_strPathName;
				m_regexCurrentSkuDirectory	= skuPath.m_regexDirectory;
				m_regexCurrentSkuFile			= skuPath.m_regexFile;
				m_skuCurrentSkuPathCount		= count;
				m_bCurrentSkuMergeFolders		= skuPath.m_bMergeFolders;
				m_currentSkuPathColour = skuPath.m_colour;

				m_regexCurrentSkuDestPathPatternToReplace = skuPath.m_regexDestPathPatternToReplace;
				m_strCurrentSkuDestPathPatternReplacement = skuPath.m_strDestPathPatternReplacement;

				m_strCurrentSkuDestPathBase = skuPath.m_strDestPath;

				_PopulateTree(this.roboTree.Nodes[0].Nodes, skuPath.m_strSourcePath, skuPath.m_strDestPath);

				count++;
			}
			return true;
			//return _PopulateTree(treeNodes, skuPath.m_strPathName, skuPath.m_strSourcePath, skuPath.m_strDestPath, skuPath.m_strFolderMask, skuPath.m_strFileMask, index);
		}

		private bool _PopulateTree(TreeNodeCollection treeNodes, string strCurrentSourcePath, string strCurrentDestPath)
		{
			try
			{
				string strCurrentSourcePath_Remapped = strCurrentSourcePath;

				if (strCurrentDestPath.Contains("traininglevel"))
				{
					Debug.WriteLine("333");
				}
				if (m_regexCurrentSkuDestPathPatternToReplace != null)
				{
					//if (m_regexCurrentSkuDestPathPatternToReplace.IsMatch(strCurrentDestPath.ToLower(), m_strCurrentSkuDestPathBase.Length) == true)
					if (m_regexCurrentSkuDestPathPatternToReplace.IsMatch(strCurrentDestPath.Substring(m_strCurrentSkuDestPathBase.Length)) == true)
					{
						strCurrentDestPath = m_regexCurrentSkuDestPathPatternToReplace.Replace(strCurrentDestPath, m_strCurrentSkuDestPathPatternReplacement, 1, m_strCurrentSkuDestPathBase.Length);
						m_regexCurrentSkuDestPathPatternToReplace = null;		// Only replace once per path.
					}
				}

				Dictionary<string, string> ExtensionsLookup = m_skus[m_currentSkuIndex].strExtensionsLookup;

				DirectoryInfo di = new DirectoryInfo(strCurrentSourcePath_Remapped);
				FileSystemInfo[] dirs = di.GetDirectories();
				FileSystemInfo[] files = di.GetFiles();
				foreach(FileSystemInfo[] fsia in new object[] { dirs, files })
				{
					foreach(FileSystemInfo fsi in fsia)
					{
						if(m_regexExcludeFromSourceFilespec.IsMatch(fsi.FullName) == false)
						{
							bool bIsDirectory = ((fsi.Attributes & FileAttributes.Directory) == FileAttributes.Directory);

							#region debug
							if(fsi.FullName.ToLower() == @"Z:\shitstorm\Working\pc\Data\image\levels\traininglevel\artlevel0000.pcr".ToLower())
							{
								Debug.WriteLine("222");
							}
							#endregion debug

							// Get the name to search for in the tree.
							string strTreeSearchName = fsi.Name;

							// If it is a file, see if we need to remap this name.
							if(bIsDirectory == false)
							{
								string strExtension = fsi.Extension.ToLower();
								if(strExtension != null)
								{
									if(ExtensionsLookup.ContainsKey(strExtension))
									{
										string strTypeName = ExtensionsLookup[strExtension];
										string strFriendlyTypeName = m_typeNameLookup[strTypeName];
										strTreeSearchName = "?" + Path.GetFileNameWithoutExtension(fsi.Name) + "." + strFriendlyTypeName;
									}
								}
							}

							// Find or create the tree object.
							TreeObject to = this.roboTree.FindTreeNodeByText(treeNodes, strTreeSearchName, m_strCurrentSkuPathName, m_bCurrentSkuMergeFolders);
							if(to == null)
							{
								to = new TreeObject(m_numberOfSkus, strTreeSearchName, m_strCurrentSkuPathName, m_currentSkuPathColour);
								treeNodes.Add(to);
							}

							TreeTagInfo tti = to.m_listTagInfo[m_currentSkuIndex];

							if(tti.m_fsiSource == null)			// If there's more than one sku pointing at this destination, keep the first one.
							{
								tti.m_fsiSource = fsi;
								Debug.Assert(tti.m_fsiDest == null, "tti.m_fsiDest == null");

								if(bIsDirectory == true)		// Assume directories are hidden until they are unhidden.
									tti.m_bhidden = true;		// This is needed because directories can be merged.
																// TODO: Should base this on m_regexDirectory? No. We have to check later anyway, so save ourselves a regex check.
							}
							else
							{
								if(bIsDirectory == false)
									throw new Exception("Existing tti.m_fsiSource. Duplicated path name in sku?");
							}

							// Get the FileSystemInfo for the destination.
							// If one already exists for this sku on this treenode (the treenode was filtered by path name), we have a problem.
							if(tti.m_fsiDest == null)
							{
								if(bIsDirectory == true)
									tti.m_fsiDest = new DirectoryInfo(strCurrentDestPath + '\\' + fsi.Name);	// TODO: If a file exists with this name, we have a problem.
								else
									tti.m_fsiDest = new FileInfo(strCurrentDestPath + '\\' + fsi.Name);			// TODO: If a directory exists with this name, we have a problem.
							}
							else
							{
								// Duplicated path name in sku. Error.
								if(bIsDirectory == false)
									throw new Exception("Existing tti.m_fsiDest. Duplicated path name in sku?");
							}

							if(bIsDirectory == true)
							{
								//tti.m_bhidden = !m_regexDirectory.IsMatch(fsi.FullName);
								if(m_regexCurrentSkuDirectory.IsMatch(fsi.FullName) == true)
									tti.m_bhidden = false;

								_PopulateTree(to.Nodes, strCurrentSourcePath_Remapped + '\\' + fsi.Name, strCurrentDestPath + '\\' + fsi.Name);

								if(tti.m_bhidden == false)
								{
									TreeObject toParent = (TreeObject)(to.Parent);

									while(toParent != null)
									{
										toParent.m_listTagInfo[m_currentSkuIndex].m_bhidden = false;
										toParent = ((TreeObject)toParent.Parent);
									}
								}
							}
							else // It is a file ...
							{
								to.m_bIsFolder = false;
								TreeObject toParent = (TreeObject)(to.Parent);
								if(((TreeObject)to.Parent).m_listTagInfo[m_currentSkuIndex].m_bhidden == true)
								{
									tti.m_bhidden = true;
								}
								else
								{
									bool bHidden = !m_regexCurrentSkuDirectory.IsMatch(Path.GetDirectoryName(fsi.FullName));

									if(bHidden == false)
										bHidden = !m_regexCurrentSkuFile.IsMatch(Path.GetFileName(fsi.FullName));

									tti.m_bhidden = bHidden;
								}
							}


							if(fsi.FullName.ToLower() == @"Z:\shitstorm\WorkingTest\360\Data\Image\Objects\another file.thing".ToLower())
							{
								Debug.WriteLine("222");
							}


							if(tti.m_bhidden == false)
							{
								to.m_oldFlag = false;

								TreeObject toParent = (TreeObject)(to.Parent);

								while(toParent != null && toParent.m_oldFlag == true)
								{
									toParent.m_oldFlag = false;
									toParent = ((TreeObject)toParent.Parent);
								}
							}
							tti.m_iconIndex = GetTreeIcon(tti);
						}
					}

					m_FileCountSoFar += files.Length;
					if(m_FileCountSoFar / 50 > m_oldFileCountSoFar / 50)
					{
						this.labelParseProgress.Text = m_FileCountSoFar.ToString();
						this.labelParseProgress.Invalidate(true);
						this.labelParseProgress.Update();
						m_oldFileCountSoFar = m_FileCountSoFar;
					}

				}
			}
			catch(Exception e)
			{
				Console.WriteLine("The process failed: {0}", e.ToString());
				//				MessageBox.Show(this.Parent, "Failed to populate the tree.\n\nThe system reports:'" + e.ToString(), "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
			}
			return true;
		}
	
		
		
		/*
		bool IsFilePathContainedIn(string pathFile, List<string> pathList)
		{
			foreach(string strMask in pathList)
			{
				if(pathFile.ToLower().IndexOf(strMask.ToLower()) != -1)
					return true;
			}
			return false;
		}
*/
		/// <summary>
		/// Takes a string of quoted strings and splits it into a list of strings.
		/// </summary>
		/// <param name="strSource">string of quoted strings</param>
		/// <returns>ArrayList of strings</returns>
		private List<string> GetQuotedStrings(string strSource, bool bMakeLowerCase)
		{
			List<string> ar = new List<string>();
			int index = 0;

			if(strSource.Length != 0)
			{
				if(strSource.IndexOf('\'', 0) == -1)
				{
					ar.Add(strSource);
				}
				else
				{
					while((index = strSource.IndexOf('\'', index)) != -1)
					{
						int end;
						if((end = strSource.IndexOf('\'', index + 1)) != -1)
						{
							// Get the string.
							string strString = strSource.Substring(index + 1, end - index - 1);
							if(bMakeLowerCase == true)
								ar.Add(strString.ToLower());
							else
								ar.Add(strString);
							index = end + 1;
						}
						else
							throw new Exception("The string\n'" + strSource + "'\nis badly formed.");
					}
				}
			}
			return ar;
		}

		private void roboTree_AfterSelect(object sender, TreeViewEventArgs e)
		{
			TreeObject to = e.Node as TreeObject;

			if(to != null)
			{
				int c = 0;
				foreach(TreeTagInfo tti in to.m_listTagInfo)
				{
					string strSource = tti.m_fsiSource == null ? "Nothing" : tti.m_fsiSource.FullName;
					string strDest = tti.m_fsiDest == null ? "Nothing" : tti.m_fsiDest.FullName;

					string strSourceType = tti.m_fsiSource == null ? "None" : tti.m_fsiSource.GetType().ToString();
					c++;
				}
			}
		}

		RoboTree.TreeItemIconIndex GetTreeIcon(TreeTagInfo tti)
		{

			if(tti != null)
			{
				return CompareFiles(tti.m_fsiSource, tti.m_fsiDest);
			}
			else
				throw new Exception("BAD. GetTreeIcon: TreeTagInfo does not exist.");
		}

		/// <summary>
		/// This is a hideously written function.
		/// </summary>
		/// <param name="fsiSource"></param>
		/// <param name="fsiDest"></param>
		/// <returns></returns>
		RoboTree.TreeItemIconIndex CompareFiles(FileSystemInfo fsiSource, FileSystemInfo fsiDest)
		{
			if((fsiSource == null) || (fsiSource.Exists == false))
				return RoboTree.TreeItemIconIndex.NoSourceFile;

			// Source exists. Destination might not.
			// Or the destination might exist but the FileSystemInfo is the wrong type (file/directory) so it pretends not to exist.

			Debug.Assert((int)fsiSource.Attributes != -1);

			if(fsiDest.Exists == true)
				if((fsiSource.Attributes & FileAttributes.Directory) != (fsiDest.Attributes & FileAttributes.Directory))
					return RoboTree.TreeItemIconIndex.FileFolderMismatch;

			if((fsiSource.Attributes & FileAttributes.Directory) == FileAttributes.Directory)
			{
				return RoboTree.TreeItemIconIndex.Folder;
			}
			// Here, the source exists and the dest is of the same type. The dest may not exist.
			if(fsiDest.Exists)
			{
				if(fsiSource.LastWriteTime.Equals(fsiDest.LastWriteTime))
					return RoboTree.TreeItemIconIndex.Same;
				else if(fsiSource.LastWriteTime.CompareTo(fsiDest.LastWriteTime) > 0)
					return RoboTree.TreeItemIconIndex.SourceFileIsNewer;
				else
					return RoboTree.TreeItemIconIndex.SourceFileIsOlder;
			}
			else
			{
				return RoboTree.TreeItemIconIndex.NoDestinationFile;
			}
		}

		bool IsCheckAllowed(TreeTagInfo tti)
		{
			RoboTree.TreeItemIconIndex compareResult = CompareFiles(tti.m_fsiSource, tti.m_fsiDest);

			bool retval = false;

			switch(compareResult)
			{
				case RoboTree.TreeItemIconIndex.FileFolderMismatch:
					break;

				case RoboTree.TreeItemIconIndex.Folder:
					retval = true;
					break;

				case RoboTree.TreeItemIconIndex.SourceFileIsNewer:
					if(m_bCopyNewerFiles == true)
						retval = true;
					break;

				case RoboTree.TreeItemIconIndex.NoDestinationFile:
					retval = true;
					break;

				case RoboTree.TreeItemIconIndex.SourceFileIsOlder:
					if(m_bCopyOlderFiles == true)
						retval = true;
					break;

				case RoboTree.TreeItemIconIndex.Problem:
					break;

				case RoboTree.TreeItemIconIndex.Same:
					if(m_bCopySameFiles == true)
						retval = true;
					break;

				case RoboTree.TreeItemIconIndex.NoSourceFile:
					break;

				default:
					Debug.Assert(false, "Bad case in 'IsCheckAllowed'");
					break;
			}

			return retval;
		}

		private void SetCheck(TreeNode tn, bool checkValue)
		{
			if(tn.Checked != checkValue)
			{
				tn.Checked = checkValue;

				TreeNode tnParent;
				if(checkValue == true)
				{
					tnParent = tn.Parent;
					//					tn.Parent.EnsureVisible();
					while(tnParent != null)
					{
						if(tnParent.IsExpanded == false)
							tnParent.Expand();
						tnParent = tnParent.Parent;
					}
				}
			}
		}

		private void roboTree_AfterCheck(object sender, TreeViewEventArgs e)
		{
			if(e.Action == TreeViewAction.ByKeyboard || e.Action == TreeViewAction.ByMouse)
			{
				TreeObject to = e.Node as TreeObject;
				bool bRemoveTick = true;

				if(to.Parent == null)				// The root node can always be ticked.
				{
					bRemoveTick = false;
				}
				else
				{
					for(int c = 0; c < to.NumTags; c++)
					{
						TreeTagInfo tti = to.m_listTagInfo[c];
						if(IsCheckAllowed(tti) == true)
						{
							bRemoveTick = false;
							break;
						}

					}
				}
				if(bRemoveTick == true)
				{
					to.Checked = false;
					MessageBox.Show(this, "Your template does not allow you to check this treenode.\n" +
						"Template options:\n" +
						"Copy newer files: " + m_bCopyNewerFiles.ToString() + "\n" +
						"Copy same files: " + m_bCopySameFiles.ToString() + "\n" +
						"Copy older files: " + m_bCopyOlderFiles.ToString() + "\n\n" +
						"Please modify your template or see your project administrator if this is a problem.",
						Application.ProductName, MessageBoxButtons.OK);
				}
				else
				{
					bool bIsfolder = true;

					if(to.Parent != null)
					{
						for(int c = 0; c < to.NumTags; c++)
						{
							TreeTagInfo tti = to.m_listTagInfo[c];
							if(tti.m_fsiSource != null)
							{
								if((tti.m_fsiSource.Attributes & FileAttributes.Directory) != FileAttributes.Directory)
								{
									bIsfolder = false;
									break;
								}
							}
						}
					}
					if(bIsfolder == true)
					{
						Debug.WriteLine("Recursing folder.");
						RoboTree.TreeItemIconIndex[] mask = { RoboTree.TreeItemIconIndex.SourceFileIsNewer, RoboTree.TreeItemIconIndex.Folder, RoboTree.TreeItemIconIndex.NoDestinationFile };
						roboTree.Recurse(to, new RoboTree.TreeNodeActionDelegate(SetTreeChildCheckState), RoboTree.Flags.All, new object[] { mask, e.Node.Checked });
					}
				}
			}
		}

		private void buttonRefresh_Click(object sender, EventArgs e)
		{
			RefreshRoboTree();
		}

		int m_oldFileCountSoFar;

		internal bool RefreshRoboTree()
		{
			Debug.WriteLine("Refresh tree");
			TreeObject toRoot = this.roboTree.Nodes[0] as TreeObject;
			roboTree.Recurse(toRoot, new RoboTree.TreeNodeActionDelegate(SetNodeOldFlag), RoboTree.Flags.All, true);
			roboTree.Recurse(toRoot, new RoboTree.TreeNodeActionDelegate(InvalidateTreeTagInfos), RoboTree.Flags.All, null);

			toRoot.m_oldFlag = false;

			this.panelParseProgress.Visible = true;
			this.panelParseProgress.Update();
			m_FileCountSoFar = 0;
			m_oldFileCountSoFar = 0;


			for(int c = 0; c < m_skus.Count; c++)
			{
				PopulateTree(c);
			}

			this.panelParseProgress.Visible = false;

			roboTree.Recurse(toRoot, new RoboTree.TreeNodeActionDelegate(DeleteOldNodes), RoboTree.Flags.All, true);
			roboTree.Recurse(toRoot, new RoboTree.TreeNodeActionDelegate(UpdateNodeTicks), RoboTree.Flags.Checked, true);

//			roboTree.Invalidate();
			Debug.WriteLine("Done");
			return true;
		}





		#region Recurse callbacks

		private bool SetNodeOldFlag(TreeObject to, object args)
		{
			bool newValue = (bool)args;
			to.m_oldFlag = newValue;
			return true;
		}

		bool SetTreeChildCheckState(TreeObject to, object args)
		{
			object[] args2 = args as object[];

			RoboTree.TreeItemIconIndex[] mask = args2[0] as RoboTree.TreeItemIconIndex[];
			bool newValue = (bool)args2[1];

			bool setCheck = false;

			for(int i = 0; i < mask.Length; i++)
			{
				for(int c = 0; c < to.NumTags; c++)
				{
					if(to.m_listTagInfo[c].m_doNotCopy == false)
					{
						RoboTree.TreeItemIconIndex imageIndex = to.m_listTagInfo[c].m_iconIndex;
						if(mask[i] == imageIndex)
						{
							setCheck = true;
							break;
						}
					}
				}
			}
			#region visibility

			if(to.Parent != null)
			{
				if(setCheck == true)
				{
					if(newValue == true)
					{
						bool bCheckAllowed = false;
						for(int c = 0; c < to.NumTags; c++)
						{
							TreeTagInfo tti = to.m_listTagInfo[c];
							if(tti.m_doNotCopy == false)
							{
								if(IsCheckAllowed(tti) == true)
								{
									bCheckAllowed = true;
									break;
								}
							}
						}
						if(bCheckAllowed == true)
							SetCheck(to, newValue);
					}
					else
						SetCheck(to, newValue);
				}
			}
			#endregion visibility

			return true;
		}

		private bool DeleteOldNodes(TreeObject to, object args)
		{
			if(to.m_oldFlag == true)
			{
				to.Remove();
			}
			return true;
		}

		private bool UpdateNodeTicks(TreeObject to, object args)
		{
			bool bRemoveTick = true;

			if(to.Parent != null)
			{
				for(int c = 0; c < to.NumTags; c++)
				{
					TreeTagInfo tti = to.m_listTagInfo[c];
					if(tti.m_doNotCopy == false)
					{
						if(IsCheckAllowed(tti) == true)
						{
							bRemoveTick = false;
							break;
						}
					}
				}
				if(bRemoveTick == true)
				{
					to.Checked = false;
				}
			}
			return true;
		}

		private bool ListAllTreeObjects(TreeObject to, object args)
		{
			List<TreeObject> fileList = args as List<TreeObject>;

			fileList.Add(to);
			return true;
		}

		private bool ListAllTreeTagInfo(TreeObject to, object args)
		{
			List<TreeTagInfo> listTti = args as List<TreeTagInfo>;

			for(int c = 0; c < to.NumTags; c++)
			{
				TreeTagInfo tti = to.m_listTagInfo[c];

				if((tti != null) && (tti.m_doNotCopy == false))
				{
					if(tti.m_fsiSource != null)
					{
						listTti.Add(tti);
						if(m_bDeleteSourceFilesAfterCopy)
						{
							//if(IsFilePathContainedIn(tti.m_fsiSource.FullName, m_listDoNotDeleteFilespec) == false)
							if(m_regexDoNotDeleteFilespec.IsMatch(tti.m_fsiSource.FullName) == false)
								tti.m_bDeleteSourceAfterCopy = true;
							else
								tti.m_bDeleteSourceAfterCopy = false;
						}
					}
				}
			}

			return true;
		}

		private bool InvalidateTreeTagInfos(TreeObject to, object args)
		{
			for(int c = 0; c < to.NumTags; c++)
			{
				TreeTagInfo tti = to.m_listTagInfo[c];

				if((tti != null) /*&& (tti.m_doNotCopy == false)*/)
				{
					tti.m_fsiSource = null;
					tti.m_fsiDest = null;
					tti.m_iconIndex = RoboTree.TreeItemIconIndex.NoSourceFile;
				}
			}

			return true;
		}

		private bool SetTtiEnabledState(TreeObject to, object args)
		{
			for(int c = 0; c < to.NumTags; c++)
			{
				TreeTagInfo tti = to.m_listTagInfo[c];

				if(tti != null)
				{
					tti.m_doNotCopy = m_skus[c].m_disabled;
				}
			}

			return true;
		}





		private bool ListExternallyRefreshedFiles(TreeObject to, object args)
		{
			List<string> fileList = args as List<string>;

			for(int c = 0; c < to.NumTags; c++)
			{
				TreeTagInfo tti = to.m_listTagInfo[c];

				if(tti.m_doNotCopy == false)
				{
					if(tti.m_fsiSource != null)
					{
						if((tti.m_fsiSource.Attributes & FileAttributes.Directory) != FileAttributes.Directory)
						{
							string strSourcePath = tti.m_fsiSource.FullName;
							FileSystemInfo fsiSource = new FileInfo(strSourcePath);

							if(fsiSource.Exists)
							{
								if(fsiSource.LastWriteTime.Equals(tti.m_fsiSource.LastWriteTime) == false)
								{
									fileList.Add(tti.m_fsiSource.FullName);
								}
							}
							else if(fsiSource.Exists != tti.m_fsiSource.Exists)
							{
								fileList.Add(tti.m_fsiSource.FullName);
							}

							string strDestPath = tti.m_fsiDest.FullName;

							FileSystemInfo fsiDest = new DirectoryInfo(strDestPath);		// Check fsiDest.Exists flag.

							if(fsiDest.Exists == false)
							{
								fsiDest = new FileInfo(strDestPath);
							}

							if(fsiDest.Exists)
							{
								if(fsiDest.LastWriteTime.Equals(tti.m_fsiDest.LastWriteTime) == false)
								{
									fileList.Add(tti.m_fsiDest.FullName);
								}
							}
							else if(fsiDest.Exists != tti.m_fsiDest.Exists)
							{
								fileList.Add(tti.m_fsiDest.FullName);
							}
						}
					}
				}
			}
			return true;
		}

		private bool ListWhereSourceFileIsOlder(TreeObject to, object args)
		{
			List<string> fileList = args as List<string>;

			for(int c = 0; c < to.NumTags; c++)
			{
				TreeTagInfo tti = to.m_listTagInfo[c];
				if(tti.m_doNotCopy == false)
				{
					if(tti.m_iconIndex == RoboTree.TreeItemIconIndex.SourceFileIsOlder)
					{
						fileList.Add(tti.m_fsiSource.FullName);
					}
				}
			}
			return true;
		}


		private bool MatchFilesAgainstList(TreeObject to, object args)
		{
			Dictionary<string, Counter> fileDictionary = args as Dictionary<string, Counter>;

			//bool bCheckval = (bool)argsArray[1];

			for (int c = 0; c < to.NumTags; c++)
			{
				TreeTagInfo tti = to.m_listTagInfo[c];

				if (m_skus[c].m_disabled == false && tti.m_bhidden == false && to.m_bIsFolder == false)
				{

					bool bMatch = false;

					foreach(string strMatch in fileDictionary.Keys)
					{
						
						// TODO: Find a match between tti.m_fsiSource and fileList
						// If match found, 

						if(tti.m_fsiSource != null)
						{
							//bMatch = false;

							string strFile;
							string strMangleMatch;
							if(m_csp.m_bStripExtensions == true)
							{
								strFile = Path.GetDirectoryName(tti.m_fsiSource.FullName) + "\\" + Path.GetFileNameWithoutExtension(tti.m_fsiSource.FullName);
								strMangleMatch = Path.GetDirectoryName(strMatch) + "\\" + Path.GetFileNameWithoutExtension(strMatch);
							}
							else
							{
								strFile = tti.m_fsiSource.FullName;
								strMangleMatch = strMatch;
							}

							if(strFile.Length > strMangleMatch.Length)
							{
								if(strFile.EndsWith(strMangleMatch, StringComparison.OrdinalIgnoreCase) == true)
								{
									if(fileDictionary[strMatch].m_count != 0)
									{
										//Debug.WriteLine("Multiple match : '" + strMatch + "' : '" + strFile + "' and '" + strMangleMatch + "'");
										m_matchForm.richTextBox1.AppendText("Multiple match ("+fileDictionary[strMatch].m_count.ToString()+") : '" + strMatch + "' : '" + strFile + "' and '" + strMangleMatch + "'");
									}
									fileDictionary[strMatch].m_count++;
									bMatch = true;
								}
							}
						}

					}

					if(bMatch == true)
					{
						SetCheck(to, m_csp.m_bRadioButtonMatches);
						break;
					}
					else
					{
						SetCheck(to, !m_csp.m_bRadioButtonMatches);
					}
				}


			}
			return true;
		}

		#endregion Recurse callbacks





		private void toolStripButton_SourceNewerOrDestMissing_Click(object sender, EventArgs e)
		{
			RoboTree.TreeItemIconIndex[] mask = { RoboTree.TreeItemIconIndex.SourceFileIsNewer, RoboTree.TreeItemIconIndex.NoDestinationFile };
			toolStripSelectFiles(sender, mask);

		}

		private void toolStripButton1_SelectDestMissing_Click(object sender, EventArgs e)
		{
			RoboTree.TreeItemIconIndex[] mask = { RoboTree.TreeItemIconIndex.NoDestinationFile };
			toolStripSelectFiles(sender, mask);			
		}

		private void toolStripButtonFilesAreEqual_Click(object sender, EventArgs e)
		{
			RoboTree.TreeItemIconIndex[] mask = { RoboTree.TreeItemIconIndex.Same };
			toolStripSelectFiles(sender, mask);
		}

		private void toolStripButton_DestFileNewer_Click(object sender, EventArgs e)
		{
			RoboTree.TreeItemIconIndex[] mask = { RoboTree.TreeItemIconIndex.SourceFileIsOlder };
			toolStripSelectFiles(sender, mask);
		}

		private void toolStripButton_SelectAll_Click(object sender, EventArgs e)
		{
			RoboTree.TreeItemIconIndex[] mask = {RoboTree.TreeItemIconIndex.FileFolderMismatch,
													RoboTree.TreeItemIconIndex.Folder,
													RoboTree.TreeItemIconIndex.NoDestinationFile,
													RoboTree.TreeItemIconIndex.NoSourceFile,
													RoboTree.TreeItemIconIndex.Problem,
													RoboTree.TreeItemIconIndex.Same,
													RoboTree.TreeItemIconIndex.SourceFileIsNewer,
													RoboTree.TreeItemIconIndex.SourceFileIsOlder,
												};
			toolStripSelectFiles(sender, mask);
		}

		private void toolStripSelectFiles(object sender, RoboTree.TreeItemIconIndex[] mask)
		{
			ToolStripButton button = sender as ToolStripButton;
			TreeObject to = this.roboTree.SelectedNode as TreeObject;

			if(to != null)
			{
				if(to.Parent == null)
					to.Checked = button.Checked;

				//				roboTree.BeginUpdate();
				roboTree.Recurse(to, new RoboTree.TreeNodeActionDelegate(SetTreeChildCheckState), RoboTree.Flags.All, new object[] { mask, button.Checked });
				//				roboTree.EndUpdate();
			}
			else
				MessageBox.Show(this, "You must have something selected in your tree view to base this action on.", "The Monkey", MessageBoxButtons.OK, MessageBoxIcon.Stop);
		}





		Thread m_publishThread;

		private void actionButtonDoIt()
		{
			TreeNodeCollection tnc = roboTree.Nodes;
			if(tnc.Count == 1)
			{
				TreeObject to = tnc[0] as TreeObject;

				List<string> fileList = new List<string>();

				Debug.WriteLine("Checking for updated files ...\n");
				roboTree.Recurse(to, new RoboTree.TreeNodeActionDelegate(ListExternallyRefreshedFiles), RoboTree.Flags.Checked, fileList);
				int numCheckedFiles = roboTree.m_numProcessedFiles;

				if(fileList.Count == 0)
				{
					List<string> fileWarningList = new List<string>();
					roboTree.Recurse(to, new RoboTree.TreeNodeActionDelegate(ListWhereSourceFileIsOlder), RoboTree.Flags.Checked, fileWarningList);

					DialogResult dr = DialogResult.Yes;

					if(fileWarningList.Count != 0)
					{
						dr = MessageBox.Show(this, fileWarningList.Count.ToString() + " file" + (fileWarningList.Count == 1 ? " is" : "s are") + " newer remotely.\n" +
							"Do you wish to continue with publishing?", "The Monkey", MessageBoxButtons.YesNo, MessageBoxIcon.Warning);
					}
					if(dr == DialogResult.Yes)
					{


						// build the task list.
						List<TreeTagInfo> listTti = new List<TreeTagInfo>();
						roboTree.Recurse(to, new RoboTree.TreeNodeActionDelegate(ListAllTreeTagInfo), RoboTree.Flags.Checked, listTti);

						List<TreeTagInfo> deleteList = new List<TreeTagInfo>();

						foreach(TreeTagInfo tti in listTti)
						{
							if(tti.m_bhidden == true)
								deleteList.Add(tti);
						}

						foreach(TreeTagInfo tti in deleteList)
						{
							listTti.Remove(tti);
						}




						if(listTti.Count == 0)
						{
							MessageBox.Show(this, "Nothing to publish.", "The Monkey", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
							this.buttonPublish.Enabled = true;
							this.buttonRefresh.Enabled = true;
						}
						else
						{
							// Give the list to a copier thread ...
							m_publishThread = PublishThread.CreatePublishThread(this, new PublishThread.DelegateProgressUpdate(UpdatePublishStatus), listTti);
						}
					}
					else
					{
						this.buttonPublish.Enabled = true;
						this.buttonRefresh.Enabled = true;
					}
				}
				else
				{
					MessageBox.Show(this, "Files have been updated since last refresh. Aborting update.\nPlease check your selection and try again.\nSee output window for details.", "Attention", MessageBoxButtons.OK, MessageBoxIcon.Stop);
					RefreshRoboTree();
					foreach(string str in fileList)
					{
					}

					return;
				}
			}
			else
			{
				// TODO: Error, bad or empty tree.
				return;
			}
		}

		bool bThereWereErrors = false;

		public void UpdatePublishStatus(PublishThread.StatusUpdate statusUpdate, object data)
		{
			string strString;
			int length;

			switch(statusUpdate)
			{
				case PublishThread.StatusUpdate.Started:
					this.roboTree.Enabled = false;
					this.progressBar_Publish.Value = 0;
					this.richTextBox_Publish.Clear();
					this.buttonOk.Enabled = false;
					this.buttonAbort.Enabled = true;
					this.panel_Feedback.Visible = true;
					this.bThereWereErrors = false;
					this.richTextBox_Publish.BackColor = System.Drawing.SystemColors.Control;
					break;
				case PublishThread.StatusUpdate.UpdateProgress:
					float progress = (float)data;
					this.progressBar_Publish.Value = (int)(progress * 100);
					break;
				case PublishThread.StatusUpdate.SendString:
					strString = data as string;
					this.richTextBox_Publish.AppendText(strString + "\n");
					this.richTextBox_Publish.Select(this.richTextBox_Publish.Text.Length, 0);
					this.richTextBox_Publish.ScrollToCaret();
					break;
				case PublishThread.StatusUpdate.SendErrorString:
					this.bThereWereErrors = true;
					this.richTextBox_Publish.BackColor = System.Drawing.SystemColors.ControlDark;

					length = this.richTextBox_Publish.Text.Length;												// Get length of text (So we know where to select from)

					strString = data as string;
					this.richTextBox_Publish.AppendText(strString + "\n");

					this.richTextBox_Publish.Select(length, this.richTextBox_Publish.Text.Length - length);		// Select the text we've just added
					this.richTextBox_Publish.SelectionColor = Color.Red;										// Set the colour of the selected text to red

					this.richTextBox_Publish.Select(this.richTextBox_Publish.Text.Length, 0);
					this.richTextBox_Publish.ScrollToCaret();
					break;
				case PublishThread.StatusUpdate.Finished:
					this.buttonAbort.Enabled = false;
					this.buttonOk.Enabled = true;
					this.progressBar_Publish.Value = 100;

					this.richTextBox_Publish.Select(this.richTextBox_Publish.Text.Length, 0);
					this.richTextBox_Publish.SelectionFont = new Font(this.richTextBox_Publish.Font.FontFamily, 22, FontStyle.Bold);
					if(this.bThereWereErrors == true)
					{
						length = this.richTextBox_Publish.Text.Length;												// Get length of text (So we know where to select from)
						this.richTextBox_Publish.AppendText("*** THERE WERE ERRORS. See above for details. ***" + "\n");
						this.richTextBox_Publish.Select(length, this.richTextBox_Publish.Text.Length - length);		// Select the text we've just added
						this.richTextBox_Publish.SelectionColor = Color.Red;										// Set the colour of the selected text to red.

						this.richTextBox_Publish.Select(this.richTextBox_Publish.Text.Length, 0);
						this.richTextBox_Publish.ScrollToCaret();
					}
					else
					{
						this.richTextBox_Publish.AppendText("*** FINISHED ***" + "\n");
						this.richTextBox_Publish.Select(this.richTextBox_Publish.Text.Length, 0);
						this.richTextBox_Publish.ScrollToCaret();
						this.richTextBox_Publish.BackColor = System.Drawing.SystemColors.ControlLightLight;


						/// After a publish, all tree nodes should be unchecked.
						TreeObject toRoot = this.roboTree.Nodes[0] as TreeObject;
						if(toRoot != null)
						{
							RoboTree.TreeItemIconIndex[] mask =
							{
										RoboTree.TreeItemIconIndex.NoSourceFile,
										RoboTree.TreeItemIconIndex.Same,
										RoboTree.TreeItemIconIndex.SourceFileIsNewer,
										RoboTree.TreeItemIconIndex.SourceFileIsOlder,
										RoboTree.TreeItemIconIndex.NoDestinationFile,
										RoboTree.TreeItemIconIndex.Problem,
										RoboTree.TreeItemIconIndex.FileFolderMismatch,
										RoboTree.TreeItemIconIndex.Folder
							};
							roboTree.Recurse(toRoot, new RoboTree.TreeNodeActionDelegate(SetTreeChildCheckState), RoboTree.Flags.All, new object[] { mask, false });

							this.toolStripButton_DestFileNewer.Checked = false;
							this.toolStripButton_FilesAreEqual.Checked = false;
							this.toolStripButton_SelectAll.Checked = false;
							this.toolStripButton_SourceNewerOrDestMissing.Checked = false;
							this.toolStripButton1_SelectDestMissing.Checked = false;
						}
					}

					break;
				default:
					break;
			}
		}

		private void buttonPublish_Click(object sender, EventArgs e)
		{
			if(VersionCheck() == true)
			{
				this.buttonPublish.Enabled = false;
				this.buttonRefresh.Enabled = false;
				actionButtonDoIt();
			//	RefreshRoboTree();
			}
		}

		private void buttonAbort_Click(object sender, EventArgs e)
		{
			m_publishThread.Abort();
			UpdatePublishStatus(PublishThread.StatusUpdate.SendErrorString, "ABORTED BY USER");
			UpdatePublishStatus(PublishThread.StatusUpdate.Finished, null);
		}

		private void buttonOk_Click(object sender, EventArgs e)
		{
			this.panel_Feedback.Visible = false;
			this.buttonAbort.Enabled = false;
			this.RefreshRoboTree();
			this.buttonPublish.Enabled = true;
			this.buttonRefresh.Enabled = true;


			this.roboTree.Enabled = true;
		}

		private void toolStripButton_ViewChecked_Click(object sender, EventArgs e)
		{
			TreeObject toRoot = this.roboTree.Nodes[0] as TreeObject;
			List<TreeObject> listTreeObjects = new List<TreeObject>();

			roboTree.Recurse(toRoot, new RoboTree.TreeNodeActionDelegate(ListAllTreeObjects), RoboTree.Flags.Checked, listTreeObjects);

			TextViewer tv = new TextViewer();

			foreach(TreeObject to in listTreeObjects)
			{
				string oldString = null;

				for(int c = 0; c < to.NumTags; c++)
				{
					TreeTagInfo tti = to.m_listTagInfo[c];
					if(tti.m_fsiSource != null)
					{
						if((tti.m_fsiSource.Attributes & FileAttributes.Directory) != FileAttributes.Directory && tti.m_bhidden == false)
						{
							string strFullName = tti.m_fsiDest.FullName;


							foreach(SkuPath skuPath in m_skus[c].m_listSkuPaths)
							{
								if(strFullName.StartsWith(skuPath.m_strDestPath))
								{
									strFullName = strFullName.Substring(skuPath.m_strDestPath.Length + 1);
									break;
								}
							}



							//if(strFullName.StartsWith(m_skus[c].strDestFolder))
							//    strFullName = strFullName.Substring(m_skus[c].strDestFolder.Length + 1);
							if(strFullName != oldString)
							{
								oldString = strFullName;
								tv.GetTextBox().AppendText(strFullName + "\n");
							}

						}
					}
				}
			}

			tv.ShowDialog();
		}



		private void toolStripButtonDeleteSelection_Click(object sender, EventArgs e)
		{
			TreeObject toRoot = this.roboTree.Nodes[0] as TreeObject;
			List<TreeObject> listTreeObjects = new List<TreeObject>();

			roboTree.Recurse(toRoot, new RoboTree.TreeNodeActionDelegate(ListAllTreeObjects), RoboTree.Flags.Checked, listTreeObjects);

			DeleteListViewer deleteListViewer = new DeleteListViewer();

			foreach(TreeObject to in listTreeObjects)
			{
				string oldString = null;

				for(int c = 0; c < to.NumTags; c++)
				{
					TreeTagInfo tti = to.m_listTagInfo[c];
					if((tti.m_fsiSource != null) && (tti.m_doNotCopy == false) && tti.m_bhidden == false)
					{
						//if((tti.m_fsiSource.Attributes & FileAttributes.Directory) != FileAttributes.Directory)
						{
							string strFullName = tti.m_fsiSource.FullName;

							//	strFullName = strFullName.Substring(m_folderPairs[c].strDestFolder.Length + 1);
							if(strFullName != oldString)
							{
								oldString = strFullName;
								deleteListViewer.AddItem(tti.m_fsiSource, true);
							}

						}
					}
				}
			}

			if(deleteListViewer.ShowDialog() == DialogResult.Yes)
				this.RefreshRoboTree();
		}

		private void toolStripButtonClose_Click(object sender, EventArgs e)
		{
			DialogResult dr = MessageBox.Show(this, "Are you sure you want to close this panel?", Application.ProductName, MessageBoxButtons.YesNo);
			if(dr == DialogResult.Yes)
			{
				OnRoboPanelClosing(new EventArgs());
			}
		}

		private void toolStripButtonConfigureSorcery_Click(object sender, EventArgs e)
		{
			ConfigureSorcery configureSorcery = new ConfigureSorcery(m_csp);

			configureSorcery.Show(this);
		}

		private void RoboPanel_Load(object sender, EventArgs e)
		{
			this.panelParseProgress.BringToFront();
			this.panelParseProgress.Show();
			//this.CreateControl();
			this.Show();
			this.Update();
			Initialise();
		}


		//private void toolStripButtonSorceryQuery_Click(object sender, EventArgs e)
		//{
		//    // TODO :
		//    // Execute the Sorcery query.
		//    //LaunchJob(ConfigureSorcery.m_sStrSorceryQuery, 
		//    // Read in the file.

		//    string strDatabase = LaunchJob("xtract", m_csp.m_StrSorceryQuery, null, false);
		//    Dictionary<string, bool> allFiles = new Dictionary<string, bool>();

		//    try
		//    {
		//        XmlDocument xDoc = new XmlDocument();
		//        xDoc.LoadXml(strDatabase);

		//        List<XmlElement> xList = XmlHelper.FindElementsByKeyValue(xDoc["Xtract"], "field", "diffuseTexture", false, false, true);
		//        foreach(XmlElement xElement in xList)
		//        {
		//            if(allFiles.ContainsKey(xElement.InnerText) == false)
		//                allFiles.Add(xElement.InnerText, false);
		//            else
		//            {
		//                Debug.WriteLine("Duplicated source entry. '"+xElement.InnerText+"'");
		//            }
		//        }
		//    }
		//    catch(Exception ex)
		//    {
		//        MessageBox.Show(this, "Error." + ex.Message);
		//        return;
		//    }

		//    // Parse the file into a List<string>.
		//    // Call MatchFilesAgainstList recursively.
		//    //TreeObject toRoot = this.roboTree.Nodes[0] as TreeObject;
		//    TreeObject to = roboTree.SelectedNode as TreeObject;
		//    roboTree.Recurse(to, new RoboTree.TreeNodeActionDelegate(MatchFilesAgainstList), RoboTree.Flags.All, new object[]{ allFiles, m_csp.m_bRadioButtonMatches});
		//}

		class Counter
		{
			public Counter(int count)
			{
				m_count = count;
			}

			public int m_count;
		}

		private void toolStripButtonSorceryQuery_Click(object sender, EventArgs e)
		{
			// TODO :
			// Execute the Sorcery query.
			//LaunchJob(ConfigureSorcery.m_sStrSorceryQuery, 
			// Read in the file.

			string strDatabase = LaunchJob("xtract", m_csp.m_strSorceryDatabase + " " + m_csp.m_StrSorceryQuery, null, false);
			Dictionary<string, Counter> allFiles = new Dictionary<string, Counter>();

			try
			{
				XmlDocument xDoc = new XmlDocument();
				xDoc.LoadXml(strDatabase);

				foreach(string strQuery in m_csp.m_sorceryQueryItems)
				{
					Dictionary<string, string> keyValuePairs = new Dictionary<string,string>();

					int delimiterIndex = strQuery.LastIndexOf(".");

					if(delimiterIndex > 0 && delimiterIndex < strQuery.Length)
					{
						keyValuePairs.Add("type", strQuery.Substring(0, delimiterIndex));
						keyValuePairs.Add("field", strQuery.Substring(delimiterIndex+1));
						List<XmlElement> xList = XmlHelper.FindElementsByKeyValues(xDoc["Xtract"], keyValuePairs, false, false, true);

						foreach(XmlElement xElement in xList)
						{
							string strKey = xElement.InnerText.ToLower();
							if(allFiles.ContainsKey(strKey) == false && strKey != "")
								allFiles.Add(strKey, new Counter(0));
							else
							{
								Debug.WriteLine("Duplicated source entry. '" + xElement.InnerText + "'");
							}
						}
					}
					else
						MessageBox.Show(this, "Bad Query entry : '"+strQuery+"'");

				}
			}
			catch(Exception ex)
			{
				MessageBox.Show(this, "Error. Sorcery returned :\n"+strDatabase);
				return;
			}

			// Parse the file into a List<string>.
			// Call MatchFilesAgainstList recursively.
			//TreeObject toRoot = this.roboTree.Nodes[0] as TreeObject;
			TreeObject to = roboTree.SelectedNode as TreeObject;



			m_matchForm.Show();
			m_matchForm.richTextBox1.Clear();
			roboTree.Recurse(to, new RoboTree.TreeNodeActionDelegate(MatchFilesAgainstList), RoboTree.Flags.All, allFiles);
		}



		public string LaunchJob(string strCommand, string strClArguments, string strWorkingDirectory, bool bHideWindowFromUser)
		{
			try
			{
				Process p = new Process();

				p.StartInfo = new ProcessStartInfo(strCommand, strClArguments);

				if(strWorkingDirectory != null)
				{
					p.StartInfo.WorkingDirectory = strWorkingDirectory;
				}
				else
				{
					p.StartInfo.WorkingDirectory = Path.GetDirectoryName(strCommand);
				}

				// Allow the window to be hidden so that it run's without being seen
				if(bHideWindowFromUser)
				{
					p.StartInfo.WindowStyle = ProcessWindowStyle.Hidden;
				}

				p.StartInfo.UseShellExecute = false;
				StringBuilder sb = new StringBuilder();
				p.StartInfo.RedirectStandardOutput = true;
				p.Start();

				while(p.HasExited == false)
				{
					sb.Append(p.StandardOutput.ReadToEnd());
					System.Threading.Thread.Sleep(5);
				}

				return sb.ToString();
			}
			catch(Exception ex)
			{
				throw (ex);
			}
		}
	}
}
