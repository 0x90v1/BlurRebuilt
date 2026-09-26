using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using System.Diagnostics;
using Executer.Helpers;
using Executer.Interfaces;
using MenuSystemLibrary;
using Global;

namespace Executer
{
	public static class ExecuterTreeUtilities
	{
		/// <summary>
		/// Returns a list of any open projects across all solutions that match the given filename.
		/// </summary>
		/// <param name="strProjectDocumentFilename">The path and filename of the projects to search for.</param>
		/// <returns>A list of matching projects, or an empty list.</returns>
		public static List<TreeProjectObject>FindMatchingProjects(ExecuterTreeView executerTreeView, string strProjectDocumentFilename)
		{
			List<TreeProjectObject> projects = new List<TreeProjectObject>();

			if(executerTreeView != null && strProjectDocumentFilename != null)
			{

				if(executerTreeView.Nodes.Count != 0)
				{
					TreeNodeCollection tnc = executerTreeView.Nodes;

					foreach(TreeSolutionObject tso in tnc)
					{
						projects.AddRange(tso.FindMatchingProjects(strProjectDocumentFilename));
					}
				}
			}
			return projects;
		}


		/// <summary> This takes the name of a container and a value it should be set to. It then searches all available templates (owned by open projects)
		/// to find the first 'type' definition that has a container with a matching name.
		/// It then creates a new tree object of the 'type', and adds it to the root of the project that owns the template it is made from.
		/// </summary>
		/// <param name="strName">The 'name' of a container. Used as a key in a search of templates.</param>
		/// <param name="strValue">The value the new tree object should have.</param>
		/// <returns>The new tree object, or null if one cannot be created.</returns>
		public static TreeUserObject CustomMakeChild(ExecuterTreeView executerTreeView, string strName, string strValue)
		{
			// 1) Decide what to add and which project to add it to.
			// - Do this by looking for the key in each template. Use the first match.

			Debug.Assert(strName != null);
			Debug.Assert(strValue != null);

			//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

			List<TreeProjectObject> treeProjectObjects;

			treeProjectObjects = GetAllProjectObjects(executerTreeView.Nodes);
			// get the 'type' entry in the template for a type that has the properties @name=strName.

			TreeProjectObject treeProjectObject = null;

			XmlElement xContainer = null;

			foreach(TreeProjectObject tpo in treeProjectObjects)
			{
				XmlDocument xTemplateDoc = tpo.XMenuTemplateDocument;

				xContainer = xTemplateDoc.SelectSingleNode("root/types//type/sub_menu_template/menu_system//container[@name='" + strName + "']") as XmlElement;

				if(xContainer != null)
				{
					treeProjectObject = tpo;
					break;
				}
			}

			if(xContainer != null)
			{
				XmlElement xType = xContainer.SelectSingleNode("ancestor::type") as XmlElement;

				Debug.Assert(xType != null);

				if(xType != null)
				{
					// What do I add it to? ...
					TreeUserObject treeUserRootObject = treeProjectObject.Nodes[0] as TreeUserObject;

					Debug.Assert(treeUserRootObject != null);

					TreeUserObject tuo = treeUserRootObject.AddTreeType(xType, System.IO.Path.GetFileNameWithoutExtension(strValue));
					tuo.OwnerProjectObject.IsDirty = true;

					XmlElement xMyContainer = tuo.XTreeNodeMenuData.SelectSingleNode(".//container[@name='" + strName + "']") as XmlElement;

					if(xMyContainer != null)
					{
						XmlHelper.AddAttribute(xMyContainer, "value", strValue);
					}

					return tuo;
				}
				else
				{
					//XXX					ShowError("ProcessCommand: Cannot determine the correct 'type' to create.\n'" + strName + "' is not found in a type in the template.\ncommand line was:\n" + strCommandAsString);
				}
			}

			//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

			return null;
		}

		// MOVE
		public static XmlElement GetNamedTypeFromTemplate(XmlDocument xTemplateDoc, string strType)
		{
			XmlElement xTypes = xTemplateDoc.SelectSingleNode("root/types") as XmlElement;
			if(xTypes != null)
			{
				foreach(XmlNode xNode in xTypes)
				{
					if(xNode.NodeType == XmlNodeType.Element)
					{
						XmlElement xType = xNode as XmlElement;

						if(xType.Name == "type" && XmlHelper.GetKeyValue(xType, "name") == strType)
							return xType;
					}
				}
			}
			return null;
		}

