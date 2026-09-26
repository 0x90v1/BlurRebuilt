using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;
using Global;
using System.Text.RegularExpressions;
using System.Drawing;

namespace RoboPublish
{
	public class Sku
	{
		public bool m_disabled = false;



		public List<SkuPath> m_listSkuPaths = new List<SkuPath>();
		//public string strDestFolder;

		public Dictionary<string, string> strExtensionsLookup = new Dictionary<string,string>();  // strExtension, strType

		public string strSkuName;

		internal bool AddPath(XmlElement xPath)
		{

			m_listSkuPaths.Add(new SkuPath
			(
				XmlHelper.GetKeyValue(xPath, "name"),
				XmlHelper.GetKeyValue(xPath, "colour"),
				XmlHelper.GetKeyValue(xPath, "source_path"),
				XmlHelper.GetKeyValue(xPath, "dest_path"),
				XmlHelper.GetKeyValue(xPath, "folder_mask"),
				XmlHelper.GetKeyValue(xPath, "file_mask"),
				XmlHelper.GetKeyBool(xPath, "merge_folders", false),
				XmlHelper.GetKeyValue(xPath, "dest_path_pattern_to_replace"),
				XmlHelper.GetKeyValue(xPath, "dest_path_pattern_replacement")
			));
			return true;
		}
	}

	public class SkuPath
	{
		public SkuPath(string strPathName, 
			string strColour, 
			string strSourcePath, 
			string strDestPath, 
			string strFolderMask, 
			string strFileMask, 
			bool bMergeFolders, 
			string strDestPathPatternToReplace,
			string strDestPathPatternReplacement
			)
		{
			m_strPathName = strPathName;
			m_colour = GetColourFromString(strColour);
			m_strSourcePath = strSourcePath;
			m_strDestPath = strDestPath;

			m_regexDirectory = new Regex(strFolderMask, RegexOptions.IgnoreCase);
			m_regexFile = new Regex(strFileMask, RegexOptions.IgnoreCase);

			m_strFolderMask = strFolderMask;
			m_strFileMask = strFileMask;
			m_bMergeFolders = bMergeFolders;

			if (strDestPathPatternToReplace != null && strDestPathPatternReplacement != null)
			{

					m_regexDestPathPatternToReplace = new Regex(strDestPathPatternToReplace, RegexOptions.IgnoreCase);
					m_strDestPathPatternReplacement = strDestPathPatternReplacement;

			}
			else
				m_regexDestPathPatternToReplace = null;
		}

		public Color GetColourFromString(string strColourName)
		{
			if(strColourName != null)
				return Color.FromName(strColourName.Replace(" ", ""));
			else
				return Color.Black;
		}

		public string m_strPathName;
		public Color m_colour;
		public string m_strSourcePath;
		public string m_strDestPath;

		public Regex m_regexDirectory;
		public Regex m_regexFile;

		private string m_strFolderMask;
		private string m_strFileMask;

		public bool m_bMergeFolders;

		public Regex m_regexDestPathPatternToReplace;
		public string m_strDestPathPatternReplacement;
	}	
}
