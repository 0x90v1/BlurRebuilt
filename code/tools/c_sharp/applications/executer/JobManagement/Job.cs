#define JAMIE

using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;
using System.Windows.Forms;
using Executer.Helpers;
using Executer.TreeObjects;
using message_handler;
using Global;
using System.Diagnostics;
using System.IO;


namespace Executer.JobManagement
{
	public class JobStatusChangedEventArgs : EventArgs
	{
		private readonly JobStatus m_newJobStatus;
		private readonly JobStatus m_oldJobStatus;

		public JobStatusChangedEventArgs(JobStatus oldJobStatus, JobStatus newJobStatus)
		{
			m_oldJobStatus = oldJobStatus;
			m_newJobStatus = newJobStatus;
		}

		public JobStatus OldJobStatus
		{
			get { return m_oldJobStatus; }
		}

		public JobStatus NewJobStatus
		{
			get { return m_newJobStatus; }
		}
	}

	public class JobMenuDataChangedEventArgs : EventArgs
	{
		private readonly XmlDocument m_xDocNewMenuData;

		public JobMenuDataChangedEventArgs(XmlDocument newMenuData)
		{
			m_xDocNewMenuData = newMenuData;
		}

		public XmlDocument NewMenuData
		{
			get { return m_xDocNewMenuData; }
		}
	}

	public class Job
	{
		#region RequestStatus enum

		public enum RequestStatus
		{
			Uninitialised = 0,
			Requested,
			Queued, // Accepted by the server.
			Failed,
			Timeout,
		}

		#endregion

		private static LocallyUniqueIdMaker m_jobDataLuid = new LocallyUniqueIdMaker();
		private readonly bool m_bHideProcessWindow = false;
		private readonly bool m_bInitialised = true;
		private readonly bool m_bIsDummyJob;
		private readonly long m_localJobId = m_jobDataLuid.GetNextLuid();
		private readonly long m_localParentJobId;
		private readonly TreeUserObject m_ownerTreeUserObject;
		private readonly string m_strClArguments = null;

		private readonly string m_strCommand = null;
		private readonly string m_strFarmComment;
		private readonly string m_strOwnerProductName;
		private readonly string m_strServerName;
		private readonly string m_strTaskDisplayName;
		private readonly string m_strTaskName;
		private readonly string m_strTempArgsFilename;
		private readonly string m_strWorkingDirectory = null;
		private readonly XmlDocument m_xArguments = null;
		private int m_jobId = -1;
		private JobStatus m_jobStatus;
		private string m_strJobId = null;

		/// <summary>
		/// This is the current state of the menu that is associated with this job.
		/// </summary>
		private XmlDocument m_xJobMenu = new XmlDocument();

