using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using System.Collections;
using System.Xml;
using System.Diagnostics;
using Executer.Farm;
using Executer.Helpers;
using Executer.Interfaces;
using Executer.JobManagement;
using Executer.TreeObjects;
using Global;
using message_handler;
using System.IO;
using MenuSystemLibrary;
using System.Collections.ObjectModel;

namespace Executer
{
	public class TreeUserObject : TreeBaseObject, IRenameable, IRemoveable, IContextMenu, IViewableObject, IMoveable, ICanReceivePaste
	{
		private static bool _bShiftHeldDuringDoTask = false;

		/// <summary>An array of jobId's (as strings) that belong to this tree node.</summary>
		private readonly List<Job> _jobList = new List<Job>();

		public TreeUserObject(TreeProjectObject ownerProject, XmlElement xTreeNodeMenuData, bool bRecurse)
		{
			AllowedByTemplate = false;
			Debug.Assert(xTreeNodeMenuData != null);

			OwnerProjectObject = ownerProject;
			XTreeNodeMenuData = xTreeNodeMenuData;

			string strTypeName = XmlHelper.GetKeyValue(xTreeNodeMenuData, "type_name");
			this.Name = strTypeName;
			this.Text = XmlHelper.GetKeyValue(xTreeNodeMenuData, "name");

			if(xTreeNodeMenuData != null)
			{
				// Get the Xml template associated with this node.
				XTypeTemplate = GetTypeTemplate();

				#region TagData
				// Note, tagdata is only needed when an export is done, so we can add it in there instead?
				// Not sure what is best here. It may be useful to have it written into the project.
				// Remove any tagdata ...
				XmlElement xExistingTagData = xTreeNodeMenuData.SelectSingleNode("tagdata") as XmlElement;
				if(xExistingTagData != null)
				{
					xTreeNodeMenuData.RemoveChild(xExistingTagData);
				}

				// Get fresh tag data.
				if(XTypeTemplate != null)
				{
					XmlElement xFreshTagData = XTypeTemplate.SelectSingleNode("tagdata") as XmlElement;
					if(xFreshTagData != null)
					{
						XmlNode xImportedTagData = xTreeNodeMenuData.OwnerDocument.ImportNode(xFreshTagData, true);
						xTreeNodeMenuData.AppendChild(xImportedTagData);
					}
				}
				#endregion

				#region Validate the node against the template it is derived from.

				if(XTypeTemplate != null)
				{
					this.AllowedByTemplate = true;

					XmlElement xMenuTemplateRootContainer = XTypeTemplate.SelectSingleNode("sub_menu_template/menu_system/container") as XmlElement;

					if(xMenuTemplateRootContainer != null)
					{
						try
						{
							#region moved down a bit because a newly created node does not yet have a '<values>' element.
							//// Ensure that all containers in xTreeNodeMenuData["values"] have a 'value' attribute.
							//XmlNodeList xContainers = xTreeNodeMenuData.SelectNodes("values//container");

							//foreach(XmlElement xContainer in xContainers)
							//{
							//    if(xContainer.Attributes.GetNamedItem("value") == null)
							//    {
							//        string strValue = XmlHelper.GetKeyValue(xContainer, "default");
							//        if(strValue == null)
							//            strValue = MenuSystem.UNINITIALISED_MARKER;
							//        XmlHelper.AddAttribute(xContainer, "value", strValue);
							//    }
							//}
							#endregion

							// Ensure that there are no 'value' attributes in the template.
							XmlNodeList xTemplateContainers = xMenuTemplateRootContainer.SelectNodes(".//container[@value]");
							Debug.Assert(xTemplateContainers.Count == 0);
							foreach(XmlElement xTemplateContainer in xTemplateContainers)
							{
								xTemplateContainer.Attributes.RemoveNamedItem("value");
							}

							// TODO: Mark the project as dirty if MergeMenuDocuments makes any changes.
							//XmlDocument xValidatedDataDocument = MenuSystem.MergeMenuDocuments(xMenuTemplateRootContainer, xTreeNodeMenuData["values"]["container"], MenuSystem.MergeMode.MasterIsTemplate);
							XmlDocument xValidatedDataDocument = MenuSystem.MergeMenuDocuments(xMenuTemplateRootContainer, xTreeNodeMenuData.SelectSingleNode("values/container") as XmlElement, MenuSystem.MergeMode.MasterIsTemplate);

							XmlNode xValues = xTreeNodeMenuData["values"];

							Debug.Assert(xValues != null);
							// FIX 001:
							if(xValues == null)
							{
								xValues = this.xMenuProjectDocument.CreateElement("values");
								XTreeNodeMenuData.AppendChild(xValues);
							}
							xValues.RemoveAll();

							XmlElement xImportedChild = this.xMenuProjectDocument.ImportNode(xValidatedDataDocument.DocumentElement, true) as XmlElement;
							xValues.AppendChild(xImportedChild);


							#region moved to here
							// Ensure that all containers in xTreeNodeMenuData["values"] have a 'value' attribute.
							// Ensure the value is set to default if [@save='false']

							XmlNodeList xContainers = xTreeNodeMenuData.SelectNodes("values//container");

							foreach(XmlElement xContainer in xContainers)
							{
								bool bUseDefault = false;
								if(XmlHelper.GetKeyBool(xContainer, "save", true) == false)
								{
									bUseDefault = true;
								}

								if((xContainer.Attributes.GetNamedItem("value") == null) || (bUseDefault == true))
								{
									string strValue = XmlHelper.GetKeyValue(xContainer, "default");
									if(strValue == null)
										strValue = MenuSystem.UNINITIALISED_MARKER;
									XmlHelper.AddAttribute(xContainer, "value", strValue);
								}
							}
							#endregion


						}
						catch
						{
							Debug.Assert(false);
						}
					}
				}
				else
				{
					this.AllowedByTemplate = false;
					ownerProject.AddObsoleteTreeNodeReference(this);
				}

				#endregion

				string strImageKey = XmlHelper.GetKeyValue(XTypeTemplate, "icon1");
				string strSelectedImageKey = XmlHelper.GetKeyValue(XTypeTemplate, "icon2");

				if(strImageKey != null)
					strImageKey = this.OwnerProjectObject.StrIconsPath + "\\" + strImageKey;
				else
					strImageKey = "Unknown";

				if(strSelectedImageKey != null)
					strSelectedImageKey = this.OwnerProjectObject.StrIconsPath + "\\" + strSelectedImageKey;
				else
					strSelectedImageKey = "Unknown";

				ImageKey = strImageKey;
				SelectedImageKey = strSelectedImageKey;

				if(bRecurse == true)
					NewPopulateChildren(this);

				bool bForceExpandByTemplate = XmlHelper.GetKeyBool(this.XTypeTemplate, "force_expand_on_load", false);

				//Debug.Assert(bForceExpandByTemplate == false);

				Debug.Assert(this.XTreeNodeMenuData.Name == "type_entry" || this.XTreeNodeMenuData.Name == "root_type_entry");
				if(this.IsExpanded == false &&
				   (Executer_MainForm.s_optionsManager.GetContainerBoolByName("restore_tree_layout", false) &&
				    XmlHelper.GetKeyBool(this.XTreeNodeMenuData, "expanded_in_ui", false) == true) ||
				   (XmlHelper.GetKeyBool(this.XTreeNodeMenuData, "force_node_expansion", false) &&
				    Executer_MainForm.s_optionsManager.GetContainerBoolByName("ignore_forced_tree_node_expansions", false) == false)
				   || bForceExpandByTemplate == true
					)
				{
					this.Expand();
				}

				// At this point, this.NodeFont is null; Setting it to null sets it to the font for the treeview?
				normalFont = this.NodeFont;

				if(XmlHelper.GetKeyBool(XTreeNodeMenuData, "auto_name", false) == true)
				{
					this.AutoRename();
					this.NodeFont = fontLockedFilename;		//  - This is done in AutoRename, but only if the project is writable.
				}

			}
			else
			{
				Debug.Assert(true);
			}
		}

		private Font normalFont;

		Font fontLockedFilename = new Font(SystemFonts.DefaultFont, FontStyle.Italic);
		//Font fontLockedFilename = new Font(SystemFonts.DefaultFont.Name, 10, FontStyle.Italic);

		public override string TypeName
		{
			get { return "Think of a bloody name for these."; }
		}

		/// <summary>
		/// A reference to the project data that defines this node.
		/// </summary>
		public XmlElement XTreeNodeMenuData { get; private set; }

		/// <summary>An array of jobId's (as strings) that belong to this tree node.</summary>
		public ReadOnlyCollection<Job> JobList
		{
			get { return new ReadOnlyCollection<Job>(_jobList); }
		}

