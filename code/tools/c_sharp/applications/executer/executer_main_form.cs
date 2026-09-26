using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Net;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using System.Diagnostics;
using System.Collections;
using Executer.Farm;
using Executer.Helpers;
using Executer.Interfaces;
using Executer.TreeObjects;
using Global;
using MenuSystemLibrary;
using System.IO;
using message_handler;
using System.Reflection;
using System.Media;
using memory_variable;

/*
It is a common delusion that you make things better by talking about them.

Dame Rose Macaulay (1881 - 1958)
*/

namespace Executer
{
	public partial class ExecuterMainForm : Form
	{
		// TODO: Move these into the static Program class

		#region Delegates

		public delegate bool DelegateProcessCommandLine(ExecuterTreeView executerTreeView, String[] args);

		#endregion

		public static bool bDisallowCancelExitDialog = false;

		public static OptionsManager s_optionsManager;
		public static SearchDialog s_searchDialog;
		public static TreeObjectViewer s_treeObjectViewer;
		public bool bQuitMeansQuitAndNotMinimise = false;

		public bool m_bApplicationHasStartedSuccesfully = false;

		/// <summary>
		/// Set this to true if a self-update has been performed.
		/// Self-update will prompt for any saves needed before updating.
		/// We don't want the form re-prompting to save unsaved data on exit
		/// if an update has been done since another instance of the app. will be
		/// starting up and using the same project files.
		/// </summary>
		private bool m_bExitDisgracefully = false;

		public DelegateProcessCommandLine _DelegateProcessCommandLine;
		public ExecuterTreeView MainTreeView;


		public ExecuterMainForm()
		{
			InitializeComponent();

			_lastNonMinimizedState = WindowState;
			_lastState = WindowState;
			this.ShowInTaskbar = true;
			this.Text = Program.AssemblyTitle;
			//RemoteObject.SetupRemotingServer();
		}

		/// <summary>
		/// This is where the xml that describes the tools/configuration screen lives.
		/// </summary>
		public string ApplicationConfigurationPathFile
		{
			get { return Application.StartupPath + "\\Configuration.Executer"; }
		}


		public void MessageBoxError(string strError)
		{
			if(InvokeRequired == true)
			{
				this.Invoke((MethodInvoker)delegate() { MessageBoxError(strError); });
				return;
			}
			MessageHandler.Show(this, strError, "Executer", MessageBoxButtons.OK, MessageBoxIcon.Error);
		}

		/// <summary>
		/// This is used to intercept keystrokes on the tree view in
		/// order to e.g. process the 'delete' key.
		/// </summary>
		/// <param name="keyData"></param>
		/// <returns></returns>
		protected override bool ProcessDialogKey(System.Windows.Forms.Keys keyData)
		{
			// TODO: SLN:
			System.Windows.Forms.Keys key = keyData;
			Debug.WriteLine(key.ToString());
			//if(ExecuterMainForm.m_bDebugModeEnabled == true && key == System.Windows.Forms.Keys.F5)
			if(key == System.Windows.Forms.Keys.F5)
			{
				if(this.MainTreeView.SelectedNode != null)
				{
					// Find what we want to reload ...
					TreeBaseObject treeObjectToReload = this.MainTreeView.SelectedNode as TreeBaseObject;
					DoReload(treeObjectToReload);
				}
				return true;
			}
			else if(key == System.Windows.Forms.Keys.Escape)
			{
				this.treeViewMain.RestoreCutColour();

				this.treeViewMain.CancelCut();

				return true;
			}
			else if(key == (System.Windows.Forms.Keys.S | System.Windows.Forms.Keys.Control | Keys.Shift))
			{
				Saveable.RecursiveSave(this.treeViewMain, true, false);
				return true;
			}
			else if(key == (System.Windows.Forms.Keys.S | System.Windows.Forms.Keys.Control))
			{
				{
					TreeBaseObject tbo = this.treeViewMain.SelectedNode as TreeBaseObject;

					ISaveable thingtoSave = null;

					bool bForceSave = false;

					if(tbo is TreeSolutionObject)
					{
						thingtoSave = tbo as ISaveable;
						bForceSave = ExecuterMainForm.s_optionsManager.GetContainerBoolByName("allow_saving_unmodified_solutions", false);
					}
					else if(tbo is TreeProjectObject)
					{
						thingtoSave = tbo as ISaveable;
						bForceSave = ExecuterMainForm.s_optionsManager.GetContainerBoolByName("allow_saving_unmodified_projects", false);
					}
					else if(tbo is TreeUserObject)
					{
						thingtoSave = ((TreeUserObject)tbo).OwnerProjectObject as ISaveable;
						bForceSave = ExecuterMainForm.s_optionsManager.GetContainerBoolByName("allow_saving_unmodified_projects", false);
					}

					if(thingtoSave != null)
					{
						bool bReadOnly;
						if(thingtoSave.ReadableState == RestrictedMode.ReadOnly || thingtoSave.ReadableState == RestrictedMode.Restricted)
							bReadOnly = true;
						else
							bReadOnly = false;

						if(thingtoSave.IsDirty || (bForceSave == true && bReadOnly == false))
						{
							Saveable.RecursiveSave(thingtoSave, false, false);
						}
					}
				}
				return true;
			}
			return base.ProcessDialogKey(keyData);
		}