		internal Job(TreeUserObject ownerTreeObject, string strTaskName, string strServerName, long parentLuid)
		{
			m_ownerTreeUserObject = ownerTreeObject;
			m_strTaskName = strTaskName;
			m_strServerName = strServerName;
			m_localParentJobId = parentLuid;

			XmlElement xTask = GetXmlTask(ownerTreeObject.XTypeTemplate, strTaskName);
			XmlElement xAction = GetXmlAction(ownerTreeObject.OwnerProjectObject.XMenuTemplateDocument, xTask);

			if(xAction != null)
			{
				// Create a unique filename.
				m_strTempArgsFilename = UtilityFunctions.GetUniqueFile(m_ownerTreeUserObject.OwnerProjectObject.StrFarmTempFolder);

				MacroExpander.MacroExpander mx = new MacroExpander.MacroExpander();
				// todo: add expansions that are available to 'AddRootItem'. No, they should now be in dictVariables.
				//mx.AddPair("%source_machine%", System.Net.Dns.GetHostName());		// Already there.
				mx.AddPair("%args_file%", m_strTempArgsFilename /*strArgsFilename*/);
				mx.AddPair("%server%", strServerName);
				mx.AddPair("%object_name%", m_ownerTreeUserObject.Text /*strJobName*/);

				mx.AddDictionary(m_ownerTreeUserObject.OwnerProjectObject._mDictVariables);

				//ok
				this.m_strCommand = XmlHelper.GetKeyValue(xAction, "command_line");

				this.m_strCommand = mx.Expand(m_strCommand, true); // NEW EXPAND
				this.m_strWorkingDirectory = XmlHelper.GetKeyValue(xAction, "working_directory");
				this.m_strWorkingDirectory = mx.Expand(m_strWorkingDirectory, true);
				this.m_strClArguments = XmlHelper.GetKeyValue(xAction, "arguments"); // Cl args are macro expanded later.
				this.m_bHideProcessWindow = XmlHelper.GetKeyBool(xAction, "hide_process", false);
				bool bIncludeChildData = XmlHelper.GetKeyBool(xAction, "include_child_data");
				bool bIncludeParentData = XmlHelper.GetKeyBool(xAction, "include_parent_data");
				string strIncludeXmlDataFile = XmlHelper.GetKeyValue(xAction, "include_xml_data_file");

				strIncludeXmlDataFile = mx.Expand(strIncludeXmlDataFile, true); // NEW EXPAND
				bool bUseLegacyArguments = XmlHelper.GetKeyBool(xAction, "use_legacy_arguments");
				this.m_xArguments = new XmlDocument();

				m_strTaskDisplayName = XmlHelper.GetKeyValue(xTask, "display_name");
				if(m_strTaskDisplayName == null)
					m_strTaskDisplayName = strTaskName;

				// Dummy jobs are on tree objects that have tasks with no action. These tree objects are just there to group child objects, and to provide a
				// task list for their children. If a tree object task has no action, then it's job is not dependent on it's children but instead, it's parent's job is.
				// That's what bDummyJob is for.
				m_bIsDummyJob = false;
				string strAction = XmlHelper.GetKeyValue(xAction, "name");
				if((strAction.StartsWith("dummy") == true))
					m_bIsDummyJob = true;

				if(ownerTreeObject.TreeView is ExecuterTreeView)
				{
					ExecuterTreeView etv = ownerTreeObject.TreeView as ExecuterTreeView;
					if(etv.CheckBoxes == true)
						if(ownerTreeObject.Checked == false)
						{
							Debug.WriteLine("Checkbox dummy job.");
							m_bIsDummyJob = true;
						}
				}

				#region Include child and parent data

				if(bUseLegacyArguments == true)
				{
					if(bIncludeChildData == true)
					{
						this.m_xArguments.AppendChild(this.m_xArguments.ImportNode(m_ownerTreeUserObject.XTreeNodeMenuData, true));
					}
					else
					{
						this.m_xArguments.AppendChild(this.m_xArguments.ImportNode(m_ownerTreeUserObject.XTreeNodeMenuData["values"], true));
					}
				}
				else
				{
					if(bIncludeChildData == true)
					{
						// Dump out the whole project branch.
						this.m_xArguments.AppendChild(this.m_xArguments.ImportNode(m_ownerTreeUserObject.XTreeNodeMenuData /*xTreeNodeMenuData*/, true));
					}
					else
					{
						// Create the top-level node, non-recursively, and then add only the 'values' child.
						this.m_xArguments.AppendChild(this.m_xArguments.ImportNode(m_ownerTreeUserObject.XTreeNodeMenuData /*xTreeNodeMenuData*/, false));
						this.m_xArguments.DocumentElement.AppendChild(this.m_xArguments.ImportNode(m_ownerTreeUserObject.XTreeNodeMenuData /*xTreeNodeMenuData*/["values"], true));
					}
				}

				if(bIncludeParentData)
				{
					// Parse the parents up the tree and append in the data.
					try
					{
						XmlElement xCurrentParentSource = m_ownerTreeUserObject.XTreeNodeMenuData.ParentNode as XmlElement;
						XmlElement xCurrentParentDest = m_xArguments["type_entry"].AppendChild(m_xArguments.CreateElement("parent_data")) as XmlElement;

						while(xCurrentParentSource != null && xCurrentParentSource.Name != "root")
						{
							XmlElement xChildNode = xCurrentParentDest.AppendChild(m_xArguments.ImportNode(xCurrentParentSource, false)) as XmlElement;

							xChildNode.AppendChild(m_xArguments.ImportNode(xCurrentParentSource["values"], true));

							xCurrentParentSource = xCurrentParentSource.ParentNode as XmlElement;

							xCurrentParentDest = xCurrentParentDest.AppendChild(m_xArguments.CreateElement("parent_data")) as XmlElement;
						}
					}
					catch(Exception ex)
					{
						DialogResult dr = MessageHandler.ShowMemory
							(null, "include_parent_data failed", "Ignore 'include_parent_data' failure?", "Cannot process 'include_parent_data'\nSystem reports : " + ex.Message + "\nDo you wish to continue processing your tasks?",
							 Program.AssemblyTitle, MessageBoxButtons.YesNo, MessageBoxIcon.Error);
						if(dr != DialogResult.Yes)
						{
							m_bInitialised = false;
							return;
						}
					}
				}

				#endregion

				#region Include XmlDataFile

				if(strIncludeXmlDataFile != null)
				{
					string[] strDataFiles = strIncludeXmlDataFile.Split(new char[] {'|'});
#if JAMIE
					XmlElement xActionData = m_xArguments.SelectSingleNode("type_entry/actiondata") as XmlElement;
					if(xActionData == null)
					{
						xActionData = m_xArguments.DocumentElement.AppendChild(m_xArguments.CreateElement("actiondata")) as XmlElement;
					}
#endif
					foreach(string strDataFile in strDataFiles)
					{
						try
						{
							XmlDocument xDoc = new XmlDocument();
							xDoc.Load(strDataFile);
#if JAMIE
							xActionData.AppendChild(this.m_xArguments.ImportNode(xDoc.DocumentElement, true));
#else
							this.m_xArguments.DocumentElement.AppendChild(this.m_xArguments.ImportNode(xDoc.DocumentElement, true));
#endif
						}
						catch(Exception ex)
						{
							DialogResult dr = MessageHandler.ShowMemory
								(Program.ExecuterMainForm, "include_xml_data_file failed", "Ignore 'include_xml_data_file' failure?",
								 "Cannot process 'include_xml_data_file'\nThe file requested to include is '" + strDataFile + "'\n System reports : " + ex.Message + "\nDo you wish to continue processing your tasks?", Program.AssemblyTitle,
								 MessageBoxButtons.YesNo, MessageBoxIcon.Error);
							if(dr != DialogResult.Yes)
							{
								m_bInitialised = false;
								return;
							}
						}
					}
				}

				#endregion

				UtilityFunctions.FixupSpecialMenuCases(this.XArguments.DocumentElement);

				////////////////////////////////
				// TODO: m_xArguments must be constructed before here.
				try
				{
					using(StringWriter swArguments = new StringWriter())
					{
						XmlTextWriter tWriter = new XmlTextWriter(swArguments);
						tWriter.Formatting = Formatting.Indented;
						m_xArguments.WriteTo(tWriter);
						tWriter.Close();
						swArguments.Close();


						using(StreamWriter streamWriter = new StreamWriter(m_strTempArgsFilename))
						{
							streamWriter.Write(swArguments);
							streamWriter.Flush();
							streamWriter.Close();
						}
					}
				}
				catch(Exception ex)
				{
					throw new ApplicationException("Cannot create the temporary file '" + m_strTempArgsFilename + "'\nDoes the path exist and do you have permission to use it?\n" + "The system reports:\n" + ex.Message);
				}

				m_strClArguments = this.ExpandXmlValues(m_strClArguments, false);

				m_strFarmComment = XmlHelper.GetKeyValue(xTask, "farm_comment");
				if(m_strFarmComment == null)
					m_strFarmComment = "Executer. there is no farm_comment key for this action.";
				else
					m_strFarmComment = this.ExpandXmlValues(m_strFarmComment, true);

				XmlElement xConfig = XmlHelper.GetNodeFromPath(m_ownerTreeUserObject.OwnerProjectObject.XMenuTemplateDocument, "root/config");
				m_strOwnerProductName = XmlHelper.GetKeyValue(xConfig, "owner_product_name");

				////////////////////

				Debug.WriteLine("Command :" + this.m_strCommand + "\n" + this.m_xArguments.InnerXml.ToString() + "\n\n");

				if(this.m_strWorkingDirectory == null)
				{
					this.m_strWorkingDirectory = Path.GetDirectoryName(this.m_strCommand);
				}

				if(this.m_strClArguments == null)
					this.m_strClArguments = m_strTempArgsFilename /*strArgsFilename*/;
				else
				{
					this.m_strClArguments = mx.Expand(m_strClArguments, true); // NEW EXPAND
				}

				///////////////////
			}
			else
				m_bInitialised = false;
		}

