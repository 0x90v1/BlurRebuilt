using System;
using System.Collections.Generic;
using System.Text;

namespace Executer
{
	interface IRemoveable
	{
		bool CanClose();
		bool Close(CloseMethod closeMethod);
	}

	public enum CloseMethod
	{
		AlwaysAsk = 0,
		AskUsingMemoryDialogUnlessDirty,
		AskIfDirty,
		Force,
		FailIfDirty
	}
}