		#region GetAllUserObjects

		class ActionUserArgs
		{
			public readonly List<TreeUserObject> TreeUserObjects = new List<TreeUserObject>();
		}

		internal static List<TreeUserObject> GetAllUserObjects(TreeNodeCollection rootCollection)
		{
			ActionUserArgs actionArgs = new ActionUserArgs();

			UtilityFunctions.EnumerableRecurse<ActionUserArgs>(rootCollection, new UtilityFunctions.ActionDelegate<ActionUserArgs>(RecurseActionGetAllUserObjects), ref actionArgs);

			return actionArgs.TreeUserObjects;
		}

		static bool RecurseActionGetAllUserObjects(object thing, ActionUserArgs actionArgs)
		{
			if(thing is TreeUserObject)
				actionArgs.TreeUserObjects.Add(thing as TreeUserObject);

			return true;
		}

		#endregion

		#region GetAllProjectObjects

		class ActionProjectArgs
		{
			public List<TreeProjectObject> m_treeUserObjects = new List<TreeProjectObject>();
		}

		internal static List<TreeProjectObject> GetAllProjectObjects(TreeNodeCollection rootCollection)
		{
			ActionProjectArgs actionArgs = new ActionProjectArgs();

			UtilityFunctions.EnumerableRecurse<ActionProjectArgs>(rootCollection, new UtilityFunctions.ActionDelegate<ActionProjectArgs>(RecurseActionGetAllProjectObjects), ref actionArgs);

			return actionArgs.m_treeUserObjects;
		}

		static bool RecurseActionGetAllProjectObjects(object thing, ActionProjectArgs actionArgs)
		{
			if(thing is TreeProjectObject)
				actionArgs.m_treeUserObjects.Add(thing as TreeProjectObject);

			return true;
		}
		#endregion

		#region Sort tree children

		#endregion

		#region GetNextTreeNode

		public static ITreeNodeContainer GetNextTreeNode(ITreeNodeContainer rootTreeNode, ITreeNodeContainer currentTreeNode, bool bWrapAtEnd)
		{
			if(currentTreeNode.ChildNodes.Count > 0)
				return currentTreeNode.ChildNodes[0] as ITreeNodeContainer;

			if(currentTreeNode == rootTreeNode)
				return rootTreeNode;


			if(currentTreeNode.NextNodeContainer != null)
				return currentTreeNode.NextNodeContainer;


			// Sanity. No need to test (currentTreeNode.Parent != null) unless rootTreeNode is neither null or a parent of currentTreeNode.
			while(currentTreeNode.ParentContainer != null && currentTreeNode.ParentContainer != rootTreeNode)
			{
				currentTreeNode = currentTreeNode.ParentContainer;
				if(currentTreeNode.NextNodeContainer != null)
					return currentTreeNode.NextNodeContainer;
			}
			if(bWrapAtEnd == true)
				return rootTreeNode;

			return null;
		}


		public static ITreeNodeContainer GetNextVisibleTreeNode(ITreeNodeContainer rootTreeNode, ITreeNodeContainer currentTreeNode)
		{

			TreeNode tn = currentTreeNode as TreeNode;		// tn can be null if the root container is the tree itself.

			if(currentTreeNode.ChildNodes.Count > 0 && (tn == null || tn.IsExpanded == true))
				return currentTreeNode.ChildNodes[0] as ITreeNodeContainer;

			//if(currentTreeNode == rootTreeNode)
			//    return rootTreeNode;


			if(currentTreeNode.NextNodeContainer != null)
				return currentTreeNode.NextNodeContainer;


			// Sanity. No need to test (currentTreeNode.Parent != null) unless rootTreeNode is neither null or a parent of currentTreeNode.
			while(currentTreeNode.ParentContainer != null && currentTreeNode.ParentContainer != rootTreeNode)
			{
				currentTreeNode = currentTreeNode.ParentContainer;
				if(currentTreeNode.NextNodeContainer != null)
					return currentTreeNode.NextNodeContainer;
			}


			//if(bWrapAtEnd == true)
			//    return rootTreeNode;


			return null;
		}


		#endregion
	}
}
