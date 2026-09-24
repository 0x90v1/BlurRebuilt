using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace Executer
{
	interface IMoveable : ICloneable
	{
		bool CanMoveUp { get; }
		bool MoveUp();

		bool CanMoveDown { get; }
		bool MoveDown();

		bool CanCut { get; }
		Moveable Cut();

		bool CanCopy { get; }
		Moveable Copy();


		//event EventHandler<PasteEventArgs> pasteEvent;
	}

	interface ICanReceivePaste
	{
		bool CanReceivePaste(Moveable thingToPaste);
		bool Paste(Moveable thingToPaste);

		bool CanReceiveSpecialPaste(Moveable thingToPaste);

		bool PasteSpecial(Moveable thingToPaste);
	}

	public enum MoveType
	{
		Nothing = 0,
		Cut,
		Copy,
		//CutAndCannotCopy
	}

	public class Moveable
	{
		readonly object m_objectSource;

		public object ObjectSource
		{
			get { return m_objectSource; }
		}

		object m_copyOfObject;

		public object CopyOfObject
		{
			get { return m_copyOfObject; }
			set { m_copyOfObject = value; }
		}

		MoveType m_moveType;

		public bool IsPasteSpecial{ get; set;}

		public MoveType MoveType
		{
			get { return m_moveType; }
			set { m_moveType = value; }
		}

		public Moveable(object objectSource, object copyOfObject, MoveType moveType)
		{
			m_objectSource = objectSource;
			m_copyOfObject = copyOfObject;
			m_moveType = moveType;
		}

		public void CutHasBeenDone()
		{
			if(m_moveType == MoveType.Cut)
				m_moveType = MoveType.Copy;
			else if(m_moveType != MoveType.Copy)
				m_moveType = MoveType.Nothing;
		}
	}

		

}
