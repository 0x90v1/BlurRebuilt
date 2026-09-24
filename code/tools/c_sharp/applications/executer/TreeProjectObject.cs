using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using System.IO;
using Executer.Helpers;
using Executer.Interfaces;
using Global;
using System.Diagnostics;
using message_handler;
using System.Drawing;

namespace Executer
{
	public class TreeProjectObject : TreeBaseObject, IRenameable, IRemoveable, IContextMenu, IViewableObject, IMoveable, ICanReceivePaste, ISaveable, ICanReloadSelf
	{
		private static readonly List<TreeProjectObject> s_listAllProjects = new List<TreeProjectObject>();
		internal readonly TreeUserObject _rootUserObject;

		internal readonly XmlDocument _xMenuProjectDocument; // A reference to the full project document.
		public XmlDocument XMenuTemplateDocument { get; private set; }
		internal readonly Dictionary<string, List<string>> _actionServersLookup = new Dictionary<string, List<string>>();
		internal readonly Dictionary<string, string> _mDictVariables;

		/// <summary>
		/// Lists any tree user objects that are obsolete according to the template.
		/// This is populated by the TreeUserObject constructor.
		/// </summary>
		private readonly List<TreeUserObject> _obsoleteTreeUserObjects = new List<TreeUserObject>();

		private RestrictedMode _restrictedMode;

		internal string _strTemplateDocumentFilename;

		/// <summary>
		/// Key is the name of a type in the template. value is the Xml Element of the type definition.
		/// (It matches type names to their definitions in the template.)
		/// </summary>
		private readonly Dictionary<string, XmlElement> _mTypeLookup = new Dictionary<string, XmlElement>();

		private CustomFileWatcher _customFileWatcher;

