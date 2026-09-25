using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using Executer.Helpers;
using Executer.Interfaces;
using message_handler;
using Global;
using System.Drawing;
using System.Diagnostics;
using System.IO;

namespace Executer
{
	public class TreeSolutionObject : TreeBaseObject, /*IRenameable,*/ ISaveable, IContextMenu, IViewableObject, ICanReloadSelf, IRemoveable, ICanReceivePaste, IMoveable
	{
		private bool m_bIsDirty = false;

		/// <summary>
		/// The filename of the solution file associated with this node.
		/// </summary>
		private string m_strPathFileName = null;

		internal XmlDocument m_xSolution = null;

		public TreeSolutionObject(ExecuterTreeView ownerTreeView)
		{
			Debug.Assert(ownerTreeView != null);
			OwnerTreeView = ownerTreeView;

			this.ImageKey = "TreeSolutionIcon";
			this.SelectedImageKey = "TreeSolutionIcon";
		}

		private TreeSolutionObject(string strSolutionFile, ExecuterTreeView ownerTreeView) : this(ownerTreeView)
		{
			m_xSolution = new XmlDocument();
			m_strPathFileName = strSolutionFile;

			try
			{
				m_xSolution.Load(strSolutionFile);
			}
			catch(Exception ex)
			{
				throw new ApplicationException("Cannot load solution '" + strSolutionFile + "'\nSystem report: " + ex.Message);
			}
			FileAttributes fileAttributes = File.GetAttributes(strSolutionFile);
			if((fileAttributes & FileAttributes.ReadOnly) == FileAttributes.ReadOnly)
			{
				//m_bIsReadonly = true;
				this.ReadableState = RestrictedMode.ReadOnly;
			}

			try
			{
				XmlNode xSolution = m_xSolution.SelectSingleNode("/root/solution");
				this.Text = "Solution '" + Path.GetFileNameWithoutExtension(strSolutionFile) + "'";
				XmlNodeList xProjects = m_xSolution.SelectNodes("/root/solution/projects/project");

				bool bIsReallyDirty = false;

				foreach(XmlElement xProject in xProjects)
				{
					string strProjectName = XmlHelper.GetKeyValue(xProject, "filename");
					if(ownerTreeView.AddOrFindProject(this, strProjectName) == null)
						bIsReallyDirty = true;						// Loading a project failed, so the solution needs saving.
				}
				this.IsDirty = bIsReallyDirty;
			}
			catch(Exception ex)
			{
				throw new ApplicationException("Failed to load solution '" + strSolutionFile + "'\nThe system error is:" + ex.Message);
			}
		}

		//public static TreeSolutionObject LoadSolution(strSolutionName)
		//{
		//    try
		//    {
		//        // Make sure it's not already loaded. If it is, return that one instead.

		//        ExecuterTreeView etv = Program.S_executerMainForm.m_executerTreeView;

		//    }
		//    catch(Exception ex)
		//    {

		//    }
		//}


		/// <summary>
		/// If the given project is already loaded into the solution, return it, otherwise add the given project to the solution.
		/// </summary>
		/// <param name="pathFilename">The location of the project.</param>
		/// <returns>The newly found or added project, or null</returns>
		public static TreeSolutionObject AddOrFindSolution(string pathFilename)
		{
			TreeSolutionObject treeSolutionObject = GetSolutionByFilename(pathFilename);

			if(treeSolutionObject != null)
				return treeSolutionObject;
			else
			{
				try
				{
					treeSolutionObject = new TreeSolutionObject(pathFilename, Program.S_executerMainForm.m_executerTreeView);

					Program.S_executerMainForm.m_executerTreeView.Nodes.Add(treeSolutionObject);
					treeSolutionObject.ImageKey = "TreeSolutionIcon";
					treeSolutionObject.SelectedImageKey = "TreeSolutionIcon";
				}
				catch(ApplicationException aex)
				{
					UtilityFunctions.ShowError(aex.Message);
					return null;
				}
				catch(Exception ex)
				{
					UtilityFunctions.ShowError("Failed to load solution '" + pathFilename + "'\n" + ex.Message);
					return null;
				}

				return treeSolutionObject;
			}
		}

