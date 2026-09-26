using System;
using System.Collections.Generic;
using System.Text;

namespace Executer.Interfaces
{
	public enum SaveableResult
	{
		Success = 0,
		Fail,
		Cancelled
	}

	/// For Projects.
	/// If a project is loaded in more than one place then only one is editable
	/// and I want the UI to inform about a multiply loaded project and whether it is editable.
	public enum RestrictedMode
	{
		Writable = 0,
		ReadOnly,
		Restricted,
		PrimaryRestricted
	}

	public interface ISaveable
	{
		bool LogicallyCanBeSaved { get; }	// this should only be false for objects that cannot be saved but do have saveable children.
		//bool IsReadOnly{get;}		// E.g. Is not currently readonly.
		RestrictedMode ReadableState { get; }
		void Save();
		void SaveAs(string newName);
		bool IsDirty { get;set;}
		string SaveName { get;}

		string FileFilter { get; }
		string SaveAsDialogTitle { get; }

		bool CanHaveSaveableChildren { get; }
		List<ISaveable> SaveableChildren { get; }
	}
}