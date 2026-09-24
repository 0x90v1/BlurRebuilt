using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using Executer.Farm;
using Executer.Helpers;
using Executer.Interfaces;
using Executer.TreeObjects;
using message_handler;
using System.Diagnostics;
using System.Xml;
using Global;
using MenuSystemLibrary;
using System.Collections;

namespace Executer
{
	enum CmdLineResults
	{
		Fail = 0,
		Skip,
		Success
	};

	static class CommandLineInterface
	{
		public static bool ProcessCommandLine(ExecuterTreeView executerTreeView, String[] args)
		{
			if(args.Length != 0)
			{
				if(args[0] == "ignore")
					return true;
			}

			int count = 0;

			if(Program.ExecuterMainForm.WindowState == FormWindowState.Minimized)
				Program.ExecuterMainForm.WindowState = Program.ExecuterMainForm._lastNonMinimizedState;
			Program.ExecuterMainForm.Activate();

			string strError = "";

			string strCommandAsString = "";					// Used for error reporting.

			foreach(string str in args)
				strCommandAsString += "\"" + str + "\" ";

			while(count < args.Length)
			{
				switch(args[count])
				{
					case "-silent":							// Skip this as it was dealt with in Main.
						{
							//if(args.Length > count + 1)
							{
								count++;
							}
						}
						break;
					case "-batch_script":
						{
							if(args.Length > count + 1)
							{
								string strScriptFile = args[count + 1];		// This key must exist.
								count += 2;

								if(BatchScript(Program.ExecuterMainForm.MainTreeView, strScriptFile) == CmdLineResults.Fail)
								{
									count = args.Length;
								}
							}
							else
							{
								//SILENTShowError("ProcessComand: Expected a filename. The command line was:\n" + strCommandAsString);
								count = args.Length;
								break;
							}
							break;
						}
						
					case "-solution_task":

						// Look for a key-value pair ...
						if(args.Length > count + 2)
						{
							string strSolutionName = args[count + 1];		// This key must exist.
							string strTaskName = args[count + 2];		// The key 'value' must have this value.

							count += 3;

							//if(ProcessFile(strCommandAsString, strKey, strValue) == null)
							if(CmdLine_ProcessSolution(Program.ExecuterMainForm.MainTreeView, strSolutionName, strTaskName, ref strError) == CmdLineResults.Fail)
							{
								count = args.Length;
								return false;
							}
						}
						else
						{
							MessageHandler.Show(Program.ExecuterMainForm, "You have requested to process a task on a solution. You have not provided a task and a solution.\nCommand line form: Executer -solution_task <solution_name> <task_name>\nCommand line: " + strCommandAsString, "Command line error.", MessageBoxButtons.OK, MessageBoxIcon.Error);
							{
								count = args.Length;
								return false;
							}
						}
						break;

					case "-process_file":		// Done for v.1.0
						{
							if(args.Length > count + 1)
							{
								string strConfigFile = args[count + 1];			// This key must exist.
								count += 2;

								CmdLineResults result = CmdLine_ProcessFile(Program.ExecuterMainForm.MainTreeView, strConfigFile, ref strError);
								if(result == CmdLineResults.Fail)
								{
									count = args.Length;						// Misleading? This means the command has been aborted.
									return false;
								}
							}
							else
							{
								//SILENTShowError("ProcessComand: Expected a filename. The command line was:\n" + strCommandAsString);
								count = args.Length;
								break;
							}
							break;
						}
						// MARKER. FIXED.
					case "-p":						// Process. Done for v.1.0
						{
							// Look for a key-value pair ...
							if(args.Length > count + 2)
							{
								if(Program.ExecuterMainForm.MainTreeView.Nodes.Count == 0)
								{
									MessageHandler.Show(Program.ExecuterMainForm, "You have requested to process a key-value pair when you have no project loaded.", "Command line error.", MessageBoxButtons.OK, MessageBoxIcon.Error);
									count = args.Length;
								}
								else
								{
									string strKey = args[count + 1];		// This key must exist.
									string strValue = args[count + 2];		// The key 'value' must have this value.

									count += 3;

									if(ProcessFile(strCommandAsString, strKey, strValue) == null)
									{
										count = args.Length;
										return false;
									}
								}
							}
							else
							{
								MessageHandler.Show(Program.ExecuterMainForm, "You have requested to process a key-value pair. You have not provided a key-value pair.\nCommand line: " + strCommandAsString, "Command line error.", MessageBoxButtons.OK, MessageBoxIcon.Error);
								{
									count = args.Length;
									return false;
								}
							}
							break;
						}
					case "-g":						// Goto.
						{
							// Look for a key-value pair ...
							if(args.Length > count + 2)
							{
								if(Program.ExecuterMainForm.MainTreeView.Nodes.Count == 0)
								{
									MessageHandler.Show(Program.ExecuterMainForm, "You have requested to find a key-value pair when you have no project loaded.", "Command line error.", MessageBoxButtons.OK, MessageBoxIcon.Error);
									count = args.Length;
								}
								else
								{
									string strValue1 = args[count + 1];		// This key must exist.
									string strValue2 = args[count + 2];		// The key 'value' must have this value.
									count += 3;
#region new
									List<TreeUserObject> results = ExecuterTreeUtilities.GetAllUserObjects(Program.ExecuterMainForm.MainTreeView.Nodes);

									string strPathQuery = "values//container[@name='" + strValue1 + "' and @value='" + strValue2 + "']";

									TreeUserObject tuo = null;
									
									foreach(TreeUserObject treeUserObject in results)
									{
										if(treeUserObject.XTreeNodeMenuData.SelectSingleNode(strPathQuery) != null)
										{
											tuo = treeUserObject;
											break;
										}
									}

									//TreeUserObject tuo = ExecuterTreeUtilities.GetCustomMatchedChild(Program.S_executerMainForm.m_executerTreeView.Nodes, strKey, strValue, true);

									if(tuo != null)
									{
										Program.ExecuterMainForm.MainTreeView.SelectedNode = tuo;
									}
#endregion new
									else
									{
										UtilityFunctions.ShowError(Program.ExecuterMainForm, "ProcessComand: Could not match a tree node. The command line was:\n" + strCommandAsString);
										count = args.Length;
									}
								}
							}
							else
							{
								MessageHandler.Show(Program.ExecuterMainForm, "You have requested to find a key-value pair. You have not provided a key-value pair.\nCommand line: " + strCommandAsString, "Command line error.", MessageBoxButtons.OK, MessageBoxIcon.Error);
								count = args.Length;
							}
							break;
						}
					case "-template":
						{
							if(args.Length > count + 1)
							{
								string strTemplateFilename = args[count + 1];

								Program.ExecuterMainForm.MainTreeView.CreateAndAddNewProject(strTemplateFilename, null, null);

								count += 2;
							}
							break;
						}

					case "-l":		// Clear existing projects then load a new one.
							if(args.Length > count + 1)
							{
								if(Program.m_sSilentCommandLine == false)
								{
									PromptSave promptSave = new PromptSave(Program.ExecuterMainForm.MainTreeView, !ExecuterMainForm.bDisallowCancelExitDialog, true);

									if(promptSave.NumEntries != 0)
									{
										DialogResult dr = promptSave.ShowDialog();

										if(dr == DialogResult.Cancel)
										{
											count = args.Length;				// Misleading? This means the command has been aborted.
											break;
										}
									}
								}
								Program.ExecuterMainForm.MainTreeView.Nodes.Clear();
							}
						goto case "-addproject";

					case "-addproject":						// Load a project into a new solution.
						{

							if(args.Length > count + 1)
							{
								// TODO: SLN: Add to Solution.

								TreeProjectObject tpo = Program.ExecuterMainForm.MainTreeView.AddOrFindProject(null, args[count + 1]);

								count += 2;
							}
							else
							{
								UtilityFunctions.ShowError(Program.ExecuterMainForm, "ProcessComand: Expected a filename. The command line was:\n" + strCommandAsString);
								count = args.Length;
							}
							break;
						}
					case "-a":						// Abort.
						{

							//							return true;
							//							DialogResult dr = MessageHandler.Show("DEBUG: Command line says quit. Comply?", "Menu", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
							//							if(dr == DialogResult.Yes)
							WaitForQueuedJobsToBeSentToTheFarm(null, true);

							Program.ExecuterMainForm.bQuitMeansQuitAndNotMinimise = true;
							Program.s_ExitCode = 1;
							ExecuterMainForm.bDisallowCancelExitDialog = true;

							Program.s_farmClientManager.ShutdownServers(true);
							Program.ExecuterMainForm.Close();
							count = args.Length;
							return false;

							//							count = args.Length;
						}
					default:
						{
							MessageHandler.Show(Program.ExecuterMainForm, "The command line switch '" + args[count] + "' is unrecognised.", "Command line error.", MessageBoxButtons.OK, MessageBoxIcon.Error);
							count = args.Length;
							break;
						}
				}
			}
			return true;
		}