		private static TreeSolutionObject GetSolutionByFilename(string strSolution)
		{
			ExecuterTreeView etv = Program.S_executerMainForm.m_executerTreeView;

			if(etv.Nodes.Count != 0)
			{
				TreeNodeCollection tnc = etv.Nodes;

				foreach(TreeSolutionObject tso in etv.Nodes)
				{
					if(tso.m_strPathFileName.ToLower() == strSolution.ToLower())
						return tso;
				}
				return null;
			}
			return null;
		}

		public override string TypeName{get { return "Solution"; }}

		public string StrPathFileName{get { return m_strPathFileName; }}

		public ExecuterTreeView OwnerTreeView { get; private set; }

		#region ICanReceivePaste Members

		public bool CanReceivePaste(Moveable thingToPaste)
		{
			// Solutions can receive projects that are being moved. Projects cannot be copied.
			bool bReadOnly;
			if(this.ReadableState == RestrictedMode.ReadOnly || this.ReadableState == RestrictedMode.Restricted)
				bReadOnly = true;
			else
				bReadOnly = false;

			if((thingToPaste != null) && (bReadOnly == false))
			{
				TreeProjectObject sourceProject = thingToPaste.ObjectSource as TreeProjectObject;

				if(sourceProject != null)
				{
					//if(thingToPaste.MoveType == MoveType.Cut)
					{
						return true;
					}
				}
			}
			return false;
		}

		public bool Paste(Moveable thingToPaste)
		{
			TreeProjectObject sourceProject = thingToPaste.ObjectSource as TreeProjectObject;


			bool bSourceReadOnly;
			if(((sourceProject.Parent) as TreeSolutionObject).ReadableState == RestrictedMode.ReadOnly || ((sourceProject.Parent) as TreeSolutionObject).ReadableState == RestrictedMode.Restricted)
				bSourceReadOnly = true;
			else
				bSourceReadOnly = false;

			bool bThisReadOnly;
			if(this.ReadableState == RestrictedMode.ReadOnly || this.ReadableState == RestrictedMode.Restricted)
				bThisReadOnly = true;
			else
				bThisReadOnly = false;

			if(bSourceReadOnly == true && thingToPaste.MoveType == MoveType.Cut)
			{
				MessageBox.Show(null, "Cannot cut from the source. The source solution is readonly.", "Executer Error");
				return false;
			}
			if(bThisReadOnly == true)
			{
				MessageBox.Show(null, "Cannot paste to the destination. The destination solution is readonly.", "Executer Error");
				return false;
			}

			if(this.GetProjectByFilename(sourceProject.StrProjectDocumentFilename) != null)
			{
				MessageBox.Show(null, "Cannot paste to the destination. The destination solution already contains the source project.", "Executer Error");
				return false;
			}
			if(thingToPaste.MoveType == MoveType.Cut)
			{
				{
					ISaveable saveableSolution = sourceProject.Parent as ISaveable;

					if (saveableSolution != null)
						saveableSolution.IsDirty = true;
				}
				sourceProject.Remove();

				this.Nodes.Add(sourceProject);
				this.IsDirty = true;

				thingToPaste.CutHasBeenDone(); // Prevent subsequent 'paste' operations.
			}
			else
			{
				//AddOrFindProject(sourceProject.StrProjectDocumentFilename);
				TreeProjectObject tpo = Program.S_executerMainForm.m_executerTreeView.AddOrFindProject(this, sourceProject.StrProjectDocumentFilename);

			}
		
			return true;
		}

		public bool CanReceiveSpecialPaste(Moveable thingToPaste)
		{
			throw new NotImplementedException();
		}

		bool ICanReceivePaste.PasteSpecial(Moveable thingToPaste)
		{
			throw new NotImplementedException();
		}

		public void PasteSpecial(Moveable thingToPaste)
		{
			throw new NotImplementedException();
		}

