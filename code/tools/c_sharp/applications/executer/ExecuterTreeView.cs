using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using Executer.Helpers;
using Executer.Interfaces;
using Executer.TreeObjects;
using message_handler;
using Global;
using System.IO;
using System.Diagnostics;
//using MacroExpander;
using memory_variable;
using System.Threading;
using System.Runtime.InteropServices;
using System.Text.RegularExpressions;

namespace Executer
{
	public partial class ExecuterTreeView : TreeView, ISaveable, ITreeNodeContainer
	{
		private const int SB_HORZ = 0;
		private const int SB_VERT = 1;
		private const int WM_HSCROLL = 0x0114;
		private const int WM_VSCROLL = 0x0115;
		public static Font fontHeading = null;
		public List<TreeBaseObject> _CheckedTreeObjects = new List<TreeBaseObject>();
		private TreeBaseObject _lastSelectedNode = null;
		private LocallyUniqueIdMaker m_luid = new LocallyUniqueIdMaker();
		private Color _oldCutColour = new Color();
		private TreeNode _oldCutNode = null;

		public TreeNode _treeRClickObjectSelection = null;

		internal XmlDocument _xConfig; // The configuration file for the application.

		#region Tree drag and drop

		private Moveable _dragMoveable = null;

		private void TreeObjectManager_ItemDrag(object sender, ItemDragEventArgs e)
		{
			string strItem = e.Item.ToString();
			IMoveable dragObject = e.Item as IMoveable;

			if(dragObject != null)
			{
				if(UtilityFunctions.Shift)
				{
					if(dragObject.CanCopy == true)
					{
						_dragMoveable = dragObject.Copy();
						DoDragDrop(strItem, DragDropEffects.Copy);
					}
				}
				else
				{
					if(dragObject.CanCut == true)
					{
						_dragMoveable = dragObject.Cut();
						DoDragDrop(strItem, DragDropEffects.Move);
					}
				}
			}
		}


		private void TreeObjectManager_DragEnter(object sender, DragEventArgs e)
		{
			if(e.Data.GetDataPresent(DataFormats.Text))
			{
				Debug.WriteLine(e.Data.GetData(DataFormats.Text));
			}
			/*
			// Determine if we are dragging something, if we are, set the effect
			if(e.Data.GetDataPresent(DataFormats.Text))
				e.Effect = DragDropEffects.Move;
			else
				e.Effect = DragDropEffects.None;
			 */
		}

		private void TreeObjectManager_DragDrop(object sender, DragEventArgs e)
		{
			if(e.Data.GetDataPresent(DataFormats.FileDrop))
			{
				string[] filenames = (string[])e.Data.GetData(DataFormats.FileDrop);

				TreeSolutionObject destSolution = this.FindTreeNodeAtPoint(e.X, e.Y) as TreeSolutionObject;

				foreach(string filename in filenames)
				{
					if(filename.ToLower().EndsWith(".xtm"))
					{
						// Add template.
						Program.ExecuterMainForm.MainTreeView.CreateAndAddNewProject(filename, null, null);
					}
					else if(filename.ToLower().EndsWith(".executersolution"))
					{
						//TreeSolutionObject solutionNode = new TreeSolutionObject(filename, this);
						//this.Nodes.Add(solutionNode);
						//solutionNode.ImageKey = "TreeSolutionIcon";
						//solutionNode.SelectedImageKey = "TreeSolutionIcon";

						/*TreeSolutionObject tso =*/
						TreeSolutionObject.AddOrFindSolution(filename);
					}
					else
					{
						if(destSolution != null)
						{
							this.AddOrFindProject(destSolution, filename);
						}
					}
				}
			}
			else
			{
				ICanReceivePaste dragDropTarget = this.FindTreeNodeAtPoint(e.X, e.Y) as ICanReceivePaste;

				if(dragDropTarget != null)
				{
					if(dragDropTarget.CanReceivePaste(_dragMoveable))
						dragDropTarget.Paste(_dragMoveable);
				}
				else
				{
					Debug.WriteLine("NOTHING.");
				}
			}
		}

		private void TreeObjectManager_DragOver(object sender, DragEventArgs e)
		{
			if(e.Data.GetDataPresent(DataFormats.FileDrop))
			{
				TreeSolutionObject destSolution = this.FindTreeNodeAtPoint(e.X, e.Y) as TreeSolutionObject;
				if(destSolution != null)
					e.Effect = DragDropEffects.Copy;
				else
				{
					string[] filenames = (string[])e.Data.GetData(DataFormats.FileDrop);

					if(filenames[0].ToLower().EndsWith(".executersolution"))
						e.Effect = DragDropEffects.Copy;
					else
						e.Effect = DragDropEffects.None;
				}
			}
			else
			{
				e.Effect = DragDropEffects.None;


				bool bAllowChild = false;

				ICanReceivePaste destTo = this.FindTreeNodeAtPoint(e.X, e.Y) as ICanReceivePaste;
				if(destTo != null)
				{
					bAllowChild = destTo.CanReceivePaste(_dragMoveable);

					// Set the drag effect depending on whether the dragged object can be dropped here.
					if(bAllowChild == true)
					{
						if(_dragMoveable.MoveType == MoveType.Copy)
							e.Effect = DragDropEffects.Copy;
						else
							e.Effect = DragDropEffects.Move;
					}
					else
						e.Effect = DragDropEffects.None;
				}
				else
					e.Effect = DragDropEffects.None;
			}
			MyDragScroll(sender, e);
		}

		#endregion Tree drag and drop

		#region oops

		private Thread _sortThread;

		private void deleteToolStripMenuItem_Click(object sender, EventArgs e)
		{
			IRemoveable removeableObject = this._treeRClickObjectSelection as IRemoveable;

			if(removeableObject != null)
			{
				removeableObject.Close(CloseMethod.AskUsingMemoryDialogUnlessDirty);
			}

			//this.m_treeRClickObjectSelection.Close(CloseMethod.AskUsingMemoryDialogUnlessDirty);
			////this.DeleteTreeNode(this.m_treeRClickObjectSelection);
		}

		private void saveToolStripMenuItem_Click(object sender, EventArgs e)
		{
			ISaveable saveableObject = this._treeRClickObjectSelection as ISaveable;

			if(saveableObject != null)
			{
				Saveable.RecursiveSave(saveableObject, true, false);
				//saveableObject.Save();
			}
			else
			{
				MessageBox.Show("Not saved.");
			}

			//TreeObject to = this.m_treeRClickObjectSelection;

			//to.OwnerProject.Save();
		}


		private void propertiesToolStripMenuItem_Click(object sender, EventArgs e)
		{
			TreeUserObject treeUserObject = sender as TreeUserObject;

			MessageHandler.Show(this, "Not implemented yet.");
		}

		//private void sortChildrenToolStripMenuItem_Click(object sender, EventArgs e)
		//{
		//    this.AsyncSortChildren(this.SelectedNode);
		//}

		public bool AsyncSortChildren(TreeNode tn)
		{
			if(tn == null)
				return false;
			// There is no good reason to push this into a thread. :-)
			//m_sortThread = new Thread(SortTreeThread);
			//m_sortThread.Priority = ThreadPriority.BelowNormal;
			//m_sortThread.Start(tn);

			SortTreeThread(tn);

			return true;
		}