		public XmlDocument xMenuTemplateDocument
		{
			get { return OwnerProjectObject.XMenuTemplateDocument; }
		}

		public XmlDocument xMenuProjectDocument
		{
			get { return OwnerProjectObject._xMenuProjectDocument; }
		}

		public XmlElement XTypeTemplate { get; private set; }

		public string OwnerTemplateFilename
		{
			get { return this.OwnerProjectObject._strTemplateDocumentFilename; }
			set { this.OwnerProjectObject._strTemplateDocumentFilename = value; }
		}

		public int LastViewedJobPanel { get; set; }

		/// <summary>
		/// This is the TreeProjectObject that owns this TreeUserObject.
		/// </summary>
		public TreeProjectObject OwnerProjectObject { get; private set; }

		public bool AllowedByTemplate { get; private set; }

		#region ICanReceivePaste Members

		public bool CanReceivePaste(Moveable thingToPaste)
		{
			bool bReadOnly;
			if(OwnerProjectObject.ReadableState == RestrictedMode.ReadOnly || OwnerProjectObject.ReadableState == RestrictedMode.Restricted)
				bReadOnly = true;
			else
				bReadOnly = false;

			if((thingToPaste != null) && (bReadOnly == false))
			{
				//if((thingToPaste.CopyOfObject is TreeUserObject) && /*(this.OwnerProjectObject.IsReadOnly == false) &&*/ this.IsTreeChildAllowed(thingToPaste.CopyOfObject as TreeUserObject))
				if((thingToPaste.CopyOfObject is TreeUserObject) && /*(this.OwnerProjectObject.IsReadOnly == false) &&*/ this.IsTreeChildAllowed(thingToPaste.ObjectSource as TreeUserObject))
					return true;
			}
			return false;
		}

		public bool Paste(Moveable thingToPaste)
		{
			if(thingToPaste.MoveType != MoveType.Nothing)
			{
				TreeUserObject tuo = (TreeUserObject) thingToPaste.CopyOfObject;

				//if(this.IsTreeChildAllowed(tuo))
				if(this.IsTreeChildAllowed(thingToPaste.ObjectSource as TreeUserObject))
				{
					bool bReadOnly;
					if(OwnerProjectObject.ReadableState == RestrictedMode.ReadOnly || OwnerProjectObject.ReadableState == RestrictedMode.Restricted)
						bReadOnly = true;
					else
						bReadOnly = false;

					if(bReadOnly == false)
					{
						if(this.XTreeNodeMenuData.OwnerDocument != tuo.XTreeNodeMenuData.OwnerDocument)
						{
							if(MessageHandler.GetAnswer("DragDrop across different projects") == DialogResult.No) // 'No' is forbidden.
								MessageHandler.SetAnswer("DragDrop across different projects", "Ask");

							// TODO: ATTENTION: See if the templates are different and warn the user?

							if(this.OwnerTemplateFilename != tuo.OwnerTemplateFilename)
							{
								DialogResult dr = MessageHandler.ShowMemory
									(Program.S_executerMainForm,
									 "DragDrop across different projects",
									 "Allow 'drag and drop' across different projects?",
									 "You are attempting to drag a node from one project to another.\n" +
									 "This may cause unpredictable results because your project templates do not match.\n" +
									 "Do you wish to continue?",
									 "Executer",
									 MessageBoxButtons.YesNo,
									 MessageBoxIcon.Warning);

								if(dr != DialogResult.Yes)
									return false;
							}
							//else
							//{
							//    DialogResult dr = MessageHandler.ShowMemory
							//        (Program.S_executerMainForm,
							//         "Safe DragDrop across different projects",
							//         "Allow safe 'drag and drop' across different projects?",
							//         "You are attempting to drag a node from one project to another.\n" +
							//         "Do you wish to continue?",
							//         "Executer",
							//         MessageBoxButtons.YesNo,
							//         MessageBoxIcon.Warning);

							//    if(dr != DialogResult.Yes)
							//        return false;
							//}
						}

						// Use a copy of the copy so that the thing being pasted is created in the context of the destination project.
						// (So moving something from a project with template A to a project with template B causes the object to be validated against
						// template B)
						TreeUserObject CopyOfCopy = (tuo).DuplicateMyself(this.OwnerProjectObject);

						if(thingToPaste.MoveType == MoveType.Cut)
						{
							Move(CopyOfCopy);

							(thingToPaste.ObjectSource as TreeUserObject).DeleteTreeNode();

							if(thingToPaste.MoveType == MoveType.Cut)
								thingToPaste.MoveType = MoveType.Copy;
							else if(thingToPaste.MoveType == MoveType.Copy)
							{
							}
							else
								thingToPaste.MoveType = MoveType.Nothing;
						}
						else if(thingToPaste.MoveType == MoveType.Copy)
						{
							Move(CopyOfCopy);
						}

						return true;
					}
				}
			}
			return false;
		}

		public bool CanReceiveSpecialPaste(Moveable thingToPaste)
		{
			return CanReceivePaste(thingToPaste);
		}

		
		public bool PasteSpecial(Moveable thingToPaste)
		{
			if(thingToPaste.MoveType != MoveType.Nothing)
			{
				TreeUserObject tuo = (TreeUserObject) thingToPaste.CopyOfObject;

				XmlElement xDestParent = this.XTreeNodeMenuData.CloneNode(true) as XmlElement; // Parent of pasted material.

				XmlElement xPasted = tuo.XTreeNodeMenuData.CloneNode(true) as XmlElement; // The xml of the thing to be pasted.

				// Assume everything is unchecked ...
				XmlNodeList xAllPastedTypeEntries = xPasted.SelectNodes(".//type_entry");
				foreach (XmlElement xTypeEntry in xAllPastedTypeEntries)
				{
					XmlHelper.AddAttribute(xTypeEntry, "temp_checked", "false");
				}

				// Now for every pasted object that is checked, temp_check it and make sure it's parents are temp_checked ...

				XmlNodeList xCheckedUserObjects = xPasted.SelectNodes(".//type_entry[@checked='true']");

				foreach (XmlElement xElement in xCheckedUserObjects)
				{
					XmlElement xCurrentElement = xElement;

					while (xCurrentElement != null)
					{
						XmlHelper.AddAttribute(xCurrentElement, "temp_checked", "true");
						xCurrentElement = xCurrentElement.ParentNode as XmlElement;
					}
				}

				// Now remove any nodes that are not checked ...

				XmlNodeList xUnCheckedUserObjects = xPasted.SelectNodes(".//type_entry[@temp_checked='false']");

				foreach (XmlElement xElement in xUnCheckedUserObjects)
				{
					xPasted.RemoveChild(xElement);
				}

				// Now merge xExtra into xBase ...

				CustomMerge(xDestParent, xPasted);

				// Now create a tree branch using xDestParent, put it into ThingToPaste, then paste it.

				XmlElement xNewChild = xDestParent.SelectSingleNode("type_entry") as XmlElement;
				//XmlElement xNewChild = xDestParent.FirstChild as XmlElement;

				//TreeUserObject tuoNew = new TreeUserObject(this.OwnerProjectObject, xDestParent, true);
				TreeUserObject tuoNew = new TreeUserObject(this.OwnerProjectObject, xNewChild, true);

				//TreeUserObject copy = tuoNew.DuplicateMyself(tuoNew.OwnerProjectObject);

				Moveable newThingToPaste = new Moveable(tuo, tuoNew, thingToPaste.MoveType);
				this.Paste(newThingToPaste);

				thingToPaste.MoveType = newThingToPaste.MoveType;


			}
			return false;
		}


		/// <summary>
		/// How the fuck am I going to do this?
		/// </summary>
		/// <param name="xBase"></param>
		/// <param name="xExtra"></param>
		/// <returns></returns>
		private bool CustomMerge(XmlElement xDestParent, XmlElement xPasted)


		{
			// TEDIOUS TEDIOUS TEDIOUS TEDIOUS TEDIOUS TEDIOUS TEDIOUS TEDIOUS TEDIOUS TEDIOUS TEDIOUS TEDIOUS 

			return _CustomMerge(xDestParent, xPasted);

		}

