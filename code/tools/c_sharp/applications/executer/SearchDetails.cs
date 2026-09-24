using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Text.RegularExpressions;
using System.Xml;
using Executer.Interfaces;
using Executer.TreeObjects;
using Global;

namespace Executer
{
	internal class SearchDetails
	{
		public SearchDetails
			(
			string strNodeName,
			string strContainerName,
			string strContainerValue,
			string strTypeName,
			string strInternalName,
/*			bool bNodeNameMatchRequired,
			bool bContainerNameMatchRequired,
			bool bContainerValueMatchRequired,
			bool bTypeNameMatchRequired,
			bool bInternalNameMatchRequired,
*/			ITreeNodeContainer searchAnchor,
			ITreeNodeContainer currentNode,
			ITreeNodeContainer endNode
			)
		{
			this.StrNodeName = strNodeName;
			this.StrContainerName = strContainerName;
			this.StrContainerValue = strContainerValue;
			this.StrTypeName = strTypeName;
			this.StrInternalName = strInternalName;
			//BNodeNameMatchRequired = bNodeNameMatchRequired;
			//BContainerNameMatchRequired = bContainerNameMatchRequired;
			//BContainerValueMatchRequired = bContainerValueMatchRequired;
			//BTypeNameMatchRequired = bTypeNameMatchRequired;
			//BInternalNameMatchRequired = bInternalNameMatchRequired;
			this.SearchAnchor = searchAnchor;
			this.CurrentNode = currentNode;
			this.EndNode = endNode;
		}


		public string StrNodeName { get; private set; }
		public string StrContainerName { get; private set; }
		public string StrContainerValue { get; private set; }
		public string StrTypeName { get; private set; }
		public string StrInternalName { get; private set; }
		public bool BNodeNameMatchRequired { get; private set; }
		public bool BContainerNameMatchRequired { get; private set; }
		public bool BContainerValueMatchRequired { get; private set; }
		public bool BTypeNameMatchRequired { get; private set; }
		public bool BInternalNameMatchRequired { get; private set; }
		public ITreeNodeContainer SearchAnchor { get; private set; }
		public ITreeNodeContainer CurrentNode { get; set; }
		public ITreeNodeContainer EndNode { get; private set; }

		internal ITreeNodeContainer GetNextTreeNode()
		{
			ITreeNodeContainer nextNode = this.CurrentNode;

			bool bLooped = false;
			while((nextNode = ExecuterTreeUtilities.GetNextTreeNode(this.SearchAnchor, nextNode, true)) != null && bLooped == false)
			{
				if(nextNode == this.EndNode)
					bLooped = true;

				bool bMatchedNodeName = false;
				bool bMatchedContainerName = false;
				bool bMatchedContainerValue = false;
				bool bMatchedTypeName = false;
				bool bMatchedInternalName = false;

				// Match node name? ...
				if(this.StrNodeName != null)
				{
					if(nextNode is TreeBaseObject)
					{
						Regex regex = new Regex(StrNodeName, RegexOptions.IgnoreCase);
						//bMatchedNodeName = (nextNode as TreeBaseObject).Text.Contains(this.StrNodeName);
						bMatchedNodeName = regex.IsMatch((nextNode as TreeBaseObject).Text);
					}
					else
					{
						Debug.WriteLine("Not a TreeBaseObject!!!");
					}
					if(bMatchedNodeName == false)
						continue;
				}

				TreeUserObject nextUserObject;
				if(nextNode is TreeProjectObject)
					nextUserObject = ((nextNode as TreeProjectObject)._rootUserObject);
				else
					nextUserObject = nextNode as TreeUserObject;
				
				if(nextUserObject != null)
				{
					XmlNodeList xAllContainers__ = nextUserObject.XTreeNodeMenuData.SelectNodes("./values//container");

					List<XmlElement> myList = new List<XmlElement>();
					foreach(XmlElement xe in xAllContainers__)
					{
						if(xe != null)
							myList.Add(xe);
					}

					//if(myList.Count > 0)
					{
//XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

						List<XmlElement> myNewList = new List<XmlElement>();

						// Match container name? ...
						if(this.StrContainerName != null)
						{
							Regex regex = new Regex(StrContainerName, RegexOptions.IgnoreCase);

							foreach(XmlElement xElement in myList)
							{
								if(regex.IsMatch(XmlHelper.GetKeyValue(xElement, "display_name") ?? "") == true)
								{
									myNewList.Add(xElement);
								}
							}
							myList.Clear();
							myList.AddRange(myNewList);				// myList represents all XmlElements that match container name.
							myNewList.Clear();
							if(myList.Count == 0)
								continue;
						}

						// Match container value? ...
						if(this.StrContainerValue != null)
						{
							Regex regex = new Regex(StrContainerValue, RegexOptions.IgnoreCase);

							foreach(XmlElement xElement in myList)
							{
								//if(XmlHelper.GetKeyValue(xElement, "value").Contains(this.StrContainerValue) == true)
								if(regex.IsMatch(XmlHelper.GetKeyValue(xElement, "value") ?? "") == true)
								{
									myNewList.Add(xElement);
								}
							}
							myList.Clear();
							myList.AddRange(myNewList);				// myList represents all remaining XmlElements that match container value.
							myNewList.Clear();
							if(myList.Count == 0)
								continue;

						}


						// Match type name? ...
						if(this.StrTypeName != null)
						{
							Regex regex = new Regex(StrTypeName, RegexOptions.IgnoreCase);

							//XmlElement xTypeContainer = nextUserObject.XTreeNodeMenuData.SelectSingleNode("./values//container") as XmlElement;


							if(regex.IsMatch(XmlHelper.GetKeyValue(nextUserObject.XTreeNodeMenuData, "type_name") ?? "") == true)
							{
								bMatchedTypeName = true;
							}
							else
							{
								continue;
							}




							//foreach(XmlElement xElement in xAllContainers)
							//{
							//    //if(XmlHelper.GetKeyValue(xElement, "type").Contains(this.StrTypeName) == true)
							//    if(regex.IsMatch(XmlHelper.GetKeyValue(xElement, "type_name") ?? "") == true)
							//    {
							//        bMatchedTypeName = true;
							//    }
							//}
							//if(bMatchedTypeName == false)
							//    continue;
						}


						// Match internal name? ...
						if(this.StrInternalName != null)
						{
							Regex regex = new Regex(StrInternalName, RegexOptions.IgnoreCase);

							foreach(XmlElement xElement in myList)
							{
								//if(XmlHelper.GetKeyValue(xElement, "name").Contains(this.StrInternalName) == true)
								if(regex.IsMatch(XmlHelper.GetKeyValue(xElement, "name") ?? "") == true)
								{
									myNewList.Add(xElement);

								}
							}

							myList.Clear();
							myList.AddRange(myNewList);				// myList represents all remaining XmlElements that match internal name.
							myNewList.Clear();
							if(myList.Count == 0)
								continue;

						}
					} //XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
					this.CurrentNode = nextNode;

					return nextNode;
				}
			}
			return null;
		}
	}
}