		// BROKEN.
		private void SortTreeThread(object data)
		{
			TreeNode tn = data as TreeNode;

			if(tn != null)
			{
				bool bMovedSomething = true;

				this.Invoke
					((MethodInvoker)delegate()
					                	{
					                		this.Enabled = false;
					                		this.Update();
					                		this.BeginUpdate();
					                	});

				do
				{
					bMovedSomething = false;

					foreach(TreeNode tnChild in tn.Nodes)
					{
						if(tnChild.Index == 0)
							continue;

						if(tnChild.Text.ToLower().CompareTo(tnChild.PrevNode.Text.ToLower()) < 0)
						{
							IMoveable moveable = tnChild as IMoveable;

							Debug.Assert(moveable != null);

							if(moveable != null)
							{
								this.Invoke((MethodInvoker)delegate() { moveable.MoveUp(); });
								bMovedSomething = true;
								break;
							}
						}
					}
				} while(bMovedSomething == true);

				this.Invoke
					((MethodInvoker)delegate()
					                	{
					                		this.Enabled = true;
					                		this.EndUpdate();
					                	});
			}
		}


		//internal int CleanupJobs(TreeUserObject tuo)
		//{
		//    List<Job> deleteList = new List<Job>(tuo.m_jobList.Count);
		//    foreach(Job job in tuo.m_jobList)
		//    {
		//        switch(job.JobStatus)
		//        {
		//            case JobStatus.None:
		//                Debug.Assert(false);
		//                break;
		//            case JobStatus.Requested:
		//                break;
		//            case JobStatus.Queued:
		//                break;
		//            case JobStatus.Failed:
		//                deleteList.Add(job);
		//                break;
		//            case JobStatus.Timeout:
		//                break;
		//            case JobStatus.Created:
		//                break;
		//            case JobStatus.Killed:
		//                deleteList.Add(job);
		//                break;
		//            case JobStatus.Abandoned:
		//                deleteList.Add(job);
		//                break;
		//            case JobStatus.Finished:
		//                deleteList.Add(job);
		//                break;
		//            case JobStatus.Cancelled:
		//                deleteList.Add(job);
		//                break;
		//            default:
		//                break;
		//        }
		//    }
		//    if(deleteList.Count != 0)
		//    {
		//        foreach(Job jobDataToDelete in deleteList)
		//        {

		//            tuo.RemoveJobDataEntry(jobDataToDelete);
		//        }
		//    }
		//    return deleteList.Count;
		//}

		#endregion oops

		//private Moveable m_moveable = null;

		public ExecuterTreeView()
			: base()
		{
			InitializeComponent();

			fontHeading = new Font(SystemFonts.CaptionFont.Name, 12, FontStyle.Bold | FontStyle.Italic);

			AddImage("Unknown", global::Executer.Properties.Resources.UnknownIcon);
			AddImage("TreeProjectIcon", global::Executer.Properties.Resources.TreeProjectIcon);

			////////////////////////////
			bool bAddedImage = false;
			string strCurrentDirectory = null;
			try
			{
				strCurrentDirectory = Environment.CurrentDirectory;
				Environment.CurrentDirectory = Path.GetDirectoryName(Application.ExecutablePath);

				Bitmap bmp = new Bitmap("TreeSolutionIcon.bmp");
				if(bmp.Width == 16 && bmp.Height == 16)
				{
					this.AddImage("TreeSolutionIcon", bmp);
					bAddedImage = true;
				}
			}
			catch
			{
			}
			finally
			{
				Environment.CurrentDirectory = strCurrentDirectory;
			}

			////////////////////////
			if(bAddedImage == false)
				AddImage("TreeSolutionIcon", global::Executer.Properties.Resources.TreeSolutionIcon);
		}

		public string ApplicationPath { get; set; }

		public string ConfigPathFile
		{
			get { return Application.LocalUserAppDataPath + "\\config.executer"; }
		}

		public int HScrollPos
		{
			get { return GetScrollPos((int)Handle, SB_HORZ); }
			set { SetScrollPos((int)Handle, SB_HORZ, value, true); }
		}

		public int VScrollPos
		{
			get { return GetScrollPos((int)Handle, SB_VERT); }
			set { SetScrollPos((int)Handle, SB_VERT, value, true); }
		}

		#region ISaveable Members

		public bool LogicallyCanBeSaved
		{
			get { return false; }
		}

		//public bool IsReadOnly
		//{
		//    get { throw new NotImplementedException(); }
		//}

		public RestrictedMode ReadableState
		{
			get { throw new NotImplementedException(); }
		}

		public void Save()
		{
			this.IsDirty = false;
		}

		public void SaveAs(string newName)
		{
			this.IsDirty = false;
		}

		public bool IsDirty { get; set; }

		public string SaveName
		{
			get { return "This is meaningless"; }
		}

		public string FileFilter
		{
			get { throw new NotImplementedException(); }
		}

		public string SaveAsDialogTitle
		{
			get { throw new NotImplementedException(); }
		}

		public bool CanHaveSaveableChildren
		{
			get { return true; }
		}

		public List<ISaveable> SaveableChildren
		{
			get
			{
				List<ISaveable> saveableChildren = new List<ISaveable>();

				foreach(TreeNode tn in this.Nodes)
				{
					if(tn is ISaveable)
					{
						saveableChildren.Add(tn as ISaveable);
					}
				}
				return saveableChildren;
			}
		}

		#endregion

		#region ITreeNodeContainer Members

		public TreeNodeCollection ChildNodes
		{
			get { return this.Nodes; }
		}

		public ITreeNodeContainer NextNodeContainer
		{
			get { return null; }
		}

		public ITreeNodeContainer ParentContainer
		{
			get { return null; }
		}

		#endregion

		[DllImport("user32.dll")]
		private static extern int SendMessage(IntPtr hWnd, int wMsg, int wParam, int lParam);


		internal void SetToolStripContainer(ToolStripContainer toolstripContainer)
		{
		}

		/// <summary>
		/// This is used to intercept keystrokes on the tree view in
		/// order to e.g. process the 'delete' key.
		/// </summary>
		/// <param name="keyData"></param>
		/// <returns></returns>
		protected override bool ProcessDialogKey(System.Windows.Forms.Keys keyData)
		{
			System.Windows.Forms.Keys key = keyData;
			if(key == System.Windows.Forms.Keys.Delete)
			{
				//Add you code logic here
				if(this.SelectedNode != null)
				{
					if(this.SelectedNode.IsEditing == false) // Don't delete it if the label is being edited and the delete key is pressed.
					{
						IRemoveable theNode = this.SelectedNode as IRemoveable;

						if(theNode != null && theNode.CanClose() == true)
						{
							theNode.Close(CloseMethod.AskUsingMemoryDialogUnlessDirty);
						}
					}
					else
						return base.ProcessDialogKey(keyData);
				}
				else
				{
					MessageHandler.Show(this, "No tree node selected.");
				}
				return true;
			}
			else if(key == System.Windows.Forms.Keys.F2)
			{
				if(this.SelectedNode != null && this.SelectedNode is IRenameable)
				{
					this.SelectedNode.BeginEdit();
					return true;
				}
			}
			return base.ProcessDialogKey(keyData);
		}

		/// <summary>
		/// Creates a new solution and adds it to the tree view.
		/// </summary>
		/// <returns>The new solution pbject.</returns>
		internal TreeSolutionObject AddNewSolutionNode()
		{
			TreeSolutionObject solutionNode = new TreeSolutionObject(this);
			solutionNode.Name = "Solution";
			solutionNode.Text = "<New Solution>";


			this.Nodes.Add(solutionNode);
			this.ImageKey = "TreeSolutionIcon";
			this.SelectedImageKey = "TreeSolutionIcon";

			return solutionNode;
		}