		private bool _CustomMerge(XmlElement xDestParent, XmlElement xPasted)
		{
		    string strPastedTypeName = XmlHelper.GetKeyValue(xPasted, "type_name");
		    string strPastedName = XmlHelper.GetKeyValue(xPasted, "name");

		    string strQuery = "type_entry[@type_name='" + strPastedTypeName + "' and @name='" + strPastedName + "']";
		    XmlElement xNodeToReplace = xDestParent.SelectSingleNode(strQuery) as XmlElement;

		    if(xNodeToReplace == null)		// If there is no node to replace, just dump the pasted branch in here.
		    {
		        xDestParent.AppendChild(xDestParent.OwnerDocument.ImportNode(xPasted, true));
		    }
		    else						// Otherwise recursively merge the node.
		    {
		        /// All pasted nodes are 'temp_checked'.
		        /// if the pasted node is 'checked', replace the contents of xNodeToReplace.
				
		        if(XmlHelper.GetKeyBool(xPasted, "checked", false) == true)
		        {
		            // Replace all children of xNodeToReplace except for type_entry.

		            XmlElement xReplacedElement = ShallowCopyType(xDestParent.OwnerDocument, xPasted);

					XmlNodeList xTypeEntries = xNodeToReplace.SelectNodes("type_entry");

					foreach(XmlElement xTypeEntryToMove in xTypeEntries)
					{
						xReplacedElement.AppendChild(xTypeEntryToMove);
					}

		        	xDestParent.ReplaceChild(xReplacedElement, xNodeToReplace);


		        	xNodeToReplace = xReplacedElement;






		            // Copy all the children of xPasted to xNodeToReplace except for any 'type_entry'.
					
					
		        }

		        /// then recurse.
		        /// 
		    	foreach (XmlElement xPastedChild in xPasted)
		    	{
		    		_CustomMerge(xNodeToReplace, xPastedChild);
		    	}

		    }

			return false;
		}

		private XmlElement ShallowCopyType(XmlDocument xDocOwner,  XmlElement xPasted)
		{
			XmlElement xCopy = xDocOwner.CreateElement("type_entry");

			foreach(XmlNode xNode in xPasted.ChildNodes)
			{
				if(xNode is XmlAttribute)
				{
					XmlAttribute xAttr = xNode as XmlAttribute;
					xCopy.SetAttribute(xAttr.Name, xAttr.Value);
				}
				else if(xNode is XmlElement)
				{
					XmlElement xElement = xNode as XmlElement;
					if(xElement.Name != "type_entry")
					{
						//xCopy.AppendChild(xDocOwner.ImportNode(xElement.CloneNode(true), true));
						xCopy.AppendChild(xDocOwner.ImportNode(xElement, true));	// Same as above?
					}
				}
			}
			return xCopy;
		}

		private string PartialPath()
		{
			string strResult = this.Text;

			if(this.Parent != null && this.Parent is TreeUserObject)
				strResult = ((TreeUserObject)this.Parent).PartialPath()+"\\" + strResult;

			return strResult;
		}

		#endregion

		#region IContextMenu Members

		public ContextMenuStrip CreateMenu()
		{
			ContextMenuStrip cms = new ContextMenuStrip();

			bool bReadOnly = (OwnerProjectObject.ReadableState == RestrictedMode.ReadOnly || OwnerProjectObject.ReadableState == RestrictedMode.Restricted) ? true : false;

			cms.Items.Add(ToolstripUtility.CreateTsmi("addChildTsmi", null, "Add Child", null, !bReadOnly));
			cms.Items.Add(ToolstripUtility.CreateTsmi("setDefaultValuesTsmi", "<setdefaultvalues>", "Set default values", tsmi_Click, !bReadOnly));
			cms.Items.Add(ToolstripUtility.CreateTss(null));
			cms.Items.Add
				(ToolstripUtility.CreateTsmi
				 	("saveProjectTsmi", "<save>", "Save Project", tsmi_Click, this.OwnerProjectObject.IsDirty ||
				 	                                                          (!bReadOnly && Executer_MainForm.s_optionsManager.GetContainerBoolByName("allow_saving_unmodified_projects", false))));
			cms.Items.Add(ToolstripUtility.CreateTss(null));
			cms.Items.Add(ToolstripUtility.CreateTss(null));
			cms.Items.Add(ToolstripUtility.CreateTsmi("deleteTsmi", "<delete>", "Delete", tsmi_Click, !bReadOnly));
			cms.Items.Add(ToolstripUtility.CreateTss(null));
			cms.Items.Add(ToolstripUtility.CreateTsmi("tasksTsmi", null, "Tasks", null, true));
			cms.Items.Add(ToolstripUtility.CreateTsmi("cleanupTsmi", "<closefinishedtasks>", "Close finished Tasks", tsmi_Click, true));
			/////////////////////////////////////////////////////////

			Debug.Assert(this.Parent is TreeProjectObject || this.Parent is TreeUserObject); // Parent must be a treenode or a project node.

			ToolStripMenuItem tmiAdd = ToolstripUtility.FindFirstToolsTripMenuItemByText(cms.Items, "Add Child");
			if(tmiAdd != null)
			{
				if(bReadOnly == false)
					this.AddChildTypes(this.Name, tmiAdd, this);

				tmiAdd.Enabled = !bReadOnly;
			}

			ToolStripMenuItem tmiRename = ToolstripUtility.FindFirstToolsTripMenuItemByText(cms.Items, "Rename");
			if(tmiRename != null)
			{
				Debug.Assert(false, "ATTENTION");
				tmiRename.Enabled = this.CanRename();
			}

			ToolStripMenuItem tmiDelete = ToolstripUtility.FindFirstToolsTripMenuItemByText(cms.Items, "Delete");
			if(tmiDelete != null)
				tmiDelete.Enabled = this.CanClose();

			ToolStripMenuItem tmiTasks = ToolstripUtility.FindFirstToolsTripMenuItemByText(cms.Items, "Tasks");
			if(tmiTasks != null)
			{
				if(this.TreeView.CheckBoxes == true)
					tmiTasks.Text = "Tasks on checked items";
				AddTasks(tmiTasks, true, false);
			}

			cms.Items.Add(ToolstripUtility.CreateTss(null));
			bool bForceExpansion = XmlHelper.GetKeyBool(this.XTreeNodeMenuData, "force_node_expansion", false);

			ToolStripMenuItem forceExpansionTsmi = ToolstripUtility.CreateTsmi("forceExpansionTsmi", "<forceexpansiononload>", "Force node expansion on load", tsmi_Click, bReadOnly == false);
			forceExpansionTsmi.CheckOnClick = true;
			forceExpansionTsmi.Checked = bForceExpansion;
			forceExpansionTsmi.Tag += "[ADVANCED]";
			cms.Items.Add(forceExpansionTsmi);

			cms.Items.Add(ToolstripUtility.CreateTss(null));
			XmlElement xMenu = this.XTreeNodeMenuData.SelectSingleNode("values/container") as XmlElement;

			cms.Items.Add(ToolstripUtility.CreateTsmi("showTsmi", "<showinseperatewindow>", "Show in seperate window", tsmi_Click, xMenu != null));

			return cms;
		}

		#endregion

		#region IMoveable Members

		public bool CanMoveUp
		{
			get
			{
				if(this.Index > 0)
				{
					bool bReadOnly;
					if(OwnerProjectObject.ReadableState == RestrictedMode.ReadOnly || OwnerProjectObject.ReadableState == RestrictedMode.Restricted)
						bReadOnly = true;
					else
						bReadOnly = false;

					return !bReadOnly;
				}
				return false;
			}
		}

		public bool MoveUp()
		{
			int index = this.Index;

			XmlElement xmlParent = this.XTreeNodeMenuData.ParentNode as XmlElement;
			XmlNodeList xmlChildren = this.XTreeNodeMenuData.ParentNode.SelectNodes("type_entry");

			Debug.Assert(xmlChildren.Count == this.Parent.Nodes.Count);
			Debug.Assert(xmlChildren[index] == this.XTreeNodeMenuData);

			XmlElement xPrevMenuData = xmlChildren[index - 1] as XmlElement;

			xmlParent.RemoveChild(XTreeNodeMenuData);

			xmlParent.InsertBefore(XTreeNodeMenuData, xPrevMenuData);

			TreeBaseObject parent = this.Parent as TreeBaseObject;

			parent.Nodes.Remove(this);

			parent.Nodes.Insert(index - 1, this);

			this.OwnerProjectObject.IsDirty = true;

			return true;
		}

		public bool CanMoveDown
		{
			get
			{
				bool bReadOnly;
				if(OwnerProjectObject.ReadableState == RestrictedMode.ReadOnly || OwnerProjectObject.ReadableState == RestrictedMode.Restricted)
					bReadOnly = true;
				else
					bReadOnly = false;

				if(this.Index < (this.Parent.Nodes.Count - 1))
					return !bReadOnly;
				return false;
			}
		}

