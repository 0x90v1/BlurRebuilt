using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using System.IO;
using System.Diagnostics;
using System.Runtime.InteropServices;
using Microsoft.VisualBasic.FileIO;

namespace RoboPublish
{
	public class PublishThread
	{
		Control m_ownerControl;
		List<TreeTagInfo> m_listTti;
		DelegateProgressUpdate m_delegateProcessUpdate;

		public enum StatusUpdate
		{
			Started = 0,
			UpdateProgress,
			SendString,
			SendErrorString,
			Finished
		}

		PublishThread(Control ownerControl, DelegateProgressUpdate delegateProcessUpdate, List<TreeTagInfo> listTti)
		{
			m_ownerControl = ownerControl;
			m_delegateProcessUpdate = delegateProcessUpdate;
			m_listTti = listTti;
		}

		public static Thread CreatePublishThread(Control ownerControl, DelegateProgressUpdate delegateProcessUpdate, List<TreeTagInfo> listTti)
		{
			PublishThread publishThread = new PublishThread(ownerControl, delegateProcessUpdate, listTti);
			Thread th = new Thread(new ThreadStart(publishThread.PublishThreadProc));
			th.Name = "Publish thread.";
			th.Start();

			return th;
		}

		public delegate void DelegateProgressUpdate(StatusUpdate statusUpdate, object data);

		void PublishThreadProc()
		{
			m_ownerControl.Invoke(m_delegateProcessUpdate, new object[] { StatusUpdate.Started, null });
			int numFiles = m_listTti.Count;

			long totalBytesToCopy = 0;
			long totalBytesCopied = 0;
			float progress = 0;

			foreach(TreeTagInfo tti in m_listTti)
			{
				if((tti.m_fsiSource.Attributes & FileAttributes.Directory) != FileAttributes.Directory)
				{
					FileInfo fi = tti.m_fsiSource as FileInfo;
					totalBytesToCopy += fi.Length;
				}
			}

			int count = 0;

			List<FileSystemInfo> listFsiDelete = new List<FileSystemInfo>();

			foreach(TreeTagInfo tti in m_listTti)
			{
				m_ownerControl.BeginInvoke(m_delegateProcessUpdate, new object[] { StatusUpdate.SendString, tti.m_fsiSource.FullName });
				m_ownerControl.BeginInvoke(m_delegateProcessUpdate, new object[] { StatusUpdate.UpdateProgress, progress });

				string strSourcePathFile = tti.m_fsiSource.FullName;
				string strDestPathFile = tti.m_fsiDest.FullName;

				if(File.Exists(strDestPathFile))
				{
					FileAttributes fa = File.GetAttributes(strDestPathFile);

					if((fa & FileAttributes.ReadOnly) == FileAttributes.ReadOnly)
						File.SetAttributes(strDestPathFile, fa ^ FileAttributes.ReadOnly);		// Un ReadOnly.
				}
				try
				{
					try
					{
						Directory.CreateDirectory(Path.GetDirectoryName(strDestPathFile));
					}
					catch
					{
						throw new Exception("Cannot create the directory '" + strDestPathFile + "'\n");
					}
					if(tti.m_iconIndex == RoboTree.TreeItemIconIndex.FileFolderMismatch)
					{
						m_ownerControl.Invoke(m_delegateProcessUpdate, new object[] { StatusUpdate.SendErrorString, "Error: File / folder mismatch for '" + strSourcePathFile + "'" });
					}
					else if(tti.m_iconIndex == RoboTree.TreeItemIconIndex.Problem)
					{
						m_ownerControl.Invoke(m_delegateProcessUpdate, new object[] { StatusUpdate.SendErrorString, "Error: Problem for '" + strSourcePathFile + "'" });
					}
					else if((tti.m_fsiSource.Attributes & FileAttributes.Directory) != FileAttributes.Directory)
					{
						try
						{
							File.Copy(strSourcePathFile, strDestPathFile, true);

							if(tti.m_bDeleteSourceAfterCopy == true)
							{
								listFsiDelete.Add(tti.m_fsiSource);
							}

							FileInfo fi = tti.m_fsiSource as FileInfo;
							totalBytesCopied += fi.Length;

							if(totalBytesToCopy > 0.01f)
								progress = (float)totalBytesCopied / (float)totalBytesToCopy;
							else
								progress = 1.0f;
						}
						catch(Exception ex)
						{
							m_ownerControl.Invoke(m_delegateProcessUpdate, new object[] { StatusUpdate.SendErrorString, "Error: " + ex.Message });
						}
					}
					else
					{
						if(tti.m_bDeleteSourceAfterCopy == true)
						{
							listFsiDelete.Add(tti.m_fsiSource);
						}
					}
				}
				catch(Exception ex)
				{
					m_ownerControl.Invoke(m_delegateProcessUpdate, new object[] { StatusUpdate.SendErrorString, "Error: " + ex.Message });
				}

				count++;
			}

			foreach(FileSystemInfo fsiSource in listFsiDelete)
			{
				Debug.WriteLine("Deleting file " + fsiSource.FullName);

				if((fsiSource.Attributes & FileAttributes.ReadOnly) == FileAttributes.ReadOnly)
					fsiSource.Attributes = fsiSource.Attributes ^ FileAttributes.ReadOnly;

				//if((fsiSource.Attributes & FileAttributes.Directory) == FileAttributes.Directory)
				//    Directory.Delete(fsiSource.FullName);
				//else
				//    File.Delete(fsiSource.FullName);


				try
				{

					if((fsiSource.Attributes & FileAttributes.Directory) == FileAttributes.Directory)
					{
						if(Directory.GetFileSystemEntries(fsiSource.FullName).Length == 0)
							FileSystem.DeleteDirectory(fsiSource.FullName, UIOption.OnlyErrorDialogs, RecycleOption.SendToRecycleBin);
					}
					else
					{
						FileSystem.DeleteFile(fsiSource.FullName, UIOption.OnlyErrorDialogs, RecycleOption.SendToRecycleBin);
					}
				}
				catch(Exception ex)
				{
				}

			}



			//			m_ownerControl.Invoke(m_delegateProcessUpdate, new object[] { StatusUpdate.SendString, "\n\n*** FINISHED ***" });
			m_ownerControl.Invoke(m_delegateProcessUpdate, new object[] { StatusUpdate.Finished, "\n\n*** FINISHED ***" });
		}



	}
}