		public string StrCommand
		{
			get { return m_strCommand; }
		}

		public string StrClArguments
		{
			get { return m_strClArguments; }
		}

		public string StrWorkingDirectory
		{
			get { return m_strWorkingDirectory; }
		}

		public XmlDocument XArguments
		{
			get { return m_xArguments; }
		}

		public bool HideProcessWindow
		{
			get { return m_bHideProcessWindow; }
		}

		public bool Initialised
		{
			get { return m_bInitialised; }
		}

		public long LocalJobId
		{
			get { return m_localJobId; }
		}

		public TreeUserObject OwnerTreeUserObject
		{
			get { return m_ownerTreeUserObject; }
		}

		public string StrTaskName
		{
			get { return m_strTaskName; }
		}

		public string StrServerName
		{
			get { return m_strServerName; }
		}

		public long LocalParentJobId
		{
			get { return m_localParentJobId; }
		}

		public bool IsDummyJob
		{
			get { return m_bIsDummyJob; }
		}

		public string StrFarmComment
		{
			get { return m_strFarmComment; }
		}

		public string StrOwnerProductName
		{
			get { return m_strOwnerProductName; }
		}

		public string StrTempArgsFilename
		{
			get { return m_strTempArgsFilename; }
		}

		public string StrTaskDisplayName
		{
			get { return m_strTaskDisplayName; }
		}

///////////////////////

