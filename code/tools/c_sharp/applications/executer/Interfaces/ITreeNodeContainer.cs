using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Executer.Interfaces
{
	public interface ITreeNodeContainer
	{
		System.Windows.Forms.TreeNodeCollection ChildNodes { get; }
		ITreeNodeContainer NextNodeContainer { get; }
		ITreeNodeContainer ParentContainer { get; }
	}
}