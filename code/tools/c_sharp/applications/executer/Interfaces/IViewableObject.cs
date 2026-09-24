using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Xml;

namespace Executer.Interfaces
{
	public interface IViewableObject
	{
		XmlElement RawXmlDocument { get; }

		Control CreateView();
	}
}