		private static TreeUserObject ProcessFile(string strCommandAsString, string strKey, string strValue)
		{
			bool bNewlyCreatedTreeNode = false;
			
			List<TreeUserObject> results = ExecuterTreeUtilities.GetAllUserObjects(Program.ExecuterMainForm.MainTreeView.Nodes);

			string strPathQuery = "values//container[@name='"+strKey+"' and @value='"+strValue+"']";

			TreeUserObject tuo = null;
			
			foreach(TreeUserObject treeUserObject in results)
			{
				if(treeUserObject.XTreeNodeMenuData.SelectSingleNode(strPathQuery) != null)
				{
					tuo = treeUserObject;
					break;
				}
			}

			//TreeUserObject tuo = ExecuterTreeUtilities.GetCustomMatchedChild(Program.S_executerMainForm.m_executerTreeView.Nodes, strKey, strValue, true);


			if(tuo == null)							// If there is no child, make one.
			{
				DialogResult dr = MessageHandler.ShowMemory(Program.ExecuterMainForm, "CmdLineCreateUnknownType", "Auto create unknown types from command line?", "Auto-create new tree node?", "Executer", MessageBoxButtons.YesNo, MessageBoxIcon.Question);

				if(dr != DialogResult.Yes)
					return null;

				tuo = ExecuterTreeUtilities.CustomMakeChild(Program.ExecuterMainForm.MainTreeView, strKey, strValue);

				if(tuo == null)
				{
					UtilityFunctions.ShowError(Program.ExecuterMainForm, "ProcessCommand: Cannot determine the correct 'type' to create.\n'" + strKey + "' is not found in a type in the template.\ncommand line was:\n" + strCommandAsString);
					return null;
				}

				bNewlyCreatedTreeNode = true;
			}

			if(tuo != null)
			{
				Debug.Assert(Program.ExecuterMainForm.MainTreeView != null, "Tree manager is null in 'ProcessCommandLine'.");
				Program.ExecuterMainForm.MainTreeView.SelectedNode = tuo;

				/////////////////////////////////////////
				//Test this test this test this
				CommandDetails cd = tuo.GetTaskByName(null, null);

				if(cd != null)
				{
					if(cd.m_strServerName != null)
					{
						if(bNewlyCreatedTreeNode == true)
						{
							DialogResult dr = MessageHandler.ShowMemory(Program.ExecuterMainForm, "RunNewCommandLineNode", "Process newly created command line nodes?", "Process newly created type?", "Executer", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
							if(dr != DialogResult.Yes)
								return tuo;			// We do not wish to quit the app. just because the new nodes aren't processed.
						}

						FarmClient se = Program.s_farmClientManager.FindClientByLocalName(cd.m_strServerName);

						if(cd.m_strServerName != "immediate")
						{
							if(WaitForConnectionToServer(se, true) == true)
							{
								try
								{
									cd.m_treeUserObject.DoTask(cd.m_strTaskName, cd.m_strServerName, cd.m_bRecurse);
								}
								catch(Exception ex)
								{
									MessageHandler.Show(Program.ExecuterMainForm, "'ProcessCommand' failed.\nSystem reports: " + ex.Message, "Executer", MessageBoxButtons.OK, MessageBoxIcon.Error);
								}
							}
							else
								UtilityFunctions.ShowError(Program.ExecuterMainForm, "Cannot connect to the chosen server and therefore cannot process the command.\nThe server name is '" + cd.m_strServerName + "'");
						}
						else
						{
							try
							{
								cd.m_treeUserObject.DoTask(cd.m_strTaskName, cd.m_strServerName, cd.m_bRecurse);
							}
							catch(Exception ex)
							{
								MessageHandler.Show(Program.ExecuterMainForm, "'ProcessCommand' failed.\nSystem reports: " + ex.Message, "Executer", MessageBoxButtons.OK, MessageBoxIcon.Error);
							}
						}

					}
					else
					{
						UtilityFunctions.ShowError(Program.ExecuterMainForm, "Your template does not define a default server to execute this command on.\n(Default server is the first server in the list.)");
					}
				}
				else
				{
					UtilityFunctions.ShowError(Program.ExecuterMainForm, "The default command to execute on this node could not be built. Your template is probably wrong.");
				}
				return tuo;
			}
			else
			{
				UtilityFunctions.ShowError(Program.ExecuterMainForm, "ProcessCommand: Could not match a tree node. The command line was:\n" + strCommandAsString);
			}
			return null;
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
				}
				while(queuedJobs != 0 && d != 0);
			}
			if(simpleMessageBox != null)
				simpleMessageBox.Close();

