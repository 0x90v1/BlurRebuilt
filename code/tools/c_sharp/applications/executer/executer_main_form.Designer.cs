namespace Executer
{
	partial class ExecuterMainForm
	{
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.IContainer components = null;

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		/// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
		protected override void Dispose(bool disposing)
		{
			if(disposing && (components != null))
			{
				components.Dispose();
			}
			base.Dispose(disposing);
		}

		#region Windows Form Designer generated code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.components = new System.ComponentModel.Container();
			System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ExecuterMainForm));
			this.panelMain = new System.Windows.Forms.Panel();
			this.panelMenu = new System.Windows.Forms.Panel();
			this.splitter1 = new System.Windows.Forms.Splitter();
			this.panelTreeView = new System.Windows.Forms.Panel();
			this.progressBar = new System.Windows.Forms.ProgressBar();
			this.labelProgrssBar = new System.Windows.Forms.Label();
			this.imageList1 = new System.Windows.Forms.ImageList(this.components);
			this.notifyIcon1 = new System.Windows.Forms.NotifyIcon(this.components);
			this.contextMenuStripNotifyIcon = new System.Windows.Forms.ContextMenuStrip(this.components);
			this.restoreToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
			this.closeToolStripMenuItem1 = new System.Windows.Forms.ToolStripMenuItem();
			this.menuStrip1 = new System.Windows.Forms.MenuStrip();
			this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.createNewSolutionToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.openSolutionToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
			this.createNewProjectToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.openExistingProjectToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.toolStripSeparator3 = new System.Windows.Forms.ToolStripSeparator();
			this.closeAllSolutionsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.toolStripSeparator4 = new System.Windows.Forms.ToolStripSeparator();
			this.saveAllToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.toolStripSeparator7 = new System.Windows.Forms.ToolStripSeparator();
			this.closeExecuterToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.toolsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.manageMemoryDialogsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.optionsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.jobViewerToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.tsmiCloseAllFinishedTasks = new System.Windows.Forms.ToolStripMenuItem();
			this.toolStripSeparator8 = new System.Windows.Forms.ToolStripSeparator();
			this.useTreeviewCheckboxesToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.autorenameSelectedToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.toolStripSeparator9 = new System.Windows.Forms.ToolStripSeparator();
			this.hideLocalFarmWhenLaunchingToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.toolStripSeparator10 = new System.Windows.Forms.ToolStripSeparator();
			this.advancedModeToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.toolStripSeparator5 = new System.Windows.Forms.ToolStripSeparator();
			this.debugModeToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.toolStripSeparator6 = new System.Windows.Forms.ToolStripSeparator();
			this.toolStripMenuItem1 = new System.Windows.Forms.ToolStripMenuItem();
			this.debugOutputToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.validateMenuTemplatesToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.farmToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.helpToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.wikiPart1ToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.aboutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.statusStrip1 = new System.Windows.Forms.StatusStrip();
			this.toolStripContainer = new System.Windows.Forms.ToolStripContainer();
			this.toolStrip2 = new System.Windows.Forms.ToolStrip();
			this.toolStripButtonSaveAll = new System.Windows.Forms.ToolStripButton();
			this.toolStripButtonLoadProject = new System.Windows.Forms.ToolStripButton();
			this.toolStripButtonFind = new System.Windows.Forms.ToolStripButton();
			this.toolStrip1 = new System.Windows.Forms.ToolStrip();
			this.toolStripLabel1 = new System.Windows.Forms.ToolStripLabel();
			this.toolStripComboBoxSearchFor = new System.Windows.Forms.ToolStripComboBox();
			this.toolStripLabel2 = new System.Windows.Forms.ToolStripLabel();
			this.toolStripComboBoxSearchIn = new System.Windows.Forms.ToolStripComboBox();
			this.toolStripButtonSearch = new System.Windows.Forms.ToolStripButton();
			this.treeViewMain = new Executer.ExecuterTreeView();
			this.panelMain.SuspendLayout();
			this.panelTreeView.SuspendLayout();
			this.contextMenuStripNotifyIcon.SuspendLayout();
			this.menuStrip1.SuspendLayout();
			this.toolStripContainer.ContentPanel.SuspendLayout();
			this.toolStripContainer.TopToolStripPanel.SuspendLayout();
			this.toolStripContainer.SuspendLayout();
			this.toolStrip2.SuspendLayout();
			this.toolStrip1.SuspendLayout();
			this.SuspendLayout();
			// 
			// panelMain
			// 
			this.panelMain.Controls.Add(this.panelMenu);
			this.panelMain.Controls.Add(this.splitter1);
			this.panelMain.Controls.Add(this.panelTreeView);
			this.panelMain.Dock = System.Windows.Forms.DockStyle.Fill;
			this.panelMain.Location = new System.Drawing.Point(0, 0);
			this.panelMain.Name = "panelMain";
			this.panelMain.Size = new System.Drawing.Size(605, 311);
			this.panelMain.TabIndex = 0;
			// 
			// panelMenu
			// 
			this.panelMenu.Dock = System.Windows.Forms.DockStyle.Fill;
			this.panelMenu.Location = new System.Drawing.Point(128, 0);
			this.panelMenu.Name = "panelMenu";
			this.panelMenu.Size = new System.Drawing.Size(477, 311);
			this.panelMenu.TabIndex = 2;
			// 
			// splitter1
			// 
			this.splitter1.Location = new System.Drawing.Point(125, 0);
			this.splitter1.Name = "splitter1";
			this.splitter1.Size = new System.Drawing.Size(3, 311);
			this.splitter1.TabIndex = 1;
			this.splitter1.TabStop = false;
			this.splitter1.SplitterMoved += new System.Windows.Forms.SplitterEventHandler(this.splitter1_SplitterMoved);
			// 
			// panelTreeView
			// 
			this.panelTreeView.Controls.Add(this.treeViewMain);
			this.panelTreeView.Controls.Add(this.progressBar);
			this.panelTreeView.Controls.Add(this.labelProgrssBar);
			this.panelTreeView.Dock = System.Windows.Forms.DockStyle.Left;
			this.panelTreeView.Location = new System.Drawing.Point(0, 0);
			this.panelTreeView.Name = "panelTreeView";
			this.panelTreeView.Size = new System.Drawing.Size(125, 311);
			this.panelTreeView.TabIndex = 3;
			// 
			// progressBar
			// 
			this.progressBar.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.progressBar.Location = new System.Drawing.Point(12, 227);
			this.progressBar.Name = "progressBar";
			this.progressBar.Size = new System.Drawing.Size(99, 23);
			this.progressBar.Style = System.Windows.Forms.ProgressBarStyle.Marquee;
			this.progressBar.TabIndex = 0;
			// 
			// labelProgrssBar
			// 
			this.labelProgrssBar.AutoSize = true;
			this.labelProgrssBar.Location = new System.Drawing.Point(13, 208);
			this.labelProgrssBar.Name = "labelProgrssBar";
			this.labelProgrssBar.Size = new System.Drawing.Size(65, 13);
			this.labelProgrssBar.TabIndex = 1;
			this.labelProgrssBar.Text = "Sorting Tree";
			// 
			// imageList1
			// 
			this.imageList1.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("imageList1.ImageStream")));
			this.imageList1.TransparentColor = System.Drawing.Color.Transparent;
			this.imageList1.Images.SetKeyName(0, "");
			// 
			// notifyIcon1
			// 
			this.notifyIcon1.BalloonTipText = "Here is some text.";
			this.notifyIcon1.BalloonTipTitle = "Menu";
			this.notifyIcon1.ContextMenuStrip = this.contextMenuStripNotifyIcon;
			this.notifyIcon1.Icon = ((System.Drawing.Icon)(resources.GetObject("notifyIcon1.Icon")));
			this.notifyIcon1.Text = "Executer\r\nBizarre Creations";
			this.notifyIcon1.DoubleClick += new System.EventHandler(this.notifyIcon1_DoubleClick);
			// 
			// contextMenuStripNotifyIcon
			// 
			this.contextMenuStripNotifyIcon.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.restoreToolStripMenuItem,
            this.toolStripSeparator1,
            this.closeToolStripMenuItem1});
			this.contextMenuStripNotifyIcon.Name = "contextMenuStripNotifyIcon";
			this.contextMenuStripNotifyIcon.Size = new System.Drawing.Size(124, 54);
			// 
			// restoreToolStripMenuItem
			// 
			this.restoreToolStripMenuItem.Name = "restoreToolStripMenuItem";
			this.restoreToolStripMenuItem.Size = new System.Drawing.Size(123, 22);
			this.restoreToolStripMenuItem.Text = "Restore";
			this.restoreToolStripMenuItem.Click += new System.EventHandler(this.restoreToolStripMenuItem_Click);
			// 
			// toolStripSeparator1
			// 
			this.toolStripSeparator1.Name = "toolStripSeparator1";
			this.toolStripSeparator1.Size = new System.Drawing.Size(120, 6);
			// 
			// closeToolStripMenuItem1
			// 
			this.closeToolStripMenuItem1.Name = "closeToolStripMenuItem1";
			this.closeToolStripMenuItem1.Size = new System.Drawing.Size(123, 22);
			this.closeToolStripMenuItem1.Text = "Close";
			this.closeToolStripMenuItem1.Click += new System.EventHandler(this.closeToolStripMenuItem1_Click);
			// 
			// menuStrip1
			// 
			this.menuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem,
            this.toolsToolStripMenuItem,
            this.helpToolStripMenuItem});
			this.menuStrip1.Location = new System.Drawing.Point(0, 0);
			this.menuStrip1.Name = "menuStrip1";
			this.menuStrip1.ShowItemToolTips = true;
			this.menuStrip1.Size = new System.Drawing.Size(630, 24);
			this.menuStrip1.TabIndex = 1;
			this.menuStrip1.Text = "menuStrip1";
			// 
			// fileToolStripMenuItem
			// 
			this.fileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.createNewSolutionToolStripMenuItem,
            this.openSolutionToolStripMenuItem,
            this.toolStripSeparator2,
            this.createNewProjectToolStripMenuItem,
            this.openExistingProjectToolStripMenuItem,
            this.toolStripSeparator3,
            this.closeAllSolutionsToolStripMenuItem,
            this.toolStripSeparator4,
            this.saveAllToolStripMenuItem,
            this.toolStripSeparator7,
            this.closeExecuterToolStripMenuItem});
			this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
			this.fileToolStripMenuItem.Size = new System.Drawing.Size(35, 20);
			this.fileToolStripMenuItem.Text = "File";
			// 
			// createNewSolutionToolStripMenuItem
			// 
			this.createNewSolutionToolStripMenuItem.Name = "createNewSolutionToolStripMenuItem";
			this.createNewSolutionToolStripMenuItem.Size = new System.Drawing.Size(207, 22);
			this.createNewSolutionToolStripMenuItem.Text = "Create New Solution";
			this.createNewSolutionToolStripMenuItem.Click += new System.EventHandler(this.createNewSolutionToolStripMenuItem_Click);
			// 
			// openSolutionToolStripMenuItem
			// 
			this.openSolutionToolStripMenuItem.Name = "openSolutionToolStripMenuItem";
			this.openSolutionToolStripMenuItem.Size = new System.Drawing.Size(207, 22);
			this.openSolutionToolStripMenuItem.Text = "Open Existing Solution ...";
			this.openSolutionToolStripMenuItem.Click += new System.EventHandler(this.openSolutionToolStripMenuItem_Click_1);
			// 
			// toolStripSeparator2
			// 
			this.toolStripSeparator2.Name = "toolStripSeparator2";
			this.toolStripSeparator2.Size = new System.Drawing.Size(204, 6);
			// 
			// createNewProjectToolStripMenuItem
			// 
			this.createNewProjectToolStripMenuItem.Name = "createNewProjectToolStripMenuItem";
			this.createNewProjectToolStripMenuItem.Size = new System.Drawing.Size(207, 22);
			this.createNewProjectToolStripMenuItem.Text = "Create new Project ...";
			this.createNewProjectToolStripMenuItem.Click += new System.EventHandler(this.createNewProjectToolStripMenuItem_Click_1);
			// 
			// openExistingProjectToolStripMenuItem
			// 
			this.openExistingProjectToolStripMenuItem.Name = "openExistingProjectToolStripMenuItem";
			this.openExistingProjectToolStripMenuItem.Size = new System.Drawing.Size(207, 22);
			this.openExistingProjectToolStripMenuItem.Text = "Open Existing Project ...";
			this.openExistingProjectToolStripMenuItem.Click += new System.EventHandler(this.openExistingProjectToolStripMenuItem_Click);
			// 
			// toolStripSeparator3
			// 
			this.toolStripSeparator3.Name = "toolStripSeparator3";
			this.toolStripSeparator3.Size = new System.Drawing.Size(204, 6);
			// 
			// closeAllSolutionsToolStripMenuItem
			// 
			this.closeAllSolutionsToolStripMenuItem.Name = "closeAllSolutionsToolStripMenuItem";
			this.closeAllSolutionsToolStripMenuItem.Size = new System.Drawing.Size(207, 22);
			this.closeAllSolutionsToolStripMenuItem.Text = "Close all Solutions";
			this.closeAllSolutionsToolStripMenuItem.Click += new System.EventHandler(this.closeAllSolutionsToolStripMenuItem_Click);
			// 
			// toolStripSeparator4
			// 
			this.toolStripSeparator4.Name = "toolStripSeparator4";
			this.toolStripSeparator4.Size = new System.Drawing.Size(204, 6);
			// 
			// saveAllToolStripMenuItem
			// 
			this.saveAllToolStripMenuItem.Name = "saveAllToolStripMenuItem";
			this.saveAllToolStripMenuItem.Size = new System.Drawing.Size(207, 22);
			this.saveAllToolStripMenuItem.Text = "Save All";
			this.saveAllToolStripMenuItem.Click += new System.EventHandler(this.saveAllToolStripMenuItem_Click);
			// 
			// toolStripSeparator7
			// 
			this.toolStripSeparator7.Name = "toolStripSeparator7";
			this.toolStripSeparator7.Size = new System.Drawing.Size(204, 6);
			// 
			// closeExecuterToolStripMenuItem
			// 
			this.closeExecuterToolStripMenuItem.Name = "closeExecuterToolStripMenuItem";
			this.closeExecuterToolStripMenuItem.Size = new System.Drawing.Size(207, 22);
			this.closeExecuterToolStripMenuItem.Text = "Close Executer";
			this.closeExecuterToolStripMenuItem.Click += new System.EventHandler(this.closeExecuterToolStripMenuItem_Click);
			// 
			// toolsToolStripMenuItem
			// 
			this.toolsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.manageMemoryDialogsToolStripMenuItem,
            this.optionsToolStripMenuItem,
            this.jobViewerToolStripMenuItem,
            this.tsmiCloseAllFinishedTasks,
            this.toolStripSeparator8,
            this.useTreeviewCheckboxesToolStripMenuItem,
            this.autorenameSelectedToolStripMenuItem,
            this.toolStripSeparator9,
            this.hideLocalFarmWhenLaunchingToolStripMenuItem,
            this.toolStripSeparator10,
            this.advancedModeToolStripMenuItem,
            this.toolStripSeparator5,
            this.debugModeToolStripMenuItem,
            this.toolStripSeparator6,
            this.toolStripMenuItem1,
            this.debugOutputToolStripMenuItem,
            this.validateMenuTemplatesToolStripMenuItem,
            this.farmToolStripMenuItem});
			this.toolsToolStripMenuItem.Name = "toolsToolStripMenuItem";
			this.toolsToolStripMenuItem.Size = new System.Drawing.Size(44, 20);
			this.toolsToolStripMenuItem.Tag = "[DEBUG]";
			this.toolsToolStripMenuItem.Text = "Tools";
			// 
			// manageMemoryDialogsToolStripMenuItem
			// 
			this.manageMemoryDialogsToolStripMenuItem.Name = "manageMemoryDialogsToolStripMenuItem";
			this.manageMemoryDialogsToolStripMenuItem.Size = new System.Drawing.Size(232, 22);
			this.manageMemoryDialogsToolStripMenuItem.Text = "Manage Memory Dialogs";
			this.manageMemoryDialogsToolStripMenuItem.Click += new System.EventHandler(this.manageMemoryDialogsToolStripMenuItem_Click);
			// 
			// optionsToolStripMenuItem
			// 
			this.optionsToolStripMenuItem.Name = "optionsToolStripMenuItem";
			this.optionsToolStripMenuItem.Size = new System.Drawing.Size(232, 22);
			this.optionsToolStripMenuItem.Tag = "";
			this.optionsToolStripMenuItem.Text = "Options";
			this.optionsToolStripMenuItem.Click += new System.EventHandler(this.configurationToolStripMenuItem_Click);
			// 
			// jobViewerToolStripMenuItem
			// 
			this.jobViewerToolStripMenuItem.Name = "jobViewerToolStripMenuItem";
			this.jobViewerToolStripMenuItem.Size = new System.Drawing.Size(232, 22);
			this.jobViewerToolStripMenuItem.Text = "Job Viewer";
			this.jobViewerToolStripMenuItem.Click += new System.EventHandler(this.jobViewerToolStripMenuItem_Click);
			// 
			// tsmiCloseAllFinishedTasks
			// 
			this.tsmiCloseAllFinishedTasks.Name = "tsmiCloseAllFinishedTasks";
			this.tsmiCloseAllFinishedTasks.Size = new System.Drawing.Size(232, 22);
			this.tsmiCloseAllFinishedTasks.Text = "Close All Finished Tasks";
			this.tsmiCloseAllFinishedTasks.Click += new System.EventHandler(this.tsmiCloseAllFinishedTasks_Click);
			// 
			// toolStripSeparator8
			// 
			this.toolStripSeparator8.Name = "toolStripSeparator8";
			this.toolStripSeparator8.Size = new System.Drawing.Size(229, 6);
			// 
			// useTreeviewCheckboxesToolStripMenuItem
			// 
			this.useTreeviewCheckboxesToolStripMenuItem.Name = "useTreeviewCheckboxesToolStripMenuItem";
			this.useTreeviewCheckboxesToolStripMenuItem.Size = new System.Drawing.Size(232, 22);
			this.useTreeviewCheckboxesToolStripMenuItem.Text = "Multiple Select";
			this.useTreeviewCheckboxesToolStripMenuItem.Click += new System.EventHandler(this.useTreeviewCheckboxesToolStripMenuItem_Click);
			// 
			// autorenameSelectedToolStripMenuItem
			// 
			this.autorenameSelectedToolStripMenuItem.Enabled = false;
			this.autorenameSelectedToolStripMenuItem.Name = "autorenameSelectedToolStripMenuItem";
			this.autorenameSelectedToolStripMenuItem.Size = new System.Drawing.Size(232, 22);
			this.autorenameSelectedToolStripMenuItem.Text = "Auto-rename selected";
			this.autorenameSelectedToolStripMenuItem.Click += new System.EventHandler(this.autorenameSelectedToolStripMenuItem_Click);
			// 
			// toolStripSeparator9
			// 
			this.toolStripSeparator9.Name = "toolStripSeparator9";
			this.toolStripSeparator9.Size = new System.Drawing.Size(229, 6);
			// 
			// hideLocalFarmWhenLaunchingToolStripMenuItem
			// 
			this.hideLocalFarmWhenLaunchingToolStripMenuItem.CheckOnClick = true;
			this.hideLocalFarmWhenLaunchingToolStripMenuItem.Name = "hideLocalFarmWhenLaunchingToolStripMenuItem";
			this.hideLocalFarmWhenLaunchingToolStripMenuItem.Size = new System.Drawing.Size(232, 22);
			this.hideLocalFarmWhenLaunchingToolStripMenuItem.Text = "Hide local farm when launching";
			this.hideLocalFarmWhenLaunchingToolStripMenuItem.Click += new System.EventHandler(this.hideLocalFarmWhenLaunchingToolStripMenuItem_Click);
			// 
			// toolStripSeparator10
			// 
			this.toolStripSeparator10.Name = "toolStripSeparator10";
			this.toolStripSeparator10.Size = new System.Drawing.Size(229, 6);
			// 
			// advancedModeToolStripMenuItem
			// 
			this.advancedModeToolStripMenuItem.CheckOnClick = true;
			this.advancedModeToolStripMenuItem.Name = "advancedModeToolStripMenuItem";
			this.advancedModeToolStripMenuItem.Size = new System.Drawing.Size(232, 22);
			this.advancedModeToolStripMenuItem.Text = "Advanced mode";
			this.advancedModeToolStripMenuItem.Click += new System.EventHandler(this.advancedModeToolStripMenuItem_Click);
			// 
			// toolStripSeparator5
			// 
			this.toolStripSeparator5.Name = "toolStripSeparator5";
			this.toolStripSeparator5.Size = new System.Drawing.Size(229, 6);
			// 
			// debugModeToolStripMenuItem
			// 
			this.debugModeToolStripMenuItem.CheckOnClick = true;
			this.debugModeToolStripMenuItem.Name = "debugModeToolStripMenuItem";
			this.debugModeToolStripMenuItem.Size = new System.Drawing.Size(232, 22);
			this.debugModeToolStripMenuItem.Text = "Debug mode";
			this.debugModeToolStripMenuItem.ToolTipText = "Debug mode (Advanced)";
			this.debugModeToolStripMenuItem.Click += new System.EventHandler(this.debugModeToolStripMenuItem_Click);
			// 
			// toolStripSeparator6
			// 
			this.toolStripSeparator6.Name = "toolStripSeparator6";
			this.toolStripSeparator6.Size = new System.Drawing.Size(229, 6);
			this.toolStripSeparator6.Tag = "[DEBUG]";
			// 
			// toolStripMenuItem1
			// 
			this.toolStripMenuItem1.Font = new System.Drawing.Font("Tahoma", 8.25F, ((System.Drawing.FontStyle)((System.Drawing.FontStyle.Bold | System.Drawing.FontStyle.Italic))), System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.toolStripMenuItem1.Name = "toolStripMenuItem1";
			this.toolStripMenuItem1.Size = new System.Drawing.Size(232, 22);
			this.toolStripMenuItem1.Tag = "[DEBUG]";
			this.toolStripMenuItem1.Text = "Debug Options";
			// 
			// debugOutputToolStripMenuItem
			// 
			this.debugOutputToolStripMenuItem.Name = "debugOutputToolStripMenuItem";
			this.debugOutputToolStripMenuItem.Size = new System.Drawing.Size(232, 22);
			this.debugOutputToolStripMenuItem.Tag = "[DEBUG]";
			this.debugOutputToolStripMenuItem.Text = "Debug output";
			this.debugOutputToolStripMenuItem.Click += new System.EventHandler(this.debugOutputToolStripMenuItem_Click);
			// 
			// validateMenuTemplatesToolStripMenuItem
			// 
			this.validateMenuTemplatesToolStripMenuItem.CheckOnClick = true;
			this.validateMenuTemplatesToolStripMenuItem.Name = "validateMenuTemplatesToolStripMenuItem";
			this.validateMenuTemplatesToolStripMenuItem.Size = new System.Drawing.Size(232, 22);
			this.validateMenuTemplatesToolStripMenuItem.Tag = "[DEBUG]";
			this.validateMenuTemplatesToolStripMenuItem.Text = "Validate menu templates";
			this.validateMenuTemplatesToolStripMenuItem.Click += new System.EventHandler(this.validateMenuTemplatesToolStripMenuItem_Click);
			// 
			// farmToolStripMenuItem
			// 
			this.farmToolStripMenuItem.Name = "farmToolStripMenuItem";
			this.farmToolStripMenuItem.Size = new System.Drawing.Size(232, 22);
			this.farmToolStripMenuItem.Tag = "[DEBUG]";
			this.farmToolStripMenuItem.Text = "Farm";
			this.farmToolStripMenuItem.Click += new System.EventHandler(this.farmToolStripMenuItem_Click);
			// 
			// helpToolStripMenuItem
			// 
			this.helpToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.wikiPart1ToolStripMenuItem,
            this.aboutToolStripMenuItem});
			this.helpToolStripMenuItem.Name = "helpToolStripMenuItem";
			this.helpToolStripMenuItem.Size = new System.Drawing.Size(40, 20);
			this.helpToolStripMenuItem.Text = "Help";
			// 
			// wikiPart1ToolStripMenuItem
			// 
			this.wikiPart1ToolStripMenuItem.Name = "wikiPart1ToolStripMenuItem";
			this.wikiPart1ToolStripMenuItem.Size = new System.Drawing.Size(114, 22);
			this.wikiPart1ToolStripMenuItem.Text = "Wiki";
			this.wikiPart1ToolStripMenuItem.Click += new System.EventHandler(this.wikiPart1ToolStripMenuItem_Click);
			// 
			// aboutToolStripMenuItem
			// 
			this.aboutToolStripMenuItem.Name = "aboutToolStripMenuItem";
			this.aboutToolStripMenuItem.Size = new System.Drawing.Size(114, 22);
			this.aboutToolStripMenuItem.Text = "About";
			this.aboutToolStripMenuItem.Click += new System.EventHandler(this.aboutToolStripMenuItem_Click);
			// 
			// statusStrip1
			// 
			this.statusStrip1.Location = new System.Drawing.Point(0, 360);
			this.statusStrip1.Name = "statusStrip1";
			this.statusStrip1.Size = new System.Drawing.Size(630, 22);
			this.statusStrip1.TabIndex = 3;
			this.statusStrip1.Text = "statusStrip1";
			// 
			// toolStripContainer
			// 
			// 
			// toolStripContainer.ContentPanel
			// 
			this.toolStripContainer.ContentPanel.Controls.Add(this.panelMain);
			this.toolStripContainer.ContentPanel.Size = new System.Drawing.Size(605, 311);
			this.toolStripContainer.Dock = System.Windows.Forms.DockStyle.Fill;
			this.toolStripContainer.Location = new System.Drawing.Point(0, 24);
			this.toolStripContainer.Name = "toolStripContainer";
			this.toolStripContainer.Size = new System.Drawing.Size(630, 336);
			this.toolStripContainer.TabIndex = 4;
			this.toolStripContainer.Text = "toolStripContainer1";
			// 
			// toolStripContainer.TopToolStripPanel
			// 
			this.toolStripContainer.TopToolStripPanel.Controls.Add(this.toolStrip2);
			this.toolStripContainer.TopToolStripPanel.Controls.Add(this.toolStrip1);
			// 
			// toolStrip2
			// 
			this.toolStrip2.Dock = System.Windows.Forms.DockStyle.None;
			this.toolStrip2.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripButtonSaveAll,
            this.toolStripButtonLoadProject,
            this.toolStripButtonFind});
			this.toolStrip2.Location = new System.Drawing.Point(3, 0);
			this.toolStrip2.Name = "toolStrip2";
			this.toolStrip2.Size = new System.Drawing.Size(81, 25);
			this.toolStrip2.TabIndex = 1;
			// 
			// toolStripButtonSaveAll
			// 
			this.toolStripButtonSaveAll.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
			this.toolStripButtonSaveAll.Image = ((System.Drawing.Image)(resources.GetObject("toolStripButtonSaveAll.Image")));
			this.toolStripButtonSaveAll.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.toolStripButtonSaveAll.Name = "toolStripButtonSaveAll";
			this.toolStripButtonSaveAll.Size = new System.Drawing.Size(23, 22);
			this.toolStripButtonSaveAll.Text = "Save All";
			this.toolStripButtonSaveAll.Click += new System.EventHandler(this.toolStripButtonSaveAll_Click);
			// 
			// toolStripButtonLoadProject
			// 
			this.toolStripButtonLoadProject.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
			this.toolStripButtonLoadProject.Image = ((System.Drawing.Image)(resources.GetObject("toolStripButtonLoadProject.Image")));
			this.toolStripButtonLoadProject.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.toolStripButtonLoadProject.Name = "toolStripButtonLoadProject";
			this.toolStripButtonLoadProject.Size = new System.Drawing.Size(23, 22);
			this.toolStripButtonLoadProject.Text = "Load Project ...";
			this.toolStripButtonLoadProject.Click += new System.EventHandler(this.toolStripButtonLoadProject_Click);
			// 
			// toolStripButtonFind
			// 
			this.toolStripButtonFind.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
			this.toolStripButtonFind.Image = global::Executer.Properties.Resources.Search;
			this.toolStripButtonFind.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.toolStripButtonFind.Name = "toolStripButtonFind";
			this.toolStripButtonFind.Size = new System.Drawing.Size(23, 22);
			this.toolStripButtonFind.Text = "Find";
			this.toolStripButtonFind.ToolTipText = "Search ...";
			this.toolStripButtonFind.Click += new System.EventHandler(this.toolStripButtonFind_Click);
			// 
			// toolStrip1
			// 
			this.toolStrip1.Dock = System.Windows.Forms.DockStyle.None;
			this.toolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripLabel1,
            this.toolStripComboBoxSearchFor,
            this.toolStripLabel2,
            this.toolStripComboBoxSearchIn,
            this.toolStripButtonSearch});
			this.toolStrip1.Location = new System.Drawing.Point(84, 0);
			this.toolStrip1.Name = "toolStrip1";
			this.toolStrip1.Size = new System.Drawing.Size(384, 25);
			this.toolStrip1.TabIndex = 0;
			// 
			// toolStripLabel1
			// 
			this.toolStripLabel1.Name = "toolStripLabel1";
			this.toolStripLabel1.Size = new System.Drawing.Size(57, 22);
			this.toolStripLabel1.Text = "Search for";
			// 
			// toolStripComboBoxSearchFor
			// 
			this.toolStripComboBoxSearchFor.Name = "toolStripComboBoxSearchFor";
			this.toolStripComboBoxSearchFor.Size = new System.Drawing.Size(150, 25);
			this.toolStripComboBoxSearchFor.Leave += new System.EventHandler(this.toolStripComboBoxSearchFor_Leave);
			// 
			// toolStripLabel2
			// 
			this.toolStripLabel2.Name = "toolStripLabel2";
			this.toolStripLabel2.Size = new System.Drawing.Size(17, 22);
			this.toolStripLabel2.Text = "In";
			// 
			// toolStripComboBoxSearchIn
			// 
			this.toolStripComboBoxSearchIn.Items.AddRange(new object[] {
            "Node name",
            "Container name",
            "Container value",
            "Type name",
            "Internal name"});
			this.toolStripComboBoxSearchIn.Name = "toolStripComboBoxSearchIn";
			this.toolStripComboBoxSearchIn.Size = new System.Drawing.Size(121, 25);
			// 
			// toolStripButtonSearch
			// 
			this.toolStripButtonSearch.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
			this.toolStripButtonSearch.Image = global::Executer.Properties.Resources.iconQuickSearch;
			this.toolStripButtonSearch.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.toolStripButtonSearch.Name = "toolStripButtonSearch";
			this.toolStripButtonSearch.Size = new System.Drawing.Size(23, 22);
			this.toolStripButtonSearch.Text = "Quick search";
			this.toolStripButtonSearch.Click += new System.EventHandler(this.toolStripButtonSearch_Click_1);
			// 
			// treeViewMain
			// 
			this.treeViewMain.AllowDrop = true;
			this.treeViewMain.ApplicationPath = resources.GetString("treeViewMain.ApplicationPath");
			this.treeViewMain.Dock = System.Windows.Forms.DockStyle.Fill;
			this.treeViewMain.HideSelection = false;
			this.treeViewMain.HScrollPos = 0;
			this.treeViewMain.ImageIndex = 0;
			this.treeViewMain.IsDirty = false;
			this.treeViewMain.LabelEdit = true;
			this.treeViewMain.Location = new System.Drawing.Point(0, 0);
			this.treeViewMain.Name = "treeViewMain";
			this.treeViewMain.SelectedImageIndex = 0;
			this.treeViewMain.ShowNodeToolTips = true;
			this.treeViewMain.Size = new System.Drawing.Size(125, 311);
			this.treeViewMain.TabIndex = 0;
			this.treeViewMain.VScrollPos = 0;
			// 
			// ExecuterMainForm
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(630, 382);
			this.Controls.Add(this.toolStripContainer);
			this.Controls.Add(this.menuStrip1);
			this.Controls.Add(this.statusStrip1);
			this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
			this.MainMenuStrip = this.menuStrip1;
			this.Name = "ExecuterMainForm";
			this.ShowInTaskbar = false;
			this.Text = "Executer";
			this.Load += new System.EventHandler(this.Executer_MainForm_Load);
			this.Move += new System.EventHandler(this.Executer_MainForm_Move);
			this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.Executer_MainForm_FormClosing);
			this.Resize += new System.EventHandler(this.Executer_MainForm_Resize);
			this.panelMain.ResumeLayout(false);
			this.panelTreeView.ResumeLayout(false);
			this.panelTreeView.PerformLayout();
			this.contextMenuStripNotifyIcon.ResumeLayout(false);
			this.menuStrip1.ResumeLayout(false);
			this.menuStrip1.PerformLayout();
			this.toolStripContainer.ContentPanel.ResumeLayout(false);
			this.toolStripContainer.TopToolStripPanel.ResumeLayout(false);
			this.toolStripContainer.TopToolStripPanel.PerformLayout();
			this.toolStripContainer.ResumeLayout(false);
			this.toolStripContainer.PerformLayout();
			this.toolStrip2.ResumeLayout(false);
			this.toolStrip2.PerformLayout();
			this.toolStrip1.ResumeLayout(false);
			this.toolStrip1.PerformLayout();
			this.ResumeLayout(false);
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.Panel panelMain;
		private System.Windows.Forms.Splitter splitter1;
		private ExecuterTreeView treeViewMain;
		private System.Windows.Forms.Panel panelMenu;
		private System.Windows.Forms.ImageList imageList1;
		private System.Windows.Forms.NotifyIcon notifyIcon1;
		private System.Windows.Forms.ToolStripMenuItem closeToolStripMenuItem1;
		private System.Windows.Forms.ContextMenuStrip contextMenuStripNotifyIcon;
		private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
		private System.Windows.Forms.ToolStripMenuItem restoreToolStripMenuItem;
		private System.Windows.Forms.MenuStrip menuStrip1;
		private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
		private System.Windows.Forms.ToolStripMenuItem toolsToolStripMenuItem;
		private System.Windows.Forms.ToolStripMenuItem manageMemoryDialogsToolStripMenuItem;
		private System.Windows.Forms.ToolStripMenuItem debugModeToolStripMenuItem;
		private System.Windows.Forms.ToolStripMenuItem helpToolStripMenuItem;
		private System.Windows.Forms.ToolStripMenuItem aboutToolStripMenuItem;
		private System.Windows.Forms.StatusStrip statusStrip1;
		private System.Windows.Forms.ToolStripMenuItem farmToolStripMenuItem;
		private System.Windows.Forms.ToolStripMenuItem debugOutputToolStripMenuItem;
		private System.Windows.Forms.ToolStripMenuItem optionsToolStripMenuItem;
		private System.Windows.Forms.ToolStripMenuItem validateMenuTemplatesToolStripMenuItem;
		private System.Windows.Forms.ToolStripMenuItem jobViewerToolStripMenuItem;
		private System.Windows.Forms.ToolStripSeparator toolStripSeparator5;
		private System.Windows.Forms.ToolStripSeparator toolStripSeparator6;
		private System.Windows.Forms.ToolStripMenuItem hideLocalFarmWhenLaunchingToolStripMenuItem;
		private System.Windows.Forms.ToolStripMenuItem tsmiCloseAllFinishedTasks;
		private System.Windows.Forms.ToolStripContainer toolStripContainer;
		private System.Windows.Forms.Panel panelTreeView;
		private System.Windows.Forms.ProgressBar progressBar;
		private System.Windows.Forms.Label labelProgrssBar;
		private System.Windows.Forms.ToolStripMenuItem useTreeviewCheckboxesToolStripMenuItem;
		private System.Windows.Forms.ToolStripMenuItem createNewSolutionToolStripMenuItem;
		private System.Windows.Forms.ToolStripMenuItem openSolutionToolStripMenuItem;
		private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
		private System.Windows.Forms.ToolStripMenuItem createNewProjectToolStripMenuItem;
		private System.Windows.Forms.ToolStripMenuItem openExistingProjectToolStripMenuItem;
		private System.Windows.Forms.ToolStripSeparator toolStripSeparator3;
		private System.Windows.Forms.ToolStripMenuItem closeAllSolutionsToolStripMenuItem;
		private System.Windows.Forms.ToolStripMenuItem closeExecuterToolStripMenuItem;
		private System.Windows.Forms.ToolStripSeparator toolStripSeparator4;
		private System.Windows.Forms.ToolStripMenuItem saveAllToolStripMenuItem;
		private System.Windows.Forms.ToolStripSeparator toolStripSeparator7;
		private System.Windows.Forms.ToolStrip toolStrip1;
		private System.Windows.Forms.ToolStripLabel toolStripLabel1;
		private System.Windows.Forms.ToolStripComboBox toolStripComboBoxSearchFor;
		private System.Windows.Forms.ToolStripLabel toolStripLabel2;
		private System.Windows.Forms.ToolStripButton toolStripButtonSearch;
		private System.Windows.Forms.ToolStripComboBox toolStripComboBoxSearchIn;
		private System.Windows.Forms.ToolStripMenuItem toolStripMenuItem1;
		private System.Windows.Forms.ToolStripMenuItem advancedModeToolStripMenuItem;
		private System.Windows.Forms.ToolStrip toolStrip2;
		private System.Windows.Forms.ToolStripButton toolStripButtonSaveAll;
		private System.Windows.Forms.ToolStripButton toolStripButtonLoadProject;
		private System.Windows.Forms.ToolStripButton toolStripButtonFind;
		private System.Windows.Forms.ToolStripMenuItem autorenameSelectedToolStripMenuItem;
		private System.Windows.Forms.ToolStripSeparator toolStripSeparator8;
		private System.Windows.Forms.ToolStripSeparator toolStripSeparator9;
		private System.Windows.Forms.ToolStripSeparator toolStripSeparator10;
		private System.Windows.Forms.ToolStripMenuItem wikiPart1ToolStripMenuItem;


	}
}

