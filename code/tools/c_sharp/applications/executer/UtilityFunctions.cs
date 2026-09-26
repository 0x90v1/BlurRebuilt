using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Diagnostics;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using message_handler;
using System.Collections;
using XDevkit;
using Global;
using System.Xml;

namespace Executer
{
	static class UtilityFunctions
	{
		public static string GetUniqueFile(string tempFilePath)
		{
			if(tempFilePath.EndsWith("\\") == false)
			{
				tempFilePath += "\\";
			}

			string strTempFile;

			int count = 0;

			do
			{
				strTempFile = tempFilePath + "Ex_" + Path.GetRandomFileName()+"."+count.ToString()+".tmp";
				count++;
				if(count == 10000)
					throw new ApplicationException("Cannot determine a unique filename.");
			}
			while(File.Exists(strTempFile) == true);

			return strTempFile;
		}

		public static void DeleteOldFiles(string strFolder, string strFileMask, int minNumDaysOld)
		{
			try
			{
				DirectoryInfo di = new DirectoryInfo(strFolder);

				FileInfo[] fiList = di.GetFiles(strFileMask);

				foreach(FileInfo fi in fiList)
				{
					//if(fi.Name.StartsWith("Ex_"))
					{
						DateTime currentTime = DateTime.Now;
						DateTime fileTime = fi.CreationTime;
						TimeSpan dif = currentTime - fileTime;

						if(dif.TotalDays > minNumDaysOld)
						{
							Debug.WriteLine("Deleting old file: " + fi.FullName);
							fi.Delete();
						}
					}
				}
			}
			catch(Exception ex)
			{
				Debug.WriteLine(ex.Message);
			}
		}

		public static bool CheckPointIsWithinWorkingAreaOnAnyScreen(int x, int y)
		{
			int index;
			int upperBound;

			// Gets an array of all the screens connected to the system.

			System.Windows.Forms.Screen[] screens = System.Windows.Forms.Screen.AllScreens;
			upperBound = screens.GetUpperBound(0);

			for(index = 0; index <= upperBound; index++)
			{
				if(screens[index].WorkingArea.Contains(x, y) == true)
					return true;
			}
			return false;
		}

		public static DialogResult GetSaveFilename(ref string strFilename, string strFilter, string strTitle)
		{
			SaveFileDialog sfd = new SaveFileDialog();

			string strOldFilename = strFilename;
			sfd.CheckFileExists = false;
			sfd.InitialDirectory = System.IO.Path.GetDirectoryName(strFilename);
			sfd.FileName = System.IO.Path.GetFileName(strFilename);

			sfd.Filter = strFilter;
			sfd.FilterIndex = 1;
			sfd.RestoreDirectory = true;

			sfd.OverwritePrompt = true;

			sfd.Title = strTitle;

			DialogResult dr = sfd.ShowDialog();

			if(dr == DialogResult.OK)
				strFilename = sfd.FileName;
			else
				strFilename = null;

			return dr;
		}

		public static DialogResult GetOpenFilename(ref string strFilename, string strFilter, string strTitle)
		{
			OpenFileDialog ofd = new OpenFileDialog();

			string strOldFilename = strFilename;
			ofd.CheckFileExists = false;
			ofd.InitialDirectory = System.IO.Path.GetDirectoryName(strFilename);
			ofd.FileName = System.IO.Path.GetFileName(strFilename);

			ofd.Filter = strFilter;
			ofd.FilterIndex = 1;
			ofd.RestoreDirectory = true;

			ofd.Title = strTitle;

			DialogResult dr = ofd.ShowDialog();

			if(dr == DialogResult.OK)
				strFilename = ofd.FileName;
			else
				strFilename = null;

			return dr;
		}

		public static bool Shift	{ get { return (System.Windows.Forms.Control.ModifierKeys & Keys.Shift) != 0; } }
		public static bool Control	{ get { return (System.Windows.Forms.Control.ModifierKeys & Keys.Control) != 0; } }

