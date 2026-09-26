using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

#if false
namespace Executer
{
	partial class ExecuterTreeView
	{

		public event EventHandler<ContentsChangedEventArgs> ContentsChangedEvent;

		protected void OnContentsChanged(ChangeEvent changeEvent, TreeBaseObject treeObjectBase)
		{
			if(ContentsChangedEvent != null)
			{
				ContentsChangedEvent(this, new ContentsChangedEventArgs(changeEvent, treeObjectBase));
			}
		}
	}

	public enum ChangeEvent
	{
		Nothing = 0,
		ItemAdded,
		ItemRemoved
	}

	public class ContentsChangedEventArgs : EventArgs
	{
		readonly ChangeEvent m_changeEvent;

		public ChangeEvent ChangeEvent
		{
			get { return m_changeEvent; }
		}

		readonly TreeBaseObject m_treeObjectBase;

		public TreeBaseObject TreeObjectBase
		{
			get { return m_treeObjectBase; }
		} 

		public ContentsChangedEventArgs(ChangeEvent changeEvent, TreeBaseObject treeObjectBase)
		{
			m_changeEvent = changeEvent;

			m_treeObjectBase = treeObjectBase;
		}
	}
}
#endif