			return (queuedJobs == 0);
		}

		private static CmdLineResults BatchScript(ExecuterTreeView executerTreeView, string strScriptFile)
		{
			//string strUniqueFile = Path.GetTempFileName() + ".bat";

			throw new NotImplementedException("static CmdLineResults BatchScript(...) is not implemented.");

//            XmlDocument docScript = new XmlDocument();

//            try
//            {
//                docScript.Load(strScriptFile);

//                XmlElement xTasks = XmlHelper.GetNodeFromPath(docScript, "batch_scripts/tasks");
//                if(xTasks == null)
//                    throw new ApplicationException("The script file" + strScriptFile + "' does not have the element 'batch_scripts/tasks'");

//                foreach(XmlNode xNode in xTasks)
//                {
//                    if(xNode is XmlElement)
//                    {
//                        XmlElement xTask = xNode as XmlElement;

//                        switch(xTask.Name)
//                        {
//                            case "update_and_process":
//                                {
//                                    string strKey = XmlHelper.GetKeyValue(xTask, "key");
//                                    if(strKey == null)
//                                        throw new ApplicationException("The task\n'" + xTask.OuterXml.ToString() + "'\ndoes not have the attribute 'key'");

//                                    string strValue = XmlHelper.GetKeyValue(xTask, "value");
//                                    if(strValue == null)
//                                        throw new ApplicationException("The task\n'" + xTask.OuterXml.ToString() + "'\ndoes not have the attribute 'value'");
//                                    string strType = XmlHelper.GetKeyValue(xTask, "type");
//                                    if(strType == null)
//                                        throw new ApplicationException("The task\n'" + xTask.OuterXml.ToString() + "'\ndoes not have the required type.");

//                                    string strTaskName = XmlHelper.GetKeyValue(xTask, "task_name");
//                                    string strServerName = XmlHelper.GetKeyValue(xTask, "server_name");
//                                    string strTemplateFilename = XmlHelper.GetKeyValue(xTask, "template_filename");
//                                    string strParentType = XmlHelper.GetKeyValue(xTask, "parent_type");
//                                    string strParentName = XmlHelper.GetKeyValue(xTask, "parent_name");

//                                    TreeUserObject tuoOwnerProject = ExecuterTreeUtilities.FindProjectByTemplate(executerTreeView, strTemplateFilename);
//                                    if(tuoOwnerProject == null)
//                                        throw new ApplicationException("The task\n'" + xTask.OuterXml.ToString() + "'\ndoes not specify a template that is in use by any projects that are loaded.");

//                                    XmlDocument xTemplateDoc = tuoOwnerProject.OwnerProjectObject.m_xMenuTemplateDocument;

//                                    Dictionary<string, string> keyValues = new Dictionary<string, string>();
//                                    keyValues.Add("name", strKey);
//                                    keyValues.Add("value", strValue);

//                                    TreeUserObject tuo = ExecuterTreeUtilities.GetCustomMatchedChild(tuoOwnerProject.Nodes, keyValues, strType, null, true);

//                                    if(tuo == null)			// Create one ...
//                                    {
//                                        TreeUserObject tuoParent = ExecuterTreeUtilities.FindTreeObjectByTypeAndName(executerTreeView, strTemplateFilename, strParentType, strParentName, false);

//                                        if(tuoParent == null) // We have to create the parent.
//                                        {
//                                            XmlElement xParentType = ExecuterTreeUtilities.GetNamedTypeFromTemplate(xTemplateDoc, strParentType);
//                                            if(xParentType == null)
//                                                throw new ApplicationException("The template defined by task\n'" + xTask.OuterXml.ToString() + "'\ndoes not contain a definition for the parent_type.");

//                                            //toParent = m_executerTreeView.AddTreeType(toOwnerProject, xParentType, null);
//                                            tuoParent = tuoOwnerProject.AddTreeType(tuoOwnerProject, xParentType, null);

//                                            tuoParent.Text = strParentName;

//                                            XmlHelper.AddAttribute(tuoParent.XTreeNodeMenuData, "name", tuoParent.Text);
//                                            tuoParent.OwnerProjectObject.IsDirty = true;
//                                        }

//                                        XmlElement xType = ExecuterTreeUtilities.GetNamedTypeFromTemplate(xTemplateDoc, strType);

//                                        tuo = tuoParent.AddTreeType(tuoParent, xType, System.IO.Path.GetFileNameWithoutExtension(strValue));

//                                        tuo.OwnerProjectObject.IsDirty = true;

//                                        // Now find the key in this treeObjects data and set it to the correct value.
//                                        List<XmlElement> results = XmlHelper.FindElementsByKeyValue(tuo.XTreeNodeMenuData, "name", strKey, true, true, true);
//                                        if(results.Count > 0)
//                                        {
//                                            XmlElement xElement = results[0] as XmlElement;
//                                            XmlHelper.AddAttribute(xElement, "value", strValue);
//                                        }
//                                    }

//                                    // Now we have the tree object to perform the task on.


//#if REFACTOR						// TODO: Do this instead ...
//                                    FarmClient farmClient = Program.m_serverManager.FindServerByLocalName(strServerName);
//                                    if(strServerName != "immediate")
//                                    {
//                                        if(WaitForConnectionToServer(farmClient, true) == false)
//                                            throw new ApplicationException("Cannot connect to the chosen server and therefore cannot process the command.\nThe server name is '" + cd.m_strServerName + "'");
//                                    }

//                                    try
//                                    {
//                                        tuo.DoTask(strTaskName, strServerName, bRecurse);
//                                    }
//                                    catch(Exception ex)
//                                    {
//                                        MessageHandler.Show(this, "'ProcessCommand' failed.\nSystem reports: " + ex.Message, "Executer", MessageBoxButtons.OK, MessageBoxIcon.Error);
//                                    }
//                                    bool bTimedOut = WaitForQueuedJobsToBeSentToTheFarm(null, true);

//                                    if(bTimedOut == false)
//                                        return CmdLineResults.Fail;
//#else
//                                    CommandDetails cd = tuo.GetTaskByName(strTaskName, strServerName);

//                                    if(cd != null)
//                                    {
//                                        FarmClient se = Program.s_farmClientManager.FindClientByLocalName(cd.m_strServerName);

//                                        if(cd.m_strServerName != "immediate")
//                                        {
//                                            if(WaitForConnectionToServer(se, true) == false)
//                                                throw new ApplicationException("Cannot connect to the chosen server and therefore cannot process the command.\nThe server name is '" + cd.m_strServerName + "'");
//                                        }
//                                        try
//                                        {
//                                            cd.m_treeUserObject.DoTask(cd.m_strTaskName, cd.m_strServerName, cd.m_bRecurse);
//                                        }
//                                        catch(Exception ex)
//                                        {
//                                            MessageHandler.Show(null, "'ProcessCommand' failed.\nSystem reports: " + ex.Message, "Executer", MessageBoxButtons.OK, MessageBoxIcon.Error);
//                                        }

//                                        if(cd.m_strServerName != "immediate")
//                                        {
//                                            bool bTimedOut = WaitForQueuedJobsToBeSentToTheFarm(se, true);

//                                            if(bTimedOut == false)
//                                                return CmdLineResults.Fail;
//                                        }
//#endif
//                                    }
//                                    else
//                                    {
//                                        throw new ApplicationException("The task defined in\n'" + xTask.OuterXml.ToString() + "'\nFailed. (task_name or server_name are not found.).");
//                                    }
//                                }
//                                break;

//                            default:
//                                {
//                                }
//                                break;
//                        }

//                    }
//                }
//                return CmdLineResults.Success;
			//}
			//catch(Exception ex)
			//{
			//    MessageHandler.Show(null, "Batch script process failed. System reports:\n'" + ex.Message + "'", "Executer", MessageBoxButtons.OK);
			//    return CmdLineResults.Fail;
			//}
		}



