using System;
using System.Collections.Generic;
using System.Text;

namespace Executer
{
	/*		
	 * TODO: Make a library for this.
			private bool BuildToolbar(ProjectData projectData)
			{
				XmlElement xToolbars;

				xToolbars = XmlHelper.GetNodeFromPath(projectData.m_xMenuTemplateDocument, "root/toolbars");

				if(xToolbars == null)
				{
					return false;
				}



				foreach(XmlElement xToolbar in xToolbars)
				{
					if(xToolbar != null)
					{
						foreach(XmlElement xToolbarItem in xToolbar)
						{
							if(xToolbarItem != null)
							{

					if(xToolbar.NodeType == XmlNodeType.Element)
					{
						string strActionName = XmlHelper.GetKeyValue(xToolbar, "name");

						XmlElement xServerList = xToolbar["server_list"];

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
										if(projectData.m_localServerList.ContainsKey(strServerName))
										{
											if(serverList.Contains(strServerName))
											{
												MessageHandler.Show(this, "The action '" + strActionName + "' contains more than one server called '" + strServerName + "'", "Executer", MessageBoxButtons.OK, MessageBoxIcon.Error);
												return false;
											}
											else
												serverList.Add(strServerName);
										}
										else
											MessageHandler.ShowMemory(this, "Missing server name in ParseActions", "An action in the template refers to a server that is not defined.",
												"The action '" + strActionName + "' refers to the server '" + strServerName + "'\nThis server is not mentioned in 'root/servers' in your template.\nPlease inform your template designer.", "Executer", MessageBoxButtons.OK, MessageBoxIcon.Warning);
									}
								}
							}
						}
						if(projectData.m_actionServersLookup.ContainsKey(strActionName))
						{
							MessageHandler.Show(this, "Failed to load the project '" + projectData.m_strProjectDocumentFilename + "'\n"
							+ "The action '" + strActionName + "' is multiply defined in your template.",
							"Executer", MessageBoxButtons.OK, MessageBoxIcon.Error);

							return false;
						}
						else
							projectData.m_actionServersLookup.Add(strActionName, serverList);
					}
				}

				return true;

			}
	*/
}
