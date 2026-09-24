using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using System.Diagnostics;
using System.Xml;
using Executer.Helpers;
using Executer.Interfaces;
using Executer.JobManagement;
using MenuSystemLibrary;
using Global;
using message_handler;

namespace Executer
{
	public partial class TreeUserObjectViewer : UserControl, IViewedObject
	{
		readonly Dictionary<Job, JobPanel> _jobEntryList = new Dictionary<Job, JobPanel>();

		/// <summary>
		/// The menuSystem that is presented to the user.
		/// </summary>
		MenuSystem _menuSystem;

		readonly TreeUserObject m_treeUserObject;

		public TreeUserObjectViewer(TreeUserObject treeUserObject)
		{
			m_treeUserObject = treeUserObject;
			InitializeComponent();

			this.Visible = true;
			this.CreateControl();
		}

		bool _bLoadEventHasOccured = false;

		private void TreeUserObjectViewer_Load(object sender, EventArgs e)
		{
			// Setting Form.ShowInTaskBar (elsewhere) causes .NET to try and recreate all the controls.
			if(_bLoadEventHasOccured == false)
			{
				SetView();
				_bLoadEventHasOccured = true;
			}
		}


		private bool SetView()
		{
			Debug.Assert(m_treeUserObject != null);

			_jobEntryList.Clear();

			this.tabControl_Main.Visible = false;

			Debug.Assert(tabControl_Main.TabPages.Count == 0);

			if(m_treeUserObject != null)
			{
				Debug.Assert(m_treeUserObject.OwnerProjectObject != null);
				Debug.Assert(m_treeUserObject.XTreeNodeMenuData != null);

				string strTypeName = XmlHelper.GetKeyValue(m_treeUserObject.XTreeNodeMenuData, "type_name");
				string strName = XmlHelper.GetKeyValue(m_treeUserObject.XTreeNodeMenuData, "name");

				// First of all create the menu data associated with the tree node and add it to the tabcontrol.
				if(m_treeUserObject.XTreeNodeMenuData != null)
				{
					try
					{
						// Get the Xml template associated with this node.
						if(m_treeUserObject.XTypeTemplate != null)
						{
							//XmlElement xMenuTemplate = m_treeUserObject.XTypeTemplate["sub_menu_template"]["menu_system"];
							XmlElement xMenuTemplate = m_treeUserObject.XTypeTemplate.SelectSingleNode("sub_menu_template/menu_system") as XmlElement;

							_menuSystem = new MenuSystem();
							//m_menuSystem.m_bReadOnly = m_treeUserObject.OwnerProjectObject.IsReadonly;
							RestrictedMode rm = m_treeUserObject.OwnerProjectObject.ReadableState;
							if(rm == RestrictedMode.ReadOnly || rm == RestrictedMode.Restricted)
								_menuSystem.m_bReadOnly = true;
							else
								_menuSystem.m_bReadOnly = false;

							//m_menuSystem.Initialise(xMenuTemplate, MenuChanged);
							_menuSystem.Initialise(xMenuTemplate);

							_menuSystem.MenuChangedEvent += m_menuSystem_MenuChangedEvent;
							_menuSystem.MenuComponentCreatedEvent += m_menuSystem_MenuComponentCreatedEvent;

							if(Program.s_bDebugModeEnabled == true && Program.s_bValidateMenuTemplates == true)
							{
								XmlElement xFirstDuplicate = this._menuSystem.CheckContainerNamesAreUnique();
								if(xFirstDuplicate != null)
								{
									string strError = "The name '" + XmlHelper.GetKeyValue(xFirstDuplicate, "name") + "' is multiply defined.";

									MessageHandler.ShowMemory(this, "template duplicate container", "Ignore template container name duplication errors", "The template for this object has an error.\n" + strError, "Executer", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
								}
							}
							XmlElement xMenuRoot = m_treeUserObject.XTreeNodeMenuData.SelectSingleNode("values/container") as XmlElement;
							UtilityFunctions.FixupSpecialMenuCases(xMenuRoot);

							_menuSystem.SetMenu(xMenuRoot, true);
						}
						else
						{
							_menuSystem = new MenuSystem();
							_menuSystem.SetError("This node type does not exist in your template.\nSee your template manager or consider deleting the node.");
						}
						_menuSystem.Dock = DockStyle.Fill;
						TabPage tabPage = new TabPage("Properties");
						tabPage.Controls.Add(_menuSystem);
						this.tabControl_Main.TabPages.Add(tabPage);
					}

					catch(Exception ex)
					{
						Debug.WriteLine("Exception in TreeUserObjectViewer.SetView when drawing the first panel: '" + ex.Message + "'");
					}
				}

				foreach(Job job in m_treeUserObject.JobList)
				{
					AddJobPanel(job);
				}
				this.tabControl_Main.SelectedIndex = m_treeUserObject.LastViewedJobPanel;
				this.tabControl_Main.Visible = true;
			}
			return true;
		}


		public bool ViewJob(Job job)
		{
			JobPanel jobPanel;


			_jobEntryList.TryGetValue(job, out jobPanel);

			if(jobPanel != null)
			{
				this.tabControl_Main.SelectedTab = jobPanel.Parent as TabPage;
			}

			return true;
		}



		void m_menuSystem_MenuComponentCreatedEvent(object sender, MenuComponentCreatedEventArgs e)
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





		private bool AddJobPanel(Job job)
		{
			Debug.Assert(m_treeUserObject == job.OwnerTreeUserObject);

			if(m_treeUserObject == job.OwnerTreeUserObject)				// If the job belongs to the currently viewed tree node ...
			{
				if(job != null)
				{
					string strDescription = "Job '" + job.StrTaskDisplayName + "' running on '" + job.StrServerName + "'";

					JobPanel jobPanel = new JobPanel(this, job, strDescription);
					_jobEntryList.Add(job, jobPanel);

					TabPage tabPage = new TabPage(job.StrTaskDisplayName);
					jobPanel.Dock = DockStyle.Fill;
					tabPage.Controls.Add(jobPanel);
					this.tabControl_Main.TabPages.Add(tabPage);

					return true;
				}
				else
					return false;
			}
			return false;
		}


		/// <summary>
		/// This menu change event is associated with the menu that belongs to the current tree object.
		/// When the menu is changed, this event is called.
		/// </summary>
		/// <param name="sender">The menu instance that sent the notification.</param>
		/// <param name="e">Details about the change.</param>
		void m_menuSystem_MenuChangedEvent(object sender, MenuSystemChangedEventArgs e)
		{
			RestrictedMode rm = m_treeUserObject.OwnerProjectObject.ReadableState;
			bool bWritable = false;
			if(rm == RestrictedMode.PrimaryRestricted || rm == RestrictedMode.Writable)
				bWritable = true;

			//if((m_treeUserObject.OwnerProjectObject.IsReadonly == false) && (e.FlagDocumentAsChanged == true))
			if((bWritable == true) && (e.FlagDocumentAsChanged == true))
				m_treeUserObject.OwnerProjectObject.IsDirty = true;

			XmlElement xTreeItem = m_treeUserObject.XTreeNodeMenuData;

			XmlNode xValues = xTreeItem["values"];
			// FIX 001:
			if(xValues == null)
			{
				xValues = m_treeUserObject.xMenuProjectDocument.CreateElement("values");
				xTreeItem.AppendChild(xValues);
			}
			xValues.RemoveAll();

			XmlElement xImportedChild = m_treeUserObject.xMenuProjectDocument.ImportNode(e.XOwnerDocument.DocumentElement, true) as XmlElement;
			xValues.AppendChild(xImportedChild);
			
			if(XmlHelper.GetKeyBool(e.XChangedElement, "is_target_of_custom_animation", false) == true)
			{
				UtilityFunctions.UpdateCustomAnimation(e.XChangedElement, UtilityFunctions.CustomUpdate.UpdateMenu, _menuSystem);
			}
			else if(XmlHelper.GetKeyValue(e.XChangedElement, "special_type") == "custom_animation")
			{
				UtilityFunctions.UpdateCustomAnimation(e.XChangedElement, UtilityFunctions.CustomUpdate.UpdateMenu, _menuSystem);
			}
			if(XmlHelper.GetKeyBool(m_treeUserObject.XTreeNodeMenuData, "auto_name", false) == true)
				m_treeUserObject.AutoRename();
		}


		/// TODO:
		/// Need to map 'custom_animation_target' to the object it is the target of.			Done.
		/// This map should be built using the m_menuSystem_MenuComponentCreatedEvent.
		/// From there we can get and update all the targets.
		/// This won't work for menus that aren't drawn before being exported.
		/// What a pain in the arse.
		private System.Collections.ObjectModel.ObservableCollection<object> oc;


		internal void RemovePanel(JobPanel jobPanel)
		{
			oc.CollectionChanged += new System.Collections.Specialized.NotifyCollectionChangedEventHandler(oc_CollectionChanged);

			TabPage tabPage = jobPanel.Parent as TabPage;
			TabControl tabControl = tabPage.Parent as TabControl;
			int currentTabSelectedIndex = tabControl.SelectedIndex;

			this.tabControl_Main.TabPages.Remove(tabPage);

			Program.s_jobManager.RemoveJob(jobPanel._job);

			if(currentTabSelectedIndex < tabControl.TabCount)
				tabControl.SelectedIndex = currentTabSelectedIndex;
			else
				tabControl.SelectedIndex = tabControl.TabCount - 1;
		}

		void oc_CollectionChanged(object sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
		{
			throw new NotImplementedException();
		}

		internal void RemoveJobEntry(Job job)
		{
			this._jobEntryList.Remove(job);
		}

		internal void SetDefaultValues()
		{
			_menuSystem.SetMenu(null, true);

			Debug.Assert(m_treeUserObject != null, "Current tree object is null in SetDefaultValues.");

			//if(m_treeUserObject.OwnerProjectObject.IsReadonly == false)

			RestrictedMode rm = m_treeUserObject.OwnerProjectObject.ReadableState;
			if(rm == RestrictedMode.Writable || rm == RestrictedMode.PrimaryRestricted)
				m_treeUserObject.OwnerProjectObject.IsDirty = true;
		}

		#region IViewedObject Members

		public bool UpdateView()
		{
			bool bRetVal = false;

			foreach(Job job in m_treeUserObject.JobList)
			{
				if(_jobEntryList.ContainsKey(job) == false)
				{
					AddJobPanel(job);
					bRetVal = true;
				}
			}
			this.tabControl_Main.SelectedIndex = this.tabControl_Main.TabCount - 1;
			return bRetVal;
		}

		#endregion

		private void tabControl_Selected(object sender, TabControlEventArgs e)
		{
			if(e.TabPageIndex != -1)
				m_treeUserObject.LastViewedJobPanel = e.TabPageIndex;
		}
	}
}