		#endregion

		#region ICanReloadSelf Members

		public TreeBaseObject ReLoad()
		{
			ExecuterTreeView ownerTreeView = this.OwnerTreeView;
			if(ownerTreeView != null)
			{
				if(this.IsDirty == true)
				{
					DialogResult dr = MessageHandler.Show(null, "Save changes to your Solution?", Program.AssemblyTitle, MessageBoxButtons.YesNoCancel, MessageBoxIcon.Warning);
					switch(dr)
					{
						case DialogResult.Yes:
							{
								if(Saveable.RecursiveSave(this, true, false) != SaveableResult.Success)
									return null;
							}
							break;
						case DialogResult.No:
							{
							}
							break;
						case DialogResult.Cancel:
							{
								return null;
							}
						default:
							break;
					}
				}

				int index = this.Index;
				ExecuterTreeView parent = this.TreeView as ExecuterTreeView;
				Debug.Assert(parent != null);
				if(parent != null)
				{
					// temporarily remove the solution so that projects in the replacement solution are not marked as duplicates.
					this.Remove();
					TreeSolutionObject replacementTso = new TreeSolutionObject(this.m_strPathFileName, this.OwnerTreeView);
					parent.Nodes.Insert(index, this);


					bool bIsDirty = this.m_bIsDirty;

					this.m_bIsDirty = false; // Prevent the 'Close' function from prompting for a save.

					if(this.Close(CloseMethod.Force) == true)
					{
						ownerTreeView.Nodes.Insert(index, replacementTso);

						replacementTso.SelectedImageKey = "TreeSolutionIcon";
						replacementTso.ImageKey = "TreeSolutionIcon";

						return replacementTso;
					}
					else
						this.m_bIsDirty = bIsDirty; // If the replacement fails, restore the dirty flag.
				}
			}
			return null;
		}

		#endregion

		#region IContextMenu Members

		public ContextMenuStrip CreateMenu()
		{
			ContextMenuStrip cms = new ContextMenuStrip();

			bool bReadOnly;
			if(this.ReadableState == RestrictedMode.ReadOnly || this.ReadableState == RestrictedMode.Restricted)
				bReadOnly = true;
			else
				bReadOnly = false;

			bool bCanSaveSolution = (this.m_bIsDirty || Executer_MainForm.s_optionsManager.GetContainerBoolByName("allow_saving_unmodified_solutions", false)) && !bReadOnly;
			string strSaveSolutionToolTip = null;
			if(this.m_bIsDirty == false)
			{
				if(Executer_MainForm.s_optionsManager.GetContainerBoolByName("allow_saving_unmodified_solutions", false) == false)
				{
					strSaveSolutionToolTip = bReadOnly ? "Solution is Readonly" : "Solution is unmodified";
				}
				else
				{
					strSaveSolutionToolTip = bReadOnly ? "Solution is Readonly" : "Solution is unmodified";
				}
			}

			cms.Items.Add(ToolstripUtility.CreateTsmi("saveSolutionTsmi", "<save>", "Save Solution", tsmi_Click, bCanSaveSolution, strSaveSolutionToolTip));
			cms.Items.Add(ToolstripUtility.CreateTsmi("saveSolutionAsTsmi", "<saveas>", "Save Solution as ...", tsmi_Click, true));
			cms.Items.Add(ToolstripUtility.CreateTsmi("closeSolutionTsmi", "<close>", "Close Solution", tsmi_Click, true));
			cms.Items.Add(ToolstripUtility.CreateTss(""));
			cms.Items.Add(ToolstripUtility.CreateTsmi("addProjectTsmi", "<addproject>", "Add Existing Project ...", tsmi_Click, !bReadOnly, bReadOnly == true ? "Solution is readonly" : null));
			cms.Items.Add(ToolstripUtility.CreateTsmi("createNewProjectTsmi", "<createnewproject>", "Create New Project ...", tsmi_Click, !bReadOnly, bReadOnly == true ? "Solution is readonly" : null));
			cms.Items.Add(ToolstripUtility.CreateTss(""));
			cms.Items.Add(ToolstripUtility.CreateTsmi("solutionTasksTsmi", "", "Solution Tasks", tsmi_Click, true));

			ToolStripMenuItem tmiTasks = ToolstripUtility.FindFirstToolsTripMenuItemByText(cms.Items, "Solution Tasks");
			if(tmiTasks != null)
			{
				this.AddSolutionTasks(tmiTasks, true, true);
			}
			cms.Items.Add(ToolstripUtility.CreateTsmi("propertiesTsmi", "<properties>", "Properties", new EventHandler(tsmi_Click), true));

			return cms;
		}