		/// <summary>
		/// This is the current state of the menu that is associated with this job.
		/// </summary>
		public XmlDocument XJobMenu
		{
			get { return m_xJobMenu; }
			//set { m_xJobMenu = value; }
		}

		public string StrJobId
		{
			get { return m_strJobId; }
			set
			{
				bool bAlreadySet = false;
				if(m_strJobId != null)
					bAlreadySet = true;

				m_strJobId = value;

				if(bAlreadySet == true)
					throw new ApplicationException("StrJobId has been reassigned. Are you sure this is what you want?");
			}
		}

		public int JobId
		{
			get { return m_jobId; }
			set
			{
				if(m_jobId == -1)
					m_jobId = value;
				else
					throw new ApplicationException("JobId can only be set once.");
			}
		}

		public JobStatus JobStatus
		{
			get { return m_jobStatus; }
			set
			{
				if(m_jobStatus != value)
				{
					JobStatus oldJobStatus = m_jobStatus;
					m_jobStatus = value;
					OnJobStatusChanged(oldJobStatus, value);
				}
			}
		}

		public string StrJobAndArguments
		{
			get { return this.m_strCommand + " " + this.m_strClArguments; }
		}

		public event EventHandler<JobStatusChangedEventArgs> JobStatusChangedEvent;
		public event EventHandler<JobMenuDataChangedEventArgs> JobMenuDataChangedEvent;

		protected void OnJobStatusChanged(JobStatus oldJobStatus, JobStatus newJobStatus)
		{
			if(JobStatusChangedEvent != null)
			{
				JobStatusChangedEvent(this, new JobStatusChangedEventArgs(oldJobStatus, newJobStatus));
			}
		}

		protected void OnJobMenuDataChanged(XmlDocument xDocNewData)
		{
			if(this.JobMenuDataChangedEvent != null)
			{
				this.JobMenuDataChangedEvent(this, new JobMenuDataChangedEventArgs(xDocNewData));
			}
		}


		public bool ReceivedMenuData(XmlDocument xDoc)
		{
			try
			{
				bool bDataWasUseful = false;

				XmlElement xRootElement = xDoc.DocumentElement;
				if(xRootElement != null)
				{
					if(xRootElement.Name == "values")
					{
						this.m_xJobMenu = xDoc; // There is no template to merge the menu data with.
						bDataWasUseful = true;
					}
					else
					{
						Debug.Assert(xRootElement.Name == "container");

						if(xRootElement.Name == "container")
						{
							string strName = XmlHelper.GetKeyValue(xRootElement, "name");
							string strType = XmlHelper.GetKeyValue(xRootElement, "type");
							string strValue = XmlHelper.GetKeyValue(xRootElement, "value");

							List<XmlElement> xElementsToUpdate = XmlHelper.FindElementsByKeyValue(m_xJobMenu.DocumentElement, "name", strName, true, true, true);

							if(xElementsToUpdate.Count > 1)
							{
								XmlHelper.AddAttribute(xElementsToUpdate[0], "value", strValue);
								bDataWasUseful = true;
							}
						}
					}
				}

				if(bDataWasUseful == true)
				{
					m_xJobMenu = xDoc;
					OnJobMenuDataChanged(xDoc);
					return true;
				}
				return false;
			}
			catch
			{
				return false;
			}
		}

