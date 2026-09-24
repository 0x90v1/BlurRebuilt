using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using System.Drawing;

namespace RoboPublish
{
	public class TreeObject : TreeNode
	{
		public List<TreeTagInfo> m_listTagInfo;
		readonly int m_numTags;
		internal bool m_oldFlag = true;			// Put this in TreeTagInfoGroup?

		public int NumTags { get { return m_numTags; } }

		public string m_strPathName;

		public Color m_textColour;
		public bool m_bIsFolder = true;				// Assume true unless told otherwise.

		public TreeObject(int numTags, string strName, string strPathName, Color textColour) : base(strName)
		{
			m_numTags = numTags;
			m_strPathName = strPathName;
			m_textColour = textColour;
			m_listTagInfo = new List<TreeTagInfo>(m_numTags);
			
			for(int c = 0; c < numTags; c++)
			{
				m_listTagInfo.Add(new TreeTagInfo(null, null));
			}
		}

		public string GetPath()
		{
			TreeObject to = this;
			string strResult = "";

			while(to != null)
			{
				strResult = "/" + to.Text +strResult;
				to = to.Parent as TreeObject;
			}
			return strResult;
		}
	}
}