		/// <summary>
		/// Adds images to the image list as long as they're not already there.
		/// </summary>
		/// <param name="strIconPath">Path to the images.</param>
		/// <returns>The number of images added.</returns>
		public int AddImages(string strIconPath)
		{
			DirectoryInfo di = new DirectoryInfo(strIconPath);
			int count = 0;

			if(di.Exists == true)
			{
				foreach(FileInfo fi in di.GetFiles())
				{
					if(fi.Extension == ".bmp")
					{
						Bitmap bmp = new Bitmap(fi.FullName);
						if(bmp.Width == 16 && bmp.Height == 16)
						{
							if(this.ImageList == null) // Done here to avoid having an imagelist if there are no icons.
								this.ImageList = new ImageList();
							if(this.ImageList.Images.Keys.Contains(fi.FullName) == false)
							{
								this.ImageList.Images.Add(fi.FullName, bmp);
								count++;
							}
						}
					}
				}
			}
			return count;
		}


		public bool AddImage(string strName, Image image)
		{
			if(this.ImageList == null) // Done here to avoid having an imagelist if there are no icons.
				this.ImageList = new ImageList();
			if(this.ImageList.Images.Keys.Contains(strName) == false)
			{
				this.ImageList.Images.Add(strName, image);
				return true;
			}
			return false;
		}

		//public bool CreateAndAddNewProject()
		//{
		//}

		internal bool CreateAndAddNewProject(string strTemplateFilename, string strProjectFilename, TreeSolutionObject ownerSolution)
		{
			string strProject = CreateNewProjectOnDisk(strTemplateFilename, strProjectFilename);

			if(strProject != null)
			{
				TreeProjectObject tpo = this.AddOrFindProject(ownerSolution, strProject);
				if(tpo != null)
				{
					tpo.IsDirty = true;
					this.SelectedNode = tpo;
					return true;
				}
			}
			return false;
		}


		public string CreateNewProjectOnDisk(string strTemplateFilename, string strProjectFilename)
		{
			Form_NewDatafile ndf = new Form_NewDatafile();

			ndf.TemplateFile = strTemplateFilename;
			ndf.DataFile = strProjectFilename;

			DialogResult dr = ndf.ShowDialog(this);

			if(dr == DialogResult.OK)
			{
				Debug.WriteLine("Initialising (creating) with'" + ndf.DataFile + "'\n");

				XmlDocument xNewDocument = new XmlDocument();

				// Get the filename of the data definition document and also load that.
				try
				{
					xNewDocument.Load(new StringReader("<root version = \"1.1\"><use_template/><root_type_entry type_name='root_node'><values/></root_type_entry></root>")); // Hooray for hardcoding. Not.
					XmlHelper.AddAttribute(xNewDocument.SelectSingleNode("root/use_template") as XmlElement, "name", ndf.TemplateFile);
					XmlHelper.AddAttribute(xNewDocument.SelectSingleNode("root/root_type_entry") as XmlElement, "name", Path.GetFileNameWithoutExtension(ndf.DataFile));

					xNewDocument.Save(ndf.DataFile);
				}
				catch(Exception ex)
				{
					UtilityFunctions.ShowError("Cannot create the data file\n'" + ndf.DataFile + "'\nSystem reports: " + ex.Message);
					return null;
				}

				return ndf.DataFile;
			}
			return null;
		}


		internal TreeNode FindTreeNodeAtPoint(int x, int y)
		{
			Point pt = new Point(x, y);
			pt = this.PointToClient(pt);

			return this.GetNodeAt(pt);
		}

		private void TreeObjectManager_BeforeLabelEdit(object sender, NodeLabelEditEventArgs e)
		{
			IRenameable ir = e.Node as IRenameable;

			if(ir != null)
			{
				if(ir.CanRename() == false)
					e.CancelEdit = true;
			}
			else
				e.CancelEdit = true;
		}

		private void TreeObjectManager_AfterLabelEdit(object sender, NodeLabelEditEventArgs e)
		{
			IRenameable ir = e.Node as IRenameable;
			if(ir != null)
			{
				if(ir.DisplayName != e.Label)
				{
					if(ir.Rename(e.Label) == false)
						e.CancelEdit = true;
				}
			}
			else
			{
				e.CancelEdit = true; // This reverts to the old value.
				throw new ApplicationException("This is wrong.");
			}
		}

		private void TreeObjectManager_AfterSelect(object sender, TreeViewEventArgs e)
		{
			if(e.Action != TreeViewAction.ByKeyboard)
			{
				if(((TreeBaseObject)this.SelectedNode) != _lastSelectedNode) // Setting Form.ShowIntaskbar makes this test necessary.
				{
					_lastSelectedNode = this.SelectedNode as TreeBaseObject;

					TreeNode tn = this.SelectedNode;
					Debug.WriteLine(tn.ToString());

					Debug.Assert(_lastSelectedNode != null);

					if(this.CheckBoxes == false)
					{
						ExecuterMainForm.s_treeObjectViewer.ViewtreeObject(_lastSelectedNode as IViewableObject);
						this.Focus();
					}
				}
			}
		}

		private void TreeObjectManager_KeyUp(object sender, KeyEventArgs e)
		{
			Debug.Assert(sender is ExecuterTreeView);
			if(_lastSelectedNode != this.SelectedNode)
			{
				_lastSelectedNode = this.SelectedNode as TreeBaseObject;

				if(this.CheckBoxes == false)
				{
					ExecuterMainForm.s_treeObjectViewer.ViewtreeObject(_lastSelectedNode as IViewableObject);
					this.Focus();
				}
			}
		}