		public bool DoReload(TreeBaseObject treeObjectToReload)
		{
			Debug.Assert(treeObjectToReload != null);

			do // If treeObjectToReload can't reload itself, look up the tree until ICanReloadSelf is found.
			{
				if(treeObjectToReload is ICanReloadSelf == false)
					treeObjectToReload = treeObjectToReload.Parent as TreeBaseObject;

				if(treeObjectToReload == null)
					return false;
				//return base.ProcessDialogKey(keyData);
			} while(treeObjectToReload is ICanReloadSelf == false);

			DialogResult dr = MessageHandler.ShowMemory(this, "Reload", "Reload on pressing F5?", "Reload '" + treeObjectToReload.Text + "'?", "Executer", MessageBoxButtons.YesNo, MessageBoxIcon.Error);

			if(dr == DialogResult.Yes)
			{
				string strLastNodePath = this.MainTreeView.SelectedNode.FullPath;
				TreeBaseObject replacedTreeObject = ((ICanReloadSelf)treeObjectToReload).ReLoad();

				if(replacedTreeObject != null)
				{
					try
					{
						this.MainTreeView.SelectedNode = MainTreeView.GetTreeNodeFromPath(strLastNodePath);
					} // This shouldn't go wrong but I don't care if it does.
					catch
					{
					}
					return true;
				}
				else
				{
					MessageHandler.ShowMemory(this, "Reload failed", "Skip warning when reload fails?", "F5 reload failed. Reverting to original project.", "Executer", MessageBoxButtons.OK, MessageBoxIcon.Error);
				}
			}
			return false;
		}


		public bool Initialise(string[] args)
		{
			m_bApplicationHasStartedSuccesfully = _Initialise(args);
			return m_bApplicationHasStartedSuccesfully;
		}


		private bool _Initialise(string[] args)
		{
			// Don't start everything up if we only then want to shut it all down.
			// Only consider the first parameter since the presence of leading parameters implies other actions need to be performed.
			if(args.Length != 0)
			{
				if(args[0] == "-a")
					return false;
			}

			MainTreeView = this.treeViewMain;

			MainTreeView.SetToolStripContainer(this.toolStripContainer);
			MainTreeView.LoadOrCreateConfigFile(); // Does what it says.

			//Program.s_farmClientManager = new FarmClientManager(true);

			//XmlElement xValues = m_executerTreeView.m_xConfig["root"]["options"];
			XmlElement xValues = MainTreeView._xConfig.SelectSingleNode("root/options") as XmlElement;
			if(xValues == null)
			{
				XmlElement xNewOptions = MainTreeView._xConfig.CreateElement("options");
				MainTreeView._xConfig["root"].AppendChild(xNewOptions);
				xValues = xNewOptions;
			}

			s_optionsManager = CreateOptionsManager(xValues);
			s_treeObjectViewer = new TreeObjectViewer();
			s_treeObjectViewer.Dock = DockStyle.Fill;
			this.panelMenu.Controls.Add(s_treeObjectViewer);

			s_searchDialog = new SearchDialog();

			//this._DelegateProcessCommandLine = new DelegateProcessCommandLine(CommandLineInterface.ProcessCommandLine);

			MainTreeView.ApplicationPath = Application.ExecutablePath;

			m_strXRestorePos = MemoryVariable.GetMemory("FormXPos");
			m_strYRestorePos = MemoryVariable.GetMemory("FormYPos");

			DialogResult drIsDebugModeOn = MessageHandler.GetDialogResultFromString(MemoryVariable.GetMemory("Is debug mode on"));

			if(drIsDebugModeOn == DialogResult.Yes)
			{
				(this.toolsToolStripMenuItem.DropDown.Items["debugModeToolStripMenuItem"] as ToolStripMenuItem).Checked = true;
				Program.s_bDebugModeEnabled = true;
			}

			DialogResult drIsAdvancedModeOn = MessageHandler.GetDialogResultFromString(MemoryVariable.GetMemory("Is advanced mode on"));

			if(drIsAdvancedModeOn == DialogResult.Yes)
			{
				(this.toolsToolStripMenuItem.DropDown.Items["advancedModeToolStripMenuItem"] as ToolStripMenuItem).Checked = true;
				Program.s_bAdvancedModeEnabled = true;
			}

			DialogResult drLoadLastConfig;

			if(args.Length == 0 || args[0] == "ignore")
			{
				if(MainTreeView.IsLastConfigEmpty() == false)
				{
					drLoadLastConfig = MessageHandler.ShowMemory
						(this.Parent, "ID_LoadLastConfiguration", "Load last configuration on startup?", "Load last configuration?", Program.AssemblyTitle, MessageBoxButtons.YesNo, MessageBoxIcon.Question);
				}
				else
				{
					drLoadLastConfig = DialogResult.No;
				}
			}
			else
			{
				drLoadLastConfig = DialogResult.Yes;
				//				dr = DialogResult.No;			/// Need to selectively load the last config. (E.g. '-a' should not load last config!)
			}

			DialogResult drHideFarm = MessageHandler.GetDialogResultFromString(MemoryVariable.GetMemory("Hide local farm when launching"));

			if(drHideFarm == DialogResult.Yes || drHideFarm == DialogResult.None)
			{
				(this.toolsToolStripMenuItem.DropDown.Items["hideLocalFarmWhenLaunchingToolStripMenuItem"] as ToolStripMenuItem).Checked = true;
				Program.s_bHideLocalFarmWhenLaunching = true;
			}

			if(drLoadLastConfig == DialogResult.Yes)
			{
				MainTreeView.LoadLastConfiguration();
			}

			
			//if(_DelegateProcessCommandLine(MainTreeView, args) == false) // Last configuration has been loaded.
			// ATTENTION: This is rubbish because the main form has not been constructed at this point.
			// See RemotingService.GetRemotingService().EnqueueArgs(args);
			//if(CommandLineInterface.ProcessCommandLine(MainTreeView, args) == false) // Last configuration has been loaded.
			//{
			//    Program.s_farmClientManager.ShutdownServers(true);
			//    return false;
			//}

			ToolstripUtility.SelectToolStripItemsByFlag(this.toolsToolStripMenuItem.DropDownItems, "[DEBUG]", Program.s_bDebugModeEnabled, true);
			ToolstripUtility.SelectToolStripItemsByFlag(this.toolsToolStripMenuItem.DropDownItems, "[ADVANCED]", Program.s_bAdvancedModeEnabled, true);

			return true;
		}

