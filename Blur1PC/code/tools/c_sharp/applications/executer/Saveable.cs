using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Diagnostics;
using CustomMessageBox;
using System.Windows.Forms;
using Executer.Helpers;
using Executer.Interfaces;

namespace Executer
{
	static class Saveable
	{
		public static List<ISaveable> GetSaveableObjectList(ISaveable saveableObject, bool bOnlyListDirtyObjects)
		{
			List<ISaveable> objectsToSave = new List<ISaveable>();

			if((saveableObject.LogicallyCanBeSaved == true) && ((saveableObject.IsDirty == true) || (bOnlyListDirtyObjects == false)))
				objectsToSave.Add(saveableObject);

			if(saveableObject.CanHaveSaveableChildren == true)
			{
				foreach(ISaveable saveableChild in saveableObject.SaveableChildren)
				{
					objectsToSave.AddRange(GetSaveableObjectList(saveableChild, bOnlyListDirtyObjects));
				}
			}
			return objectsToSave;
		}

		public static SaveableResult RecursiveSave(ISaveable saveableObject, bool bOnlyListDirtyObjects, bool bAllowFail)
		{
			SaveableResult result = SaveableResult.Success;

			List<ISaveable> objectsToSave = GetSaveableObjectList(saveableObject, bOnlyListDirtyObjects);

			foreach(ISaveable objectToSave in objectsToSave)
			{
				result = Save(objectToSave, false);
				if(result != SaveableResult.Success)
					return result;
			}
			return result;
		}

		public static SaveableResult Save(ISaveable saveableObject, bool bAllowFail)
		{
			if(saveableObject.SaveName == null)
			{
				return SaveAs(saveableObject, null, bAllowFail);
			}

			try
			{
				saveableObject.Save();
				return SaveableResult.Success;
			}
			catch(Exception ex)
			{
				string strMessage;
				// TODO: Relative paths may be wrong as the cwd may have changed, therefore GetFileAttributes can throw an exception.
				//if((System.IO.File.GetAttributes(saveableObject.SaveName) & System.IO.FileAttributes.ReadOnly) == System.IO.FileAttributes.ReadOnly)
				//	strMessage = "Error saving  \n'" + saveableObject.SaveName + "'\nbecause it is read only.";
				//else
					strMessage = "Error saving\n'." + saveableObject.SaveName + "'\n" + "The system reports: '" + ex.Message + "'";

				if(bAllowFail == false)
				{
					string strCaption = "Executer ... " + System.IO.Path.GetFileName(saveableObject.SaveName);
					if(AskSaveAs(strMessage, strCaption) == true)
						return SaveAs(saveableObject, null, bAllowFail);
					else
						return SaveableResult.Cancelled;
				}

				return SaveableResult.Fail;
			}
		}

		private static bool AskSaveAs(string strMessage, string strCaption)
		{
			CustomMessageBoxForm cmb = new CustomMessageBoxForm(strMessage, strCaption, null);

			cmb.AddButton("Save As ...", true);
			cmb.AddButton("Cancel", false);

			cmb.ShowDialog();

			return (bool)cmb.Result;
		}

		public static SaveableResult SaveAs(ISaveable objectToSave, string strNewName, bool bAllowFail)
		{
			bool bNameIsBad = false;

			do
			{
				DialogResult dr = DialogResult.OK;

				if((strNewName == null) || (bNameIsBad == true))
				{
					if(strNewName == null) strNewName = objectToSave.SaveName;		// Anchor the save dialog to somewhere useful.
					dr = UtilityFunctions.GetSaveFilename(ref strNewName, objectToSave.FileFilter, objectToSave.SaveAsDialogTitle);
				}
				switch(dr)
				{
					case DialogResult.Cancel:
						return SaveableResult.Cancelled;
					case DialogResult.OK:
						try
						{
							objectToSave.SaveAs(strNewName);
							return SaveableResult.Success;
						}
						catch
						{
							if(bAllowFail == true)
								return SaveableResult.Fail;
							bNameIsBad = true;
						}
						break;
					default:
						Debug.Assert(true, "Numpty.");
						break;
				}
			} while(true);
		}
	}
}