		public bool MoveDown()
		{
			int index = this.Index;

			XmlElement xmlParent = this.XTreeNodeMenuData.ParentNode as XmlElement;
			XmlNodeList xmlChildren = this.XTreeNodeMenuData.ParentNode.SelectNodes("type_entry");

			Debug.Assert(xmlChildren.Count == this.Parent.Nodes.Count);
			Debug.Assert(xmlChildren[index] == this.XTreeNodeMenuData);

			XmlElement xNextMenuData = xmlChildren[index + 1] as XmlElement;

			xmlParent.RemoveChild(XTreeNodeMenuData);

			xmlParent.InsertAfter(XTreeNodeMenuData, xNextMenuData);

			TreeBaseObject parent = this.Parent as TreeBaseObject;

			parent.Nodes.Remove(this);

			parent.Nodes.Insert(index + 1, this);

			this.OwnerProjectObject.IsDirty = true;

			return true;
		}

		public bool CanCut
		{
			get
			{
				bool bReadOnly;
				if(OwnerProjectObject.ReadableState == RestrictedMode.ReadOnly || OwnerProjectObject.ReadableState == RestrictedMode.Restricted)
					bReadOnly = true;
				else
					bReadOnly = false;

				return !bReadOnly;
			}
		}

		public Moveable Cut()
		{
			TreeUserObject copy = this.DuplicateMyself(this.OwnerProjectObject);
			return new Moveable(this, copy, MoveType.Cut);
		}

		public bool CanCopy
		{
			get { return true; }
		}

		public Moveable Copy()
		{
			TreeUserObject copy = this.DuplicateMyself(this.OwnerProjectObject);
			return new Moveable(this, copy, MoveType.Copy);
		}

		#endregion

		#region IRemoveable Members

		public bool CanClose()
		{
			RestrictedMode rm = OwnerProjectObject.ReadableState;

			if(rm == RestrictedMode.ReadOnly || rm == RestrictedMode.Restricted)
				return false;
			else
				return true;

			//return !this.OwnerProjectObject.IsReadonly;
		}

		public bool Close(CloseMethod closeMethod)
		{
			bool bDoClose = false;

			switch(closeMethod)
			{
				case CloseMethod.AlwaysAsk:
					goto label1;
				case CloseMethod.AskUsingMemoryDialogUnlessDirty:
					label1:
					// A tree object cannot be dirty.
					if(MessageHandler.GetAnswer("ID_DeleteTreeNode") == DialogResult.No) // 'No' is forbidden.
						MessageHandler.SetAnswer("ID_DeleteTreeNode", "Ask");

					DialogResult dr = MessageHandler.ShowMemory
						(Program.S_executerMainForm, "ID_DeleteTreeNode", "Delete tree nodes?", "Are you sure you wish to delete this node?", Program.AssemblyTitle, MessageBoxButtons.YesNo, MessageBoxIcon.Question, false);
					if(dr == DialogResult.Yes || dr == DialogResult.OK)
						bDoClose = true;
					break;
				case CloseMethod.AskIfDirty:
					// A tree user object cannot be dirty.
					bDoClose = true;
					break;
				case CloseMethod.Force:
					bDoClose = true;
					break;
				case CloseMethod.FailIfDirty:
					// A tree user object cannot be dirty.
					bDoClose = true;
					break;
				default:
					throw new ApplicationException("TreeUserObject.close() bad CloseMethod : '" + closeMethod.ToString() + "'");
			}

			if(bDoClose == true)
			{
				DeleteTreeNode();
				return true;
			}
			return false;
		}

		#endregion

		#region IRenameable Members

		public bool CanRename()
		{
			RestrictedMode rm = this.OwnerProjectObject.ReadableState;
			if(rm == RestrictedMode.PrimaryRestricted || rm == RestrictedMode.Writable)
				return true;

			return false;
		}

		public bool Rename(string newName)
		{
			XmlElement xElement = this.XTreeNodeMenuData;

			if(!string.IsNullOrEmpty(newName))
			{
				XmlHelper.AddAttribute(xElement, "auto_name", "false");		// Gets overidden if need be by the caller.
				this.NodeFont = normalFont;
				XmlHelper.AddAttribute(xElement, "name", newName);
				this.Text = newName;
				this.OwnerProjectObject.IsDirty = true;
				return true;
			}
			return false;
		}

		public string DisplayName
		{
			get { return this.Text; }
		}

		#endregion

		#region IViewableObject Members

		public XmlElement RawXmlDocument
		{
			get { return this.XTreeNodeMenuData; }
		}

		public Control CreateView()
		{
			TreeUserObjectViewer treeUserObjectViewer = new TreeUserObjectViewer(this);
			return treeUserObjectViewer;
		}

		#endregion