		/// <summary>
		/// Used to select and open a document. The existing documents are left intact.
		/// </summary>
		[Obsolete]
		private void AppendFile()
		{
			OpenFileDialog ofd = new OpenFileDialog();
			ofd.CheckFileExists = true;
			ofd.Multiselect = false;
			ofd.Filter = "Executer Project Files (*.xpr;*.xml)|*.xpr;*.xml|All Files (*.*)|*.*";
			ofd.DefaultExt = ".xpr";
			ofd.FileOk += new CancelEventHandler(ofd_AppendOk);
			ofd.ShowDialog(this);
		}

		/// <summary> This looks at a given farmserver or all farmservers and waits until the jobqueue for the server(s) is empty.
		/// This should be quick as long as the farmserver is behaving itself; the queues are emptied by a thread
		/// that has to wait for a TCP confirmation after each job request.
		/// </summary>
		/// <param name="farmClient">The FarmClient to wait on, or null to wait on all clients.</param>
		/// <param name="bShowProgress">If true, a simple progress form is displayed for the duration of the wait.</param>
		/// <returns></returns>
		private static bool WaitForQueuedJobsToBeSentToTheFarm(FarmClient farmClient, bool bShowProgress)
		{
			// Wait for the jobs to be sent out to the farm, then bail out.
			int d = 30;
			int queuedJobs = 0;

			List<FarmClient> clients = new List<FarmClient>();

			if(farmClient != null)
				clients.Add(farmClient);
			else
			{
				foreach(FarmClient fc in Program.s_farmClientManager.m_farmClients.Values)
				{
					clients.Add(fc);
				}
			}

			int oldNumberOfQueuedJobs = -1;

			SimpleMessageBox simpleMessageBox = null;

			foreach(FarmClient se2 in clients)
			{
				do
				{
					queuedJobs = se2.GetNumberOfQueuedJobs();
					Console.WriteLine("ProcessCommandLine is waiting for the job queue to be empty. There are " + queuedJobs.ToString() + " left on server '" + se2.StrLocalName + "'.");

					if(bShowProgress)
					{
						if(simpleMessageBox == null)
						{
							simpleMessageBox = new SimpleMessageBox("Queued jobs .", "Executer");
							simpleMessageBox.Show(null);
						}

						simpleMessageBox.LabelText = "Server: '" + se2.StrLocalName + "' Remaining jobs to de-queue:\n" + queuedJobs.ToString();
						simpleMessageBox.Update();
					}

					if(queuedJobs != oldNumberOfQueuedJobs)
						d = 250;

					oldNumberOfQueuedJobs = queuedJobs;

					d--;

					System.Threading.Thread.Sleep(100);
				} while(queuedJobs != 0 && d != 0);
			}
			if(simpleMessageBox != null)
				simpleMessageBox.Close();

			return (queuedJobs == 0);
		}