		#endregion

		#region IMoveable Members

		public bool CanMoveUp
		{
			get
			{
				if(this.TreeView != null && this.Index > 0)
					return true;
				
				return false;
			}
		}

		public bool MoveUp()
		{
			TreeNodeCollection parentNodes = this.TreeView.Nodes;

			parentNodes.Remove(this);

			parentNodes.Insert(this.Index - 1, this);

			return true;
		}

		public bool CanMoveDown
		{
			get
			{
				if(this.TreeView != null)
				{
					if(this.Index < (this.TreeView.Nodes.Count - 1))
						return true;
				}
				return false;
			}
		}

		public bool MoveDown()
		{
			TreeNodeCollection parentNodes = this.TreeView.Nodes;

			parentNodes.Remove(this);

			parentNodes.Insert(this.Index + 1, this);

			return true;
		}

		public bool CanCut
		{
			get { return false; }
		}

		public Moveable Cut()
		{
			throw new NotImplementedException();
		}

		public bool CanCopy
		{
			get { return false; }
		}

		public Moveable Copy()
		{
			throw new NotImplementedException();
		}

		#endregion

		#region IRemoveable Members

		public bool Close(CloseMethod closeMethod)
		{
			if(this.IsDirty == true && closeMethod != CloseMethod.Force)
			{
				PromptSave promptSave = new PromptSave(this as ISaveable, true, true);

				if(promptSave.IsEmpty == false)
				{
					DialogResult dr = promptSave.ShowDialog(Program.S_executerMainForm);

					if(dr == DialogResult.Cancel)
					{
						return false;
					}
				}
			}

			// Clear the properties panel ...
			Executer_MainForm.s_treeObjectViewer.ClearViewer();

			// close any closeable children ...
			List<IRemoveable> removeableChildren = new List<IRemoveable>(this.Nodes.Count);
			foreach(TreeNode tn in this.Nodes)
			{
				if(tn is IRemoveable)
					removeableChildren.Add(tn as IRemoveable);
			}

			foreach(IRemoveable iRemoveable in removeableChildren)
			{
				iRemoveable.Close(CloseMethod.Force);
			}

			this.OwnerTreeView.SetCheck(this, false, true);
			this.Remove();

			return true;
		}

		public bool CanClose()
		{
			return true;
		}

		#endregion

		#region ISaveable Members

		public bool LogicallyCanBeSaved
		{
			get { return true; }
		}

		public RestrictedMode ReadableState { get; set; }

		public void Save()
		{
			SaveAs(this.m_strPathFileName); // Prompts the user if m_strPathFileName is null or another problem arises.
		}