		private static CmdLineResults CmdLine_ProcessSolution(ExecuterTreeView executerTreeView, string strSolutionName, string strTaskName, ref string strError)
		{
			//TreeSolutionObject tso = new TreeSolutionObject(strSolutionName, executerTreeView);
			//executerTreeView.Nodes.Add(tso);
			//tso.ImageKey = "TreeSolutionIcon";
			//tso.SelectedImageKey = "TreeSolutionIcon";

			TreeSolutionObject tso = TreeSolutionObject.AddOrFindSolution(strSolutionName);

			tso.DoTask(strTaskName);

			return CmdLineResults.Success;
		}



		private static CmdLineResults CmdLine_ProcessFile(ExecuterTreeView executerTreeView, string strConfigFile, ref string strError)
		{
			XmlElement xMenuDataBackup = null;
			TreeBaseObject tbo = null;
			TreeUserObject tuo = null;
			bool bReplaceOverrides = true;

			try
			{
				XmlDocument xDoc = new XmlDocument();
				xDoc.Load(strConfigFile);

				//XmlElement xOptions = xDoc["root"]["options"];

				XmlNodeList xOptionsList = xDoc.SelectNodes("root/options");

				if(xOptionsList != null)
				{
					foreach(XmlElement xOptions in xOptionsList)
					{
						try
						{

							if(xOptions == null)
							{
								MessageHandler.Show(executerTreeView, "Problem processing file. Cannot find element 'root/options' in the file '" + strConfigFile + "'", "Executer", MessageBoxButtons.OK);
								return CmdLineResults.Fail;
							}

							string strProjectName = XmlHelper.GetKeyValue(xOptions, "project_name");
							string strTreeNodePath = XmlHelper.GetKeyValue(xOptions, "tree_node_path");
							string strAddNodeName = XmlHelper.GetKeyValue(xOptions, "add_node_name");
							string strAddNodeType = XmlHelper.GetKeyValue(xOptions, "add_node_type");
							string strTaskName = XmlHelper.GetKeyValue(xOptions, "task_name");
							string strServerLocalName = XmlHelper.GetKeyValue(xOptions, "server_local_name");
							bool bClearProjects = XmlHelper.GetKeyBool(xOptions, "close_existing_projects", false);

							bool bRecurse = XmlHelper.GetKeyBool(xOptions, "recurse", false);

							bool bPersistOverrides = XmlHelper.GetKeyBool(xOptions, "persist_overrides", false);

							string strFullTreeNodePath = strTreeNodePath;

							if(strProjectName == null)
							{
								MessageHandler.Show(executerTreeView, "The key 'project_name' is not found in 'root/options' in the config file '" + strConfigFile + "'", "Executer", MessageBoxButtons.OK);
								break;// return CmdLineResults.Fail;
							}
							if(strTreeNodePath == null)
							{
								MessageHandler.Show(executerTreeView, "The key 'tree_node_path' is not found in 'root/options' in the config file '" + strConfigFile + "'", "Executer", MessageBoxButtons.OK);
								break;// return CmdLineResults.Fail;
							}
							//if(strTaskName == null)
							//{
							//    MessageHandler.Show(this, "The key 'task_name' is not found in 'root/options' in the config file '" + strConfigFile + "'", "Executer", MessageBoxButtons.OK);
							//    break;// return CmdLineResults.Fail;
							//}

							//if(strServerLocalName == null)
							//{
							//    MessageHandler.Show(this, "The key 'server_local_name' is not found in 'root/options' in the config file '" + strConfigFile + "'", "Executer", MessageBoxButtons.OK);
							//    return false;
							//}

							if(bClearProjects == true)
							{
								if(Program.m_sSilentCommandLine == false)
								{
									PromptSave promptSave = new PromptSave(executerTreeView, !ExecuterMainForm.bDisallowCancelExitDialog, true);

									if(promptSave.NumEntries != 0)
									{
										DialogResult dr = promptSave.ShowDialog();

										if(dr == DialogResult.Cancel)
										{
											return CmdLineResults.Fail;
										}
									}
								}
								executerTreeView.Nodes.Clear();
							}

							List<TreeProjectObject> projects = ExecuterTreeUtilities.FindMatchingProjects(executerTreeView, strProjectName);

							TreeProjectObject projectNode = projects.Count > 0 ? projects[0] : null;

							if(projectNode == null)
							{
								// TODO: SLN: Add to Solution.
								projectNode = executerTreeView.AddOrFindProject(null, strProjectName);

								if(projectNode == null)
								{
									MessageHandler.Show(executerTreeView, "Cannot open the project '" + strProjectName + "'", "Executer", MessageBoxButtons.OK);
									break;// return CmdLineResults.Fail;
								}
							}

							//TreeUserObject tuoRoot = projectNode.Nodes[0] as TreeUserObject;
							TreeBaseObject tboRoot = projectNode as TreeBaseObject;

							if(tboRoot != null)
							{
								//tbo = ExecuterTreeView.GetTreeNodeFromPath(tboRoot, strTreeNodePath) as TreeBaseObject;
								tbo = executerTreeView.GetTreeNodeFromPathAndType(projectNode.Parent as ITreeNodeContainer, strTreeNodePath) as TreeBaseObject;
							}
							if(tbo == null)
							{
								MessageHandler.Show(executerTreeView, "The tree node '" + strTreeNodePath + "' does not exist in the project '" + strProjectName + "'", "Executer", MessageBoxButtons.OK);
								break;// return CmdLineResults.Fail;
							}

							tuo = (tbo is TreeUserObject) ? tbo as TreeUserObject: (tbo as TreeProjectObject)._rootUserObject;
							
							executerTreeView.SelectedNode = tuo;

							bool bNodeHasBeenCreated = false;
							bool bOverrideHasBeenApplied = false;

							if(strAddNodeType != null)
							{
								// Make a tree object of type 'strAddNodeType' and name it 'strAddNodeName'.

								XmlElement xType = tuo.OwnerProjectObject.GetTemplateTypeFromTypeName(strAddNodeType);

								if(xType != null)
								{
									// See if there's already one there with the right name and the right type ...

									//TreeBaseObject toExisting = executerTreeView.GetTreeNodeFromPathAndType(tboRoot, strTreeNodePath) as TreeUserObject;
									TreeBaseObject toExisting = executerTreeView.GetTreeNodeFromPathAndType(tbo, strAddNodeName+"|"+strAddNodeType) as TreeUserObject;

									//TreeBaseObject toExisting = executerTreeView.GetTreeNodeFromPathAndType(tboRoot, strTreeNodePath, strAddNodeName, strAddNodeType) as TreeUserObject;

									if(toExisting != null)
									{
										TreeUserObject tuoExisting = (toExisting is TreeUserObject) ? toExisting as TreeUserObject : (toExisting as TreeProjectObject)._rootUserObject;
										tuo = tuoExisting;
									}
									else
									{
										// When a TreeUserObject is created, it validates itself against its template.
										tuo = tuo.AddTreeType(xType, strAddNodeName);
									}
									strFullTreeNodePath += ("\\" + strAddNodeName);
									//???to.m_projectData.m_bProjectDocumentChanged = true;								???
								}
								else
								{
									UtilityFunctions.ShowError(executerTreeView, "Cannot create a node of type '" + strAddNodeName + "' because it does not exist in the template.");
									break;// return CmdLineResults.Fail;
								}

								if(tuo == null)
								{
									UtilityFunctions.ShowError(executerTreeView, "Cannot create a node of type '" + strAddNodeName + "' because 'AddTreeType' failed. Please inform the tools team.");
									break;// return CmdLineResults.Fail;
								}

								bNodeHasBeenCreated = true;
							}

							#region Apply overrides

							xMenuDataBackup = tuo.XTreeNodeMenuData["values"].CloneNode(true) as XmlElement;

							//XmlNodeList xOverrides = xDoc.SelectNodes("root/overrides/override");
							XmlNodeList xOverrides = xOptions.SelectNodes("overrides/override");
							foreach(XmlElement xOverride in xOverrides)
							{
								string strName = XmlHelper.GetKeyValue(xOverride, "name");
								string strValue = XmlHelper.GetKeyValue(xOverride, "value");

								if(strName != null && strValue != null)
								{
									XmlElement xElement = XmlHelper.FindNodeByKeyValue(tuo.XTreeNodeMenuData["values"].ChildNodes, "name", strName, true) as XmlElement;
									if(xElement != null)
									{
										XmlHelper.AddAttribute(xElement, "value", strValue);
										bOverrideHasBeenApplied = true;
									}
								}
							}

							#endregion

							// If we've made a new node or if overrides should persist then save the changes.
							if((bNodeHasBeenCreated == true) || ((bOverrideHasBeenApplied == true) && (bPersistOverrides == true)))
							{
								bReplaceOverrides = false;
							}
							if(strTaskName != null)
							{
								CommandDetails cd = tuo.GetTaskByName(strTaskName, strServerLocalName);

								if(cd != null)
								{
									cd.m_bRecurse = bRecurse;
									if(cd.m_strServerName != null)
									{
										FarmClient se = Program.s_farmClientManager.FindClientByLocalName(cd.m_strServerName);

										// Since we've just started up, the farm manager may not have connected to
										// the required server yet, so give it a chance.
										bool bSuccess = WaitForConnectionToServer(se, true);

										if(bSuccess == true)
										{
											try
											{// ATTENTION ATTENTION ATTENTION ATTENTION ATTENTION ATTENTION ATTENTION ATTENTION ATTENTION ATTENTION 
												cd.m_treeUserObject.DoTask(cd.m_strTaskName, cd.m_strServerName, cd.m_bRecurse);


												//to.m_xTreeNodeMenuData.ReplaceChild(xMenuDataBackup, to.m_xTreeNodeMenuData["values"]);

												bool bWaitSucceeded = WaitForQueuedJobsToBeSentToTheFarm(se, true);

												if(bWaitSucceeded == false)
													MessageHandler.Show(executerTreeView, "The task '" + strTaskName + "' on node '" + strFullTreeNodePath + "' could not be sent to the farm.", "Executer", MessageBoxButtons.OK, MessageBoxIcon.Error);
												//if(bWaitSucceeded == true)
												//    return CmdLineResults.Success;
												//else
												//    return CmdLineResults.Fail;
											}
											catch(Exception ex)
											{
												MessageHandler.Show(executerTreeView, "'ProcessCommand' failed.\nSystem reports: " + ex.Message, "Executer", MessageBoxButtons.OK, MessageBoxIcon.Error);
											}
											//return CmdLineResults.Fail;
										}
										else
										{
											UtilityFunctions.ShowError(executerTreeView, "Your template does not define a valid server to execute this command on.\n(Leave it out to use the first server in the list.)");
										}
									}
									else
									{
										UtilityFunctions.ShowError(executerTreeView, "Your template does not define a valid server to execute this command on.\n(Leave it out to use the first server in the list.)");
									}
								}
								else
								{
									UtilityFunctions.ShowError(executerTreeView, "The default command to execute on this node could not be built. Your template is probably wrong.");
								}
							}
						}


						catch(Exception ex)
						{

							MessageHandler.Show(executerTreeView, "Problem processing file. System reports:\n" + ex.Message, "Executer", MessageBoxButtons.OK);
							//return CmdLineResults.Fail;
						}
						finally
						{
							if(bReplaceOverrides == true)
							{
								if(tuo != null && xMenuDataBackup != null)
								{
									tuo.XTreeNodeMenuData.ReplaceChild(xMenuDataBackup, tuo.XTreeNodeMenuData["values"]);
								}
							}
						}
					}
				}
			}
			catch(Exception ex)
			{
				Debug.WriteLine(ex.Message);
			}

			return CmdLineResults.Success;
		}

