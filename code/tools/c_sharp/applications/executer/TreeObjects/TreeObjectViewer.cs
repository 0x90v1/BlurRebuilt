using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;
using System.Xml;
using System.IO;
using Executer.Helpers;
using Executer.Interfaces;
using MenuSystemLibrary;
using Global;

namespace Executer.TreeObjects
{
	public partial class TreeObjectViewer : UserControl
	{
		IViewableObject _currentlyViewedTreeObject = null;
		object o = new NameTable();
		public IViewableObject CurrentlyViewedTreeObject
		{
			get { return _currentlyViewedTreeObject; }
		}

		public TreeObjectViewer()
		{
			InitializeComponent();
		}

		public bool ViewtreeObject(IViewableObject objectToView)
		{
			Debug.Assert(((TreeNode)objectToView).TreeView.CheckBoxes == false);

			_currentlyViewedTreeObject = objectToView;

			ClearViewer();

			if(objectToView != null)
			{
				Control viewedObject = objectToView.CreateView();

				viewedObject.Dock = DockStyle.Fill;

				this.Controls.Add(viewedObject);

				return true;
			}
			else
			{
				return false;
			}
		}

		/// <summary>
		/// This menu change event is associated with the menu that belongs to the current tree object.
		/// When the menu is changed, this event is called.
		/// </summary>
		/// <param name="sender">The menu instance that sent the notification.</param>
		/// <param name="e">Details about the change.</param>
		void m_menuSystem_MenuChangedEvent(object sender, MenuSystemChangedEventArgs e)
		{
			string strName = Global.XmlHelper.GetKeyValue(e.XChangedElement, "name");
			string strValue = Global.XmlHelper.GetKeyValue(e.XChangedElement, "value");
			
			if(strName != null)
			{
				if(strValue != MenuSystem.CONFLICTED_MARKER)
				{
					foreach(TreeUserObject treeUserObject in m_viewedTree._CheckedTreeObjects)
					{
						string strPathQuery = ".//container[@name='" + strName + "']";

						XmlElement xDest = treeUserObject.XTreeNodeMenuData.SelectSingleNode(strPathQuery) as XmlElement;

						if(xDest != null)
						{
							// The named attribute will not exist if it has never been set.
							XmlAttribute xAttr = xDest.Attributes.GetNamedItem("value") as XmlAttribute;
							//xAttr.Value = strValue;
							XmlHelper.AddAttribute(xDest, "value", strValue);

							if(XmlHelper.GetKeyBool(treeUserObject.XTreeNodeMenuData, "auto_name", false) == true)
								treeUserObject.AutoRename();


							treeUserObject.OwnerProjectObject.IsDirty = true;
						}
					}
				}
			}
			MenuSystem ms = sender as MenuSystem;

			if(XmlHelper.GetKeyBool(e.XChangedElement, "is_target_of_custom_animation", false) == true)
			{
				UtilityFunctions.UpdateCustomAnimation(e.XChangedElement, UtilityFunctions.CustomUpdate.UpdateMenu, ms);
			}
			else if(XmlHelper.GetKeyValue(e.XChangedElement, "special_type") == "custom_animation")
			{
				UtilityFunctions.UpdateCustomAnimation(e.XChangedElement, UtilityFunctions.CustomUpdate.UpdateMenu, ms);
			}




			//XmlElement xTypeEntry = e.XChangedElement.SelectSingleNode("ancestor::type_entry") as XmlElement;

			//Debug.Assert(xTypeEntry != null);

			//if(xTypeEntry != null)
			//{e.
			//    // What do I add it to? ...
			//    TreeUserObject treeUserRootObject = treeProjectObject.Nodes[0] as TreeUserObject;








		}

		ExecuterTreeView m_viewedTree = null;