		//what the fuck are you doing?
		public void SaveAs(string strSolutionFilename)
		{
			XmlDocument xSolution = m_xSolution;

			if(xSolution == null || (xSolution.SelectSingleNode("/root/solution/projects") == null))
			{
				xSolution = new XmlDocument();
				xSolution.LoadXml("<root><solution name='" + Path.GetFileName(strSolutionFilename) + "'><projects></projects></solution></root>");
			}

			XmlHelper.AddAttribute(xSolution.SelectSingleNode("/root/solution"), "name", this.Text);

			XmlElement xCurrent = xSolution.SelectSingleNode("/root/solution/projects") as XmlElement;

			xCurrent.RemoveAll();

			foreach(ISaveable saveableObject in this.Nodes)
			{
				Debug.Assert(saveableObject != null);
				if(saveableObject != null)
				{
					XmlElement xProject = xSolution.CreateElement("project");
					XmlHelper.AddAttribute(xProject, "filename", saveableObject.SaveName);
					xCurrent.AppendChild(xProject);
				}
			}

			string strCurrentDirectory = null;

			try
			{
				strCurrentDirectory = Environment.CurrentDirectory;
				Environment.CurrentDirectory = Path.GetDirectoryName(Application.ExecutablePath);

				xSolution.Save(strSolutionFilename);
				m_strPathFileName = strSolutionFilename;
				//this.m_bIsReadOnly = false;
				// ATTENTION RESTRICTED : Ensure that we haven't just overwritten a solution that is loaded elsewhere.
				//Debug.Assert(false);
				this.ReadableState = RestrictedMode.Writable;
				this.IsDirty = false;
				this.Text = "Solution '" + Path.GetFileNameWithoutExtension(strSolutionFilename) + "'";

				return;
			}
			catch(Exception ex)
			{
				throw ex;
			}
			finally
			{
				Environment.CurrentDirectory = strCurrentDirectory;
			}
		}

		public bool IsDirty
		{
			get { return m_bIsDirty; }

			set
			{
				m_bIsDirty = value;
				if(value == true)
				{
					ISaveable saveableParent = this.TreeView as ISaveable;

					if(saveableParent != null)
					{
						saveableParent.IsDirty = true;
					}
				}
			}
		}

		public string SaveName{get { return this.m_strPathFileName; }}

		public string FileFilter {get { return "Executer Solution Files (*.Executersolution)|*.ExecuterSolution|All Files (*.*)|*.*"; }}

		public string SaveAsDialogTitle {get { return "Save Solution '" + Path.GetFileName(this.SaveName) + "' as ..."; }}

		public bool CanHaveSaveableChildren {get { return true; }}

		public List<ISaveable> SaveableChildren
		{
			get
			{
				List<ISaveable> saveableChildren = new List<ISaveable>();

				foreach(TreeNode tn in this.Nodes)
				{
					if(tn is ISaveable)
					{
						saveableChildren.Add(tn as ISaveable);
					}
				}
				return saveableChildren;
			}
		}

		#endregion

		#region IViewableObject Members

		public XmlElement RawXmlDocument{get { return null; }}

		public Control CreateView()
		{
			MenuSystemLibrary.MenuSystem menuSystem = new MenuSystemLibrary.MenuSystem {m_bReadOnly = true};

			menuSystem.Initialise(null);

			XmlDocument xDoc = new XmlDocument();

			xDoc.Load
				(new StringReader
				 	(
				 	/*"<values>" +*/
				 	"<container name = 'properties' type = 'basic' display_name = 'Solution Properties' width='610'>" +
				 	"<container name='solution_name' display_name='Filename' type='string' pre_format='cr;ytab(15);' width='600'	default='No selection' />" +
				 	"<container name='readonly' display_name = '' type='label'	pre_format='cr;ytab(15);' />" +
				 	"</container>" /*+
						"</values>"*/));

			XmlNode myNode = xDoc.SelectSingleNode("//container[@name='solution_name']");
			XmlHelper.AddAttribute(myNode, "value", this.m_strPathFileName);

			myNode = xDoc.SelectSingleNode("//container[@name='readonly']");

			string strReadableValue;

			switch(this.ReadableState)
			{
				case RestrictedMode.Writable:
					strReadableValue = "Solution is Writable";
					break;
				case RestrictedMode.ReadOnly:
					strReadableValue = "Solution is Readonly";
					break;
				case RestrictedMode.Restricted:
					strReadableValue = "Solution is loaded elsewhere. This instance is editable because it is the primary Solution.";
					break;
				case RestrictedMode.PrimaryRestricted:
					strReadableValue = "Solution cannot be edited as it is loaded elsewhere.\nSet it to be the primary Solution to edit it.";
					break;
				default:
					strReadableValue = "ERROR";
					break;
			}

			XmlHelper.AddAttribute(myNode, "value", strReadableValue);

			menuSystem.ForceMenu(xDoc.FirstChild as XmlElement);

			return menuSystem;
		}