		/// <summary>
		/// Captures a right click on a tree node and creates the appropriate context menu.
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void TreeObjectManager_NodeMouseClick(object sender, TreeNodeMouseClickEventArgs e)
		{
			if(e.Button == MouseButtons.Left && e.Node != null)
			{
				if(UtilityFunctions.Shift && this.CheckBoxes)
				{
					TreeBaseObject tbo = this.SelectedNode as TreeBaseObject;
					TreeBaseObject tboStart;
					TreeBaseObject tboEnd;

					while(tbo != e.Node && tbo != null)
					{
						tbo = NextVisibleNode(tbo);
					}

					if(tbo == null)
					{
						tboStart = e.Node as TreeBaseObject;
						tboEnd = this.SelectedNode as TreeBaseObject;
					}
					else
					{
						tboStart = this.SelectedNode as TreeBaseObject;
						tboEnd = e.Node as TreeBaseObject;
					}


					while(tboStart != tboEnd && tboStart != null)
					{
						tboStart.Checked = true;
						tboStart = NextVisibleNode(tboStart);
					}
					e.Node.Checked = true;
				}
			}


			this.SelectedNode = e.Node; // On right click, set the selected node to the clicked node.

			if(e.Button == MouseButtons.Right)
			{
				this._treeRClickObjectSelection = e.Node;

				IContextMenu contextMenuContainer = e.Node as IContextMenu;

				if(contextMenuContainer != null)
				{
					ContextMenuStrip cms = contextMenuContainer.CreateMenu();

					if(this.SelectedNode is IMoveable)
					{
						cms.Items.Add(ToolstripUtility.CreateTss(null));
						bool bCanMoveUp = (this.SelectedNode as IMoveable).CanMoveUp;
						cms.Items.Add(ToolstripUtility.CreateTsmi("moveUpTsmi", "<moveup>", "Move up", tsmi_Click, bCanMoveUp));
						bool bCanMoveDown = (this.SelectedNode as IMoveable).CanMoveDown;
						cms.Items.Add(ToolstripUtility.CreateTsmi("moveDownTsmi", "<movedown>", "Move down", tsmi_Click, bCanMoveDown));


						bool bCanCut = (this.SelectedNode as IMoveable).CanCut;
						cms.Items.Add(ToolstripUtility.CreateTsmi("cutTsmi", "<cut>", "Cut", tsmi_Click, bCanCut));
						bool bCanCopy = (this.SelectedNode as IMoveable).CanCopy;
						cms.Items.Add(ToolstripUtility.CreateTsmi("copyTsmi", "<copy>", "Copy", tsmi_Click, bCanCopy));


						bool bCanSort = true;
						// See if the first child can be moved down. If so, the children can be sorted.
						if(e.Node.Nodes.Count > 0)
						{
							IMoveable moveableChild = e.Node.Nodes[0] as IMoveable;
							if(moveableChild != null)
								bCanSort = !moveableChild.CanMoveDown;
						}

						cms.Items.Add(ToolstripUtility.CreateTsmi("sortChildrenTsmi", "<sortchildren>", "Sort Children", tsmi_Click, !bCanSort));
					}

					if(this.SelectedNode is ICanReceivePaste)
					{
						cms.Items.Add(ToolstripUtility.CreateTss(null));
						bool bCanReceivePaste = (this.SelectedNode as ICanReceivePaste).CanReceivePaste(_dragMoveable);

						cms.Items.Add(ToolstripUtility.CreateTsmi("pasteTsmi", "<paste>", "Paste", tsmi_Click, bCanReceivePaste));

						if((this.SelectedNode is TreeUserObject) && this.CheckBoxes == true) // Restrict 'Paste Special' to TreeUserObjects and only when multiple-select is on.
						{
							//bool bCanReceivePasteSpecial = (this.SelectedNode as ICanReceivePaste).CanReceiveSpecialPaste(m_dragMoveable);
							//cms.Items.Add(ToolstripUtility.CreateTsmi("pasteSpecialTsmi", "<pastespecial>", "Paste for mad people", tsmi_Click, bCanReceivePaste));
						}
					}

					if(this.SelectedNode is ICanReloadSelf)
					{
						cms.Items.Add(ToolstripUtility.CreateTss(null));
						cms.Items.Add(ToolstripUtility.CreateTsmi("reloadTsmi", "<reload>", "Reload Project", tsmi_Click, true));
					}

					bool bHasExtraDelimiter = false;

					if(this.SelectedNode as IRenameable != null)
					{
						cms.Items.Add(ToolstripUtility.CreateTss(null));
						bHasExtraDelimiter = true;
						cms.Items.Add(ToolstripUtility.CreateTsmi("renameTsmi", "<rename>", "Rename", tsmi_Click, (this.SelectedNode as IRenameable).CanRename()));

						if(this.SelectedNode is TreeUserObject)
						{
							TreeUserObject tuo = this.SelectedNode as TreeUserObject;

							XmlElement xAutoNameElement = tuo.XTypeTemplate.SelectSingleNode("auto_name_property") as XmlElement;


							if(xAutoNameElement != null)
							{
								cms.Items.Add(ToolstripUtility.CreateTsmi("autoRenameTsmi", "<autorename>", "Auto Rename", tsmi_Click,
								                                          (this.SelectedNode as IRenameable).CanRename()));
							}
						}
					}

					cms.Items.Add(ToolstripUtility.CreateTss(null));
					if(bHasExtraDelimiter == false)
					{
						cms.Items.Add(ToolstripUtility.CreateTss(null));
						bHasExtraDelimiter = true;
					}

					cms.Items.Add(ToolstripUtility.CreateTsmi("expandChildrenTsmi", "<expandchildren>", "Expand children", tsmi_Click, this.SelectedNode.Nodes.Count > 0));
					cms.Items.Add(ToolstripUtility.CreateTsmi("collapseChildrenTsmi", "<collapsechildren>", "Collapse children", tsmi_Click, this.SelectedNode.Nodes.Count > 0));

					cms.Items.Add(ToolstripUtility.CreateTsmi("countChildren", "<countchildren>", "Count children", tsmi_Click, this.SelectedNode.Nodes.Count > 0));


					if(this.CheckBoxes == true)
					{
						cms.Items.Add(ToolstripUtility.CreateTss(null));
						if(bHasExtraDelimiter == false)
						{
							cms.Items.Add(ToolstripUtility.CreateTss(null));
							bHasExtraDelimiter = true;
						}
						cms.Items.Add(ToolstripUtility.CreateTsmi("toggleChildrenTsmi", "<togglechildselection>", "Toggle child selection", tsmi_Click, this.SelectedNode.Nodes.Count > 0));
						cms.Items.Add(ToolstripUtility.CreateTsmi("toggleChildrenRecursivelyTsmi", "<togglechildselectionrecurse>", "Toggle child selection recursively", tsmi_Click, this.SelectedNode.Nodes.Count > 0));

						cms.Items.Add(ToolstripUtility.CreateTsmi("selectChildrenTsmi", "<selectchildren>", "Select chlidren", tsmi_Click, this.SelectedNode.Nodes.Count > 0));
						cms.Items.Add(ToolstripUtility.CreateTsmi("selectChildrenRecursivelyTsmi", "<selectchildrenrecursively>", "Select children recursively", tsmi_Click, this.SelectedNode.Nodes.Count > 0));
						cms.Items.Add(ToolstripUtility.CreateTsmi("deselectChildrenTsmi", "<deselectchildren>", "DeSelect chlidren", tsmi_Click, this.SelectedNode.Nodes.Count > 0));
						cms.Items.Add(ToolstripUtility.CreateTsmi("deselectChildrenRecursivelyTsmi", "<deselectchildrenrecursively>", "DeSelect children recursively", tsmi_Click, this.SelectedNode.Nodes.Count > 0));
					}

					ToolstripUtility.SelectToolStripItemsByFlag(cms.Items, "[DEBUG]", Program.s_bDebugModeEnabled, true);
					ToolstripUtility.SelectToolStripItemsByFlag(cms.Items, "[ADVANCED]", Program.s_bAdvancedModeEnabled, true);
					cms.Show(this, e.X, e.Y);
				}
			}
		}

//		internal ITreeNodeContainer GetNextTreeNode()
		private TreeBaseObject NextVisibleNode(TreeBaseObject tbo)
		{
			TreeBaseObject tboResult = ExecuterTreeUtilities.GetNextVisibleTreeNode(this, tbo as ITreeNodeContainer) as TreeBaseObject;

			return tboResult;
		}