		public static void ShowError(string strError)
		{
			MessageHandler.Show(null, strError, "Executer - Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
		}

		public static void ShowError(IWin32Window owner, string strError)
		{
			MessageHandler.Show(owner, strError, "Executer - Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
		}

		//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

		public delegate bool ActionDelegate<T>(object thing, T ActionArgs);

		public static bool EnumerableRecurse<TActionArgs>(IEnumerable thingCollection, ActionDelegate<TActionArgs> DoAction, ref TActionArgs actionArgs)
		{
			foreach(TreeNode childObject in thingCollection)
			{
				if(DoAction(childObject, actionArgs) == true)
					EnumerableRecurse<TActionArgs>(childObject.Nodes, DoAction, ref actionArgs);
				else
					return false;
			}
			return true;
		}

		//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

		static XDevkit.XboxManagerClass xbmc;

		public static string GetDefault360()
		{
			try
			{
				if (xbmc == null)
					xbmc = new XboxManagerClass();
				if (xbmc != null)
				{
					return ((XboxManagerClass)xbmc).DefaultConsole;
				}
			}
			catch { };// { }

			return MenuSystemLibrary.MenuSystem.UNINITIALISED_MARKER;
		}

		public static int FixupSpecialMenuCases(XmlElement xRoot)
		{
			if(xRoot != null)
			{
				string strdefault360 = GetDefault360();
				int count = 0;
				XmlNodeList xList = xRoot.SelectNodes(".//container[@type='x360name']");

				foreach(XmlElement xe in xList)
				{
					XmlHelper.AddAttribute(xe, "default", strdefault360);
					XmlHelper.AddAttribute(xe, "value", strdefault360);
				}

				count = xList.Count;

				XmlNodeList xList2 = xRoot.SelectNodes(".//container[@type='xml_value']");

				foreach(XmlElement xe in xList2)
				{
					string strDataPath = XmlHelper.GetKeyValue(xe, "default");

					if(strDataPath != null)
					{
						string[] parts = strDataPath.Split(new char[] { '|' }, StringSplitOptions.RemoveEmptyEntries);

						if(parts.Length == 3)
						{
							try
							{
								string strHeading = parts[0];
								string strFilename = parts[1];
								string strxPath = parts[2];

								//this.groupBox1.Text = strHeading;

								XmlDocument docDataDocument = new XmlDocument();
								docDataDocument.Load(strFilename);

								XmlNode xNode = docDataDocument.SelectSingleNode(strxPath);
								XmlHelper.AddAttribute(xe, "value", xNode.Value);
							}
							catch
							{
								XmlHelper.AddAttribute(xe, "value", "Cannot parse XmlDataContainer.");
							}
						}
					}
				}
				count += xList2.Count;

				count += FixupCustomAnimationContainers(xRoot);

				count += FixupCheckedness(xRoot);
				return count;

			}
			return 0;
		}

		private static int FixupCheckedness(XmlElement xRoot)
		{
			int count = 0;
			if(xRoot != null)
			{
				XmlNodeList xList = xRoot.SelectNodes(".//type_entry");

				foreach(XmlElement xe in xList)
				{
					if(xe.Attributes["checked"] == null)
						XmlHelper.AddAttribute(xe, "checked", "true");
				}
			}
			return count;
		}


		public static int SetCheckedness(XmlElement xRoot, bool bValue)
		{
			int count = 0;
			if(xRoot != null)
			{
				XmlNodeList xList = xRoot.SelectNodes(".//type_entry");

				foreach(XmlElement xe in xList)
				{
					XmlHelper.AddAttribute(xe, "checked", bValue.ToString().ToLower());
				}
			}
			return count;
		}



		public class CustomAnimationDetails
		{
			public XmlDocument xLookup { get; set; }
			// The cycle filename managed by the custom animation.
			public XmlElement xCycleFilename { get; set; }
			// The output location managed by the custom animation.
			public XmlElement xOutputLocation { get; set; }
			// The output directory managed by the custom animation.
			public XmlElement xOutputDirectory { get; set; }
			/// The custom animation that manages everything.
			public XmlElement xCustomAnimation { get; set; }
			public string StrErrorString { get; set; }
			// The lookup used by the custom animation.
			public Dictionary<string, string> CustomLookup { get; set; }
		}


		private static Dictionary<string, CustomAnimationDetails> CustomAnimationEntries;


		public static int FixupCustomAnimationContainers(XmlElement xRoot)
		{
			int count = 0;
			if(xRoot != null)
			{
				XmlNodeList xList = xRoot.SelectNodes(".//container[@special_type='custom_animation']");

				foreach(XmlElement xe in xList)
				{
					SetupCustomAnimation(xe);
					count++;
				}
			}
			return count;
		}

		public static void SetupCustomAnimation(XmlElement xObject)
		{
			string strXmlLookup = XmlHelper.GetKeyValue(xObject, "xml_lookup");
			CustomAnimationEntries = new Dictionary<string, CustomAnimationDetails>();
			CustomAnimationDetails cad = new CustomAnimationDetails();

			if(!string.IsNullOrEmpty(strXmlLookup))
			{
				try
				{
					cad.xLookup = new XmlDocument();
					cad.xLookup.Load(strXmlLookup);

					XmlNodeList xKeys = cad.xLookup.SelectNodes("animation_converter_config/output_dir_rules/output_dir_rule");

					cad.CustomLookup = new Dictionary<string, string>(xKeys.Count);

					foreach(XmlElement xKey in xKeys)
					{
						if(xKey != null)
						{
							string strName = XmlHelper.GetKeyValue(xKey, "pattern");
							string strValue = XmlHelper.GetKeyValue(xKey, "outputDir");

							if(!string.IsNullOrEmpty(strName) && !string.IsNullOrEmpty(strValue))
							{
								cad.CustomLookup.Add(strName, strValue);
							}
						}
					}
				}
				catch(Exception ex)
				{
					cad.StrErrorString = "Cannot load the Xml lookup document. The system reports:\n'" + ex.Message + "'";
				}

				if(cad.StrErrorString == null)
				{
					string strCycleFilename = XmlHelper.GetKeyValue(xObject, "cycle_filename");

					if(!string.IsNullOrEmpty(strCycleFilename))
					{
						cad.xCycleFilename = XmlHelper.GetNodeFromKeyPath(xObject, "name", strCycleFilename) as XmlElement;
						XmlHelper.AddAttribute(cad.xCycleFilename, "is_target_of_custom_animation", "true");
						CustomAnimationEntries.Add(XmlHelper.GetKeyValue(cad.xCycleFilename, "name"), cad);

						string strOutputLocation = XmlHelper.GetKeyValue(xObject, "output_location");

						if(!string.IsNullOrEmpty(strOutputLocation))
						{
							cad.xOutputLocation = XmlHelper.GetNodeFromKeyPath(xObject, "name", strOutputLocation) as XmlElement;
							XmlHelper.AddAttribute(cad.xOutputLocation, "is_target_of_custom_animation", "true");
							CustomAnimationEntries.Add(XmlHelper.GetKeyValue(cad.xOutputLocation, "name"), cad);

							string strOutputDirectory = XmlHelper.GetKeyValue(xObject, "output_directory");

							if(!string.IsNullOrEmpty(strOutputDirectory))
							{
								cad.xOutputDirectory = XmlHelper.GetNodeFromKeyPath(xObject, "name", strOutputDirectory) as XmlElement;
								XmlHelper.AddAttribute(cad.xOutputDirectory, "is_target_of_custom_animation", "true");
								CustomAnimationEntries.Add(XmlHelper.GetKeyValue(cad.xOutputDirectory, "name"), cad);

								//XmlHelper.AddAttribute(cad.xOutputDirectory, "custom_animation", "true");

								// At this point, all the information needed from the xml container has been successfully gathered.
								//m_xObject.OwnerDocument.NodeChanged += OwnerDocument_NodeChanged;

								// Add the source of the custom animation to our list too.
								CustomAnimationEntries.Add(XmlHelper.GetKeyValue(xObject, "name"), cad);

								cad.xCustomAnimation = xObject;

								UpdateCustomAnimation(xObject, CustomUpdate.UpdateXml, null);

							}
							else
							{
								cad.StrErrorString = "Attribute 'output_directory' not found on Container_CustomAnimation";
							}
						}
						else
						{
							cad.StrErrorString = "Attribute 'output_location' not found on Container_CustomAnimation";
						}
					}
					else
					{
						cad.StrErrorString = "Attribute 'cycle_filename' not found on Container_CustomAnimation";
					}
				}
			}


			/*
			 * TODO:
			 * Get xml lookup file.
			 * Put an event 'watch' on the external input field. UIse Xml events on the document?
			 * Hack in to the 'output directory external field so values can be set.
			 * 
			 * Toggle Output directory colour depending on validity of lookup.
			 */

		}

		public enum CustomUpdate
		{
			UpdateXml = 0,
			UpdateMenu
		}


		// Do not key dictionary on xObject, since it looks like the menu changed event gives a copy of the xObject.
		// Instead, key on the name of the object since this is unique.

		public static void UpdateCustomAnimation(XmlElement xObject, CustomUpdate customUpdate, MenuSystemLibrary.MenuSystem menuSystem)
		{
			// Something in a part of a custom animation has changed.
			if(CustomAnimationEntries.ContainsKey(XmlHelper.GetKeyValue(xObject, "name")))
			{
				CustomAnimationDetails cad = CustomAnimationEntries[XmlHelper.GetKeyValue(xObject, "name")];

				if(XmlHelper.GetKeyBool(cad.xCustomAnimation, "value", false) == true)
				{
					string strKey = XmlHelper.GetKeyValue(cad.xOutputLocation, "value");
					if(string.IsNullOrEmpty(strKey))
					{
						strKey = XmlHelper.GetKeyValue(cad.xCycleFilename, "value");
						strKey = Path.GetFileNameWithoutExtension(strKey);
					}
					if(!string.IsNullOrEmpty(strKey))
					{
						Match bestMatch = null;
						string strMatchValue = null;

						foreach(string strValue in cad.CustomLookup.Keys)
						{
							Match m = Regex.Match(strKey, strValue, RegexOptions.IgnoreCase);

							if(m.Length > 0 && (bestMatch == null || m.Length > bestMatch.Length))
							{
								bestMatch = m;
								strMatchValue = cad.CustomLookup[strValue];
							}
						}

						if(bestMatch == null || bestMatch.Length == 0)
						{
							strMatchValue = "** NO MATCH **";
						}
						else
						{
							strMatchValue = strMatchValue;
						}

						if(customUpdate == CustomUpdate.UpdateXml)
						{
							XmlHelper.AddAttribute(cad.xOutputDirectory, "value", strMatchValue);
						}
						else
						{
							menuSystem.SetSingleContainer(XmlHelper.GetKeyValue(cad.xOutputDirectory, "name"), strMatchValue);			// Updating the menu system will trigger an update of the underlying xml.
						}
					}
					else
					{
						if(customUpdate == CustomUpdate.UpdateXml)
						{
							XmlHelper.AddAttribute(cad.xOutputDirectory, "value", "*** NOTHING ***");
						}
						else
						{
							menuSystem.SetSingleContainer(XmlHelper.GetKeyValue(cad.xOutputDirectory, "name"), "*** NOTHING ***");		// Updating the menu system will trigger an update of the underlying xml.
						}
					}
				}
			}
		}

		//internal static void ClearCustomAnimationEntries()
		//{
		//    CustomAnimationEntries = new Dictionary<string, Executer.UtilityFunctions.CustomAnimationDetails>();
		//}



		public static string FormatXML(XmlElement xElement)
		{
			// Create a stream buffer that can be read as a string
			using(StringWriter sw = new StringWriter())

			// Create a specialized writer for XML code
			using(XmlTextWriter xtw = new XmlTextWriter(sw))
			{
				// Set the writer to use indented (hierarchical) elements
				xtw.Formatting = System.Xml.Formatting.Indented;

				// Write the XML document to the stream
				xElement.WriteTo(xtw);

				// Return the stream as a string
				return sw.ToString();
			}
		}
	}
}
#if mightbeusefuloneday


//And for posterity, here is some working code. Thanks again!

//private DateTime DateCompiled()

//// Assumes that in AssemblyInfo.cs,
//// the version is specified as 1.0.* or the like,
//// with only 2 numbers specified;
//// the next two are generated from the date.
//// This routine decodes them.

//{

//System.Version v =
//System.Reflection.Assembly.GetExecutingAssembly(). GetName().Version;

//// v.Build is days since Jan. 1, 2000
//// v.Revision*2 is seconds since local midnight
//// (NEVER daylight saving time)

//DateTime t = new DateTime(
//v.Build * TimeSpan.TicksPerDay +
//v.Revision * TimeSpan.TicksPerSecond * 2
//).AddYears(1999);

//return t;
//}

#endif