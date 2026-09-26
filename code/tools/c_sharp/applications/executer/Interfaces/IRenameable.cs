using System;
using System.Collections.Generic;
using System.Text;

namespace Executer.Interfaces
{
	interface IRenameable
	{
		bool CanRename();		// E.g. Is not currently readonly.
		bool Rename(string newName);
		string DisplayName { get; }
	}
}