		private void tsmi_Click(object sender, EventArgs e)
		{
			ToolStripMenuItem toolstripMenuItem = sender as ToolStripMenuItem;

			Debug.Assert(toolstripMenuItem != null);

			if(toolstripMenuItem != null)
			{
				string strCommand = ToolstripUtility.GetCommand(toolstripMenuItem);

				switch(strCommand)
				{
					case "moveup":
						(this._treeRClickObjectSelection as IMoveable).MoveUp();
						this.SelectedNode = this._treeRClickObjectSelection;
						break;
					case "movedown":
						(this._treeRClickObjectSelection as IMoveable).MoveDown();
						this.SelectedNode = this._treeRClickObjectSelection;
						break;
					case "cut":
						IMoveable moveable = _treeRClickObjectSelection as IMoveable;

						Debug.Assert(moveable != null);

						if(moveable != null)
						{
							RestoreCutColour();
							SetCutColour(_treeRClickObjectSelection);
							this._dragMoveable = moveable.Cut();
						}

						break;
					case "copy":
						IMoveable moveable2 = _treeRClickObjectSelection as IMoveable;

						Debug.Assert(moveable2 != null);

						if(moveable2 != null)
						{
							RestoreCutColour();
							this._dragMoveable = moveable2.Copy();
						}

						break;
					case "sortchildren":
						this.AsyncSortChildren(this.SelectedNode);
						break;

					case "reload":
						Program.ExecuterMainForm.DoReload(this._treeRClickObjectSelection as TreeBaseObject);
						break;

					case "paste":
						RestoreCutColour();
						(this._treeRClickObjectSelection as ICanReceivePaste).Paste(_dragMoveable);
						break;

					case "pastespecial":
						RestoreCutColour();
						(this._treeRClickObjectSelection as ICanReceivePaste).PasteSpecial(_dragMoveable);

						break;

					case "rename":
						this._treeRClickObjectSelection.BeginEdit();
						break;

					case "expandchildren":
						if(this.SelectedNode.Nodes.Count > 0)
						{
							bool newValue = !this.SelectedNode.Nodes[0].Checked;
							ExpandBranch(this.SelectedNode, true);
						}
						break;

					case "collapsechildren":
						if(this.SelectedNode.Nodes.Count > 0)
						{
							bool newValue = !this.SelectedNode.Nodes[0].Checked;
							ExpandBranch(this.SelectedNode, false);
						}
						break;

					case "countchildren":
						{
							int count = CountChildren(this.SelectedNode);
							MessageBox.Show(this, "The selected node has " + count.ToString() + " child nodes.", "Executer", MessageBoxButtons.OK);
						}
						break;

					case "togglechildselection":
						if(this.SelectedNode.Nodes.Count > 0)
						{
							this.Enabled = false;
							bool newValue = !this.SelectedNode.Nodes[0].Checked;
							foreach(TreeNode tnChild in this.SelectedNode.Nodes)
							{
								SetCheck(tnChild, newValue, false);
							}
							this.Enabled = true;
							ExecuterMainForm.s_treeObjectViewer.ViewMultipleTreeObjects(this);
						}
						break;

					case "togglechildselectionrecurse":
						if(this.SelectedNode.Nodes.Count > 0)
						{
							this.Enabled = false;
							bool newValue = !this.SelectedNode.Nodes[0].Checked;
							foreach(TreeNode tnChild in this.SelectedNode.Nodes)
								SetCheck(tnChild, newValue, true);
							this.Enabled = true;
							ExecuterMainForm.s_treeObjectViewer.ViewMultipleTreeObjects(this);
						}
						break;


					case "selectchildren":
						this.Enabled = false;
						foreach(TreeNode tnChild in this.SelectedNode.Nodes)
							SetCheck(tnChild, true, false);
						this.Enabled = true;
						ExecuterMainForm.s_treeObjectViewer.ViewMultipleTreeObjects(this);
						break;

					case "selectchildrenrecursively":
						this.Enabled = false;
						foreach(TreeNode tnChild in this.SelectedNode.Nodes)
							SetCheck(tnChild, true, true);
						this.Enabled = true;
						ExecuterMainForm.s_treeObjectViewer.ViewMultipleTreeObjects(this);
						break;

					case "deselectchildren":
						this.Enabled = false;
						foreach(TreeNode tnChild in this.SelectedNode.Nodes)
							SetCheck(tnChild, false, false);
						this.Enabled = true;
						ExecuterMainForm.s_treeObjectViewer.ViewMultipleTreeObjects(this);
						break;

					case "deselectchildrenrecursively":
						this.Enabled = false;
						foreach(TreeNode tnChild in this.SelectedNode.Nodes)
							SetCheck(tnChild, false, true);
						this.Enabled = true;
						ExecuterMainForm.s_treeObjectViewer.ViewMultipleTreeObjects(this);
						break;

					case "autorename":
						TreeUserObject tuo = this.SelectedNode as TreeUserObject;

						if(tuo != null)
							tuo.AutoRename();
						break;


					default:
						Debug.Assert(true, "Bad Item Clicked in treeUserObject.ItemClicked.");
						break;
				}
			}
		}

		private int CountChildren(TreeNode treeNode)
		{
			int count = 0;

			foreach(TreeNode treeChild in treeNode.Nodes)
			{
				count += CountChildren(treeChild);
			}
			count += treeNode.Nodes.Count;

			return count;
		}


		//private void cutToolStripMenuItem_Click(object sender, EventArgs e)
		//{
		//    IMoveable moveable = m_treeRClickObjectSelection as IMoveable;

		//    Debug.Assert(moveable != null);

		//    if(moveable != null)
		//    {
		//        RestoreCutColour();
		//        SetCutColour(m_treeRClickObjectSelection);
		//        this.m_dragMoveable = moveable.Cut();
		//    }
		//}

		private void SetCutColour(TreeNode tn)
		{
			_oldCutNode = tn;
			_oldCutColour = tn.BackColor;
			tn.BackColor = Color.Gray;
		}

		public void RestoreCutColour()
		{
			if(_oldCutNode != null)
				_oldCutNode.BackColor = _oldCutColour;
			_oldCutNode = null;
		}

		//private void copyToolStripMenuItem_Click(object sender, EventArgs e)
		//{
		//    IMoveable moveable = m_treeRClickObjectSelection as IMoveable;

		//    Debug.Assert(moveable != null);

		//    if(moveable != null)
		//    {
		//        RestoreCutColour();
		//        this.m_dragMoveable = moveable.Copy();
		//    }
		//}


		//private void pasteToolStripMenuItem_Click(object sender, EventArgs e)
		//{
		//    RestoreCutColour();
		//    (this.m_treeRClickObjectSelection as ICanReceivePaste).Paste(m_dragMoveable);
		//}


		//private void moveUpToolStripMenuItem_Click(object sender, EventArgs e)
		//{
		//    (this.m_treeRClickObjectSelection as IMoveable).MoveUp();
		//    this.SelectedNode = this.m_treeRClickObjectSelection;
		//}

		//private void moveDownToolStripMenuItem_Click(object sender, EventArgs e)
		//{
		//    (this.m_treeRClickObjectSelection as IMoveable).MoveDown();
		//    this.SelectedNode = this.m_treeRClickObjectSelection;
		//}


		//private void renameToolStripMenuItem_Click(object sender, EventArgs e)
		//{
		//    this.m_treeRClickObjectSelection.BeginEdit();
		//}

		//private void expandChildrenToolStripMenuItem_Click(object sender, EventArgs e)
		//{
		//    if(this.SelectedNode.Nodes.Count > 0)
		//    {
		//        bool newValue = !this.SelectedNode.Nodes[0].Checked;
		//        ExpandBranch(this.SelectedNode, true);
		//    }
		//}

		//private void collapseChildrenToolStripMenuItem_Click(object sender, EventArgs e)
		//{
		//    if(this.SelectedNode.Nodes.Count > 0)
		//    {
		//        bool newValue = !this.SelectedNode.Nodes[0].Checked;
		//        ExpandBranch(this.SelectedNode, false);
		//    }
		//}

		private void ExpandBranch(TreeNode treeParent, bool newValue)
		{
			if(newValue == true)
			{
				if(treeParent.IsExpanded != true)
					treeParent.Expand();
			}
			else if(newValue == false)
			{
				if(treeParent.IsExpanded != false)
					treeParent.Collapse();
			}

			foreach(TreeNode toChild in treeParent.Nodes)
			{
				ExpandBranch(toChild, newValue);
			}
		}