		public bool ReceivedMenuData(string strXmlData)
		{
			try
			{
				XmlDocument xDoc = new XmlDocument();
				xDoc.Load(new StringReader(strXmlData));

				return ReceivedMenuData(xDoc);
			}
			catch
			{
				return false;
			}
		}

///////////////////////

		/// <summary>
		/// This takes the template for a tree object and the name of a task that should be in it.
		/// It returns the XmlElement of the named task.
		/// </summary>
		/// <param name="xTemplate">The template of a tree object.</param>
		/// <param name="strTaskName">The name of a task that should be in it.</param>
		/// <returns></returns>
		private XmlElement GetXmlTask(XmlElement xTemplate, string strTaskName)
		{
			if(xTemplate != null)
			{
				XmlElement xTaskList = xTemplate["task_list"];

				if(xTaskList != null)
				{
					return XmlHelper.FindNodeByKeyValue(xTaskList.ChildNodes, "name", strTaskName) as XmlElement;
				}
			}
			return null;
		}

		private XmlElement GetXmlAction(XmlDocument xDefinitionDocument, XmlElement xTask)
		{
			string strActionName = XmlHelper.GetKeyValue(xTask, "action_name");

			XmlElement xActions = XmlHelper.GetNodeFromPath(xDefinitionDocument, "root/actions");
			XmlElement xAction = XmlHelper.FindChildNode(xActions, "action", "name", strActionName);

			if(xAction != null)
			{
				// Now we have the correct action from root/actions/...
				return xAction;
			}
			return null;
		}

		private string ExpandXmlValues(string strInput, bool bCollapseFilenames)
		{
			string strRawString = strInput;

			int indexStart = 0;
			if(strRawString != null)
			{
				Dictionary<string, string> replaceTable = new Dictionary<string, string>();

				indexStart = strRawString.IndexOf("%");
				while(indexStart != -1)
				{
					int indexEnd = strRawString.IndexOf("%", indexStart + 1);
					if(indexEnd != -1)
					{
						string strSubstitute = strRawString.Substring(indexStart + 1, indexEnd - 1 - indexStart);

						if(strSubstitute.Length > 0)
						{
							XmlElement xContainer = XmlHelper.FindNodeByKeyValue(m_ownerTreeUserObject.XTreeNodeMenuData, "name", strSubstitute) as XmlElement;
							string newValue = XmlHelper.GetKeyValue(xContainer, "value");
							if(newValue != null)
							{
								string strType = XmlHelper.GetKeyValue(xContainer, "type");
								if(strType == "filename" && bCollapseFilenames == true)
								{
									newValue = System.IO.Path.GetFileName(newValue);
								}
								replaceTable.Add(strRawString.Substring(indexStart, indexEnd - indexStart + 1), newValue);
							}
						}
					}
					indexStart = strRawString.IndexOf("%", indexEnd + 1);
				}

				foreach(string strFrom in replaceTable.Keys)
				{
					strRawString = strRawString.Replace(strFrom, replaceTable[strFrom] as string);
				}

				MacroExpander.MacroExpander mx = new MacroExpander.MacroExpander();
				mx.AddPair("%object_name%", m_ownerTreeUserObject.Text);

				mx.AddDictionary(m_ownerTreeUserObject.OwnerProjectObject._mDictVariables);

				strRawString = mx.Expand(strRawString, true);
			}
			Debug.WriteLine(strRawString);
			return strRawString;
		}

		internal bool IsStatusChangedEventEmpty()
		{
			if(this.JobStatusChangedEvent == null)
				return true;

			return false;
		}
	}
}