		private void tsmi_Click(object sender, EventArgs e)
		{
			ToolStripMenuItem toolstripMenuItem = sender as ToolStripMenuItem;

			Debug.Assert(toolstripMenuItem != null);

			if(toolstripMenuItem != null)
			{
				string strCommand = ToolstripUtility.GetCommand(toolstripMenuItem);

				switch(strCommand)
				{
					case "setdefaultvalues":
						if(MessageHandler.GetAnswer("Set default values") == DialogResult.No) // 'No' is forbidden.
							MessageHandler.SetAnswer("Set default values", "Ask");

						DialogResult dr = MessageHandler.ShowMemory
							(null, "Set default values", "Set tree nodes to default value without asking?", "Are you sure you want to set this to default values?", "Executer", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
						if(dr == DialogResult.Yes)
							Executer_MainForm.s_treeObjectViewer.SetDefaultValues();
						break;

					case "save":
						Saveable.RecursiveSave(this.OwnerProjectObject, false, false);
						break;

					case "delete":
						this.Close(CloseMethod.AskUsingMemoryDialogUnlessDirty);
						break;

					case "closefinishedtasks":
						this.CleanupJobs(false);
						break;

					case "forceexpansiononload":
						ToolStripMenuItem tsmi = sender as ToolStripMenuItem;

						if(tsmi != null)
						{
							XmlHelper.AddAttribute(this.XTreeNodeMenuData, "force_node_expansion", tsmi.Checked.ToString());
							this.OwnerProjectObject.IsDirty = true;
						}
						break;

					case "showinseperatewindow":
						showToolStripMenuItem_ClickExtracted();
						break;

					default:
						Debug.Assert(true, "Bad Item Clicked in treeUserObject.ItemClicked.");
						break;
				}
			}
		}

		internal void showToolStripMenuItem_ClickExtracted()
		{
			XmlDocument xCopyOfXTreNodeMenuData = new XmlDocument();
			xCopyOfXTreNodeMenuData.AppendChild(xCopyOfXTreNodeMenuData.ImportNode(this.XTreeNodeMenuData, true));

			Form bareForm = new Form {Text = "Executer - "};
			try
			{
				string strName = xCopyOfXTreNodeMenuData.SelectSingleNode("(type_entry|root_type_entry)/@name").Value;
				bareForm.Text += strName;
			}
			catch
			{
				bareForm.Text += "Error : Node does not have a name.";
			}
			XmlNodeList xAlwaysEnabledAttributes = xCopyOfXTreNodeMenuData.SelectNodes("//@always_enabled");
			if(xAlwaysEnabledAttributes != null)
			{
				foreach(XmlAttribute xAttr in xAlwaysEnabledAttributes)
				{
					if(xAttr.OwnerElement != null) xAttr.OwnerElement.RemoveAttributeNode(xAttr);
				}
			}
			MenuSystem ms = new MenuSystemLibrary.MenuSystem();
			ms.Initialise(this.XTypeTemplate.SelectSingleNode("sub_menu_template/menu_system") as XmlElement);
			XmlElement xMenu = xCopyOfXTreNodeMenuData.SelectSingleNode("(type_entry|root_type_entry)/values/container") as XmlElement;
			if(xMenu != null)
			{
				ms.m_bReadOnly = true;
				ms.SetMenu(xMenu, false);
				ms.MenuChangedEvent += ms_ShowMenuChangedEvent;
				ms.Dock = DockStyle.Fill;
				bareForm.Controls.Add(ms);
				bareForm.Show();
			}
		}

		private void NewPopulateChildren(TreeUserObject treeUserObject)
		{
			foreach(XmlElement xCurrentChild in treeUserObject.XTreeNodeMenuData)
			{
				if(xCurrentChild.Name == "type_entry")
				{
					TreeUserObject treeChild = null;
					try
					{
						treeChild = new TreeUserObject(OwnerProjectObject, xCurrentChild, true);
					}
					catch
					{
					}

					if(treeChild != null)
					{
						this.Nodes.Add(treeChild);
						if((this.IsExpanded == false) && (treeChild.IsExpanded == true))
							this.Expand();
					}
				}
			}
		}

		internal void AddJobDataEntry(Job job)
		{
			if(_jobList.Contains(job))
			{
				throw new Exception("TreeUserObject:AddJobEntry. Attempt to add a jobEntry that is already there.");
			}
			else
			{
				_jobList.Add(job);
			}
		}

		internal void RemoveJobDataEntry(Job job)
		{
			if(_jobList.Contains(job) == false)
			{
				throw new Exception("TreeUserObject:RemoveJobId. Attempt to remove a jobId that is not there.");
			}
			else
			{
				_jobList.Remove(job);
			}
		}

		/// <summary>
		/// Gets the template 'type' entry associated with this node.
		/// </summary>
		/// <returns></returns>
		private XmlElement GetTypeTemplate()
		{
			string strTypeName = XmlHelper.GetKeyValue(this.XTreeNodeMenuData, "type_name");
			XmlElement xTemplate = this.OwnerProjectObject.GetTemplateTypeFromTypeName(strTypeName);
			return xTemplate;
		}

		/// <summary>
		/// Determines whether a given TreeObject can legitimately become a child of this TreeObject.
		/// The child must be a type that is allowed by this tree object and it must not be an ancestor
		/// of this tree object.
		/// </summary>
		/// <param name="treeChild"></param>
		/// <returns></returns>
		internal bool IsTreeChildAllowed(TreeUserObject treeChild)
		{
			if(treeChild == null)
				return false;

			bool bAllowChild = false;

			List<XmlElement> validChildren = null;
			this.OwnerProjectObject.ChildTypes.TryGetValue(this.Name, out validChildren);

			if(validChildren != null)
			{
				foreach(XmlNode xNode in validChildren)
				{
					string strTypeName = XmlHelper.GetKeyValue(xNode, "name");

					if(strTypeName == treeChild.Name)
					{
						bAllowChild = true;
						break;
					}
				}

				// Make sure the treeChild is not an ancestor of 'this'.
				// (A tree node cannot become the parent of an ancestor.)
				if(bAllowChild == true)
				{
					TreeNode tn = this;

					while(tn != null)
					{
						if(tn == treeChild)
						{
							bAllowChild = false;
							break;
						}
						tn = tn.Parent;
					}
				}
			}
			return bAllowChild;
		}

		/// <summary>
		/// Finds the TreeUserObject at the top of the current branch.
		/// </summary>
		/// <param name="treeNode"></param>
		/// <returns></returns>
		public TreeUserObject GetProjectRootItem()
		{
			TreeUserObject treeUserObject = this;

			while(treeUserObject.Parent != null && treeUserObject.Parent is TreeUserObject)
				treeUserObject = treeUserObject.Parent as TreeUserObject;

			return treeUserObject;
		}

		/// <summary>
		/// Process the task associated with this tree object. Perhaps recursively.
		/// </summary>
		internal bool DoTask(string strTaskName, string strServerName, bool bRecurse)
		{
			_bShiftHeldDuringDoTask = UtilityFunctions.Shift;

			Job jd = new Job(this, strTaskName, strServerName, -1);

			//UtilityFunctions.FixupSpecialMenuCases(jd.XArguments.DocumentElement);

			return _DoTask(jd, bRecurse);
		}

		private bool _DoTask(Job jd, bool bRecurse)
		{
			if(bRecurse == true && _bShiftHeldDuringDoTask == false)
			{
				foreach(TreeUserObject toChild in this.Nodes)
				{
					bool bSkipThisOne = false;

					XmlNodeList xCrippleElements = toChild.XTreeNodeMenuData.SelectNodes("./values//container[@disable_recursive_export_if_value_matches]");

					foreach(XmlElement xCrippled in xCrippleElements)
					{
						string strInput = XmlHelper.GetKeyValue(xCrippled, "value");

						string strPattern = XmlHelper.GetKeyValue(xCrippled, "disable_recursive_export_if_value_matches");

						if(strInput != null && strPattern != null)
						{
							if(Regex.IsMatch(strInput, strPattern, RegexOptions.IgnoreCase) == true)
							{
								bSkipThisOne = true;
								break;
							}
						}
					}

					if(bSkipThisOne == false)
					{
						long parentJobLuid = (jd.IsDummyJob) ? jd.LocalParentJobId : jd.LocalJobId;

						Job childJobData = new Job(toChild, jd.StrTaskName, jd.StrServerName, parentJobLuid);
						//UtilityFunctions.FixupSpecialMenuCases(childJobData.XArguments.DocumentElement);

						if(toChild._DoTask(childJobData, bRecurse) == false) // If for some reason the job cannot be accepted, abort.
							return false;
					}
				}
			}

			if(jd.IsDummyJob == false)
			{
				if(jd.Initialised == true)
				{
					if(_bShiftHeldDuringDoTask == true)
					{
						string strFormattedText = UtilityFunctions.FormatXml(jd.XArguments.DocumentElement);
						return DebugDoTask(jd.StrTempArgsFilename, jd.StrCommand, jd.StrClArguments, strFormattedText);
					}
					else
					{
						bool bSuccess = false;

						// Start the job, either directly or via the farm.
						if(jd.StrServerName == "immediate")
						{
							try
							{
								bool bHideProcessWindow = jd.HideProcessWindow;
								launch_process.LaunchJob(jd.StrCommand, jd.StrClArguments, jd.StrWorkingDirectory, bHideProcessWindow, false);
								bSuccess = true;
							}
							catch(Exception ex)
							{
								string strQuestion = "Failed to spawn the process '" + jd.StrCommand;
								MessageHandler.ShowMemory
									(this.TreeView, strQuestion, strQuestion, strQuestion + "'\nwith the command line:\n'" + jd.StrTempArgsFilename + "'\nThe system reports: " + ex.Message, "Executer Error", MessageBoxButtons.OK,
									 MessageBoxIcon.Error);
							}
						}
						else
						{
							bSuccess = Program.s_jobManager.RequestJob(jd);
						}
						return bSuccess;
					}
				}
				else
				{
					if(_bShiftHeldDuringDoTask == true)
					{
						debug_text_dialog dtd = new debug_text_dialog(jd.StrTempArgsFilename, "Cannot create job data.", "Arguments are broken.");
						dtd.Show();
					}
					return false;
				}
			}
			else
			{
				Debug.WriteLine("_DoTask skipping dummy job on tree node " + this.Name);
				return true;
			}
		}

		private static bool DebugDoTask(string strFilename, string strCommand, string strClArguments, string strArguments)
		{
			if(Executer_MainForm.s_optionsManager.GetContainerBoolByName("radio_copy_command_line", false) == true)
			{
				Clipboard.SetDataObject(strCommand + " " + strClArguments);
			}
			else if(Executer_MainForm.s_optionsManager.GetContainerBoolByName("radio_copy_arguments", false) == true)
			{
				Clipboard.SetDataObject(strClArguments);
			}
			else if(Executer_MainForm.s_optionsManager.GetContainerBoolByName("radio_copy_arguments_filename", false) == true)
			{
				Clipboard.SetDataObject(strFilename);
			}
			else if(Executer_MainForm.s_optionsManager.GetContainerBoolByName("radio_copy_job_xml_description", false) == true)
			{
				Clipboard.SetDataObject(strArguments);
			}
			else if(Executer_MainForm.s_optionsManager.GetContainerBoolByName("radio_copy_none", false) == true)
			{
			}

			MacroExpander.MacroExpander mx = new MacroExpander.MacroExpander();

			mx.AddPair("%command_line%", strCommand);
			mx.AddPair("%args%", strClArguments);
			mx.AddPair("%args_filename%", strFilename);

			if(Executer_MainForm.s_optionsManager.GetContainerBoolByName("check_write_command_line", false) == true)
			{
				string strOptionsCommandLineFilename = Executer_MainForm.s_optionsManager.GetContainerStringByName("write_command_line");
				if(strOptionsCommandLineFilename != null)
				{
					strOptionsCommandLineFilename = mx.Expand(strOptionsCommandLineFilename, true);

					try
					{
						using(StreamWriter sw = new StreamWriter(strOptionsCommandLineFilename, false))
						{
							sw.Write(strCommand + " " + strClArguments);
						}
					}
					catch(Exception ex)
					{
						MessageHandler.Show(null, "Cannot write the command line. The system reports:\n" + ex.Message, "Executer", MessageBoxButtons.OK);
					}
				}
			}

			if(Executer_MainForm.s_optionsManager.GetContainerBoolByName("check_write_arguments", false) == true)
			{
				string strOptionsArgumentsFilename = Executer_MainForm.s_optionsManager.GetContainerStringByName("write_arguments");
				if(strOptionsArgumentsFilename != null)
				{
					strOptionsArgumentsFilename = mx.Expand(strOptionsArgumentsFilename, true);

					try
					{
						using(StreamWriter sw = new StreamWriter(strOptionsArgumentsFilename, false))
						{
							sw.Write(strClArguments);
						}
					}
					catch(Exception ex)
					{
						MessageHandler.Show(null, "Cannot write the command arguments. The system reports:\n" + ex.Message, "Executer", MessageBoxButtons.OK);
					}
				}
			}

			if(Executer_MainForm.s_optionsManager.GetContainerBoolByName("check_write_arguments_filename", false) == true)
			{
				string strArgumentsFilenameFilename = Executer_MainForm.s_optionsManager.GetContainerStringByName("write_arguments_filename");
				if(strArgumentsFilenameFilename != null)
				{
					strArgumentsFilenameFilename = mx.Expand(strArgumentsFilenameFilename, true);

					try
					{
						using(StreamWriter sw = new StreamWriter(strArgumentsFilenameFilename, false))
						{
							sw.Write(strFilename);
						}
					}
					catch(Exception ex)
					{
						MessageHandler.Show(null, "Cannot write the arguments filename. The system reports:\n" + ex.Message, "Executer", MessageBoxButtons.OK);
					}
				}
			}

			if(Executer_MainForm.s_optionsManager.GetContainerBoolByName("check_write_job_xml_description", false) == true)
			{
				string strJobDescriptionFilename = Executer_MainForm.s_optionsManager.GetContainerStringByName("write_job_xml_description");
				if(strJobDescriptionFilename != null)
				{
					strJobDescriptionFilename = mx.Expand(strJobDescriptionFilename, true);

					try
					{
						using(StreamWriter sw = new StreamWriter(strJobDescriptionFilename, false))
						{
							sw.Write(strArguments);
						}
					}
					catch(Exception ex)
					{
						MessageHandler.Show(null, "Cannot write the arguments filename. The system reports:\n" + ex.Message, "Executer", MessageBoxButtons.OK);
					}
				}
			}

			if(Executer_MainForm.s_optionsManager.GetContainerBoolByName("check_execute", false) == true)
			{
				string strExecuteString = Executer_MainForm.s_optionsManager.GetContainerStringByName("execute");
				if(strExecuteString != null)
				{
					strExecuteString = mx.Expand(strExecuteString, true);

					string strArgs = Executer_MainForm.s_optionsManager.GetContainerStringByName("execute_args");
					strArgs = mx.Expand(strArgs, true);

					try
					{
						launch_process.LaunchJob(strExecuteString, strArgs, null, false, false);
					}
					catch(Exception ex)
					{
						MessageHandler.Show(null, "Cannot launch a debug process. The system reports:\n" + ex.Message, "Executer", MessageBoxButtons.OK);
					}
				}
			}

			if(Executer_MainForm.s_optionsManager.GetContainerBoolByName("check_show_debug", false) == true)
			{
				debug_text_dialog dtd = new debug_text_dialog(strFilename, strCommand + " " + strClArguments, strArguments);
				dtd.Show();
			}

			return true;
		}

		/// <summary>
		/// Removes jobs that have finished or terminated from the list of current jobs in this node.
		/// Jobs that have timed out are left alone for the user to manually remove.
		/// The viewer may be refreshed if necessary.
		/// </summary>
		/// <returns>The number of jobs removed.</returns>
		public int CleanupJobs(bool bRecurse)
		{
			if(bRecurse == true)
			{
				foreach(TreeUserObject toChild in this.Nodes)
				{
					toChild.CleanupJobs(true);
				}
			}

			List<Job> deleteList = new List<Job>(this._jobList.Count);
			foreach(Job job in this._jobList)
			{
				switch(job.JobStatus)
				{
					case JobStatus.None:
						Debug.Assert(false);
						break;
					case JobStatus.Requested:
						break;
					case JobStatus.Queued:
						break;
					case JobStatus.Failed:
						deleteList.Add(job);
						break;
					case JobStatus.Timeout:
						break;
					case JobStatus.Created:
						break;
					case JobStatus.Killed:
						deleteList.Add(job);
						break;
					case JobStatus.Abandoned:
						deleteList.Add(job);
						break;
					case JobStatus.Finished:
						deleteList.Add(job);
						break;
					case JobStatus.Cancelled:
						deleteList.Add(job);
						break;
					default:
						break;
				}
			}
			if(deleteList.Count != 0)
			{
				deleteList.ForEach(this.RemoveJobDataEntry);
				// If this node is viewed and a job panel has been deleted, redraw the node.
				if(this.IsSelected == true)
				{
					Executer_MainForm.s_treeObjectViewer.ViewtreeObject((IViewableObject) this);
				}
			}
			return deleteList.Count;
		}


		/// <summary>
		/// Given a tree object, this looks for a given task that can be run on a given server, and constructs a
		/// CommandDetails class to describe it. 
		/// Recursion is set from the template like this: true => true; ask => false; false => false.
		/// </summary>
		/// <param name="strTaskName">The name of the task to find, or null to use the first task.</param>
		/// <param name="strServer">The name of the server to use, or null to use the first server permitted.</param>
		/// <returns>A new CommandDetails, or null if one cannot be constructed.</returns>
		public CommandDetails GetTaskByName(string strTaskName, string strServer)
		{
			try
			{
				//XmlElement xTemplate = this.m_xTemplate;
				XmlElement xTaskList = this.XTypeTemplate["task_list"];
				XmlElement xActions = XmlHelper.GetNodeFromPath(this.OwnerProjectObject.XMenuTemplateDocument, "root/actions");

				if(xTaskList != null)
				{
					XmlElement xTask;
					if(strTaskName == null)
					{
						xTask = xTaskList.SelectSingleNode("task") as XmlElement;
						strTaskName = XmlHelper.GetKeyValue(xTask, "name");
					}
					else
						xTask = XmlHelper.FindChildNode(xTaskList, "task", "name", strTaskName) as XmlElement;

					string strDisplayName = XmlHelper.GetKeyValue(xTask, "display_name");
					string strActionName = XmlHelper.GetKeyValue(xTask, "action_name");

					//					XmlNode xAction = XmlHelper.FindChildNode(xActions, "action", "name", strActionName);

					List<string> childValidServers = this.OwnerProjectObject._actionServersLookup[strActionName];

					// Look for the required server, or use the first one in the list?
					if(strServer != null)
					{
						if(childValidServers.Contains(strServer) == false)
							return null;
					}
					else
						strServer = childValidServers[0];

					bool bRecurse = this.Nodes.Count == 0 ? false : XmlHelper.GetKeyBool(xTask, "process_recursively", false);

					return new CommandDetails(strServer, bRecurse, this, strTaskName);
				}
			}
			catch(Exception ex)
			{
				Debug.WriteLine("Exception in 'GetTaskByName' : " + ex.Message);
			}

			return null;
		}

		private void DeleteTreeNode()
		{
			if(this.OwnerProjectObject != null)
				this.OwnerProjectObject.IsDirty = true;

			XmlElement xSelection = this.XTreeNodeMenuData;

			// Remove from Xml data ...
			if(xSelection != null)
			{
				XmlNode xParent = xSelection.ParentNode;
				xParent.RemoveChild(xSelection);
			}
			// Remove from tree ...

			ExecuterTreeView etv = this.TreeView as ExecuterTreeView;
			if(etv != null)
				etv.SetCheck(this, false, true);

			this.Remove();
		}


		private static void ms_ShowMenuChangedEvent(object sender, MenuSystemLibrary.MenuSystemChangedEventArgs e)
		{
			if(e.XChangedElement != null)
				MessageBox.Show("No.");
		}

		/// <summary>
		/// Given a toolstrip menu item and a TreeUserObject, it takes all the valid children that can be added to the tree node
		/// and adds them to the toolstrip menu item. Each menu item also has a reference to the Xml definition of the child type it represents.
		/// </summary>
		/// <param name="strParentName"></param>
		/// <param name="tmiAdd">The menu item to which to add the children.</param>
		/// <param name="tuo">The tree object whose valid child types should be added.</param>
		internal void AddChildTypes(string strParentName, ToolStripMenuItem tmiAdd, TreeUserObject tuo)
		{
			//string strParentName = tuo.Name;
			if(tuo.OwnerProjectObject.ChildTypes != null)
			{
				List<XmlElement> validChildren = null;
				tuo.OwnerProjectObject.ChildTypes.TryGetValue(strParentName, out validChildren);
				tmiAdd.DropDownItems.Clear();
				tmiAdd.Enabled = false;
				if(validChildren != null && validChildren.Count != 0)
				{
					foreach(XmlElement xType in validChildren)
					{
						string strTypeName = XmlHelper.GetKeyValue(xType, "name");
						string strDisplayName = XmlHelper.GetKeyValue(xType, "display_name") ?? XmlHelper.GetKeyValue(xType, "name");

						ToolStripMenuItem newItem = tmiAdd.DropDownItems.Add(strDisplayName, null, newItem_Click) as ToolStripMenuItem;
						newItem.Name = strTypeName;
						newItem.Tag = xType; // This is a reference to the Xml definition for this type in the definition document.
					}
					tmiAdd.Enabled = true;
				}
			}
		}

		/// <summary>
		/// This is called by a tree context menu when the user has requested to add an item to the tree.
		/// It takes a 'type' and adds it to the Project document.
		/// It also creates a tree object, gives the tree object a reference to it's Xml data and adds the tree object to the tree.
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void newItem_Click(object sender, EventArgs e)
		{
			ToolStripMenuItem mi = sender as ToolStripMenuItem;
			XmlElement xType = mi.Tag as XmlElement; // Reference to 'type' in the definition document.

			TreeUserObject treeChildUserObject = AddTreeType(xType, null);

			this.OwnerProjectObject.TreeView.SelectedNode = treeChildUserObject;
			treeChildUserObject.BeginEdit();
		}

		/// <summary>
		/// Creates a new 'type' and adds it to both ExecuterTreeView and the project's Xml document.
		/// </summary>
		/// 
		/// <param name="xType">Xml (from the template) describing the new type.</param>
		/// <param name="strName">The friendly name to be given to the type. (As is is viewed in the tree view)</param>
		/// <returns></returns>
		internal TreeUserObject AddTreeType(XmlElement xType, string strName)
		{
			TreeUserObject treeParent = this;

			XmlElement xParent = treeParent.XTreeNodeMenuData; // This is where the type must be added to the project document.

			string strTypeName = XmlHelper.GetKeyValue(xType, "name");
			string strTypeDisplayName = XmlHelper.GetKeyValue(xType, "display_name");
			if(strTypeDisplayName == null)
				strTypeDisplayName = strTypeName;

			XmlElement xNewChild = xParent.OwnerDocument.CreateElement("type_entry");

			XmlHelper.AddAttribute(xNewChild, "type_name", strTypeName);

			XmlNode xValues = treeParent.xMenuProjectDocument.CreateElement("values");
			xNewChild.AppendChild(xValues);

			xParent.AppendChild(xNewChild);

			if(strName == null)
				strName = "New " + strTypeDisplayName;
			XmlHelper.AddAttribute(xNewChild, "name", strName);

			TreeUserObject treeChild = new TreeUserObject(treeParent.OwnerProjectObject, xNewChild, false);

			treeChild.ForeColor = this.OwnerProjectObject.Colour;


			if(this.Parent == null) //HACK to see if this is a hidden root node.
			{
				// Hidden root node.
				this.OwnerProjectObject.Nodes.Add(treeChild);
			}
			else
				this.Nodes.Add(treeChild);

			treeParent.OwnerProjectObject.IsDirty = true; // A tree node has been added, so mark the file as dirty.
			treeChild.EnsureVisible();
			return treeChild;
		}

		/// <summary>
		/// Brute force build the task list.
		/// </summary>
		/// <param name="tmiTasks"></param>
		/// <param name="tuo"></param>
		/// <returns></returns>
		internal bool AddTasks(ToolStripMenuItem tmiTasks, bool bClearExistingTasks, bool bMergeDuplicates)
		{
			if(bClearExistingTasks == true)
				tmiTasks.DropDownItems.Clear();
			try
			{
				XmlElement xTemplate = this.XTypeTemplate;

				if(xTemplate != null)
				{
					XmlElement xTaskList = xTemplate["task_list"];

					if(xTaskList != null)
					{
						List<string> listStrTasks = new List<string>();

						foreach(XmlNode xNode in xTaskList)
						{
							if(xNode.NodeType == XmlNodeType.Element)
							{
								XmlElement xTask = xNode as XmlElement;
								string strTaskName = XmlHelper.GetKeyValue(xTask, "name");
								string strDisplayName = XmlHelper.GetKeyValue(xTask, "display_name");
								string strActionName = XmlHelper.GetKeyValue(xTask, "action_name");

								if(listStrTasks.Contains(strTaskName))
								{
									ToolStripMenuItem newItem = tmiTasks.DropDownItems.Add("ERROR IN TEMPLATE : DUPLICATED TASK NAME '" + strTaskName + "'") as ToolStripMenuItem;
									newItem.BackColor = System.Drawing.Color.Red;
								}
								else
								{
									listStrTasks.Add(strTaskName);

									if((strDisplayName.ToLower().StartsWith("[debug]") == false) || (Program.s_bDebugModeEnabled == true))
									{
										if(strDisplayName.ToLower().StartsWith("[dummy]") == false)
										{
											if(strActionName == "separator")
											{
												tmiTasks.DropDownItems.Add(new ToolStripSeparator());
											}
											else
											{
												//										XmlNode xAction = XmlHelper.FindChildNode(xActions, "action", "name", strActionName);

												List<string> childValidServers;
												if(this.OwnerProjectObject._actionServersLookup.TryGetValue(strActionName, out childValidServers) == false)
												{
													// Not found.
													childValidServers = new List<string>();
													strDisplayName = "(ERROR: missing action)" + strDisplayName;
												}
												ToolStripMenuItem newItem = tmiTasks.DropDownItems.Add(strDisplayName) as ToolStripMenuItem;
												List<string> childValidRecurseServers = this.GetValidRecurseServers(strTaskName);

												#region Recurse

												string strRecurse = XmlHelper.GetKeyValue(xTask, "process_recursively");

												if((strRecurse == null) || (strRecurse.ToLower() != "ask") || (this.Nodes.Count == 0)) // If there are no children there is no need to ask about recursive processing.
												{
													this.AddServers(newItem, childValidServers, XmlHelper.GetKeyBool(xTask, "process_recursively", false), strTaskName);
												}
												else
												{
													ToolStripLabel tsl = new ToolStripLabel("Process children?");
													tsl.Font = ExecuterTreeView.fontHeading;
													newItem.DropDownItems.Add(tsl);

													ToolStripMenuItem tmiRecurse = newItem.DropDownItems.Add("Yes") as ToolStripMenuItem;
													ToolStripMenuItem tmiNoRecurse = newItem.DropDownItems.Add("No") as ToolStripMenuItem;
													this.AddServers(tmiRecurse, childValidRecurseServers, true, strTaskName);
													this.AddServers(tmiNoRecurse, childValidServers, false, strTaskName);
												}

												#endregion Recurse
											}
										}
									}
								}
							}
						}
					}
				}
			}
			catch(Exception ex)
			{
				Debug.WriteLine("Exception in 'AddTasks' : " + ex.Message);
				Debug.Assert(false);
			}

			if(tmiTasks.DropDownItems.Count == 0)
			{
				tmiTasks.Enabled = false;
			}
			else
			{
				tmiTasks.Enabled = true;
				//ToolStripLabel tsl = new ToolStripLabel("Choose your task");
				//tsl.Font = ExecuterTreeView.fontHeading;
				//tsl.Width = 105;								// HACK: For some reason the first time this is shown it is too narrow. Subsequent views of it are fine.
				//tmiTasks.DropDownItems.Insert(0, tsl);
			}

			return true;
		}

		private void AddServers(ToolStripMenuItem newItem, List<string> childValidServers, bool bRecurse, string strTaskName)
		{
			if(childValidServers == null)
			{
				ToolStripLabel tsl2 = new ToolStripLabel("Choose your server");
				tsl2.Font = ExecuterTreeView.fontHeading;
				newItem.DropDownItems.Add(tsl2);
				newItem.DropDownItems.Add("There are no servers defined that can do this for you.");
				newItem.DropDownItems.Add("Please attend to your template.");
				return;
			}

			if(childValidServers.Count == 0)
			{
				newItem.Enabled = false;
			}
			else
			{
				if(childValidServers.Count > 1)
				{
					ToolStripLabel tsl2 = new ToolStripLabel("Choose your server");
					tsl2.Font = ExecuterTreeView.fontHeading;
					newItem.DropDownItems.Add(tsl2);
				}


				foreach(string strServer in childValidServers)
				{
					ToolStripMenuItem tmiCurrent;

					if(childValidServers.Count == 1) // If we don't need to make a menu entry for this one ...
					{
						tmiCurrent = newItem;
					}
					else
					{
						tmiCurrent = newItem.DropDownItems.Add(strServer, null, null) as ToolStripMenuItem;
					}
					tmiCurrent.Click += taskToolStripMenuItem_Click;
					tmiCurrent.Tag = new CommandDetails(strServer, bRecurse, this, strTaskName);
					FarmClient se = Program.s_farmClientManager.FindClientByLocalName(strServer);
					Debug.Assert(strServer == "immediate" || se != null, "AddTasks: Cannot find server by name.");

					if(se != null)
					{
						if(se.Connected != true)
						{
							tmiCurrent.Enabled = false;
						}
					}
				}
			}
		}

		private List<string> GetValidRecurseServers(string strTaskName)
		{
			// Get the server list this TreeUserObject action can run on ...
			List<string> validServers;

			string strActionName = null;
			XmlElement xTemplate = this.XTypeTemplate;
			XmlElement xTaskList = xTemplate["task_list"];
			if(xTaskList != null)
			{
				XmlElement xTask = XmlHelper.FindNodeByKeyValue(xTaskList, "name", strTaskName) as XmlElement;
				strActionName = XmlHelper.GetKeyValue(xTask, "action_name");
			}

			if(strActionName != null && this.OwnerProjectObject._actionServersLookup.TryGetValue(strActionName, out validServers) == true)
			{
				List<string> recurseServers = new List<string>(validServers.Count); // We do not want to modify validServers, so copy it.
				foreach(string strServer in validServers)
				{
					recurseServers.Add(strServer);
				}

				// recurseServers is our return value. First we must 'merge' it with the tree object children.
				foreach(TreeUserObject toChild in this.Nodes)
				{
					List<string> childValidServers = toChild.GetValidRecurseServers(strTaskName);
					if(childValidServers != null)
					{
						List<string> newServers = new List<string>(recurseServers.Count);

						foreach(string strServer in childValidServers)
						{
							if(recurseServers.Contains(strServer))
							{
								newServers.Add(strServer);
							}
						}
						recurseServers = newServers;
					}
					else
						return null;
				}
				return recurseServers;
			}
			else
				return null;
		}

		/// <summary>
		/// Called via the tree context menu when we want to process a task.
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private static void taskToolStripMenuItem_Click(object sender, EventArgs e)
		{
			CommandDetails cd = (sender as ToolStripMenuItem).Tag as CommandDetails;
			bool bRecurse = cd.m_bRecurse;
			TreeUserObject tuo = cd.m_treeUserObject;
			try
			{
				cd.m_treeUserObject.DoTask(cd.m_strTaskName, cd.m_strServerName, cd.m_bRecurse);
			}
			catch(Exception ex)
			{
				MessageHandler.Show(null, "'DoTask' failed.\nSystem reports: " + ex.Message, "Executer", MessageBoxButtons.OK, MessageBoxIcon.Error);
			}
		}

		private TreeUserObject DuplicateMyself(TreeProjectObject newOwnerProject)
		{
			XmlElement tempTreeNodeMenuData = newOwnerProject._xMenuProjectDocument.ImportNode(XTreeNodeMenuData, true) as XmlElement;

			return new TreeUserObject(newOwnerProject, tempTreeNodeMenuData, true);
		}

		internal bool Move(TreeUserObject moveFrom)
		{
			if(this.IsTreeChildAllowed(moveFrom))
			{
				// Do the move.
				this.XTreeNodeMenuData.AppendChild(moveFrom.XTreeNodeMenuData);

				if(this.Parent == null) //HACK to see if this is a hidden root node.
				{
					// Hidden root node.
					this.OwnerProjectObject.Nodes.Add(moveFrom);
				}
				else
					this.Nodes.Add(moveFrom);
			}
			return true;
		}

		internal bool AutoRename()
		{
			try
			{
				//if (CanRename() == true)
				{
					XmlElement xAutoNameElement = XTypeTemplate.SelectSingleNode("auto_name_property") as XmlElement;

					if (xAutoNameElement != null)
					{
						XmlNodeList xStringBuilderElements = xAutoNameElement.SelectNodes("stringbuilder");

						string strFullString = "";

						foreach (XmlElement xCurrent in xStringBuilderElements)
						{
							string strXpath = XmlHelper.GetKeyValue(xCurrent, "xpath");

							string strInput = "";

							try
							{
								strInput = XTreeNodeMenuData.SelectSingleNode(strXpath).Value;
							}
							catch (Exception)
							{
							}

							string strPart;


							string strPattern = XmlHelper.GetKeyValue(xCurrent, "regexp");

							if (!string.IsNullOrEmpty(strPattern))
							{
								try
								{
									strPart = Regex.Match(strInput, strPattern, RegexOptions.IgnoreCase).Value;
								}
								catch(Exception ex)
								{
									MessageHandler.Show(Program.S_executerMainForm,
									                    "Perform the auto-rename. Bad pattern:\n'"+strPattern+"'\nSystem reports:\n'" + ex.Message + "'", "Executer",
									                    MessageBoxButtons.OK);
									return false;
								}
							}
							else
							{
								strPart = strInput;
							}

							strFullString += strPart;
						}

						string strFinalPattern = XmlHelper.GetKeyValue(xAutoNameElement, "regexp");

						string strFinalAnswer;

						if (!string.IsNullOrEmpty(strFinalPattern))
						{
							try
							{
								strFinalAnswer = Regex.Match(strFullString, strFinalPattern, RegexOptions.IgnoreCase).Value;
							}
							catch(Exception ex)
							{
								MessageHandler.Show(Program.S_executerMainForm,
													"Perform the auto-rename. Bad final pattern:\n'" + strFinalPattern + "'\nSystem reports:\n'" + ex.Message + "'", "Executer",
													MessageBoxButtons.OK);
								return false;
							}
						}
						else
						{
							strFinalAnswer = strFullString;
						}


						if(this.Text != strFinalAnswer)
						{
							bool bDirty = this.OwnerProjectObject.IsDirty;

							RestrictedMode rm = this.OwnerProjectObject.ReadableState;
							
							Rename(strFinalAnswer);

							if(this.CanRename() == false)
							{
								Debug.Assert(bDirty == false);
								this.OwnerProjectObject.IsDirty = bDirty;
								this.OwnerProjectObject.ReadableState = rm;
							}
						}


						if(this.CanRename() == true && XmlHelper.GetKeyBool(XTreeNodeMenuData, "auto_name", false) == false)
						{
							this.OwnerProjectObject.IsDirty = true;
							XmlHelper.AddAttribute(XTreeNodeMenuData, "auto_name", "true");
						}

						this.NodeFont = fontLockedFilename;
						// Overrides 'false' set by the rename method call.

						return true;
					}
				}
			}
			catch (Exception ex)
			{
				MessageHandler.Show(Program.S_executerMainForm, "Perform the auto-rename. System reports:\n'" + ex.Message + "'", "Executer", MessageBoxButtons.OK);
			}
			return false;
		}

		/// <summary>
		/// Returns a dictionary of &lt;strTaskName, strDisplayName&gt; of tasks on this node.
		/// </summary>
		/// <returns></returns>
		public Dictionary<string, string> GetTasks()
		{
			Dictionary<string, string> results = null;
			try
			{
				XmlElement xTemplate = this.XTypeTemplate;


				if(xTemplate != null)
				{
					XmlNodeList xNodes = xTemplate.SelectNodes("task_list/task");

					if(xNodes != null && xNodes.Count > 0)
					{
						results = new Dictionary<string, string>(xNodes.Count);

						foreach(XmlElement xTask in xNodes)
						{
							string strTaskName = XmlHelper.GetKeyValue(xTask, "name");
							string strDisplayName = XmlHelper.GetKeyValue(xTask, "display_name");
							string strActionName = XmlHelper.GetKeyValue(xTask, "action_name");


							if((strDisplayName.ToLower().Contains("[debug]") == false) || (Program.s_bDebugModeEnabled == true))
							{
								if(strDisplayName.ToLower().Contains("[dummy]") == false)
								{
									if(strActionName != "separator")
									{
										results.Add(strTaskName, strDisplayName);
									}
								}
							}
						}
					}
				}
			}
			catch(Exception ex)
			{
				MessageBox.Show(null, "Exception in TreeUserObject.GetTasks:\n'" + ex.Message + "'", "Exception", MessageBoxButtons.OK, MessageBoxIcon.Error);
				return null;
			}
			return results;
		}

							
	}
}