		public bool ViewMultipleTreeObjects(ExecuterTreeView treeView)
		{
			m_viewedTree = treeView;

			Debug.Assert(treeView.CheckBoxes == true);

			ClearViewer();

			if(treeView != null)
			{
				bool bHasCheckboxes = treeView.CheckBoxes;

				if(bHasCheckboxes)
				{
					XmlDocument xMergedDocument = new XmlDocument();

					//MenuSystemLibrary.MenuSystem.MergeMode mergeMode = MenuSystemLibrary.MenuSystem.MergeMode.MasterIsTemplate;
					MenuSystemLibrary.MenuSystem.MergeMode mergeMode = MenuSystemLibrary.MenuSystem.MergeMode.MayExistInEither;

					//string strCompareMode = ExecuterMainForm.s_optionsManager.GetContainerStringByName("multiselect_mode");

					//switch(strCompareMode)
					//{
					//    case "Union":
					//        mergeMode = MenuSystem.MergeMode.MayExistInEither;
					//        break;
					//    case "Intersection":
					//        mergeMode = MenuSystem.MergeMode.MustExistInBoth;
					//        break;
					//    case "Like for Like":
					//        mergeMode = MenuSystem.MergeMode.MayExistInEither;		// In this mode, the input must be filtered.
					//        break;
					//    default:
					//        goto case "Union";

					//}

					foreach(TreeBaseObject treeObject in treeView._CheckedTreeObjects)
					{
						IViewableObject viewableObject = treeObject as IViewableObject;

						if(viewableObject != null)
						{
							if(viewableObject.RawXmlDocument != null)
							{
								//XmlElement xMenu = viewableObject.RawXmlDocument["values"]["container"];
								XmlElement xMenu = viewableObject.RawXmlDocument.SelectSingleNode("values/container") as XmlElement;

								if(xMenu != null)
								{
									xMergedDocument = MenuSystemLibrary.MenuSystem.MergeMenuDocuments(xMenu, xMergedDocument.DocumentElement, mergeMode);

									//mergeMode = MenuSystemLibrary.MenuSystem.MergeMode.MustExistInBoth;
								}
							}
						}
					}

					MenuSystemLibrary.MenuSystem ms = new MenuSystemLibrary.MenuSystem();

					ms.Initialise(null);

					try
					{
						ms.MenuComponentCreatedEvent += new EventHandler<MenuComponentCreatedEventArgs>(ms_MenuComponentCreatedEvent);
						ms.MenuChangedEvent += m_menuSystem_MenuChangedEvent;
						ms.ForceMenu(xMergedDocument.DocumentElement);
						ms.Dock = DockStyle.Fill;

						this.Controls.Add(ms);
						return true;
					}
					catch
					{
						return false;
					}
				}
			}
			return false;
		}

		void ms_MenuComponentCreatedEvent(object sender, MenuComponentCreatedEventArgs e)
		{
			XmlElement xObject = e._createdComponent.m_xObject;

			string strSpecialType = XmlHelper.GetKeyValue(xObject, "special_type");

			if(strSpecialType != null)
			{
				if(strSpecialType == "custom_animation")
				{
					UtilityFunctions.SetupCustomAnimation(xObject);
				}
				else
				{
					// Error.
				}
			}
		}




		public bool ClearViewer()
		{
			List<Control> controlsToRemove = new List<Control>();

			Debug.Assert(this.Controls.Count < 2);

			foreach(Control control in this.Controls)
			{
				controlsToRemove.Add(control);
			}

			foreach(Control control in controlsToRemove)
			{
				this.Controls.Remove(control);
				control.Dispose();
			}

			return true;
		}

		public bool SetDefaultValues()
		{
			MessageBox.Show("SetDefaultValues has beed disabled for the safety of mankind.", "Executer - Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
			return false;
			throw new ApplicationException("TreeObjectViewer.SetDefaultValues Not implemented.");
		}

		internal void UpdateView()
		{
			Debug.Assert(this.Controls.Count < 2);
			
			if(this.Controls.Count == 1)
			{
				IViewedObject viewedObject = this.Controls[0] as IViewedObject;

				if(viewedObject != null)
				{
					viewedObject.UpdateView();
				}
			}
		}
	}
}