		#endregion

		/// <summary>
		/// If the given project is already loaded into the solution, return it, otherwise add the given project to the solution.
		/// </summary>
		/// <param name="pathFilename">The location of the project.</param>
		/// <returns>The newly found or added project, or null</returns>
		public TreeProjectObject AddOrFindProject(string pathFilename)
		{
			TreeProjectObject treeProjectObject = this.GetProjectByFilename(pathFilename);

			if(treeProjectObject != null)
				return treeProjectObject;
			else
			{
				try
				{
					treeProjectObject = new TreeProjectObject(pathFilename);

					if(treeProjectObject.StrIconsPath != null)
					{
						OwnerTreeView.AddImages(treeProjectObject.StrIconsPath);
					}
				}
				catch(ApplicationException aex)
				{
					UtilityFunctions.ShowError(aex.Message);
					return null;
				}
				catch(Exception ex)
				{
					UtilityFunctions.ShowError("Failed to load project '" + pathFilename + "'\n" + ex.Message);
					return null;
				}

				this.AddWithIcons(treeProjectObject, treeProjectObject.StrIcon1, treeProjectObject.StrIcon2);
				treeProjectObject.ReadableState = treeProjectObject.ReadableState;

				this.IsDirty = true; // This is reset if AddProject is called from the TreeSolutionObject constructor.

				return treeProjectObject;
			}
		}

		/// <summary>
		/// Brute force build the task list.
		/// </summary>
		internal bool AddSolutionTasks(ToolStripMenuItem tmiSolutionTasks, bool bClearExistingTasks, bool bMergeDuplicates)
		{
			if(bClearExistingTasks == true)
				tmiSolutionTasks.DropDownItems.Clear();

			if(ExecuterTreeView.fontHeading == null)
				ExecuterTreeView.fontHeading = new Font(tmiSolutionTasks.Font.FontFamily, tmiSolutionTasks.Font.Size, FontStyle.Bold | FontStyle.Italic);

			try
			{
				Dictionary<string, string> allTasks = new Dictionary<string, string>();		// Dictionary is (taskname, displayname).

				foreach(TreeProjectObject tpo in this.Nodes)
				{
					Dictionary<string, string> tasks = tpo._rootUserObject.GetTasks();

					if(tasks != null)
					{
						foreach(KeyValuePair<string, string> kvp in tasks)
						{
							if(allTasks.ContainsKey(kvp.Key) == false)
							{
								allTasks.Add(kvp.Key, kvp.Value);
							}
						}
					}
				}

				foreach(KeyValuePair<string, string> kvp in allTasks)
				{
					ToolStripMenuItem newItem = tmiSolutionTasks.DropDownItems.Add(kvp.Value) as ToolStripMenuItem;

					newItem.Tag = kvp.Key;

					newItem.Click += solutionTaskToolStripMenuItem_Click;


					/// TODO:
					/// Add a callback to the dropdown item.
					/// Callback is to call DoTask on each Solution child, passing in the taskname associated with the displayname, and a flag to recurse.
					/// (Dictionary is taskname, displayname)
				}
					// ATTENTION
					//tpo.AddProjectTasks(tmiSolutionTasks, bClearExistingTasks, bMergeDuplicates, new EventHandler(solutionTaskToolStripMenuItem_Click));
			}
		
			catch(Exception ex)
			{
				Debug.WriteLine("Exception in 'AddSolutionTasks' : " + ex.Message);
				Debug.Assert(false);
			}

			if(tmiSolutionTasks.DropDownItems.Count == 0)
			{
				tmiSolutionTasks.Enabled = false;
			}
			else
			{
				tmiSolutionTasks.Enabled = true;
				//ToolStripLabel tsl = new ToolStripLabel("Choose your task");
				//tsl.Font = ExecuterTreeView.fontHeading;
				//tsl.Width = 105;								// HACK: For some reason the first time this is shown it is too narrow. Subsequent views of it are fine.
				//tmiSolutionTasks.DropDownItems.Insert(0, tsl);
				////				tmiTasks.Width = Math.Max(tmiTasks.Width, tsl.Width);
			}

			return true;
		}

