using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.IO;
using System.Windows.Forms;

namespace Executer.Helpers
{
	/// TODO: 
	/// Offer reload if modified externally. If there are unsaved changes offer to save them elsewhere.
	/// Offer to reload if external attributes are modified. If there are unsaved changes offer to save them elsewhere.
	/// 
	/// What am I?
	/// I am a helper class that watches the readonly status of a file.
	/// What am I for? (specifically)
	/// It 's my purpose to notify an Executer project or Solution that it's readonly status has changed, in order for the project to be optionally reloaded.
	internal class CustomFileWatcher
	{
		private readonly string _strPathFile;
		private FileSystemWatcher _watcher;
		private readonly Timer _timer;
		private bool _bIsReadonly;
		private DateTime _lastWriteTime;

		public event EventHandler<CustomFileWatcherEventArgs> CustomFileWatcherEvent;

		public class CustomFileWatcherEventArgs : EventArgs
		{
			public bool IsReadOnly { get; private set; }
			public DateTime Timestamp { get; private set; }
			public bool ReadOnlyHasChanged { get; private set; }
			public bool LastWriteTimeHasChanged { get; private set; }

			public CustomFileWatcherEventArgs(bool isReadOnly, bool readOnlyHasChanged, DateTime dt, bool lastWriteTimeHasChanged)
			{
				this.IsReadOnly = isReadOnly; // Naughty!
				this.ReadOnlyHasChanged = readOnlyHasChanged;
				this.Timestamp = dt;
				this.LastWriteTimeHasChanged = lastWriteTimeHasChanged;
			}
		}

		public CustomFileWatcher(string strPathFile)
		{
			_timer = new Timer();

			_strPathFile = strPathFile;

			FileAttributes fa = File.GetAttributes(strPathFile);
			_bIsReadonly = (fa & FileAttributes.ReadOnly) == FileAttributes.ReadOnly;
			_lastWriteTime = File.GetLastWriteTime(strPathFile);

			_timer.Interval = 20000;
			_timer.Tick += new EventHandler(TimerTick);
			_timer.Start();

			// Create a new FileSystemWatcher and set its properties.
			_watcher = new FileSystemWatcher();
			_watcher.Path = Path.GetDirectoryName(strPathFile);

			// Watch for changes in LastAccess and LastWrite times, and the renaming of files or directories like this:
			//_watcher.NotifyFilter = NotifyFilters.LastAccess | NotifyFilters.LastWrite | NotifyFilters.FileName | NotifyFilters.DirectoryName;

			_watcher.NotifyFilter = NotifyFilters.Attributes | NotifyFilters.FileName | NotifyFilters.DirectoryName;
			_watcher.Filter = Path.GetFileName(strPathFile);

			// Add event handlers.
			_watcher.Changed += new FileSystemEventHandler(OnChanged);
			_watcher.Created += new FileSystemEventHandler(OnChanged);
			_watcher.Deleted += new FileSystemEventHandler(OnChanged);
			_watcher.Renamed += new RenamedEventHandler(OnRenamed);

			// Begin watching.
			_watcher.EnableRaisingEvents = true;
		}

		~CustomFileWatcher()
		{
			_timer.Stop();
		}

		private void TimerTick(object sender, EventArgs e)
		{
			TestAttributeChanged();
		}

		private readonly object lockObject = new object();

		private void TestAttributeChanged()
		{
			lock(lockObject) // I'm not sure if the filewatcher and the timer can collide on this, so I'll lock it just in case.
			{
				try
				{
					FileAttributes fa = File.GetAttributes(_strPathFile);
					bool bIsReadonly = (fa & FileAttributes.ReadOnly) == FileAttributes.ReadOnly;

					DateTime lastWriteTime = File.GetLastWriteTime(_strPathFile);

					bool bReadonlynessHasChanged = false;
					bool bLastWriteTimeHasChanged = false;

					if(bIsReadonly != _bIsReadonly)
					{
						_bIsReadonly = bIsReadonly;
						bReadonlynessHasChanged = true;
					}

					if(lastWriteTime.CompareTo(_lastWriteTime) != 0)
					{
						_lastWriteTime = lastWriteTime;
						bLastWriteTimeHasChanged = true;
					}

					if(CustomFileWatcherEvent != null)
					{
						CustomFileWatcherEventArgs e = new CustomFileWatcherEventArgs(bIsReadonly, bReadonlynessHasChanged, lastWriteTime, bLastWriteTimeHasChanged);
						CustomFileWatcherEvent(this, e);
					}
				}
				catch(Exception ex)
				{
					Debug.WriteLine(ex.Message);
				}
			}
		}

		private void OnChanged(object sender, FileSystemEventArgs e)
		{
			TestAttributeChanged();
		}

		private void OnRenamed(object sender, RenamedEventArgs e)
		{
		}

		public void Suspend()
		{
			_watcher.EnableRaisingEvents = false;
		}

		public void Resume()
		{
			_watcher.EnableRaisingEvents = true;
		}

		public void Stop()
		{
			_watcher.EnableRaisingEvents = false;
			//_watcher.Dispose();
			_watcher = null;
		}
	}
}