		//private void selectChildrenToolStripMenuItem_Click(object sender, EventArgs e)
		//{
		//    if(this.SelectedNode.Nodes.Count > 0)
		//    {
		//        this.Enabled = false;
		//        bool newValue = !this.SelectedNode.Nodes[0].Checked;
		//        foreach(TreeNode tnChild in this.SelectedNode.Nodes)
		//        {
		//            SetCheck(tnChild, newValue, false);
		//        }
		//        this.Enabled = true;
		//        ExecuterMainForm.s_treeObjectViewer.ViewMultipleTreeObjects(this);
		//    }
		//}

		//private void selectChildrenRecursivelyToolStripMenuItem_Click(object sender, EventArgs e)
		//{
		//    if(this.SelectedNode.Nodes.Count > 0)
		//    {
		//        this.Enabled = false;
		//        bool newValue = !this.SelectedNode.Nodes[0].Checked;
		//        foreach(TreeNode tnChild in this.SelectedNode.Nodes)
		//        {
		//            SetCheck(tnChild, newValue, true);
		//        }
		//        this.Enabled = true;
		//    }
		//}

		public void SetCheck(TreeNode tn, bool newValue, bool recurse)
		{
			if(tn.Checked != newValue)
				tn.Checked = newValue;

			if(recurse == true)
			{
				foreach(TreeNode tnChild in tn.Nodes)
				{
					SetCheck(tnChild, newValue, true);
				}
			}
		}

		internal void SaveAll()
		{
			// save everything that supports ISaveable ...
			TreeNodeCollection tnc = this.Nodes;
			Debug.Assert(true, "Bollocks.");
			_SaveAll(tnc);
		}

		private static void _SaveAll(TreeNodeCollection tnc)
		{
			Debug.Assert(true, "Bollocks.");
			foreach(TreeNode tn in tnc)
			{
				if(tn is ISaveable)
				{
					((ISaveable)tn).Save();
				}
				_SaveAll(tn.Nodes);
			}
		}

		//void setDefaultValuesToolStripMenuItem_Click(object sender, System.EventArgs e)
		//{
		//    if(MessageHandler.GetAnswer("Set default values") == DialogResult.No)		// 'No' is forbidden.
		//        MessageHandler.SetAnswer("Set default values", "Ask");

		//    DialogResult dr = MessageHandler.ShowMemory(this, "Set default values", "Set tree nodes to default value without asking?", "Are you sure you want to set this to default values?", "Executer", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
		//    if(dr == DialogResult.Yes)
		//        ExecuterMainForm.s_treeObjectViewer.SetDefaultValues();
		//}

		private void createDefaultTreeToolStripMenuItem_Click(object sender, System.EventArgs e)
		{
			Debug.Assert(false);
		}

