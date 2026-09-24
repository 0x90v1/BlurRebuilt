using System;
using System.Collections.Generic;
using System.Text;

namespace Executer
{
	class LocallyUniqueIdMaker
	{
		long m_uid;

		public LocallyUniqueIdMaker()
		{
			m_uid = 0;
		}

		public LocallyUniqueIdMaker(int startValue)
		{
			m_uid = startValue;
		}

		public long GetNextLuid()
		{
			return m_uid++;
		}
	}
}