		private OptionsManager CreateOptionsManager(XmlElement xValues)
		{
			XmlDocument xDoc;

			xDoc = new XmlDocument();

			try
			{
				xDoc.Load(ApplicationConfigurationPathFile);
			}
			catch(Exception ex)
			{
				MessageHandler.Show(this, "Your Application configuration file is missing or corrupt.\nThe system reports: '" + ex.Message + "'", "Executer", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
				return null;
			}

			XmlElement xOptionsTemplate = XmlHelper.GetNodeFromPath(xDoc, "root/options_template");

			if(xOptionsTemplate != null)
			{
				try
				{
					return new OptionsManager(xOptionsTemplate, xValues);
				}
				catch(Exception ex)
				{
					MessageHandler.Show(this, "Your Application configuration file is corrupt.\nThe system reports: '" + ex.Message + "'", "Executer", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
				}
			}
			else
			{
				MessageHandler.Show(this, "Your Application configuration file is corrupt.\nIt does not contain the entry 'root/options_template'", "Executer", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
			}

			return null;
		}

		private void useTreeviewCheckboxesToolStripMenuItem_Click(object sender, EventArgs e)
		{
			ToolStripMenuItem tsMenuItem = sender as ToolStripMenuItem;

			StoreTreeExpandedState();

			if(tsMenuItem.Checked == true)
			{
				tsMenuItem.Checked = false;
				this.treeViewMain.CheckBoxes = false;
				SetAllProjectCheckedness(true); // Everything is checked when checkboxes are not present.
				this.treeViewMain.ClearStoredCheckedItems();
				tsmiCloseAllFinishedTasks.Enabled = true;
				autorenameSelectedToolStripMenuItem.Enabled = false;
				ExecuterMainForm.s_treeObjectViewer.ViewtreeObject(MainTreeView.SelectedNode as IViewableObject);
			}
			else
			{
				tsMenuItem.Checked = true;
				this.treeViewMain.CheckBoxes = true;
				SetAllProjectCheckedness(false); // Nothing is checked when checkboxes are first present.
				ExecuterMainForm.s_treeObjectViewer.ClearViewer();
				// TODO: Put some instructions for checkboxes into the view panel?
				//ExecuterMainForm.s_treeObjectViewer.ViewMultipleTreeObjects(m_executerTreeView);
				tsmiCloseAllFinishedTasks.Enabled = false;
				autorenameSelectedToolStripMenuItem.Enabled = true;
			}
			ExecuterMainForm.s_searchDialog.buttonSelectInTree.Enabled = this.treeViewMain.CheckBoxes;

			this.Focus();
			RestoreTreeExpandedState();
		}


		private bool SetAllProjectCheckedness(bool bValue)
		{
			List<TreeUserObject> results = ExecuterTreeUtilities.GetAllUserObjects(Program.ExecuterMainForm.MainTreeView.Nodes);

			foreach(TreeUserObject treeUserObject in results)
			{
				XmlHelper.AddAttribute(treeUserObject.XTreeNodeMenuData, "checked", bValue.ToString().ToLower());
			}
			return true;
		}

		private void toolStripButtonSearch_Click(object sender, EventArgs e)
		{
			s_searchDialog.Show(this);
		}

		private void toolStripButtonSaveAll_Click(object sender, EventArgs e)
		{
			Saveable.RecursiveSave(this.treeViewMain, true, false);
		}


		private void toolStripStatusLabel1_LocationChanged(object sender, EventArgs e)
		{
		}

		private void toolStripButtonSearch_Click_1(object sender, EventArgs e)
		{
			string strNodeName = null;
			string strContainerName = null;
			string strContainerValue = null;
			string strTypeName = null;
			string strInternalName = null;

			if(string.IsNullOrEmpty(this.toolStripComboBoxSearchFor.Text) == false)
			{
				switch(this.toolStripComboBoxSearchIn.Text)
				{
					case "Node name":
						strNodeName = this.toolStripComboBoxSearchFor.Text;
						break;
					case "Container name":
						strContainerName = this.toolStripComboBoxSearchFor.Text;
						break;
					case "Container value":
						strContainerValue = this.toolStripComboBoxSearchFor.Text;
						break;
					case "Type name":
						strTypeName = this.toolStripComboBoxSearchFor.Text;
						break;
					case "Internal name":
						strInternalName = this.toolStripComboBoxSearchFor.Text;
						break;
					default:
						MessageHandler.Show(this, "The 'Search in' field is invalid.");
						return;
				}
				//Node name
				//Container name
				//Container value
				//Type name
				//Internal name
				ITreeNodeContainer anchorNode = Program.ExecuterMainForm.MainTreeView;
				ITreeNodeContainer currentNode = Program.ExecuterMainForm.MainTreeView.SelectedNode as ITreeNodeContainer ?? anchorNode;


				SearchDetails searchDetails = new SearchDetails
					(
					strNodeName,
					strContainerName,
					strContainerValue,
					strTypeName,
					strInternalName,
					anchorNode,
					currentNode,
					currentNode
					);

				if(searchDetails != null)
				{
					ITreeNodeContainer tn = searchDetails.GetNextTreeNode();

					if(tn != null && tn is TreeNode)
						Program.ExecuterMainForm.MainTreeView.SelectedNode = tn as TreeNode;
				}
			}
		}

		private void toolStripComboBoxSearchFor_Leave(object sender, EventArgs e)
		{
			ToolStripComboBox cb = sender as ToolStripComboBox;

			if(cb != null)
			{
				if(cb.Items.Contains(cb.Text) == false)
				{
					cb.Items.Add(cb.Text);
				}
			}
		}

		#region File menu ...

		private void createNewSolutionToolStripMenuItem_Click(object sender, EventArgs e)
		{
			treeViewMain.AddNewSolutionNode();
		}

		private void openSolutionToolStripMenuItem_Click_1(object sender, EventArgs e)
		{
			string strSolutionFilename = null;


			string strFilter = "Executer Solution Files (*.Executersolution)|*.ExecuterSolution|All Files (*.*)|*.*";
			if(UtilityFunctions.GetOpenFilename(ref strSolutionFilename, strFilter, "Executer - Open Solution ...") == DialogResult.OK)
			{
				//TreeSolutionObject tso = new TreeSolutionObject(strSolutionFilename, this.treeViewMain);
				//this.treeViewMain.Nodes.Add(tso);
				//tso.ImageKey = "TreeSolutionIcon";
				//tso.SelectedImageKey = "TreeSolutionIcon";

				TreeSolutionObject tso = TreeSolutionObject.AddOrFindSolution(strSolutionFilename);
			}
		}

		/// <summary>
		/// Create and add a new project.
		/// If the current tree view selection is or is a descendant of a solution, add it to that solution
		/// otherwise create a new solution and put it there.
		/// </summary>
		/// <param name="sender">The main toolstrip. (File->Create new project ...)</param>
		/// <param name="e"></param>
		private void createNewProjectToolStripMenuItem_Click_1(object sender, EventArgs e)
		{
			TreeNode tn = this.MainTreeView.SelectedNode;

			while(tn != null && (tn is TreeSolutionObject == false))
			{
				tn = tn.Parent;
			}

			this.MainTreeView.CreateAndAddNewProject(null, null, tn as TreeSolutionObject);
		}

		/// <summary>
		/// Load an existing project into a new solution.
		/// If the project is already open in another solution, offer to
		/// select that project instead.
		/// </summary>
		/// <param name="sender">Toolstrip button.</param>
		/// <param name="e"></param>
		private void toolStripButtonLoadProject_Click(object sender, EventArgs e)
		{
			this.LoadProject();
		}

		/// <summary>
		/// Load an existing project into a new solution.
		/// If the project is already open in another solution, offer to
		/// select that project instead.
		/// </summary>
		/// <param name="sender">The main toolstrip. (File->Create new project ...)</param>
		/// <param name="e"></param>
		private void openExistingProjectToolStripMenuItem_Click(object sender, EventArgs e)
		{
			LoadProject();
		}

		private void LoadProject()
		{
			string strProjectFilename = null;
			const string strFilter = "Executer Project Files (*.xpr;*.xml)|*.xpr;*.xml|All Files (*.*)|*.*";

			if(UtilityFunctions.GetOpenFilename(ref strProjectFilename, strFilter, "Executer - Open Project ...") == DialogResult.OK)
			{
				if(ExecuterTreeUtilities.FindMatchingProjects(MainTreeView, strProjectFilename).Count != 0)
				{
					DialogResult dr = MessageBox.Show
						("The project\n'" + strProjectFilename + "'\nis already open in another Solution. Do you wish to open it in a new solution? (Not recommended)", "Executer - Project already open.", MessageBoxButtons.YesNo);

					if(dr != DialogResult.Yes)
						return;
				}
				MainTreeView.AddOrFindProject(null, strProjectFilename);
			}
		}

		private void closeAllSolutionsToolStripMenuItem_Click(object sender, EventArgs e)
		{
			PromptSave promptSave = new PromptSave(this.treeViewMain as ISaveable, true, true);
			if(promptSave.NumEntries != 0)
			{
				DialogResult dr = promptSave.ShowDialog(this);

				if(dr == DialogResult.Cancel)
				{
					return;
				}
			}

			List<TreeSolutionObject> solutions = new List<TreeSolutionObject>(this.treeViewMain.Nodes.Count);

			foreach(TreeSolutionObject tso in this.treeViewMain.Nodes)
			{
				solutions.Add(tso);
			}

			foreach(TreeSolutionObject tso in solutions)
			{
				tso.Close(CloseMethod.Force);
			}
		}

		private void saveAllToolStripMenuItem_Click(object sender, EventArgs e)
		{
			Saveable.RecursiveSave(this.treeViewMain, true, false);
		}

		private void closeExecuterToolStripMenuItem_Click(object sender, EventArgs e)
		{
			bQuitMeansQuitAndNotMinimise = true;
			this.Close();
		}

		#endregion file menu.

		#region UI callbacks

		public string[] m_args;
		private string m_strXRestorePos, m_strYRestorePos;

		/// <summary>
		/// If there are unsaved changes, prompt and offer to save.
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void Executer_MainForm_FormClosing(object sender, FormClosingEventArgs e)
		{
			DialogResult dr;
			if((WindowState != FormWindowState.Minimized) && (bQuitMeansQuitAndNotMinimise == false))
				dr = MessageHandler.ShowMemory
					(this, "MinimiseInsteadOfQuit", "Minimise on quit?", "Would you like to minimise to the system tray instead of quit?", Program.AssemblyTitle, MessageBoxButtons.YesNo, MessageBoxIcon.Question);
			else
				dr = DialogResult.None;

			if(WindowState != FormWindowState.Minimized && bQuitMeansQuitAndNotMinimise == false && dr == DialogResult.Yes)
			{
				WindowState = FormWindowState.Minimized;
				e.Cancel = true;

				return;
			}
			else
			{
				if((m_bExitDisgracefully == false) && (Program.m_sSilentCommandLine == false))
				{
					PromptSave promptSave = new PromptSave(this.treeViewMain, !bDisallowCancelExitDialog, true);
					//promptSave.Owner = this;

					if(promptSave.IsEmpty == false)
					{
						dr = promptSave.ShowDialog(this);

						if(dr == DialogResult.Cancel)
						{
							e.Cancel = true;
							return;
						}
					}
				}
				MainTreeView.SaveConfig();

				Program.s_farmClientManager.ShutdownServers(true);
			}
		}


		private void ofd_AppendOk(object sender, CancelEventArgs e)
		{
			OpenFileDialog ofd = sender as OpenFileDialog;

			if(ExecuterTreeUtilities.FindMatchingProjects(MainTreeView, ofd.FileName).Count != 0)
			{
				UtilityFunctions.ShowError(MainTreeView, "The project '" + ofd.FileName + "' is already open.");
			}
			else
			{
				// TODO: SLN: Add to Solution.
				MainTreeView.AddOrFindProject(null, ofd.FileName);
			}
		}


		/*		private void setDefaultValuesToolStripMenuItem_Click(object sender, EventArgs e)
				{

					if(m_treeObjectManager.m_treeRClickSelection.m_menuSystem == null)
						return;

					XmlElement xTreeNodeMenuData = m_treeObjectManager.m_treeRClickSelection.m_xTreeNodeMenuData;
					XmlElement xValues = xTreeNodeMenuData["values"];

					m_treeObjectManager.m_treeRClickSelection.m_menuSystem.SetMenu(null, true);					/// Merge with template.

					// Refresh the values panel directly or indirectly through a tree node selection callback.
					if(m_treeObjectManager.m_treeRClickSelection == treeViewMain.SelectedNode)
						m_treeObjectViewer.CreateNewMenuView(m_treeObjectManager.m_treeRClickSelection);
					else
						treeViewMain.SelectedNode = m_treeObjectManager.m_treeRClickSelection;
				}
		*/

		private void aboutToolStripMenuItem_Click(object sender, EventArgs e)
		{
			AboutBox ab = new AboutBox();
			ab.ShowDialog();
		}


		private void restoreToolStripMenuItem_Click(object sender, EventArgs e)
		{
			WindowState = _lastNonMinimizedState;
		}

		private void manageMemoryDialogsToolStripMenuItem_Click(object sender, EventArgs e)
		{
			MessageHandler.ManageMemory(this);
		}

		private void debugModeToolStripMenuItem_Click(object sender, EventArgs e)
		{
			ToolStripMenuItem tsMenuItem = sender as ToolStripMenuItem;

			if(MessageHandler.GetAnswer("debug mode warning") == DialogResult.No) // 'No' is forbidden.
				MessageHandler.SetAnswer("debug mode warning", "Ask");

			if(tsMenuItem.Checked == true)
			{
				DialogResult dr = MessageHandler.ShowMemory
					(this, "debug mode warning", "Warning: Do you want to be able to enable debug mode?", "Enabling debug mode may change the behaviour of Executer in unpredictable ways.\nAre you sure you wish to enable debug mode?",
					 "Executer", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
				if(dr != DialogResult.Yes)
				{
					tsMenuItem.Checked = false;
				}
			}

			Program.s_bDebugModeEnabled = tsMenuItem.Checked;

			MemoryVariable.SetMemory("Is debug mode on", tsMenuItem.Checked == true ? "Yes" : "No");
			ToolstripUtility.SelectToolStripItemsByFlag(this.toolsToolStripMenuItem.DropDownItems, "[DEBUG]", Program.s_bDebugModeEnabled, true);
			ToolstripUtility.SelectToolStripItemsByFlag(this.toolsToolStripMenuItem.DropDownItems, "[ADVANCED]", Program.s_bAdvancedModeEnabled, true);
		}

		private void advancedModeToolStripMenuItem_Click(object sender, EventArgs e)
		{
			ToolStripMenuItem tsMenuItem = sender as ToolStripMenuItem;

			Program.s_bAdvancedModeEnabled = tsMenuItem.Checked;

			MemoryVariable.SetMemory("Is advanced mode on", tsMenuItem.Checked == true ? "Yes" : "No");
			ToolstripUtility.SelectToolStripItemsByFlag(this.toolsToolStripMenuItem.DropDownItems, "[DEBUG]", Program.s_bDebugModeEnabled, true);
			ToolstripUtility.SelectToolStripItemsByFlag(this.toolsToolStripMenuItem.DropDownItems, "[ADVANCED]", Program.s_bAdvancedModeEnabled, true);
		}

		private void farmToolStripMenuItem_Click(object sender, EventArgs e)
		{
			Program.s_farmClientManager.ShowServerViewer();
		}

		private void saveToolStripMenuItem1_Click(object sender, EventArgs e)
		{
			ISaveable saveableObject = this.MainTreeView._treeRClickObjectSelection as ISaveable;

			if(saveableObject != null)
			{
				Saveable.RecursiveSave(saveableObject, true, false);
			}
		}

		private void debugOutputToolStripMenuItem_Click(object sender, EventArgs e)
		{
			DebugWindow.ShowDebugWindow(true);
		}

		private void configurationToolStripMenuItem_Click(object sender, EventArgs e)
		{
			if(s_optionsManager != null)
			{
				s_optionsManager.ShowDialog();
			}
		}

		private void validateMenuTemplatesToolStripMenuItem_Click(object sender, EventArgs e)
		{
			ToolStripMenuItem tsMenuItem = sender as ToolStripMenuItem;

			Program.s_bValidateMenuTemplates = tsMenuItem.Checked;
		}

		private void jobViewerToolStripMenuItem_Click(object sender, EventArgs e)
		{
			JobManager.ShowJobViewer();
		}

		private void hideLocalFarmWhenLaunchingToolStripMenuItem_Click(object sender, EventArgs e)
		{
			ToolStripMenuItem tsMenuItem = sender as ToolStripMenuItem;

			Program.s_bHideLocalFarmWhenLaunching = tsMenuItem.Checked;
			MemoryVariable.SetMemory("Hide local farm when launching", tsMenuItem.Checked == true ? "Yes" : "No");
		}


		private void tsmiCloseAllFinishedTasks_Click(object sender, EventArgs e)
		{
			// Tasks cannot be seen when CheckBoxes is true.
			Debug.Assert(this.MainTreeView.CheckBoxes == false);

			TreeUserObject tuoSelected = this.MainTreeView.SelectedNode as TreeUserObject;

			int numJobs = 0;
			if(tuoSelected != null)
				numJobs = tuoSelected.JobList.Count;

			List<TreeUserObject> results = ExecuterTreeUtilities.GetAllUserObjects(Program.ExecuterMainForm.MainTreeView.Nodes);

			foreach(TreeUserObject treeUserObject in results)
			{
				treeUserObject.CleanupJobs(false);
			}
		}

		private void splitter1_SplitterMoved(object sender, SplitterEventArgs e)
		{
			MemoryVariable.SetMemory("TreeSplitterWidth", this.splitter1.Location.X.ToString());
		}

		private void Executer_MainForm_Move(object sender, EventArgs e)
		{
			if(WindowState == FormWindowState.Normal)
			{
				MemoryVariable.SetMemory("FormXPos", this.Location.X.ToString());
				MemoryVariable.SetMemory("FormYPos", this.Location.Y.ToString());
			}
		}

		private void Executer_MainForm_Load(object sender, EventArgs e)
		{
			//			Initialise(m_args);

			#region Restore window size and splitter position ...

			string strWidth = MemoryVariable.GetMemory("FormWidth");
			string strHeight = MemoryVariable.GetMemory("FormHeight");

			string strTreeSplitterPosition = MemoryVariable.GetMemory("TreeSplitterWidth");

			int tempInt;

			if(int.TryParse(strWidth, out tempInt) == true && tempInt != 0) this.Width = tempInt;
			if(int.TryParse(strHeight, out tempInt) == true && tempInt != 0) this.Height = tempInt;
			if(int.TryParse(strTreeSplitterPosition, out tempInt) == true) splitter1.SplitPosition = tempInt;

			int xPos;
			int yPos;

			if(int.TryParse(m_strXRestorePos, out xPos) == true)
			{
				if(int.TryParse(m_strYRestorePos, out yPos) == true)
				{
					if(UtilityFunctions.CheckPointIsWithinWorkingAreaOnAnyScreen(xPos, yPos) == true)
					{
						this.Location = new Point(xPos, yPos);
					}
				}
			}

			#endregion Restore window size and splitter position.
		}

		private void toolStripButtonFind_Click(object sender, EventArgs e)
		{
			s_searchDialog.Show(this);
		}

		#region Minimise to tray

		public FormWindowState _lastNonMinimizedState;
		public FormWindowState _lastState;
		private int oldHScrollPos = 0;
		private int oldVScrollPos = 0;

		private void notifyIcon1_DoubleClick(object sender, EventArgs e)
		{
			WindowState = _lastNonMinimizedState;
			this.Activate();
			this.BringToFront();
		}

		private void closeToolStripMenuItem1_Click(object sender, EventArgs e)
		{
			this.Close();
		}

		private void Executer_MainForm_Resize(object sender, EventArgs e)
		{
			if(WindowState == FormWindowState.Normal)
			{
				MemoryVariable.SetMemory("FormWidth", this.Width.ToString());
				MemoryVariable.SetMemory("FormHeight", this.Height.ToString());
			}

			bool bNeedToRestore = false;

			if(WindowState == FormWindowState.Minimized)
				StoreTreeExpandedState();
			else if(_lastState == FormWindowState.Minimized)
				bNeedToRestore = true;


			if(WindowState == FormWindowState.Minimized && (s_optionsManager == null || s_optionsManager.GetContainerBoolByName("minimise_to_tray", true) == true))
			{
				ShowInTaskbar = false;
				notifyIcon1.Visible = true;
			}
			else
			{
				ShowInTaskbar = true;
				notifyIcon1.Visible = false;
				// Store the non-minimized window state.
				_lastNonMinimizedState = WindowState;
			}

			if(bNeedToRestore == true)
			{
				RestoreTreeExpandedState();
				Program.s_jobViewer.SetDesiredVisibility();
			}
			_lastState = WindowState;
		}

		private void RestoreTreeExpandedState()
		{
			this.treeViewMain.BeginUpdate();

			foreach(TreeBaseObject tbo in this.treeViewMain.Nodes)
			{
				if(tbo != null)
				{
					tbo.RestoreExpandedState();
				}
			}
			if(this.treeViewMain.SelectedNode != null)
			{
				this.treeViewMain.SelectedNode.EnsureVisible();
			}

			treeViewMain.HScrollPos = oldHScrollPos;
			treeViewMain.VScrollPos = oldVScrollPos;

			this.treeViewMain.EndUpdate();
		}

		private void StoreTreeExpandedState()
		{
			foreach(TreeBaseObject tbo in this.treeViewMain.Nodes)
			{
				if(tbo != null)
				{
					tbo.StoreExpandedState();
				}
			}
			oldHScrollPos = treeViewMain.HScrollPos;
			oldVScrollPos = treeViewMain.VScrollPos;
		}

		#endregion Minimise to tray

		private void autorenameSelectedToolStripMenuItem_Click(object sender, EventArgs e)
		{
			/// TODO:
			/// Get a list of all the selected nodes.
			/// If they aren't readonly, call auto-rename on them.

			List<TreeUserObject> results = ExecuterTreeUtilities.GetAllUserObjects(Program.ExecuterMainForm.MainTreeView.Nodes);

			foreach(TreeUserObject treeUserObject in results)
			{
				if(treeUserObject.Checked == true)
					treeUserObject.AutoRename();
			}
			return;
		}

		private void wikiPart1ToolStripMenuItem_Click(object sender, EventArgs e)
		{
			LaunchProcess.LaunchJob(@"http://biznet/index.php?title=Teams/Tools/User_Guides%2C_FAQs_and_Technical_Documents/Executer", null, null, false, false, null);
		}

		#endregion UI callbacks


	}
}

/*
Uploading and downloading files ... code snippet.
public form1()
{
    InitializeComponent();
// Download ...
    System.Net.WebClient Client = new WebClient();
    Stream strm = Client.OpenRead("http://www.csharpfriends.com");
    StreamReader sr = new StreamReader(strm);
    string line;
    do
    {
        line = sr.ReadLine();
        listbox1.Items.Add(line);
    }
    while (line !=null);
    strm.Close();

// upload ...

WebClient  Client = new WebClient();
Client.UploadFile("http://www.csharpfriends.com/Members/index.aspx", 
     "c:\wesiteFiles\newfile.aspx");

byte [] image;

//code to initialise image so it contains all the binary data for some jpg file
client.UploadData("http://www.csharpfriends.com/Members/images/logocc.jpg", image);


}


*/