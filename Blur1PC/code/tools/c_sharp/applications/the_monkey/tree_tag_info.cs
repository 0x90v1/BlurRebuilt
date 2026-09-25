using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Drawing;

namespace RoboPublish
{
	public class TreeTagInfo
	{
		public TreeTagInfo(FileSystemInfo fsiSource, FileSystemInfo fsiDest)
		{
			m_fsiSource = fsiSource;
			m_fsiDest = fsiDest;
		}

		internal FileSystemInfo m_fsiSource;
		internal FileSystemInfo m_fsiDest;
		internal RoboTree.TreeItemIconIndex m_iconIndex;
		internal bool m_bDeleteSourceAfterCopy;		// This is used only by the file copier.

		internal bool m_doNotCopy = false;

		internal bool m_bhidden = false;// Nonsense?				// Multiple source folders can be merged into one TreeTagInfo
													// so if one is unhidden, all are unhidden.
													// Files are just assigned one way or the other.

		//internal string m_strOwnerPathName;			// The name of the path in the template that this was created for.
		//											// Irrelevant for folders.

	}
}