		private void solutionTaskToolStripMenuItem_Click(object sender, EventArgs e)
		{
			ToolStripMenuItem tsmi = sender as ToolStripMenuItem;

			string strTaskName = tsmi.Tag as string;

			DoTask(strTaskName);
		}

		public void DoTask(string strTaskName)
		{
			foreach(TreeProjectObject tpo in this.Nodes)
			{
				tpo.DoTask(strTaskName, true, false);
			}
		}

		private void tsmi_Click(object sender, EventArgs e)
		{
			ToolStripMenuItem toolstripMenuItem = sender as ToolStripMenuItem;

			Debug.Assert(toolstripMenuItem != null);

			if(toolstripMenuItem != null)
			{
				string strCommand = ToolstripUtility.GetCommand(toolstripMenuItem);

				switch(strCommand)
				{
					case "save":
						bool bForceSave = Executer_MainForm.s_optionsManager.GetContainerBoolByName("allow_saving_unmodified_solutions", false);
						if(bForceSave == true)
							this.IsDirty = true;
						Saveable.RecursiveSave(this, true, false);
						break;
					case "saveas":
						Saveable.SaveAs(this, null, false);
						break;
					case "close":
						this.Close(CloseMethod.AlwaysAsk);
						break;
					case "task":
						Debug.Assert(false, "This should not be clickable.");
						break;
					case "properties":
						MessageBox.Show("Solution properties not yet implemented.");
						break;
					case "addproject":
						OpenFileDialog ofd = new OpenFileDialog();
						ofd.CheckFileExists = true;
						ofd.Multiselect = false;
						ofd.Filter = "Executer Project Files (*.xpr;*.xml)|*.xpr;*.xml|All Files (*.*)|*.*";
						ofd.DefaultExt = ".xpr";
						DialogResult dr = ofd.ShowDialog(this.TreeView);

						if(dr == DialogResult.OK)
						{
							if(GetProjectByFilename(ofd.FileName) != null)
							{
								UtilityFunctions.ShowError(String.Format("The project\n'{0}'\nis already open in this solution.", ofd.FileName));
							}
							else
							{
								this.OwnerTreeView.AddOrFindProject(this, ofd.FileName);
							}
						}
						break;
					case "createnewproject":

						string strNewProjectFilename = ((ExecuterTreeView) this.TreeView).CreateNewProjectOnDisk(null, null);

						if(strNewProjectFilename != null)
						{
							this.OwnerTreeView.AddOrFindProject(this, strNewProjectFilename);
							//this.AddOrFindProject(strNewProjectFilename);
						}
						break;

					default:
						Debug.Assert(false, "Bad Item Clicked in treeSolutionObject.ItemClicked.");
						break;
				}
			}
		}

		private TreeProjectObject GetProjectByFilename(string strProject)
		{
			if(this.Nodes.Count != 0)
			{
				TreeNodeCollection tnc = this.Nodes;

				foreach(TreeProjectObject tpo in this.Nodes)
				{
					if(tpo.StrProjectDocumentFilename.ToLower() == strProject.ToLower())
						return tpo;
				}
				return null;
			}
			return null;
		}

		/// <summary>
		/// Returns a list of child projects that 
		/// </summary>
		/// <param name="strProjectDocumentFilename"></param>
		/// <returns></returns>
		internal List<TreeProjectObject> FindMatchingProjects(string strProjectDocumentFilename)
		{
			List<TreeProjectObject> projects = new List<TreeProjectObject>();

			foreach(TreeProjectObject tpo in this.Nodes)
			{
				if(tpo.StrProjectDocumentFilename.ToLower() == strProjectDocumentFilename.ToLower())
					projects.Add(tpo);
			}
			return projects;
		}
	}
}