		/// <summary>
		/// Waits until a connection to the specified server is established.
		/// </summary>
		/// <param name="se">The server entry of the server we want to connect to.</param>
		/// <param name="bShowProgress">Whether or not to show a dialog to the user explaining what is going on.</param>
		/// <returns>false if the connection cannot be established within a timeout period, otherwise true.</returns>
		private static bool WaitForConnectionToServer(FarmClient se, bool bShowProgress)
		{
			if(se != null)
			{
				if(se.Connected != true)
				{
					SimpleMessageBox smb = null;
					if(bShowProgress == true)
					{
						smb = new SimpleMessageBox("Waiting for server connection .", "Executer");
						smb.Show(null);
					}

					int c = 30;

					while(se.Connected == false)
					{
						smb.LabelText += ".";
						//smb.Update();
						//for(int e = 0; e < 1000; e++)
						//{
						//    Program.S_executerMainForm.Update();
						//    System.Threading.Thread.Sleep(1);
						//}
						Console.WriteLine("ProcessCommandLine is waiting for a server to appear.");
						System.Threading.Thread.Sleep(500);
						if(--c == 0)
							break;
					}
					if(bShowProgress == true)
						smb.Close();

					return se.Connected;
				}
				return true;
			}
			return false;
		}

		//private void DebugWindowWrite(string strMessage)
		//{
		//    if(Program.s_bDebugModeEnabled == true)
		//    {
		//        DebugWindow.WriteToDebugWindow(strMessage);
		//    }
		//}

#if thingy
		public static CommandLineResult ProcessCommandFile(string strXmlCommandFile)
		{
			throw new Exception("Not implemented.");
		}

		static bool LoadLastConfig()
		{
			throw new Exception("Not implemented.");
		}

		/// <summary>
		/// Loads a given solution.
		/// </summary>
		/// <param name="strSolutionName">The filename of the solution to load.</param>
		static TreeSolutionObject LoadSolution(string strSolutionName)
		{
			throw new Exception("Not implemented.");
		}

		static TreeProjectObject LoadProject(string strSolutionName, string strProjectName)
		{
			throw new Exception("Not implemented.");
		}
#endif
	}
}