		private void dEBUGShowProcessCmdLineToolStripMenuItem_Click(object sender, EventArgs e)
		{
			MessageHandler.Show
				(this,
				 "This option is obselete.\nTo get debug info. you should hold SHIFT when you click the task you want to run.\nThis only works when debug mode is enabled.\nDebug mode is currently enabled.\nSee 'Tools->Options' then 'Debug->Debug task options' for details.",
				 "Executer", MessageBoxButtons.OK);
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="tso"></param>
		/// <param name="strFilename"></param>
		/// <returns></returns>
		internal TreeProjectObject AddOrFindProject(TreeSolutionObject tso, string strFilename)
		{
			List<TreeProjectObject> treeProjectObjects = ExecuterTreeUtilities.FindMatchingProjects(this, strFilename);
			// New. If the project already exists anywhere, mark it as Restricted.
			bool bWritableDuplicateProjectFound = false;
			TreeProjectObject primaryrestrictedProject = null;
			foreach(TreeProjectObject tpoCurrent in treeProjectObjects)
			{
				if(tpoCurrent.StrProjectDocumentFilename.ToLower() == strFilename.ToLower())
				{
					if(tpoCurrent.ReadableState != RestrictedMode.ReadOnly)
						bWritableDuplicateProjectFound = true;

					// There is no need to save a dirty project because the new one will be marked Restricted, not PrimaryRestricted.
					// YES THERE IS!!! BUG!!! THE NEW PROJECT MUST 
					Debug.Assert((tpoCurrent.Parent as TreeSolutionObject) != null);
					// If the duplicated project resides in the target solution, just return that one. Solutions cannot duplicate projects.
					if((tpoCurrent.Parent as TreeSolutionObject) == tso)
					{
						this.SelectedNode = tpoCurrent;
						return tpoCurrent;
					}

					switch(tpoCurrent.ReadableState)
					{
						case RestrictedMode.Writable:
							if(tpoCurrent.IsDirty == true)
							{
								Debug.Assert(primaryrestrictedProject == null);
								primaryrestrictedProject = tpoCurrent;
								tpoCurrent.ReadableState = RestrictedMode.PrimaryRestricted;
							}
							else
								tpoCurrent.ReadableState = RestrictedMode.Restricted;
							break;
						case RestrictedMode.ReadOnly:
							//tpoCurrent.ReadableState = RestrictedMode.Restricted;
							break;
						case RestrictedMode.Restricted:
							//tpoCurrent.ReadableState = RestrictedMode.Restricted;
							break;
						case RestrictedMode.PrimaryRestricted:
							Debug.Assert(primaryrestrictedProject == null);
							primaryrestrictedProject = tpoCurrent;
							//tpoCurrent.ReadableState = RestrictedMode.PrimaryRestricted;
							break;
						default:
							break;
					}
				}
			}

			bool bSolutionIsNew = false;

			if(tso == null)
			{
				bSolutionIsNew = true;
				tso = this.AddNewSolutionNode();
			}
			else
			{
				tso.IsDirty = true;
			}

			TreeProjectObject tpo = tso.AddOrFindProject(strFilename);

			if(tpo == null)
			{
				if(bSolutionIsNew == true)
					tso.Close(CloseMethod.Force);
				return null;
			}
			else
			{
				//Debug.Assert(tpo.Parent == null);
				if(bWritableDuplicateProjectFound == true && tpo.ReadableState != RestrictedMode.ReadOnly)
					tpo.ReadableState = RestrictedMode.Restricted;
				this.SelectedNode = tpo;
				return tpo;
			}
		}

		internal void UpdateReadableState(string strFilename)
		{
			List<TreeProjectObject> treeProjectObjects = ExecuterTreeUtilities.FindMatchingProjects(this, strFilename);

			if(treeProjectObjects.Count == 1)
			{
				switch(treeProjectObjects[0].ReadableState)
				{
					case RestrictedMode.Writable:
						break;
					case RestrictedMode.ReadOnly:
						break;
					case RestrictedMode.Restricted:
						treeProjectObjects[0].ReadableState = RestrictedMode.Writable;
						break;
					case RestrictedMode.PrimaryRestricted:
						treeProjectObjects[0].ReadableState = RestrictedMode.Writable;
						break;
					default:
						break;
				}
			}
			else
			{
				bool bFoundPrimaryRestricted = false;

				foreach(TreeProjectObject tpoCurrent in treeProjectObjects)
				{
					Debug.Assert(tpoCurrent.StrProjectDocumentFilename.ToLower() == strFilename.ToLower());
					Debug.Assert((tpoCurrent.Parent as TreeSolutionObject) != null);

					switch(tpoCurrent.ReadableState)
					{
						case RestrictedMode.Writable:
							Debug.Assert(false);
							tpoCurrent.ReadableState = RestrictedMode.Restricted;
							break;
						case RestrictedMode.ReadOnly:
							break;
						case RestrictedMode.Restricted:
							break;
						case RestrictedMode.PrimaryRestricted:
							Debug.Assert(bFoundPrimaryRestricted == false);
							bFoundPrimaryRestricted = true;
							break;
						default:
							break;
					}
				}
			}
		}

		private void ExecuterTreeView_AfterCheck(object sender, TreeViewEventArgs e)
		{
			Debug.Assert(this.CheckBoxes == true);

			TreeBaseObject treeObject = e.Node as TreeBaseObject;

			if(treeObject != null)
			{
				if(treeObject.Checked == true)
				{
					_CheckedTreeObjects.Add(treeObject);
				}
				else
				{
					_CheckedTreeObjects.Remove(treeObject);
				}

				TreeUserObject tuo = e.Node as TreeUserObject;
				if(tuo != null)
				{
					XmlHelper.AddAttribute(tuo.XTreeNodeMenuData, "checked", treeObject.Checked.ToString().ToLower());
				}


				if(this.Enabled == true)
					//ExecuterMainForm.s_treeObjectViewer.ViewtreeObject(m_lastSelectedNode as IViewableObject);
					ExecuterMainForm.s_treeObjectViewer.ViewMultipleTreeObjects(this);
			}
		}

		public bool ClearStoredCheckedItems()
		{
			_CheckedTreeObjects.Clear();
			return true;
		}

		private void ms_MenuChangedEvent(object sender, MenuSystemLibrary.MenuSystemChangedEventArgs e)
		{
			if(e.XChangedElement != null)
				MessageBox.Show("No.");
		}

		private void ExecuterTreeView_AfterCollapseOrExpand(object sender, TreeViewEventArgs e)
		{
			//bool bExpanded;
			//if(e.Action == TreeViewAction.Expand)
			//    bExpanded = true;
			//else if(e.Action == TreeViewAction.Collapse)
			//    bExpanded = false;
			//else
			//    return;

			//if(e.Node is TreeProjectObject)
			//{
			//    TreeProjectObject tpo = e.Node as TreeProjectObject;
			//    if(tpo != null && ExecuterMainForm.s_optionsManager.GetContainerBoolByName("store_tree_layout", true))
			//    {
			//        XmlHelper.AddAttribute(tpo.m_xMenuProjectDocument.DocumentElement, "expanded_in_ui", bExpanded.ToString());
			//    }
			//}
			//else if(e.Node is TreeUserObject)
			//{
			//    TreeUserObject tuo = e.Node as TreeUserObject;

			//    if(tuo != null && ExecuterMainForm.s_optionsManager.GetContainerBoolByName("store_tree_layout", true))
			//    {
			//        Debug.Assert(tuo.XTreeNodeMenuData.Name == "type_entry" || tuo.XTreeNodeMenuData.Name == "root_type_entry");
			//        XmlHelper.AddAttribute(tuo.XTreeNodeMenuData, "expanded_in_ui", bExpanded.ToString());
			//    }
			//}
		}

		// Implement an "autoscroll" routine for drag
		//  and drop. If the drag cursor moves to the bottom
		//  or top of the treeview, call the Windows API
		//  SendMessage function to scroll up or down automatically.
		private void MyDragScroll(object sender, DragEventArgs e)
		{
			// Set a constant to define the autoscroll region
			const Single scrollRegion = 20;

			// See where the cursor is
			Point pt = this.PointToClient(Cursor.Position);

			// See if we need to scroll up or down
			if((pt.Y + scrollRegion) > this.Height)
			{
				// Call the API to scroll down
				SendMessage(this.Handle, (int)277, (int)1, 0);
			}
			else if(pt.Y < (this.Top + scrollRegion))
			{
				// Call thje API to scroll up
				SendMessage(this.Handle, (int)277, (int)0, 0);
			}
		}

		internal void CancelCut()
		{
			if(_dragMoveable != null)
				if(_dragMoveable.MoveType == MoveType.Cut)
					_dragMoveable = null;
			//m_dragMoveable.MoveType = MoveType.Nothing;
		}

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		private static extern int GetScrollPos(int hWnd, int nBar);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		private static extern int SetScrollPos(int hWnd, int nBar, int nPos, bool bRedraw);

		public event ScrollEventHandler Scroll;

		protected override void WndProc(ref Message m)
		{
			if(Scroll != null)
			{
				switch(m.Msg)
				{
					case WM_VSCROLL:
						{
							ScrollEventType t = (ScrollEventType)Enum.Parse(typeof(ScrollEventType), (m.WParam.ToInt32() & 65535).ToString());
							Scroll(m.HWnd, new ScrollEventArgs(t, ((int)(m.WParam.ToInt64() >> 16)) & 255));
						}
						break;
				}
			}
			base.WndProc(ref m);
		}

		#region Configuration file functions.

		public XmlDocument LoadOrCreateConfigFile()
		{
			_xConfig = new XmlDocument();

			try
			{
				try
				{
					_xConfig.Load(ConfigPathFile);
				}
				catch
				{
					_xConfig.Load(Application.LocalUserAppDataPath + "//config.xml");
				}

				XmlElement xSolutions = _xConfig.SelectSingleNode("root/solutions") as XmlElement;
				if(xSolutions == null)
				{
					xSolutions = _xConfig.CreateElement("solutions");
					_xConfig["root"].AppendChild(xSolutions);
				}

				XmlNodeList xLegacyProjectNodes = _xConfig.SelectNodes("root/projects/project/@filename");

				if(xLegacyProjectNodes.Count != 0)
				{
					// We have an old config. to deal with.
					// Remove obsolete elements, and remove memory dialogs so that the user has a clean list
					// and a clear view that they have a new Executer.

					XmlElement xYnMemory = _xConfig.SelectSingleNode("root/yes_no_memory") as XmlElement;
					if(xYnMemory != null)
						xYnMemory.ParentNode.RemoveChild(xYnMemory);

					XmlElement xMemoryDialogs = _xConfig.SelectSingleNode("root/memory_dialogs") as XmlElement;
					if(xMemoryDialogs != null)
						xMemoryDialogs.InnerXml = ""; // RemoveChild();

					XmlElement xMemoryVariables = _xConfig.SelectSingleNode("root/memory_variables") as XmlElement;
					if(xMemoryVariables != null)
						xMemoryVariables.InnerXml = "";
				}


				MessageHandler.SetMemoryRoot(_xConfig.SelectSingleNode("root/memory_dialogs") as XmlElement);

				MemoryVariable.SetMemoryRoot(_xConfig.SelectSingleNode("root/memory_variables") as XmlElement);
			}
			catch(Exception ex)
			{
				if(ex.GetType() != Type.GetType("System.IO.FileNotFoundException")) // If it's not found, quietly create it.
				{
					UtilityFunctions.ShowError("Cannot load your config. file - Creating a new one.\nSystem report: " + ex.Message);
				}
				Debug.WriteLine(ex.Message);
				try
				{
					_xConfig.Load(new StringReader("<root><solutions/></root>"));
					_xConfig.Save(ConfigPathFile);
				}
				catch(Exception ex2)
				{
					UtilityFunctions.ShowError("Cannot create the config. file. System report:\n" + ex2.Message);
				}
			}

			return _xConfig;
		}

		public bool IsLastConfigEmpty()
		{
			return ((null == (_xConfig.SelectSingleNode("root/solutions/solution/@filename"))) && (null == _xConfig.SelectSingleNode("root/projects/project/@filename")));
		}

		/// <summary>
		/// Load each solution that is listed in the configuration file.
		/// </summary>
		/// <returns>true or false.</returns>
		public bool LoadLastConfiguration()
		{
			try
			{
				string strLastNodePath = null;

				XmlNodeList xLegacyProjectNodes = _xConfig.SelectNodes("root/projects/project/@filename");

				if(xLegacyProjectNodes.Count != 0)
				{
					// We have an old config. to deal with.

					TreeSolutionObject tso = new TreeSolutionObject(this);

					foreach(XmlNode xChild in xLegacyProjectNodes)
					{
						string strProjectName = xChild.Value;
						//TreeProjectObject tpo = new TreeProjectObject(strProjectName);
						//tso.Nodes.Add(tpo);
						//tso.AddOrFindProject(strProjectName);
						this.AddOrFindProject(tso, strProjectName);
					}
					this.Nodes.Add(tso);
					tso.IsDirty = true;
					tso.ImageKey = "TreeSolutionIcon";
					tso.SelectedImageKey = "TreeSolutionIcon";
					_xConfig["root"].RemoveChild(_xConfig.SelectSingleNode("/root/projects"));
				}

				XmlNodeList xSolutionNodes = _xConfig.SelectNodes("root/solutions/solution/@filename"); // Get the filename attributes from any solution element.

				foreach(XmlNode xChild in xSolutionNodes)
				{
					string strSolutionName = xChild.Value;
					if(strSolutionName != null && strSolutionName.Length > 0)
					{
						//TreeSolutionObject tso = new TreeSolutionObject(strSolutionName, this);
						//this.Nodes.Add(tso);
						//tso.ImageKey = "TreeSolutionIcon";
						//tso.SelectedImageKey = "TreeSolutionIcon";

						TreeSolutionObject tso = TreeSolutionObject.AddOrFindSolution(strSolutionName);
					}
				}

				XmlAttribute xLastSelectedNode = _xConfig.SelectSingleNode("root/solutions/last_selected_node/@value") as XmlAttribute;

				if(xLastSelectedNode != null)
				{
					strLastNodePath = xLastSelectedNode.Value;

					try
					{
						this.SelectedNode = GetTreeNodeFromPath(strLastNodePath);
					} // This shouldn't go wrong but I don't care if it does.
					catch
					{
					}
				}
				return true;
			}
			catch(Exception ex)
			{
				UtilityFunctions.ShowError("Failed to load the last configuration. The system error is:" + ex.Message);
			}
			return false;
		}

		public TreeNode GetTreeNodeFromPath(string strPath)
		{
			return GetTreeNodeFromPath(this.Nodes, strPath);
		}


		internal static TreeNode GetTreeNodeFromPath(TreeNode tn, string strPath)
		{
			string[] split = strPath.Split(new Char[] { '\\' }, 2);

			if(split.Length != 0)
			{
				if(tn.Text == split[0])
				{
					if(split.Length == 1)
						return tn;
					else
					{
						return GetTreeNodeFromPath(tn.Nodes, split[1]);
					}
				}
			}
			return null;
		}


		private static TreeNode GetTreeNodeFromPath(TreeNodeCollection tnc, string strPath)
		{
			string[] split = strPath.Split(new Char[] { '\\' });

			TreeNode tn = null;

			foreach(string s in split)
			{
				tn = FindTreeNodeByText(tnc, s);

				if(tn == null)
					return null;
				else
					tnc = tn.Nodes;
			}
			return tn;
		}


		internal TreeBaseObject GetTreeNodeFromPathAndType(ITreeNodeContainer tncRoot, string strTreeNodePath)
		{
			if(tncRoot == null)
				tncRoot = this;

			string[] strSplit = strTreeNodePath.Split(new Char[] { '\\' }, 2);

			if(strSplit.Length != 0)
			{
				string[] strSplitParts = strSplit[0].Split(new Char[] { '|' }, 2);

				string strNodeName = strSplitParts.Length > 0 ? strSplitParts[0] : null;

				string strNodeType = strSplitParts.Length > 1 ? strSplitParts[1] : null;

				TreeNodeCollection children = tncRoot.ChildNodes;

				foreach(ITreeNodeContainer treeChild in children)
				{
					if((treeChild as TreeBaseObject).Text == strNodeName)
					{
						TreeUserObject tuo = (treeChild is TreeUserObject) ? treeChild as TreeUserObject : (treeChild as TreeProjectObject)._rootUserObject;

						string strTypeName = XmlHelper.GetKeyValue(tuo.XTreeNodeMenuData, "type_name");

						if(strNodeType == null || (strTypeName == strNodeType))
						{
							if(strSplit.Length == 1)
							{
								return treeChild as TreeBaseObject;
							}
							else
							{
								return GetTreeNodeFromPathAndType(treeChild, strSplit[1]);
							}
						}
					}
				}
			}
			return null;
		}


		//public TreeUserObject GetTreeNodeFromPathAndType(TreeNode tn, string strPath, string strName, string strType)
		//{
		//    ITreeNodeContainer anchorNode = GetTreeNodeFromPath(tn, strPath) as ITreeNodeContainer;

		//    if(anchorNode != null)
		//    {
		//        SearchDetails sd = new SearchDetails
		//            (
		//            strName,
		//            null,
		//            null,
		//            strType,
		//            null,
		//            anchorNode,
		//            anchorNode,
		//            anchorNode
		//            );

		//        return sd.GetNextTreeNode() as TreeUserObject;
		//    }
		//    return null;
		//}


		private static TreeNode FindTreeNodeByText(TreeNodeCollection tnc, string s)
		{
			foreach(TreeNode tn in tnc)
			{
				if(tn.Text == s)
					return tn;
			}
			return null;
		}


		public bool SaveConfig()
		{
			try
			{
				XmlElement xSolutions = XmlHelper.GetNodeFromPath(_xConfig, "root/solutions"); // FIX1

				if(xSolutions != null)
				{
					xSolutions.RemoveAll(); // Remove them all from the config, then re-add the ones that remain.

					foreach(TreeSolutionObject tso in this.Nodes)
					{
						XmlElement xSolution = _xConfig.CreateElement("solution");
						XmlHelper.AddAttribute(xSolution, "filename", tso.StrPathFileName);
						xSolutions.AppendChild(xSolution);
					}

					if(this.SelectedNode != null)
					{
						XmlElement xLastNode = _xConfig.CreateElement("last_selected_node");
						XmlHelper.AddAttribute(xLastNode, "value", this.SelectedNode.FullPath);
						xSolutions.AppendChild(xLastNode);
					}

					XmlElement xMemoryDialogs = MessageHandler.GetMemoryRoot();

					XmlElement xImportedDialogs = _xConfig.ImportNode(xMemoryDialogs, true) as XmlElement;

					XmlNode xOld = _xConfig["root"][xImportedDialogs.Name];
					if(xOld != null)
						_xConfig["root"].RemoveChild(xOld);

					_xConfig["root"].AppendChild(xImportedDialogs);

					XmlElement xMemoryVariables = MemoryVariable.GetMemoryRoot();

					XmlElement xImportedVariables = _xConfig.ImportNode(xMemoryVariables, true) as XmlElement;

					xOld = _xConfig["root"][xMemoryVariables.Name];
					if(xOld != null)
						_xConfig["root"].RemoveChild(xOld);

					_xConfig["root"].AppendChild(xImportedVariables);


					_xConfig.Save(ConfigPathFile);
					return true;
				}
				else
				{
					UtilityFunctions.ShowError("Your configuration file is corrupt. It does not contain '/root/solutions'.");
					return false;
				}
			}
			catch(Exception ex)
			{
				UtilityFunctions.ShowError("Cannot save the configuration file. System Error:\n" + ex.Message);
				return false;
			}
		}

		#endregion Configuration file functions.
	}
}