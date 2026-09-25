using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

namespace Executer
{
	public class ToolstripUtility
	{
		/// <summary>
		/// Given a collection of toolstrip items this assumes each toolstrip item tag is a string. If this string contains
		/// strToggle, it sets the toolstrip 'enabled' flag to bEnabled.
		/// </summary>
		/// <param name="tsItems">The toolstrip items to manipulate.</param>
		/// <param name="strToggle">The search string to trigger the action.</param>
		/// <param name="bEnabled">What to set the items visibility to if there is a match.</param>
		/// <param name="bRecurse">Whether to recurse the toolstrip items. Is this necessary?</param>
		public static void SelectToolStripItemsByFlag(ToolStripItemCollection tsItems, string strToggle, bool bEnabled, bool bRecurse)
		{
			foreach(object o in tsItems)
			{
				if(o is ToolStripMenuItem)
				{
					ToolStripMenuItem tsi = o as ToolStripMenuItem;

					if(tsi.Tag is string)
					{
						if((tsi.Tag as string).ToLower().Contains(strToggle.ToLower()))
							tsi.Visible = bEnabled;
					}

					if(tsi.DropDownItems != null && bRecurse == true)
					{
						SelectToolStripItemsByFlag(tsi.DropDownItems, strToggle, bEnabled, bRecurse);
					}
				}
				else if(o is ToolStripSeparator)
				{
					ToolStripSeparator tss = o as ToolStripSeparator;

					if(tss.Tag is string)
					{
						if((tss.Tag as string).Contains(strToggle))
							tss.Visible = bEnabled;
					}
				}
			}
		}

		/// <summary>
		/// Searches each toolstrip item until it finds one that has the provided text.
		/// </summary>
		/// <param name="toolStripItemCollection">The toolstrip items to search.</param>
		/// <param name="strText">The text to look for in each toolstrip item.</param>
		/// <returns>The first toolstrip item whose text property matches strText, or null if none found.</returns>
		public static ToolStripMenuItem FindFirstToolsTripMenuItemByText(ToolStripItemCollection toolStripItemCollection, string strText)
		{
			foreach(object o in toolStripItemCollection)
			{
				if(o is ToolStripMenuItem)
				{
					ToolStripMenuItem tmi = o as ToolStripMenuItem;

					if(tmi.Text == strText)
						return tmi;
				}
			}
			return null;
		}

		/// <summary>
		/// Assuming the tag of the provided ToolStripMenuItem is a string,
		/// look for the first string contained in '<' and '>'.
		/// </summary>
		/// <param name="toolstripMenuItem">The ToolStripMenuItem whose tag we're interested in.</param>
		/// <returns>a string, or null if none found.</returns>
		internal static string GetCommand(ToolStripMenuItem toolstripMenuItem)
		{
			string strTag = toolstripMenuItem.Tag as string;

			if(strTag != null)
			{
				// Look for first <blah>.
				int startIndex = strTag.IndexOf('<');
				if(startIndex >= 0)
				{
					int endIndex = strTag.IndexOf('>');
					if(endIndex >= 0)
					{
						if(endIndex > startIndex)
						{
							string strCommand = strTag.Substring(startIndex + 1, endIndex - startIndex - 1);
							return strCommand;
						}
					}
				}
			}
			return null;
		}


		/// <summary>
		/// Creates a ToolStripMenuItem with the given properties.
		/// </summary>
		/// <param name="strName"></param>
		/// <param name="strTag"></param>
		/// <param name="strText"></param>
		/// <param name="handler"></param>
		/// <param name="bEnabled"></param>
		/// <returns></returns>
		internal static ToolStripMenuItem CreateTsmi(string strName, string strTag, string strText, EventHandler handler, bool bEnabled)
		{
			ToolStripMenuItem tsmi = new ToolStripMenuItem {Name = strName, Tag = strTag, Text = strText};

			tsmi.Click += handler;

			tsmi.Enabled = bEnabled;

			return tsmi;
		}

		/// <summary>
		/// Creates a ToolStripseparator and sets its tag to the given string.
		/// </summary>
		/// <param name="strTag">A string to set the tag to</param>
		/// <returns></returns>
		internal static ToolStripSeparator CreateTss(string strTag)
		{
			ToolStripSeparator tss = new ToolStripSeparator();
			tss.Tag = strTag;
			return tss;
		}

		/// <summary>
		/// Creates a ToolStripMenuItem with the given properties.
		/// </summary>
		/// <param name="strName"></param>
		/// <param name="strTag"></param>
		/// <param name="strText"></param>
		/// <param name="handler"></param>
		/// <param name="bEnabled"></param>
		/// <param name="strTooltipText"></param>
		/// <returns></returns>
		internal static ToolStripMenuItem CreateTsmi(string strName, string strTag, string strText, EventHandler handler, bool bEnabled, string strTooltipText)
		{
			ToolStripMenuItem tsmi = CreateTsmi(strName, strTag, strText, handler, bEnabled);
			tsmi.ToolTipText = strTooltipText;

			return tsmi;
		}
	}
}