		public TreeProjectObject(string pathFilename)
		{
			_customFileWatcher = new CustomFileWatcher(pathFilename);
			_customFileWatcher.CustomFileWatcherEvent += new EventHandler<CustomFileWatcher.CustomFileWatcherEventArgs>(_customFileWatcherEvent);

			ChildTypes = new Dictionary<string, List<XmlElement>>();
			RequiredServerList = new Dictionary<string, XmlElement>();
			StrIconsPath = null;
			Debug.WriteLine("Initialising with'" + pathFilename + "'\n");

			this.ToolTipText = "Project '" + Path.GetFileNameWithoutExtension(pathFilename) + "'";

			XmlDocument menuTemplateDocument = new XmlDocument();
			XmlDocument menuProjectDocument = new XmlDocument();
			XmlDocument menuVariablesDocument = new XmlDocument();

			MacroExpander.MacroExpander mx = new MacroExpander.MacroExpander();
			mx.AddPair("%source_machine%", System.Net.Dns.GetHostName());
			mx.AddPair("%executer_folder%", Application.ExecutablePath);

			pathFilename = mx.Expand(pathFilename, true); // EXPAND

			mx.AddPair("%project_folder%", Path.GetDirectoryName(pathFilename));

			// Get the filename of the data definition document and also load that.
			try
			{
				// Set the cwd to the executable, so a relative project path is relative to the exe.
				Environment.CurrentDirectory = Path.GetDirectoryName(Application.ExecutablePath);
				menuProjectDocument.Load(pathFilename);
				// Set the cwd to the project, so if the template is on a relative path, it is relative to the project.
				Environment.CurrentDirectory = Path.GetDirectoryName(pathFilename);
			}
			catch(Exception ex)
			{
				throw new ApplicationException("Cannot load the data file\n'" + pathFilename + "'\nThe system reports the following error:\n" + ex.Message.ToString());
			}

			UtilityFunctions.FixupSpecialMenuCases(menuProjectDocument.DocumentElement);

			string strTemplateFilename;
			try
			{
				//XmlElement xTemplateFilename = menuProjectDocument["root"]["use_template"];
				XmlElement xTemplateFilename = menuProjectDocument.SelectSingleNode("root/use_template") as XmlElement;
				strTemplateFilename = XmlHelper.GetKeyValue(xTemplateFilename, "name");
			}
			catch(Exception ex)
			{
				//ShowError("'" + pathFilename + "' is corrupt. It does not have the entry 'root/use_template'.\nSystem reports: " + ex.Message);
				//return null;
				throw new ApplicationException("'" + pathFilename + "' is corrupt. It does not have the entry 'root/use_template'.\nSystem reports: " + ex.Message);
			}


			Dictionary<string, string> dictVariables = new Dictionary<string, string>();

			#region New variable stuff.

			try
			{
				strTemplateFilename = mx.Expand(strTemplateFilename, true);
				mx.AddPair("%template_folder%", Path.GetDirectoryName(strTemplateFilename));
				menuTemplateDocument.Load(strTemplateFilename);

				if(menuTemplateDocument.SelectSingleNode("/root/types") == null)
					throw new ApplicationException("The file '" + strTemplateFilename + "' is not a template file. It is missing the /root/types element.");

				try
				{
					XmlNodeList xVariablesNodes = menuTemplateDocument.SelectNodes("/root/use_variables");

					foreach(XmlElement xElement in xVariablesNodes)
					{
						if(xElement != null)
						{
							Debug.Assert(xElement.Name == "use_variables");

							string strVariablesFilename = XmlHelper.GetKeyValue(xElement, "filename");
							strVariablesFilename = mx.Expand(strVariablesFilename, true);

							bool bFailedToload = false;

							try
							{
								menuVariablesDocument.Load(strVariablesFilename);
							}
							catch(Exception ex)
							{
								bFailedToload = true;
								MessageHandler.ShowMemory
									(null, "load_variables_document_failure", "Cannot load the variables document.",
									 "Cannot load the 'use_variables' file specified in your template. Filename\n'" + strVariablesFilename + "'\nSystem reports: " + ex.Message, "Executer", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
							}

							if(bFailedToload == false)
							{
								mx.AddPair("%variables_folder%", Path.GetDirectoryName(strVariablesFilename));

								Dictionary<string, string> tempVariables = GetVariablesFromDocument(menuVariablesDocument);

								foreach(KeyValuePair<string, string> kvp in tempVariables)
								{
									if(dictVariables.ContainsKey(kvp.Key) == false)
										dictVariables.Add(kvp.Key, kvp.Value);
									else
									{
										MessageHandler.ShowMemory
											(null, "Duplicated memory variable",
											 "Do not warn about duplicate variables?",
											 "The variable '" + kvp.Key + "' is redefined in the file\n'" + strVariablesFilename + "'\nused by the template\n'" + strTemplateFilename + "'\nin the project\n'" + pathFilename +
											 "'.\nThe original value of\n'" + dictVariables[kvp.Key] + "'\nwill be used.",
											 "Executer", MessageBoxButtons.OK,
											 MessageBoxIcon.Exclamation);

										Debug.WriteLine("AddRootItem: dictVariables already has :'" + kvp.ToString());
									}
								}
							}
						}
					}
					mx.AddDictionary(dictVariables);
				}
				catch
				{
					//ShowError("'" + pathFilename + "' is corrupt. It does not have the entry 'root/use_variables'.\nSystem reports: " + ex.Message);
					//return null;
				}

				#endregion

				XmlNodeList xIncludeNodes = menuTemplateDocument.SelectNodes("//include");

				int nestLevelsRemaining = 10;

				while(xIncludeNodes.Count > 0)
				{
					nestLevelsRemaining--;
					if(nestLevelsRemaining == 0)
						throw new ApplicationException("The template\n'" + strTemplateFilename + "'\nhas too many (>10) nested includes. You may have a cyclic dependency.");

					foreach(XmlElement xElementToReplace in xIncludeNodes)
					{
						try
						{
							ExpandElementAttributes(xElementToReplace, "include", mx);
						}
						catch
						{
						}
						string strIncludeFileName = XmlHelper.GetKeyValue(xElementToReplace, "filename");
						string strRootNodePath = XmlHelper.GetKeyValue(xElementToReplace, "root_node_path");

						if(strIncludeFileName != null)
						{
							XmlDocument xIncludedDocument = new XmlDocument();
							try
							{
								xIncludedDocument.Load(strIncludeFileName);
							}
							catch(Exception ex)
							{
								throw new ApplicationException
									("The template\n'" + strTemplateFilename + "'\nUsed by the project\n'" + pathFilename + "'\ncannot include the file\n'" + strIncludeFileName + "'" + "\nthe system reports:\n" + ex.Message);
							}

							if(strRootNodePath == null)
							{
								XmlElement xNodeToImport = xIncludedDocument.DocumentElement;
								if(xNodeToImport == null)
									throw new ApplicationException("The template\n'" + strTemplateFilename + "'\nUsed by the project\n'" + pathFilename + "'\ncannot find the root node of the included file\n'" + strIncludeFileName + "'");

								XmlElement xImportedNode = (XmlElement)menuTemplateDocument.ImportNode(xNodeToImport, true);

								xElementToReplace.ParentNode.ReplaceChild(xImportedNode, xElementToReplace);
							}
							else
							{
								//xNodeToImport = (XmlElement)xIncludedDocument.SelectSingleNode(strRootNodePath);
								XmlNodeList xNodesToImport = xIncludedDocument.SelectNodes(strRootNodePath);
								if(xNodesToImport == null)
									throw new ApplicationException
										("The template\n'" + strTemplateFilename + "'\nUsed by the project\n'" + pathFilename + "'\ncannot find the element(s) at path\n'" + strRootNodePath + "'\nin the included file\n'" + strIncludeFileName + "'");

								XmlElement xParent = xElementToReplace.ParentNode as XmlElement;
								if(xParent != null)
								{
									foreach(XmlElement xNodeToImport in xNodesToImport)
									{
										if(xNodeToImport != null)
										{
											XmlElement xImportedNode = (XmlElement)menuTemplateDocument.ImportNode(xNodeToImport, true);
											//xElementToReplace.ParentNode.ReplaceChild(xImportedNode, xElementToReplace);
											xParent.InsertBefore(xImportedNode, xElementToReplace);
										}
									}
									xParent.RemoveChild(xElementToReplace);
								}
							}
						}
					}
					xIncludeNodes = menuTemplateDocument.SelectNodes("//include");
				}
			}
			catch(Exception ex)
			{
				//ShowError("Cannot load the template file\n'" + strTemplateFilename + "'\nThe system reports the following error:\n" + ex.Message.ToString());
				//return null;
				throw new ApplicationException("Cannot load the template file\n'" + strTemplateFilename + "'\nThe system reports the following error:\n" + ex.Message.ToString());
			}

			#region new variable stuff part 2. Why is it in 2 parts?

			// Add all contents of MacroExpander to dictVariables, since dictVariables is the lookup used by the ProjectData class.
			// Do not pass the MacroExpander to the ProjectData constructor, since the ProjectData class gives it's dictVariables to a Job class that
			// creates it's own custom MacroExpander from the dictVariables plus some of it's own macros specific to the job.

			Dictionary<string, string> macroDictionary = mx.GetDictionary();

			foreach(KeyValuePair<string, string> kvp in macroDictionary)
			{
				if(dictVariables.ContainsKey(kvp.Key) == false)
					dictVariables.Add(kvp.Key, kvp.Value);
				else
				{
					Debug.WriteLine("AddRootItem: dictVariables already has :'" + kvp.ToString());
				}
			}

			#endregion

			try
			{
				//ExpandTypeAttributes(menuTemplateDocument["root"]["types"], mx);
				ExpandTypeAttributes(menuTemplateDocument.SelectSingleNode("root/types") as XmlElement, mx);
			}
			catch
			{
			}

			try
			{
				//ExpandServerAttributes(menuTemplateDocument["root"]["servers"], mx);
				ExpandServerAttributes(menuTemplateDocument.SelectSingleNode("root/servers") as XmlElement, mx);
			}
			catch
			{
			}


			try
			{
				XmlElement xElement = menuTemplateDocument.SelectSingleNode("root/tree_icon_folder") as XmlElement;
				StrIconsPath = XmlHelper.GetKeyValue(xElement, "name");
				if(StrIconsPath != null)
				{
					StrIconsPath = mx.Expand(StrIconsPath, true);

					//AddImages(m_strIconsPath);
				}
			}
			catch
			{
			}

			XMenuTemplateDocument = menuTemplateDocument;
			_xMenuProjectDocument = menuProjectDocument;
			_mDictVariables = dictVariables;
			StrProjectDocumentFilename = pathFilename;
			_strTemplateDocumentFilename = strTemplateFilename;
			IsProjectDocumentDirty = false;

			PopulateLocalServers();

			FileAttributes fileAttributes = File.GetAttributes(pathFilename);
			if((fileAttributes & FileAttributes.ReadOnly) == FileAttributes.ReadOnly)
			{
				//m_bIsReadonly = true;
				this.ReadableState = RestrictedMode.ReadOnly;
			}

			try
			{
				//XmlElement xElement = menuTemplateDocument["root"]["config"];
				XmlElement xElement = menuTemplateDocument.SelectSingleNode("root/config") as XmlElement;

				StrIcon1 = StrIconsPath + "\\" + XmlHelper.GetKeyValue(xElement, "icon1");
				StrIcon2 = StrIconsPath + "\\" + XmlHelper.GetKeyValue(xElement, "icon2");

				string strOwnerProductName = XmlHelper.GetKeyValue(xElement, "owner_product_name");
				if(strOwnerProductName != null)
				{
					strOwnerProductName = mx.Expand(strOwnerProductName, true);
					XmlHelper.AddAttribute(xElement, "owner_product_name", strOwnerProductName);
				}

				this.StrFarmTempFolder = XmlHelper.GetKeyValue(xElement, "farm_temp_folder");

				string strOverrideTempFolder = Environment.GetEnvironmentVariable("EXECUTER_TEMP");

				if(strOverrideTempFolder != null)
				{
					MessageHandler.WriteLine("Temp. folder overidden by environment variable to '" + strOverrideTempFolder + "'");
					this.StrFarmTempFolder = strOverrideTempFolder;
				}
				this.StrFarmTempFolder = mx.Expand(this.StrFarmTempFolder, true); // NEW EXPAND

				try
				{
					Directory.CreateDirectory(this.StrFarmTempFolder);
				}
				catch
				{
					throw new Exception
						("The farm temporary folder for the project '" +
						 pathFilename +
						 "' does not exist and cannot be created.\n" +
						 "Please check file permissions and your xml template.\n" +
						 "The template specifies '" +
						 this.StrFarmTempFolder +
						 "' as your temporary folder.\n" +
						 "The project will not be loaded.");
				}

				UtilityFunctions.DeleteOldFiles(this.StrFarmTempFolder, "Ex_*", 7);
			}
			catch(Exception ex)
			{
				throw (ex);
			}

			if(ParseTypes() != false)
			{
				if(ParseActions() != false)
				{
					// Types and actions parsed okay. (They throw an exception if they fail.)
				}
			}


			// Create all the child nodes of this project ...

			{
				//XmlElement xElementy = this.m_xMenuProjectDocument["root"]["root_type_entry"];

				//// Version increment. Change root_type_entry to type_entry.
				//if(xElementy != null)
				//    XmlHelper.RenameNode(xElementy, null, "type_entry");

				Debug.Assert(this._obsoleteTreeUserObjects.Count == 0);

				XmlNodeList xRootNodes = this._xMenuProjectDocument.SelectNodes("/root/type_entry");

				bool bExpandThisLater = false;

				//foreach(XmlElement xRootTypeEntry in xRootNodes)
				{
					//XmlElement xRootTypeEntry = this.m_xMenuProjectDocument["root"]["root_type_entry"];
					XmlElement xRootTypeEntry = this._xMenuProjectDocument.SelectSingleNode("root/root_type_entry") as XmlElement;

					//XmlHelper.AddAttribute(xRootTypeEntry, "type_name", "root_node");
					TreeUserObject treeRootItem = new TreeUserObject(this, xRootTypeEntry, true);

					this.Text = XmlHelper.GetKeyValue(treeRootItem.XTreeNodeMenuData, "name");

					_rootUserObject = treeRootItem;

					foreach(TreeUserObject tuo in treeRootItem.Nodes)
					{
						#region blah

						string strImageKey = XmlHelper.GetKeyValue(tuo.XTypeTemplate, "icon1");
						string strSelectedImageKey = XmlHelper.GetKeyValue(tuo.XTypeTemplate, "icon2");

						if(strImageKey != null)
							strImageKey = this.StrIconsPath + "\\" + strImageKey;
						else
							strImageKey = "Unknown";

						if(strSelectedImageKey != null)
							strSelectedImageKey = this.StrIconsPath + "\\" + strSelectedImageKey;
						else
							strSelectedImageKey = "Unknown";

						ImageKey = strImageKey;
						SelectedImageKey = strSelectedImageKey;

						#endregion blah

						//treeRootItem.Nodes.Remove(tuo); // Not while we're enumerating treeRootItem!

						this.AddWithIcons(tuo, strImageKey, strSelectedImageKey);
					}

					if(treeRootItem.IsExpanded)
						bExpandThisLater = true;


					//_rootUserObject.Nodes.Clear();	// 
				}

				if((_obsoleteTreeUserObjects.Count != 0) && (this.ReadableState == RestrictedMode.Writable))
				{
					string strRemovedItems = string.Format("YOUR PROJECT TEMPLATE HAS CHANGED AND {0} NODE{1} OBSOLETE ...", _obsoleteTreeUserObjects.Count.ToString(), (_obsoleteTreeUserObjects.Count == 1 ? " IS" : "S ARE"));

					strRemovedItems += string.Format("\n\nDO YOU WISH TO REMOVE {0} PERMANENTLY FROM YOUR PROJECT?", (_obsoleteTreeUserObjects.Count == 1 ? "IT" : "THEM"));
					strRemovedItems += "\n\n" + this.Text;
					DialogResult dr = MessageHandler.Show(null, strRemovedItems, "Executer - Remove obsolete items", MessageBoxButtons.YesNo, MessageBoxIcon.Exclamation);

					if(dr == DialogResult.Yes)
					{
						dr = MessageHandler.Show
							(null, "ARE YOU SURE? IF YOU PROCEED AND IT WAS A MISTAKE TO DO SO, DO NOT SAVE YOUR PROJECT\n\n" + this.Text, "Executer - Remove obsolete items", MessageBoxButtons.YesNo, MessageBoxIcon.Exclamation);

						if(dr == DialogResult.Yes)
						{
							foreach(TreeUserObject tuo in _obsoleteTreeUserObjects)
							{
								tuo.Close(CloseMethod.Force);
							}
						}
					}
				}

				if(this.IsExpanded == false &&
				   (Executer_MainForm.s_optionsManager.GetContainerBoolByName("restore_tree_layout", false) &&
				    XmlHelper.GetKeyBool(this._xMenuProjectDocument.DocumentElement, "expanded_in_ui", false) ||
				    (XmlHelper.GetKeyBool(this._xMenuProjectDocument.DocumentElement, "force_node_expansion", false) &&
				     Executer_MainForm.s_optionsManager.GetContainerBoolByName("ignore_forced_tree_node_expansions", false) == false) ||
				    bExpandThisLater == true
				   ) == true)
				{
					this.Expand();

					TreeNode tn = this.Parent;

					// Nonsense. Parent is always null.
					while(tn != null && tn.IsExpanded == false)
					{
						Debug.Assert(false);
						tn.Expand();
						tn = tn.Parent;
					}
				}
			}

			AddToGlobalProjectsList(this);
		}

		private delegate void MethodInvoker_customFileWatcher_ReadOnlyChangedEvent(object sender, CustomFileWatcher.CustomFileWatcherEventArgs e);

		private void _customFileWatcherEvent(object sender, CustomFileWatcher.CustomFileWatcherEventArgs e)
		{
			if(Program.S_executerMainForm.InvokeRequired)
			{
				Program.S_executerMainForm.Invoke(new MethodInvoker_customFileWatcher_ReadOnlyChangedEvent(_customFileWatcherEvent), new object[] { sender, e });
				return;
			}

			if(e.ReadOnlyHasChanged == true)
			{
				if(e.IsReadOnly == true)
				{
					this.ReadableState = RestrictedMode.ReadOnly;
					RefreshViews();
				}
				else
				{
					// TODO: See if the project should be 'restricted' or writable.
					RemoveReadonlyness();
					RefreshViews();
				}
			}

			if(e.LastWriteTimeHasChanged == true)
			{
				// This will not fire if the write time was modified by Executer.

				// The readonlyness of this project has changed.
				DialogResult dr = MessageHandler.ShowMemory(Program.S_executerMainForm, "ReloadWhenLastWriteTimeChanges", "Reload when project is modified externally?",
				                                            "The project\n'" + this.StrProjectDocumentFilename + "'\nhas been modified extrnally.\nDo you want to Reload it?", "Executer", MessageBoxButtons.YesNo, MessageBoxIcon.Error);
				if(dr == DialogResult.Yes)
				{
					if(this.IsDirty == true)
					{
						dr = MessageHandler.ShowMemory(Program.S_executerMainForm, "ReloadWhenReadonlynessChanges_Confirm", "Reload when project is modified externally even if it has been modified?",
						                               "PAY ATTENTION!\n\nYOU ARE RELOADING THE PROJECT\n'" + this.StrProjectDocumentFilename + "'\n\nYOU WILL LOSE ANY CHANGES YOU HAVE MADE.\n\nDO YOU WANT TO PROCEED?\n\nIF YOU DECLINE, YOU RISK OBLITERATING SOMEONE ELSE'S WORK IF YOU SUBSEQUENTLY SAVE THIS PROJECT AND CHECK IT IN.", "Executer", MessageBoxButtons.YesNo, MessageBoxIcon.Error);
						if(dr == DialogResult.Yes)
						{
							this.IsDirty = false;
							ExecuterTreeView etv = this.TreeView as ExecuterTreeView;

							string strLastNodePath = etv.SelectedNode.FullPath;
							TreeProjectObject newTpo = this.ReLoad() as TreeProjectObject;
							try
							{
								newTpo.TreeView.SelectedNode = etv.GetTreeNodeFromPath(strLastNodePath);
							}
							catch
							{
							}
						}
					}
				}
			}
		}

		private void RefreshViews()
		{
			/// TODO: Close any context menus. Does invalidating the treeview do this?

			/// Redraw the tree view.
			this.TreeView.Invalidate();
			/// Recreate and redraw the menu system.
			if(this.TreeView.CheckBoxes == false)
			{
				Executer_MainForm.s_treeObjectViewer.ViewtreeObject((this.TreeView as ExecuterTreeView).SelectedNode as IViewableObject);
			}
			else
			{
				Executer_MainForm.s_treeObjectViewer.ViewMultipleTreeObjects(this.TreeView as ExecuterTreeView);
			}
		}

		private void RemoveReadonlyness()
		{
			ExecuterTreeView etv = this.TreeView as ExecuterTreeView;
			List<TreeProjectObject> treeProjectObjects = ExecuterTreeUtilities.FindMatchingProjects(etv, this.StrProjectDocumentFilename);
			if(treeProjectObjects.Count > 1) // If this project is loaded elsewhere ...
			{
				this.ReadableState = RestrictedMode.Restricted;
			}
			else
			{
				this.ReadableState = RestrictedMode.Writable;
			}
		}

		public string StrProjectDocumentFilename { get; private set; }

		public override string TypeName
		{
			get { return "Project"; }
		}

		public bool IsProjectDocumentDirty { get; private set; }

		public string StrFarmTempFolder { get; private set; }

		internal Dictionary<string, XmlElement> RequiredServerList { get; private set; }

		public string StrIconsPath { get; private set; }

		public string StrIcon1 { get; private set; }

		public string StrIcon2 { get; private set; }

		public Color Colour { get; private set; }

		/// <summary>
		/// Key is the name of a type in the template. value is a List of XmlElements that describe valid child types for this type.
		/// (It matches a type name to a list of it's 'inheritable nodes'.)
		/// </summary>
		public Dictionary<string, List<XmlElement>> ChildTypes { get; private set; }

		#region ICanReceivePaste Members

		public bool CanReceivePaste(Moveable thingToPaste)
		{
			if(_rootUserObject != null)
				return _rootUserObject.CanReceivePaste(thingToPaste);
			return false;
		}

		public bool Paste(Moveable thingToPaste)
		{
			if(_rootUserObject != null)
				return _rootUserObject.Paste(thingToPaste);

			return false;
		}

		public bool CanReceiveSpecialPaste(Moveable thingToPaste)
		{
			if(_rootUserObject != null)
				return _rootUserObject.CanReceivePaste(thingToPaste);
			return false;
		}

		public bool PasteSpecial(Moveable thingToPaste)
		{
			if(_rootUserObject != null)
				return _rootUserObject.PasteSpecial(thingToPaste);

			return false;
		}

		#endregion

		#region ICanReloadSelf Members

		public TreeBaseObject ReLoad()
		{
			// Hackitty hack. Urgh.
			TreeSolutionObject treeParent = this.Parent as TreeSolutionObject;

			if(treeParent != null)
			{
				if(this.IsDirty == true)
				{
					DialogResult dr = MessageHandler.Show(null, "Save changes to your Project?", Program.AssemblyTitle, MessageBoxButtons.YesNoCancel, MessageBoxIcon.Warning);
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

				bool bIsSolutionDirty = treeParent.IsDirty;

				// Temporarily remove the current project so that reloading it doesn't clash with the existing one.
				int index = this.Index;
				this.Remove();

				//TreeProjectObject replacementTpo = new TreeProjectObject(this.m_strProjectDocumentFilename);
				TreeProjectObject replacementTpo = treeParent.OwnerTreeView.AddOrFindProject(treeParent, this.StrProjectDocumentFilename);

				// ATTENTION, null reference on reload project, caused by failure to reload the project.
				if(replacementTpo != null)
				{
					treeParent.Nodes.Remove(replacementTpo);
					treeParent.Nodes.Insert(index, replacementTpo);
				}
				// Add the old one back so it can be either closed properly or left in place.
				treeParent.Nodes.Insert(index, this);

				if(replacementTpo != null)
				{
					bool bIsProjectDirty = this.IsProjectDocumentDirty;

					this.IsProjectDocumentDirty = false; // Prevent the 'Close' function from prompting for a save.

					if(this.Close(CloseMethod.Force) == true)
					{
						//replacementTpo.SelectedImageKey = replacementTpo.StrIcon1;
						//replacementTpo.ImageKey = StrIcon2;
						//treeParent.Nodes.Insert(index, replacementTpo);

						//this.Remove();

						//treeParent.OwnerTreeView
						treeParent.IsDirty = bIsSolutionDirty;
						return replacementTpo as TreeBaseObject;
					}
					else
						this.IsProjectDocumentDirty = bIsProjectDirty; // If the replacement fails, restore the dirty flag.
					treeParent.IsDirty = bIsSolutionDirty;

					// Then remove it again.
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

			//cms.Items.Add(ToolstripUtility.CreateTsmi("renameProjectTsmi", "<rename>", "Rename", tsmi_Click, !this.IsReadonly));
			cms.Items.Add(ToolstripUtility.CreateTsmi("addChildTsmi", null, "Add Child", null, !bReadOnly));
			cms.Items.Add
				(ToolstripUtility.CreateTsmi
				 	("saveProjectTsmi", "<save>", "Save Project", tsmi_Click, this.IsDirty || (!bReadOnly && Executer_MainForm.s_optionsManager.GetContainerBoolByName("allow_saving_unmodified_projects", false))));
			cms.Items.Add(ToolstripUtility.CreateTsmi("saveProjectAsTsmi", "<saveas>", "Save Project As ...", tsmi_Click, true));
			cms.Items.Add(ToolstripUtility.CreateTsmi("closeProjectTsmi", "<close>", "Close Project", tsmi_Click, this.CanClose()));
			cms.Items.Add(ToolstripUtility.CreateTss(""));
			cms.Items.Add(ToolstripUtility.CreateTsmi("projectTasksTsmi", "<task>", "Tasks", tsmi_Click, true));
			cms.Items.Add(ToolstripUtility.CreateTsmi("cleanupTsmi", "<closefinishedtasks>", "Close finished Tasks", tsmi_Click, true));
			cms.Items.Add(ToolstripUtility.CreateTsmi("closeAllFinishedTasksTsmi", "<closeallfinishedtasks>", "Close all finished tasks", tsmi_Click, true));
			cms.Items.Add(ToolstripUtility.CreateTsmi("propertiesTsmi", "<properties>", "Properties", tsmi_Click, true));

			XmlElement xMenu = this._rootUserObject.XTreeNodeMenuData.SelectSingleNode("values/container") as XmlElement;

			cms.Items.Add(ToolstripUtility.CreateTsmi("showTsmi", "<showinseperatewindow>", "Show in seperate window", tsmi_Click, xMenu != null));

			ToolStripMenuItem tmiAdd = ToolstripUtility.FindFirstToolsTripMenuItemByText(cms.Items, "Add Child");
			if(tmiAdd != null)
			{
				if(bReadOnly == false)
				{
					tmiAdd.Enabled = true;
					this._rootUserObject.AddChildTypes(this._rootUserObject.Name, tmiAdd, _rootUserObject);
				}
				else
					tmiAdd.Enabled = false;
			}

			ToolStripMenuItem tmiTasks = ToolstripUtility.FindFirstToolsTripMenuItemByText(cms.Items, "Tasks");
			if(tmiTasks != null)
			{
				this._rootUserObject.AddTasks(tmiTasks, false, false);
				//this.AddProjectTasks(tmiTasks, true, true, projectTaskToolStripMenuItem_Click);

				if(tmiTasks.DropDownItems.Count == 0)
					tmiTasks.Enabled = false;
			}

			cms.Items.Add(ToolstripUtility.CreateTss(null));
			bool bForceExpansion = XmlHelper.GetKeyBool(this._xMenuProjectDocument.DocumentElement, "force_node_expansion", false);

			ToolStripMenuItem forceExpansionTsmi = ToolstripUtility.CreateTsmi("forceExpansionTsmi", "[ADVANCED]<forceexpansion>", "Force node expansion on load", new EventHandler(tsmi_Click), bReadOnly == false);
			forceExpansionTsmi.CheckOnClick = true;
			forceExpansionTsmi.Checked = bForceExpansion;
			cms.Items.Add(forceExpansionTsmi);

			if(this.ReadableState == RestrictedMode.Restricted)
			{
				cms.Items.Add(ToolstripUtility.CreateTss(null));
				cms.Items.Add(ToolstripUtility.CreateTsmi("primaryRestrictedTsmi", "<primary>", "Set as primary project", new EventHandler(tsmi_Click), true));
			}

			return cms;
		}

		#endregion

		#region IMoveable Members

		public bool CanMoveUp
		{
			get
			{
				TreeSolutionObject parentSolution = this.Parent as TreeSolutionObject;

				if(parentSolution != null)
				{
					bool bParentReadOnly;
					if(parentSolution.ReadableState == RestrictedMode.ReadOnly || parentSolution.ReadableState == RestrictedMode.Restricted)
						bParentReadOnly = true;
					else
						bParentReadOnly = false;
					if(this.Index > 0)
						return !bParentReadOnly;
				}
				return false;
			}
		}

		public bool MoveUp()
		{
			TreeSolutionObject parentSolution = this.Parent as TreeSolutionObject;

			if(parentSolution != null && this.Index > 0)
			{
				int index = this.Index;

				parentSolution.Nodes.Remove(this);
				parentSolution.Nodes.Insert(index - 1, this);
				return true;
			}
			return false;
		}

		public bool CanMoveDown
		{
			get
			{
				TreeSolutionObject parentSolution = this.Parent as TreeSolutionObject;

				if(parentSolution != null)
				{
					bool bParentReadOnly;
					if(parentSolution.ReadableState == RestrictedMode.ReadOnly || parentSolution.ReadableState == RestrictedMode.Restricted)
						bParentReadOnly = true;
					else
						bParentReadOnly = false;

					if(this.Index < (parentSolution.Nodes.Count - 1))
						return !bParentReadOnly;
				}
				return false;
			}
		}

		public bool MoveDown()
		{
			TreeSolutionObject parentSolution = this.Parent as TreeSolutionObject;

			if(parentSolution != null && this.Index < (parentSolution.Nodes.Count - 1))
			{
				int index = this.Index;

				parentSolution.Nodes.Remove(this);
				parentSolution.Nodes.Insert(index + 1, this);
				return true;
			}
			return false;
		}

		public bool CanCut
		{
			get
			{
				TreeSolutionObject parentSolution = this.Parent as TreeSolutionObject;

				if(parentSolution != null)
				{
					bool bParentReadOnly;
					if(parentSolution.ReadableState == RestrictedMode.ReadOnly || parentSolution.ReadableState == RestrictedMode.Restricted)
						bParentReadOnly = true;
					else
						bParentReadOnly = false;

					return !bParentReadOnly;
				}
				return false;
			}
		}

		public Moveable Cut()
		{
			return new Moveable(this, null, MoveType.Cut);
		}

		public bool CanCopy
		{
			get { return true; }
		}

		public Moveable Copy()
		{
			return new Moveable(this, null, MoveType.Copy);
			//throw new ApplicationException("'Copy' is not possible on a project node.");
		}

		#endregion

		#region IRemoveable Members

		public bool CanClose()
		{
			return true;
		}

		public bool Close(CloseMethod closeMethod)
		{
			if(this.IsDirty == true)
			{
				DialogResult dr = MessageHandler.Show(null, "Save changes to project\n'" + this.StrProjectDocumentFilename + "'?", Program.AssemblyTitle, MessageBoxButtons.YesNo, MessageBoxIcon.Warning);
				switch(dr)
				{
					case DialogResult.Yes:
						{
							if(Saveable.RecursiveSave(this, true, false) != SaveableResult.Success)
								return false;
						}
						break;
					case DialogResult.No:
						{
						}
						break;
						//case DialogResult.Cancel:
						//    {
						//        return false;
						//    }
					default:
						break;
				}
			}

			// Clear the properties panel ...
			//Executer_MainForm.s_treeObjectViewer.ClearViewer();

			// Clear any recorded 'checks' for this project.
			ExecuterTreeView etv = this.TreeView as ExecuterTreeView;
			etv.SetCheck(this, false, true);

			RemoveFromGlobalProjectsList(this);

			this._customFileWatcher.Stop();
			this._customFileWatcher = null;

			ISaveable SaveableParent = this.Parent as ISaveable;

			if(SaveableParent != null)
				SaveableParent.IsDirty = true;

			this.Remove();

			etv.UpdateReadableState(this.StrProjectDocumentFilename);

			return true;
		}

		#endregion

		#region IRenameable Members

		public bool CanRename()
		{
			RestrictedMode rm = this.ReadableState;
			if(rm == RestrictedMode.PrimaryRestricted || rm == RestrictedMode.Writable)
				return true;
			else
				return false;
		}

		public bool Rename(string newName)
		{
			XmlElement xElement = this._rootUserObject.XTreeNodeMenuData;

			if(!string.IsNullOrEmpty(newName))
			{
				XmlHelper.AddAttribute(xElement, "name", newName);
				this.Text = newName;
				this.IsDirty = true;
				return true;
			}
			return false;
		}

		public string DisplayName
		{
			get { return this.Text; }
		}

		#endregion

		#region ISaveable Members

		public bool LogicallyCanBeSaved
		{
			get { return true; }
		}

		public RestrictedMode ReadableState
		{
			get { return _restrictedMode; }
			set
			{
				if(value == RestrictedMode.ReadOnly || value == RestrictedMode.Restricted)
				{
					//Debug.Assert(this.IsDirty == false, "Error. Setting a project's restricted mode to '" + value.ToString() + "' when it is dirty.");
				}
				if(value == RestrictedMode.ReadOnly)
				{
					this.SetColour(R_READONLY_COLOUR);
				}
				else if(value == RestrictedMode.Restricted)
				{
					this.SetColour(R_RESTRICTED_COLOR);
				}
				else if(value == RestrictedMode.PrimaryRestricted)
				{
					this.SetColour(R_PRIMARY_RESTRICTED_COLOR);
				}
				else if(value == RestrictedMode.Writable)
					this.SetColour(R_WRITABLE_COLOUR);

				_restrictedMode = value;
			}
		}

		public void Save()
		{
			SaveAs(this.StrProjectDocumentFilename);
		}

		public void SaveAs(string strNewName)
		{
			string strCurrentDirectory = null;

			if(Executer_MainForm.s_optionsManager.GetContainerBoolByName("store_tree_layout", true) == true)
			{
				StoreTreeExpandiness(this.TreeView.Nodes);
			}

			try
			{
				strCurrentDirectory = Environment.CurrentDirectory;
				Environment.CurrentDirectory = Path.GetDirectoryName(Application.ExecutablePath);

				this._customFileWatcher.Suspend();
				this._xMenuProjectDocument.Save(strNewName);
				this._customFileWatcher.Resume();

				StrProjectDocumentFilename = strNewName;
				this.StrProjectDocumentFilename = strNewName;
				//this.IsReadonly = false;
				// ATTENTION RESTRICTED : Ensure that we haven't just overwritten a project that is loaded elsewhere.
				//Debug.Assert(false);

				Debug.Assert((this.ReadableState == RestrictedMode.PrimaryRestricted) || (this.ReadableState == RestrictedMode.Writable));
				//this.ReadableState = RestrictedMode.Writable;

				if(this.ReadableState == RestrictedMode.PrimaryRestricted)
				{
					/// 1) Reload the other duplicated projects.
					/// 2) Restore the current tree selection.

					TreeNode tnSelected = this.TreeView.SelectedNode;

					List<TreeProjectObject> treeProjectObjects = ExecuterTreeUtilities.FindMatchingProjects(this.TreeView as ExecuterTreeView, strNewName);

					foreach(TreeProjectObject tpo in treeProjectObjects)
					{
						if(tpo != this)
						{
							tpo.ReLoad();
						}
						else
						{
							Debug.Assert(tpo.ReadableState == RestrictedMode.PrimaryRestricted); // Of course it is. See preceeding 'if'.
						}
					}
					this.TreeView.SelectedNode = tnSelected;
				}


				this.IsDirty = false;
				this.ToolTipText = "Project '" + Path.GetFileNameWithoutExtension(strNewName) + "'";

				return;
			}
			catch(Exception ex)
			{
				throw ex;
			}
			finally
			{
				if(strCurrentDirectory != null)
					Environment.CurrentDirectory = strCurrentDirectory;
			}
		}

		/// <summary>
		/// Gets or sets whether the project document has been modified.
		/// </summary>
		public bool IsDirty
		{
			get { return IsProjectDocumentDirty; }
			set
			{
				IsProjectDocumentDirty = value;
				//if(value == true)
				//{
				//    ISaveable saveableParent = this.Parent as ISaveable;

				//    if(saveableParent != null)
				//    {
				//        saveableParent.IsDirty = true;
				//    }
				//}
			}
		}

		public string SaveName
		{
			get { return this.StrProjectDocumentFilename; }
		}

		public bool CanHaveSaveableChildren
		{
			get { return false; }
		}

		public string FileFilter
		{
			get { return "Executer Project Files (*.xpr;*.xml)|*.xpr;*.xml|All Files (*.*)|*.*"; }
		}

		public string SaveAsDialogTitle
		{
			get { return "Save Project '" + Path.GetFileName(this.SaveName) + "' as ..."; }
		}

		public List<ISaveable> SaveableChildren
		{
			get { throw new NotImplementedException(); }
		}

		#endregion

		#region IViewableObject Members

		public XmlElement RawXmlDocument
		{
			get { return this._rootUserObject.XTreeNodeMenuData; }
		}

		public Control CreateView()
		{
			TreeUserObjectViewer treeUserObjectViewer = new TreeUserObjectViewer(this._rootUserObject);
			return treeUserObjectViewer;

			//MenuSystemLibrary.MenuSystem menuSystem = new MenuSystemLibrary.MenuSystem();
			//menuSystem.m_bReadOnly = true;

			//menuSystem.Initialise(null);

			////menuSystem.MenuChangedEvent += 

			//XmlDocument xDoc = new XmlDocument();

			//xDoc.Load(new StringReader(
			//                "<container name = 'properties' type = 'basic' display_name = 'Project Properties'>" +
			//                    "<container name='project_name' display_name='Filename' type='string' pre_format='cr;ytab(15);'  width='600' />" +
			//                    "<container name='template_name' display_name='Template filename' type='string'	pre_format='cr;ytab(15);'  width='600' />" +
			//                    "<container name='temporary_folder' display_name = 'Temporary folder' type='string'	pre_format='cr;ytab(15);'  width='600' />" +
			//                    "<container name='readonly' display_name = '' type='label'	pre_format='cr;ytab(15);' />" +
			//                    "<container name = 'required_servers' display_name='Required Servers' type='basic' pre_format='cr;ytab(15);'/>" +
			//                "</container>"
			//            ));

			//XmlNode myNode = xDoc.SelectSingleNode("//container[@name='project_name']");
			//XmlHelper.AddAttribute(myNode, "value", this.m_strProjectDocumentFilename);

			//myNode = xDoc.SelectSingleNode("//container[@name='template_name']");
			//XmlHelper.AddAttribute(myNode, "value", this.m_strTemplateDocumentFilename);

			//myNode = xDoc.SelectSingleNode("//container[@name='temporary_folder']");
			//XmlHelper.AddAttribute(myNode, "value", this.m_strFarmTempFolder);

			//myNode = xDoc.SelectSingleNode("//container[@name='readonly']");


			////if(this.m_bIsReadonly == true)
			////    XmlHelper.AddAttribute(myNode, "value", "Project is readonly");
			////else
			////    XmlHelper.AddAttribute(myNode, "value", "Project is writable");

			//string strReadableValue;

			//switch(this.ReadableState)
			//{
			//    case RestrictedMode.Writable:
			//        strReadableValue = "Project is Writable";
			//        break;
			//    case RestrictedMode.ReadOnly:
			//        strReadableValue = "Project is Readonly";
			//        break;
			//    case RestrictedMode.Restricted:
			//        strReadableValue = "Project cannot be edited as it is loaded elsewhere.\nSet it to be the Primary Project to edit it.";
			//        break;
			//    case RestrictedMode.PrimaryRestricted:
			//        strReadableValue = "Project is loaded elsewhere. This instance is editable because it is the Primary Project.";
			//        break;
			//    default:
			//        strReadableValue = "ERROR";
			//        break;
			//}
			//XmlHelper.AddAttribute(myNode, "value", strReadableValue);


			//myNode = xDoc.SelectSingleNode("//container[@name='required_servers']");

			//foreach(string strServer in this.RequiredServerList.Keys)
			//{
			//    XmlElement xNew = xDoc.CreateElement("container");
			//    XmlHelper.AddAttribute(xNew, "name", "");
			//    XmlHelper.AddAttribute(xNew, "type", "label");
			//    XmlHelper.AddAttribute(xNew, "pre_format", "cr;");
			//    XmlHelper.AddAttribute(xNew, "value", strServer);
			//    myNode.AppendChild(xNew);
			//}

			//menuSystem.ForceMenu(xDoc.FirstChild as XmlElement);

			//return menuSystem;
		}

		#endregion

		private static void AddToGlobalProjectsList(TreeProjectObject treeProjectObject)
		{
			s_listAllProjects.Add(treeProjectObject);
			UpdateFarmconnections();
		}

		private static void RemoveFromGlobalProjectsList(TreeProjectObject treeProjectObject)
		{
			Debug.Assert(s_listAllProjects.Contains(treeProjectObject));

			s_listAllProjects.Remove(treeProjectObject);
			UpdateFarmconnections();
		}

		private static bool UpdateFarmconnections()
		{
			List<XmlElement> xAllServers = new List<XmlElement>();

			foreach(TreeProjectObject tpo in s_listAllProjects)
			{
				XmlDocument xDefinition = tpo.XMenuTemplateDocument;

				XmlElement xServers = xDefinition.SelectSingleNode("root/servers") as XmlElement;

				if(xServers != null)
				{
					xAllServers.Add(xServers);
				}
			}
			return Program.s_farmClientManager.PopulateFarmConnection(xAllServers);
		}

		private static Dictionary<string, string> GetVariablesFromDocument(XmlDocument xVariablesDoc)
		{
			Dictionary<string, string> dictLookup = new Dictionary<string, string>();
			try
			{
				//XmlElement xVars = xVariablesDoc["root"]["vars"];
				XmlElement xVars = xVariablesDoc.SelectSingleNode("root/vars") as XmlElement;

				foreach(XmlElement xVar in xVars)
				{
					if(xVar != null)
					{
						if(xVar.Name == "var")
						{
							string key = "%" + XmlHelper.GetKeyValue(xVar, "name") + "%";
							string value = XmlHelper.GetKeyValue(xVar, "value");

							if(dictLookup.ContainsKey(key) == true)
							{
								// Error
								Debug.WriteLine("Error - Duplicated variable name in document: '" + xVariablesDoc.Name + "'");
							}
							else
							{
								dictLookup.Add(key, value);
							}
						}
					}
				}
			}
			catch
			{
			}
			return dictLookup;
		}

		private static bool ExpandTypeAttributes(XmlElement xmlTypes, MacroExpander.MacroExpander mx)
		{
			if(xmlTypes != null)
			{
				foreach(XmlNode xChildNode in xmlTypes)
				{
					XmlElement xChildElement = xChildNode as XmlElement;

					if(xChildElement != null)
					{
						if(xChildElement.Name == "type")
						{
							try
							{
								XmlElement xRootContainer = xChildElement.SelectSingleNode("sub_menu_template/menu_system/container") as XmlElement;
								if(xRootContainer != null)
									ExpandElementAttributes(xRootContainer, "container", mx);
							}
							catch
							{
							}
						}
					}
				}
				return true;
			}
			return false;
		}

		private static bool ExpandServerAttributes(XmlElement xmlServers, MacroExpander.MacroExpander mx)
		{
			try
			{
				ExpandElementAttributes(xmlServers, "server_definition", mx);
			}
			catch
			{
			}
			return true;
		}

		private static bool ExpandElementAttributes(XmlElement xmlElement, string strElementName, MacroExpander.MacroExpander mx)
		{
			if(xmlElement != null)
			{
				if(strElementName == null || strElementName == "" || strElementName == xmlElement.Name)
				{
					foreach(XmlAttribute xAttribute in xmlElement.Attributes)
					{
						//bool bFound = false;
						//if(xAttribute.Name == "save")
						//{
						//    Debug.WriteLine("Found you.");
						//    bFound = true;
						//}
						xAttribute.Value = mx.Expand(xAttribute.Value, true);
						//if(bFound == true && xAttribute.Value == "")
						//{
						//    Debug.WriteLine("Found you again.");
						//    bFound = true;
						//}
					}
				}

				//string strType = xmlElement.Name;

				XmlNodeList xSourceNodes = xmlElement.ChildNodes;

				if(xSourceNodes != null)
				{
					foreach(XmlNode xChildNode in xSourceNodes)
					{
						XmlElement xChildElement = xChildNode as XmlElement;

						if(xChildElement != null)
						{
							ExpandElementAttributes(xChildElement, strElementName, mx);
						}
					}
				}
				return true;
			}
			return false;
		}

		/// <summary>
		/// Builds a map of all the available types and the child types they support.
		/// The map is stored in the root item tag associated with the tree root item.
		/// </summary>
		/// <param name="projectData">The project data for this project.</param>
		private bool ParseTypes()
		{
			XmlElement xTypes = XmlHelper.GetNodeFromPath(this.XMenuTemplateDocument, "root/types");

			if(xTypes == null)
			{
				//ShowError("The definition document is invalid.\nIt does not contain '<root><types>'");
				//return false;
				throw new ApplicationException("The definition document is invalid.\nIt does not contain '<root><types>'");
			}

			// First pass, build a dictionary that maps type names to their Xml definitions.
			foreach(XmlNode xType in xTypes)
			{
				if(xType.NodeType == XmlNodeType.Element)
				{
					if(xType.Name == "type")
					{
						string strTypeName = XmlHelper.GetKeyValue(xType, "name");
						if(this._mTypeLookup.ContainsKey(strTypeName))
						{
							throw new ApplicationException
								("Failed to load the project '" + this.StrProjectDocumentFilename + "'\n" +
								 "Your template has the type '" + strTypeName + "' multiply defined.");
						}

						this._mTypeLookup.Add(strTypeName, xType as XmlElement);
					}
				}
			}

			// Second pass, build a dictionary that maps type names to a List of the Xml definitions of all children the type can have.
			foreach(XmlNode xType in xTypes)
			{
				if(xType.NodeType == XmlNodeType.Element)
				{
					if(xType.Name == "type")
					{
						List<XmlElement> validChildren = new List<XmlElement>();

						XmlElement xInheritableTypes = xType["inheritable_types"];

						if(xInheritableTypes != null)
						{
							foreach(XmlNode xChildType in xInheritableTypes)
							{
								if(xChildType.NodeType == XmlNodeType.Element)
								{
									if(xChildType.Name == "inheritable_type")
									{
										string strChildTypeName = XmlHelper.GetKeyValue(xChildType, "name");
										//										validChildren.Add(xChildType as XmlElement);
										if(this._mTypeLookup.ContainsKey(strChildTypeName))
										{
											validChildren.Add(this._mTypeLookup[strChildTypeName]);
										}
										else
										{
											throw new ApplicationException
												("Template parse error.\nThe type '" + XmlHelper.GetKeyValue(xType, "name") +
												 "' claims it can have an inheritable_type called '" + strChildTypeName + "'\n" +
												 "where type '" + strChildTypeName + "' does not exist.");
										}
									}
								}
							}
						}
						string strTypeName = XmlHelper.GetKeyValue(xType, "name");
						this.ChildTypes.Add(strTypeName, validChildren);
					}
					else
					{
						string strQuestion = "Ignoring '" + xType.Name + "' in root/types.";
						MessageHandler.ShowMemory(null, "Unwanted elements in 'types' element", strQuestion, strQuestion, "Executer",
						                          MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
					}
				}
			}

			//// Third pass, add a dictionary entry for suitable root types.

			//XmlNodeList xRootChildTypes = xTypes.SelectNodes("type[@can_be_root='true']");
			//List<XmlElement> validRootChildren = new List<XmlElement>();
			//foreach(XmlNode xRootType in xRootChildTypes)
			//{
			//    validRootChildren.Add(xRootType as XmlElement);
			//}
			//this.m_childTypes.Add("*ROOT*", validRootChildren);

			return true;
		}

		/// <summary>
		/// Builds a map of all the available actions and the servers they can be sent to.
		/// The map is stored in the root item tag associated with the tree root item.
		/// </summary>
		/// <param name="treeRoot">The tree node that is at the root level of this tree.</param>
		private bool ParseActions()
		{
			XmlElement xActions = XmlHelper.GetNodeFromPath(this.XMenuTemplateDocument, "root/actions");

			if(xActions == null)
			{
				throw new ApplicationException("The definition document is invalid.\nIt does not contain '<root><actions>'");
			}

			foreach(XmlNode xAction in xActions)
			{
				if(xAction.NodeType == XmlNodeType.Element)
				{
					List<string> serverList = new List<string>();

					string strActionName = XmlHelper.GetKeyValue(xAction, "name");

					XmlElement xServerList = xAction["server_list"];

					if(xServerList != null)
					{
						foreach(XmlNode xChildServer in xServerList)
						{
							if(xChildServer.NodeType == XmlNodeType.Element)
							{
								if(xChildServer.Name == "server")
								{
									// TODO: Check that the server exists in the servers list.
									// Check the server list for this project, not the global list for all projects.

									string strServerName = XmlHelper.GetKeyValue(xChildServer, "local_name");
									if(this.RequiredServerList.ContainsKey(strServerName))
									{
										if(serverList.Contains(strServerName))
										{
											throw new ApplicationException("The action '" + strActionName + "' contains more than one server called '" + strServerName + "'");
										}
										else
											serverList.Add(strServerName);
									}
									else
									{
										throw new ApplicationException
											(String.Format("The action '{0}' refers to the server '{1}'\nThis server is not mentioned in 'root/servers' in your template.\nPlease inform your template designer.", strActionName, strServerName));
									}
								}
							}
						}
					}
					if(this._actionServersLookup.ContainsKey(strActionName))
					{
						throw new ApplicationException
							("Failed to load the project '" + this.StrProjectDocumentFilename + "'\n"
							 + "The action '" + strActionName + "' is multiply defined in your template.");
					}
					else
						this._actionServersLookup.Add(strActionName, serverList);
				}
			}

			return true;
		}

		/// <summary>
		/// Builds a dictionary of Xml definitions of servers needed by this project.
		/// The dictionary is keyed by the servers' local name.
		/// </summary>
		/// <returns>true</returns>
		private bool PopulateLocalServers()
		{
			XmlElement xServers = XmlHelper.GetNodeFromPath(XMenuTemplateDocument, "root/servers");
			RequiredServerList.Add("immediate", null);

			if(xServers != null)
			{
				foreach(XmlNode xNode in xServers)
				{
					if(xNode.NodeType == XmlNodeType.Element)
					{
						XmlElement xServer = xNode as XmlElement;
						if(xServer.Name == "server_definition")
						{
							try
							{
								string strLocalName = XmlHelper.GetKeyValue(xServer, "local_name");
								RequiredServerList.Add(strLocalName, xServer);
							}
							catch(Exception ex)
							{
								ex.ToString();
							}
						}
					}
				}
			}
			return true;
		}

		internal XmlElement GetTemplateTypeFromTypeName(string strTypeName)
		{
			if(strTypeName != null)
			{
				try
				{
					return _mTypeLookup[strTypeName] as XmlElement;
				}
				catch
				{
					return null;
				}
			}
			else
				return null;
		}

		internal bool DoTask(string strTaskName, bool bRecurse, bool bReportIfTaskDoesNotExist)
		{
			bool bRetval = true;

			TreeUserObject tuo = _rootUserObject;

			//CommandDetails cd = tuo.GetTaskByName(strTaskName, null);

			//tuo.DoTask(

			//foreach(TreeUserObject tuo in this.Nodes)
			//{
			CommandDetails cd = tuo.GetTaskByName(strTaskName, null);

			if(cd != null)
			{
				Debug.Assert(cd.m_treeUserObject == tuo);

				if(cd.m_strServerName != null)
				{
					//FarmClient se = Program.s_farmClientManager.FindClientByLocalName(cd.m_strServerName);

					//if(cd.m_strServerName != "immediate")
					{
						try
						{
							bRetval = cd.m_treeUserObject.DoTask(cd.m_strTaskName, cd.m_strServerName, cd.m_bRecurse);
						}
						catch(Exception ex)
						{
							MessageHandler.Show(Program.S_executerMainForm, "'TreeProjectObject.DoTask' failed.\nSystem reports: " + ex.Message, "Executer", MessageBoxButtons.OK, MessageBoxIcon.Error);
						}
					}
					//else
					//{
					//    try
					//    {
					//        bRetval = cd.m_treeUserObject.DoTask(cd.m_strTaskName, cd.m_strServerName, cd.m_bRecurse);
					//    }
					//    catch(Exception ex)
					//    {
					//        MessageHandler.Show(Program.S_executerMainForm, "'TreeProjectObject.DoTask' failed.\nSystem reports: " + ex.Message, "Executer", MessageBoxButtons.OK, MessageBoxIcon.Error);
					//    }
					//}
				}
				else
				{
					UtilityFunctions.ShowError(Program.S_executerMainForm, "Your template does not define a default server to execute this command on.\n(Default server is the first server in the list.)");
				}
			}
			else
			{
				if(bReportIfTaskDoesNotExist == true)
					UtilityFunctions.ShowError(Program.S_executerMainForm, "TreeProjectObject.DoTask: The default command to execute on this node could not be built.");
			}
			return bRetval;
			//}
			//return bRetval;
		}

		public static void StoreTreeExpandiness(TreeNodeCollection tnCollection)
		{
			bool bExpanded;

			foreach(TreeNode tn in tnCollection)
			{
				if(tn is TreeProjectObject)
				{
					TreeProjectObject tpo = tn as TreeProjectObject;
					bExpanded = tpo.IsExpanded;
					XmlHelper.AddAttribute(tpo._xMenuProjectDocument.DocumentElement, "expanded_in_ui", bExpanded.ToString());
				}
				else if(tn is TreeUserObject)
				{
					TreeUserObject tuo = tn as TreeUserObject;
					bExpanded = tuo.IsExpanded;

					Debug.Assert(tuo.XTreeNodeMenuData.Name == "type_entry" || tuo.XTreeNodeMenuData.Name == "root_type_entry");
					XmlHelper.AddAttribute(tuo.XTreeNodeMenuData, "expanded_in_ui", bExpanded.ToString());
				}
				else if((tn is TreeSolutionObject) == false)
				{
					Debug.Assert(false, "Bad tree node in tree.");
				}

				if(tn.Nodes != null)
					StoreTreeExpandiness(tn.Nodes);
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
					case "rename":
						this.BeginEdit();
						break;
					case "save":
						Saveable.RecursiveSave(this, false, false);
						break;
					case "saveas":
						Saveable.SaveAs(this, null, false);
						Saveable.RecursiveSave(this, true, false);
						break;
					case "close":
						this.Close(CloseMethod.AlwaysAsk);
						break;
					case "task":
						//Debug.Assert(false, "This should not be clickable.");
						break;
					case "closefinishedtasks":
						_rootUserObject.CleanupJobs(false);
						break;
					case "closeallfinishedtasks":
						_rootUserObject.CleanupJobs(true);
						break;
					case "properties":
						TreeObjectProperties top = new TreeObjectProperties(this);
						top.Show(null);
						break;
					case "showinseperatewindow":
						this._rootUserObject.showToolStripMenuItem_ClickExtracted();

						break;
					case "forceexpansiononload":
						XmlHelper.AddAttribute(this._xMenuProjectDocument.DocumentElement, "force_node_expansion", toolstripMenuItem.Checked.ToString());
						this.IsDirty = true;
						break;
					case "primary":
						/// TODO:
						/// 
						TreeSolutionObject treeParent = this.Parent as TreeSolutionObject;

						List<TreeProjectObject> treeProjectObjects = ExecuterTreeUtilities.FindMatchingProjects(treeParent.OwnerTreeView, this.StrProjectDocumentFilename);
						bool bDuplicateProjectFound = false;
						TreeProjectObject primaryrestrictedProject = null;
						foreach(TreeProjectObject tpoCurrent in treeProjectObjects)
						{
							Debug.Assert(tpoCurrent.StrProjectDocumentFilename.ToLower() == StrProjectDocumentFilename.ToLower());
							bDuplicateProjectFound = true;

							if(tpoCurrent == this)
							{
								Debug.Assert(tpoCurrent.ReadableState == RestrictedMode.Restricted);
							}
							else
							{
								//Debug.Assert((tpoCurrent.ReadableState == RestrictedMode.Restricted) || (tpoCurrent.ReadableState == RestrictedMode.PrimaryRestricted));
								if(tpoCurrent.ReadableState == RestrictedMode.PrimaryRestricted)
								{
									if(tpoCurrent.IsDirty == true)
									{
										DialogResult dr = MessageHandler.ShowMemory
											(null, "ID_SaveCurrentPrimaryProjectBeforeSettingPrimaryProject",
											 "Save Primary Project before changing it?", "Do you want to change the current modified Primary Project first?", Program.AssemblyTitle, MessageBoxButtons.YesNo, MessageBoxIcon.Question, true);
										if(dr == DialogResult.Yes || dr == DialogResult.OK)
										{
											if(Saveable.Save(tpoCurrent, false) != SaveableResult.Success)
												return;
										}
										else
											return;
									}
								}
								Debug.Assert(this.IsDirty == false);
								tpoCurrent.ReadableState = RestrictedMode.Restricted;
							}
						}

						string strLastNodePath = this.TreeView.SelectedNode.FullPath;

						treeParent.TreeView.BeginUpdate();
						foreach(TreeProjectObject tpoCurrent in treeProjectObjects)
						{
							if(tpoCurrent != this)
								tpoCurrent.ReLoad();
						}
						TreeProjectObject tpoNew = this.ReLoad() as TreeProjectObject;
						// 'this' is now an orphan. It has been removed from the tree and replaced with tpoNew.

						treeParent.TreeView.EndUpdate();

						tpoNew.ReadableState = RestrictedMode.PrimaryRestricted;

						try
						{
							tpoNew.TreeView.SelectedNode = (tpoNew.TreeView as ExecuterTreeView).GetTreeNodeFromPath(strLastNodePath);
						}
						catch
						{
						} // This shouldn't go wrong but I don't care if it does.


						//////////////////////////////////////


						//if((this.TreeView as ExecuterTreeView).CheckBoxes == false)
						//{
						//    Executer_MainForm.s_treeObjectViewer.ViewtreeObject(this as IViewableObject);
						//    //this.Focus();
						//}
						//////////////////////////////////////

						break;

					default:
						Debug.Assert(true, "Bad Item Clicked in treeSolutionObject.ItemClicked.");
						break;
				}
			}
		}


		//[Obsolete]
		//internal void AddProjectTasks(ToolStripMenuItem tmiSolutionTasks, bool bClearExistingTasks, bool bMergeDuplicates, EventHandler clickEventHandler)
		//{
		//    foreach(TreeUserObject tuo in this.Nodes)				// There should only be one, until code is overhauled.
		//    {
		//        XmlElement xTemplate = tuo.XTypeTemplate;

		//        if(xTemplate != null)
		//        {
		//            XmlElement xTaskList = xTemplate["task_list"];

		//            if(xTaskList != null)
		//            {
		//                List<string> listStrTasks = new List<string>();

		//                foreach(XmlNode xNode in xTaskList)
		//                {
		//                    if(xNode.NodeType == XmlNodeType.Element)
		//                    {
		//                        XmlElement xTask = xNode as XmlElement;
		//                        string strTaskName = XmlHelper.GetKeyValue(xTask, "name");
		//                        string strDisplayName = XmlHelper.GetKeyValue(xTask, "display_name");
		//                        string strActionName = XmlHelper.GetKeyValue(xTask, "action_name");

		//                        if(listStrTasks.Contains(strTaskName))
		//                        {
		//                            if(bMergeDuplicates == false)
		//                            {
		//                                ToolStripMenuItem newItem = tmiSolutionTasks.DropDownItems.Add("ERROR IN TEMPLATE : DUPLICATED TASK NAME '" + strTaskName + "'") as ToolStripMenuItem;
		//                                newItem.BackColor = System.Drawing.Color.Red;
		//                            }
		//                        }
		//                        else
		//                        {
		//                            listStrTasks.Add(strTaskName);

		//                            if((strDisplayName.ToLower().Contains("[debug]") == false) || (Program.s_bDebugModeEnabled == true))
		//                            {
		//                                if(strDisplayName.ToLower().Contains("[dummy]") == false)
		//                                {
		//                                    if(strActionName == "separator")
		//                                    {
		//                                        tmiSolutionTasks.DropDownItems.Add(new ToolStripSeparator());
		//                                    }
		//                                    else
		//                                    {
		//                                        ToolStripMenuItem newItem = tmiSolutionTasks.DropDownItems.Add(strDisplayName) as ToolStripMenuItem;
		//                                        newItem.Tag = strTaskName;

		//                                        newItem.Click += clickEventHandler;
		//                                    }
		//                                }
		//                            }
		//                        }
		//                    }
		//                }
		//            }
		//        }
		//    }
		//}

		//private void projectTaskToolStripMenuItem_Click(object sender, EventArgs e)
		//{
		//    ToolStripMenuItem tsmi = sender as ToolStripMenuItem;

		//    if (tsmi != null)
		//    {
		//        string strTaskName = tsmi.Tag as string;
		//        this.DoTask(strTaskName, true, true);
		//    }
		//}

		internal void AddObsoleteTreeNodeReference(TreeUserObject treeUserObject)
		{
			Debug.Assert(treeUserObject != null);

			if(treeUserObject != null)
				_obsoleteTreeUserObjects.Add(treeUserObject);
		}

		internal void SetColour(Color colour)
		{
			this.Colour = colour;

			_SetColour(this, colour);
		}

		private static void _SetColour(TreeNode treeNode, Color colour)
		{
			treeNode.ForeColor = colour;

			foreach(TreeNode treeChild in treeNode.Nodes)
			{
				_SetColour(treeChild, colour);
			}
		}
	}
}