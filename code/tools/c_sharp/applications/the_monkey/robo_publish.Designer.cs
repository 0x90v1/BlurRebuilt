namespace RoboPublish
{
	partial class RoboPanel
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

		#region Component Designer generated code

		/// <summary> 
		/// Required method for Designer support - do not modify 
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.components = new System.ComponentModel.Container();
			System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(RoboPanel));
			this.toolStripContainer1 = new System.Windows.Forms.ToolStripContainer();
			this.toolStrip2 = new System.Windows.Forms.ToolStrip();
			this.toolStripButtonConfigureSorcery = new System.Windows.Forms.ToolStripButton();
			this.toolStripButtonSorceryQuery = new System.Windows.Forms.ToolStripButton();
			this.panelParseProgress = new System.Windows.Forms.Panel();
			this.progressBar1 = new System.Windows.Forms.ProgressBar();
			this.labelPleaseWait = new System.Windows.Forms.Label();
			this.labelParseProgress = new System.Windows.Forms.Label();
			this.panel_Feedback = new System.Windows.Forms.Panel();
			this.groupBox_ThreadFeedback = new System.Windows.Forms.GroupBox();
			this.buttonOk = new System.Windows.Forms.Button();
			this.buttonAbort = new System.Windows.Forms.Button();
			this.progressBar_Publish = new System.Windows.Forms.ProgressBar();
			this.richTextBox_Publish = new System.Windows.Forms.RichTextBox();
			this.toolStrip1 = new System.Windows.Forms.ToolStrip();
			this.toolStripButton_SourceNewerOrDestMissing = new System.Windows.Forms.ToolStripButton();
			this.toolStripButton1_SelectDestMissing = new System.Windows.Forms.ToolStripButton();
			this.toolStripButton_FilesAreEqual = new System.Windows.Forms.ToolStripButton();
			this.toolStripButton_DestFileNewer = new System.Windows.Forms.ToolStripButton();
			this.toolStripButton_SelectAll = new System.Windows.Forms.ToolStripButton();
			this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
			this.toolStripButton_ViewChecked = new System.Windows.Forms.ToolStripButton();
			this.toolStripButtonDeleteSelection = new System.Windows.Forms.ToolStripButton();
			this.toolStripSeparator3 = new System.Windows.Forms.ToolStripSeparator();
			this.toolStripDropDownButtonFilterSKU = new System.Windows.Forms.ToolStripDropDownButton();
			this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
			this.toolStripButtonClose = new System.Windows.Forms.ToolStripButton();
			this.labelName = new System.Windows.Forms.Label();
			this.panelHeading = new System.Windows.Forms.Panel();
			this.groupBox1 = new System.Windows.Forms.GroupBox();
			this.buttonPublish = new System.Windows.Forms.Button();
			this.buttonRefresh = new System.Windows.Forms.Button();
			this.roboTree = new RoboPublish.RoboTree();
			this.toolStripContainer1.BottomToolStripPanel.SuspendLayout();
			this.toolStripContainer1.ContentPanel.SuspendLayout();
			this.toolStripContainer1.TopToolStripPanel.SuspendLayout();
			this.toolStripContainer1.SuspendLayout();
			this.toolStrip2.SuspendLayout();
			this.panelParseProgress.SuspendLayout();
			this.panel_Feedback.SuspendLayout();
			this.groupBox_ThreadFeedback.SuspendLayout();
			this.toolStrip1.SuspendLayout();
			this.panelHeading.SuspendLayout();
			this.groupBox1.SuspendLayout();
			this.SuspendLayout();
			// 
			// toolStripContainer1
			// 
			this.toolStripContainer1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
						| System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			// 
			// toolStripContainer1.BottomToolStripPanel
			// 
			this.toolStripContainer1.BottomToolStripPanel.Controls.Add(this.toolStrip2);
			// 
			// toolStripContainer1.ContentPanel
			// 
			this.toolStripContainer1.ContentPanel.Controls.Add(this.panelParseProgress);
			this.toolStripContainer1.ContentPanel.Controls.Add(this.panel_Feedback);
			this.toolStripContainer1.ContentPanel.Controls.Add(this.roboTree);
			this.toolStripContainer1.ContentPanel.Size = new System.Drawing.Size(484, 383);
			this.toolStripContainer1.Location = new System.Drawing.Point(6, 45);
			this.toolStripContainer1.Name = "toolStripContainer1";
			this.toolStripContainer1.Size = new System.Drawing.Size(484, 433);
			this.toolStripContainer1.TabIndex = 1;
			this.toolStripContainer1.Text = "toolStripContainer1";
			// 
			// toolStripContainer1.TopToolStripPanel
			// 
			this.toolStripContainer1.TopToolStripPanel.Controls.Add(this.toolStrip1);
			// 
			// toolStrip2
			// 
			this.toolStrip2.AllowItemReorder = true;
			this.toolStrip2.Dock = System.Windows.Forms.DockStyle.None;
			this.toolStrip2.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripButtonConfigureSorcery,
            this.toolStripButtonSorceryQuery});
			this.toolStrip2.Location = new System.Drawing.Point(3, 0);
			this.toolStrip2.Name = "toolStrip2";
			this.toolStrip2.Size = new System.Drawing.Size(222, 25);
			this.toolStrip2.TabIndex = 6;
			this.toolStrip2.Text = "Sorcery";
			this.toolStrip2.Visible = false;
			// 
			// toolStripButtonConfigureSorcery
			// 
			this.toolStripButtonConfigureSorcery.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
			this.toolStripButtonConfigureSorcery.Image = ((System.Drawing.Image)(resources.GetObject("toolStripButtonConfigureSorcery.Image")));
			this.toolStripButtonConfigureSorcery.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.toolStripButtonConfigureSorcery.Name = "toolStripButtonConfigureSorcery";
			this.toolStripButtonConfigureSorcery.Size = new System.Drawing.Size(98, 22);
			this.toolStripButtonConfigureSorcery.Text = "Configure Sorcery";
			this.toolStripButtonConfigureSorcery.Click += new System.EventHandler(this.toolStripButtonConfigureSorcery_Click);
			// 
			// toolStripButtonSorceryQuery
			// 
			this.toolStripButtonSorceryQuery.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
			this.toolStripButtonSorceryQuery.Image = ((System.Drawing.Image)(resources.GetObject("toolStripButtonSorceryQuery.Image")));
			this.toolStripButtonSorceryQuery.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.toolStripButtonSorceryQuery.Name = "toolStripButtonSorceryQuery";
			this.toolStripButtonSorceryQuery.Size = new System.Drawing.Size(81, 22);
			this.toolStripButtonSorceryQuery.Text = "Sorcery Query";
			this.toolStripButtonSorceryQuery.Click += new System.EventHandler(this.toolStripButtonSorceryQuery_Click);
			// 
			// panelParseProgress
			// 
			this.panelParseProgress.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
						| System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.panelParseProgress.BackColor = System.Drawing.SystemColors.ControlDarkDark;
			this.panelParseProgress.Controls.Add(this.progressBar1);
			this.panelParseProgress.Controls.Add(this.labelPleaseWait);
			this.panelParseProgress.Controls.Add(this.labelParseProgress);
			this.panelParseProgress.Location = new System.Drawing.Point(20, 189);
			this.panelParseProgress.Name = "panelParseProgress";
			this.panelParseProgress.Size = new System.Drawing.Size(211, 92);
			this.panelParseProgress.TabIndex = 2;
			// 
			// progressBar1
			// 
			this.progressBar1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.progressBar1.Location = new System.Drawing.Point(3, 48);
			this.progressBar1.Name = "progressBar1";
			this.progressBar1.Size = new System.Drawing.Size(205, 23);
			this.progressBar1.Style = System.Windows.Forms.ProgressBarStyle.Marquee;
			this.progressBar1.TabIndex = 2;
			// 
			// labelPleaseWait
			// 
			this.labelPleaseWait.AutoSize = true;
			this.labelPleaseWait.Location = new System.Drawing.Point(84, 16);
			this.labelPleaseWait.Name = "labelPleaseWait";
			this.labelPleaseWait.Size = new System.Drawing.Size(73, 13);
			this.labelPleaseWait.TabIndex = 1;
			this.labelPleaseWait.Text = "Please wait ...";
			// 
			// labelParseProgress
			// 
			this.labelParseProgress.AutoSize = true;
			this.labelParseProgress.Location = new System.Drawing.Point(81, 32);
			this.labelParseProgress.Name = "labelParseProgress";
			this.labelParseProgress.Size = new System.Drawing.Size(0, 13);
			this.labelParseProgress.TabIndex = 0;
			// 
			// panel_Feedback
			// 
			this.panel_Feedback.Controls.Add(this.groupBox_ThreadFeedback);
			this.panel_Feedback.Location = new System.Drawing.Point(14, 41);
			this.panel_Feedback.Name = "panel_Feedback";
			this.panel_Feedback.Size = new System.Drawing.Size(273, 126);
			this.panel_Feedback.TabIndex = 1;
			this.panel_Feedback.Visible = false;
			// 
			// groupBox_ThreadFeedback
			// 
			this.groupBox_ThreadFeedback.Controls.Add(this.buttonOk);
			this.groupBox_ThreadFeedback.Controls.Add(this.buttonAbort);
			this.groupBox_ThreadFeedback.Controls.Add(this.progressBar_Publish);
			this.groupBox_ThreadFeedback.Controls.Add(this.richTextBox_Publish);
			this.groupBox_ThreadFeedback.Dock = System.Windows.Forms.DockStyle.Fill;
			this.groupBox_ThreadFeedback.Location = new System.Drawing.Point(0, 0);
			this.groupBox_ThreadFeedback.Name = "groupBox_ThreadFeedback";
			this.groupBox_ThreadFeedback.Size = new System.Drawing.Size(273, 126);
			this.groupBox_ThreadFeedback.TabIndex = 1;
			this.groupBox_ThreadFeedback.TabStop = false;
			this.groupBox_ThreadFeedback.Text = "Copying ...";
			// 
			// buttonOk
			// 
			this.buttonOk.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonOk.Location = new System.Drawing.Point(111, 97);
			this.buttonOk.Name = "buttonOk";
			this.buttonOk.Size = new System.Drawing.Size(75, 23);
			this.buttonOk.TabIndex = 3;
			this.buttonOk.Text = "OK";
			this.buttonOk.UseVisualStyleBackColor = true;
			this.buttonOk.Click += new System.EventHandler(this.buttonOk_Click);
			// 
			// buttonAbort
			// 
			this.buttonAbort.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonAbort.Location = new System.Drawing.Point(192, 97);
			this.buttonAbort.Name = "buttonAbort";
			this.buttonAbort.Size = new System.Drawing.Size(75, 23);
			this.buttonAbort.TabIndex = 2;
			this.buttonAbort.Text = "Abort";
			this.buttonAbort.UseVisualStyleBackColor = true;
			this.buttonAbort.Click += new System.EventHandler(this.buttonAbort_Click);
			// 
			// progressBar_Publish
			// 
			this.progressBar_Publish.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.progressBar_Publish.Location = new System.Drawing.Point(6, 68);
			this.progressBar_Publish.Name = "progressBar_Publish";
			this.progressBar_Publish.Size = new System.Drawing.Size(261, 23);
			this.progressBar_Publish.TabIndex = 1;
			// 
			// richTextBox_Publish
			// 
			this.richTextBox_Publish.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
						| System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.richTextBox_Publish.BackColor = System.Drawing.SystemColors.Control;
			this.richTextBox_Publish.Location = new System.Drawing.Point(6, 19);
			this.richTextBox_Publish.Name = "richTextBox_Publish";
			this.richTextBox_Publish.ReadOnly = true;
			this.richTextBox_Publish.Size = new System.Drawing.Size(261, 43);
			this.richTextBox_Publish.TabIndex = 0;
			this.richTextBox_Publish.Text = "";
			// 
			// toolStrip1
			// 
			this.toolStrip1.Dock = System.Windows.Forms.DockStyle.None;
			this.toolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripButton_SourceNewerOrDestMissing,
            this.toolStripButton1_SelectDestMissing,
            this.toolStripButton_FilesAreEqual,
            this.toolStripButton_DestFileNewer,
            this.toolStripButton_SelectAll,
            this.toolStripSeparator1,
            this.toolStripButton_ViewChecked,
            this.toolStripButtonDeleteSelection,
            this.toolStripSeparator3,
            this.toolStripDropDownButtonFilterSKU,
            this.toolStripSeparator2,
            this.toolStripButtonClose});
			this.toolStrip1.Location = new System.Drawing.Point(3, 0);
			this.toolStrip1.Name = "toolStrip1";
			this.toolStrip1.Size = new System.Drawing.Size(413, 25);
			this.toolStrip1.TabIndex = 1;
			this.toolStrip1.Text = "toolStrip1";
			// 
			// toolStripButton_SourceNewerOrDestMissing
			// 
			this.toolStripButton_SourceNewerOrDestMissing.CheckOnClick = true;
			this.toolStripButton_SourceNewerOrDestMissing.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
			this.toolStripButton_SourceNewerOrDestMissing.Image = global::RoboPublish.Properties.Resources.SourceNewerOrDestMissing;
			this.toolStripButton_SourceNewerOrDestMissing.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.toolStripButton_SourceNewerOrDestMissing.Name = "toolStripButton_SourceNewerOrDestMissing";
			this.toolStripButton_SourceNewerOrDestMissing.Size = new System.Drawing.Size(23, 22);
			this.toolStripButton_SourceNewerOrDestMissing.ToolTipText = "Select all files on or below the current tree node where:\r\nLocal file is newest\r\n" +
				"or\r\nRemote file is missing.\r\nUse this to select new work to publish.";
			this.toolStripButton_SourceNewerOrDestMissing.Click += new System.EventHandler(this.toolStripButton_SourceNewerOrDestMissing_Click);
			// 
			// toolStripButton1_SelectDestMissing
			// 
			this.toolStripButton1_SelectDestMissing.CheckOnClick = true;
			this.toolStripButton1_SelectDestMissing.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
			this.toolStripButton1_SelectDestMissing.Image = global::RoboPublish.Properties.Resources.DestFileMissing;
			this.toolStripButton1_SelectDestMissing.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.toolStripButton1_SelectDestMissing.Name = "toolStripButton1_SelectDestMissing";
			this.toolStripButton1_SelectDestMissing.Size = new System.Drawing.Size(23, 22);
			this.toolStripButton1_SelectDestMissing.ToolTipText = "Select all files on or below the current tree node where\r\nthe remote file does no" +
				"t exist.";
			this.toolStripButton1_SelectDestMissing.Click += new System.EventHandler(this.toolStripButton1_SelectDestMissing_Click);
			// 
			// toolStripButton_FilesAreEqual
			// 
			this.toolStripButton_FilesAreEqual.CheckOnClick = true;
			this.toolStripButton_FilesAreEqual.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
			this.toolStripButton_FilesAreEqual.Image = global::RoboPublish.Properties.Resources.FilesEqual;
			this.toolStripButton_FilesAreEqual.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.toolStripButton_FilesAreEqual.Name = "toolStripButton_FilesAreEqual";
			this.toolStripButton_FilesAreEqual.Size = new System.Drawing.Size(23, 22);
			this.toolStripButton_FilesAreEqual.Text = "toolStripButton1";
			this.toolStripButton_FilesAreEqual.ToolTipText = "Select all files on or below the current tree node\r\nwhere source and dest files m" +
				"atch.";
			this.toolStripButton_FilesAreEqual.Click += new System.EventHandler(this.toolStripButtonFilesAreEqual_Click);
			// 
			// toolStripButton_DestFileNewer
			// 
			this.toolStripButton_DestFileNewer.CheckOnClick = true;
			this.toolStripButton_DestFileNewer.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
			this.toolStripButton_DestFileNewer.Image = global::RoboPublish.Properties.Resources.DestFileNewer;
			this.toolStripButton_DestFileNewer.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.toolStripButton_DestFileNewer.Name = "toolStripButton_DestFileNewer";
			this.toolStripButton_DestFileNewer.Size = new System.Drawing.Size(23, 22);
			this.toolStripButton_DestFileNewer.Text = "toolStripButton1";
			this.toolStripButton_DestFileNewer.ToolTipText = "Select all files on or below the current tree node\r\nwhere the remote file is newe" +
				"r than the local file.";
			this.toolStripButton_DestFileNewer.Click += new System.EventHandler(this.toolStripButton_DestFileNewer_Click);
			// 
			// toolStripButton_SelectAll
			// 
			this.toolStripButton_SelectAll.CheckOnClick = true;
			this.toolStripButton_SelectAll.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
			this.toolStripButton_SelectAll.Image = global::RoboPublish.Properties.Resources.All;
			this.toolStripButton_SelectAll.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.toolStripButton_SelectAll.Name = "toolStripButton_SelectAll";
			this.toolStripButton_SelectAll.Size = new System.Drawing.Size(23, 22);
			this.toolStripButton_SelectAll.Text = "toolStripButton1";
			this.toolStripButton_SelectAll.ToolTipText = "Select all files on or below the current tree node that your configuration allows" +
				".";
			this.toolStripButton_SelectAll.Click += new System.EventHandler(this.toolStripButton_SelectAll_Click);
			// 
			// toolStripSeparator1
			// 
			this.toolStripSeparator1.Name = "toolStripSeparator1";
			this.toolStripSeparator1.Size = new System.Drawing.Size(6, 25);
			// 
			// toolStripButton_ViewChecked
			// 
			this.toolStripButton_ViewChecked.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
			this.toolStripButton_ViewChecked.Image = ((System.Drawing.Image)(resources.GetObject("toolStripButton_ViewChecked.Image")));
			this.toolStripButton_ViewChecked.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.toolStripButton_ViewChecked.Name = "toolStripButton_ViewChecked";
			this.toolStripButton_ViewChecked.Size = new System.Drawing.Size(78, 22);
			this.toolStripButton_ViewChecked.Text = "View selection";
			this.toolStripButton_ViewChecked.Click += new System.EventHandler(this.toolStripButton_ViewChecked_Click);
			// 
			// toolStripButtonDeleteSelection
			// 
			this.toolStripButtonDeleteSelection.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
			this.toolStripButtonDeleteSelection.Image = ((System.Drawing.Image)(resources.GetObject("toolStripButtonDeleteSelection.Image")));
			this.toolStripButtonDeleteSelection.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.toolStripButtonDeleteSelection.Name = "toolStripButtonDeleteSelection";
			this.toolStripButtonDeleteSelection.Size = new System.Drawing.Size(87, 22);
			this.toolStripButtonDeleteSelection.Text = "Delete selection";
			this.toolStripButtonDeleteSelection.Click += new System.EventHandler(this.toolStripButtonDeleteSelection_Click);
			// 
			// toolStripSeparator3
			// 
			this.toolStripSeparator3.Name = "toolStripSeparator3";
			this.toolStripSeparator3.Size = new System.Drawing.Size(6, 25);
			// 
			// toolStripDropDownButtonFilterSKU
			// 
			this.toolStripDropDownButtonFilterSKU.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
			this.toolStripDropDownButtonFilterSKU.Image = ((System.Drawing.Image)(resources.GetObject("toolStripDropDownButtonFilterSKU.Image")));
			this.toolStripDropDownButtonFilterSKU.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.toolStripDropDownButtonFilterSKU.Name = "toolStripDropDownButtonFilterSKU";
			this.toolStripDropDownButtonFilterSKU.Size = new System.Drawing.Size(66, 22);
			this.toolStripDropDownButtonFilterSKU.Text = "Filter SKU";
			// 
			// toolStripSeparator2
			// 
			this.toolStripSeparator2.Name = "toolStripSeparator2";
			this.toolStripSeparator2.Size = new System.Drawing.Size(6, 25);
			// 
			// toolStripButtonClose
			// 
			this.toolStripButtonClose.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
			this.toolStripButtonClose.Image = ((System.Drawing.Image)(resources.GetObject("toolStripButtonClose.Image")));
			this.toolStripButtonClose.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.toolStripButtonClose.Name = "toolStripButtonClose";
			this.toolStripButtonClose.Size = new System.Drawing.Size(37, 22);
			this.toolStripButtonClose.Text = "Close";
			this.toolStripButtonClose.Click += new System.EventHandler(this.toolStripButtonClose_Click);
			// 
			// labelName
			// 
			this.labelName.Dock = System.Windows.Forms.DockStyle.Fill;
			this.labelName.Font = new System.Drawing.Font("Microsoft Sans Serif", 15.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.labelName.Location = new System.Drawing.Point(0, 0);
			this.labelName.Name = "labelName";
			this.labelName.Size = new System.Drawing.Size(490, 23);
			this.labelName.TabIndex = 2;
			this.labelName.Text = "Put Name Here";
			this.labelName.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
			// 
			// panelHeading
			// 
			this.panelHeading.Controls.Add(this.labelName);
			this.panelHeading.Dock = System.Windows.Forms.DockStyle.Top;
			this.panelHeading.Location = new System.Drawing.Point(3, 16);
			this.panelHeading.Name = "panelHeading";
			this.panelHeading.Size = new System.Drawing.Size(490, 23);
			this.panelHeading.TabIndex = 3;
			// 
			// groupBox1
			// 
			this.groupBox1.Controls.Add(this.buttonPublish);
			this.groupBox1.Controls.Add(this.buttonRefresh);
			this.groupBox1.Controls.Add(this.panelHeading);
			this.groupBox1.Controls.Add(this.toolStripContainer1);
			this.groupBox1.Dock = System.Windows.Forms.DockStyle.Fill;
			this.groupBox1.Location = new System.Drawing.Point(0, 0);
			this.groupBox1.Name = "groupBox1";
			this.groupBox1.Size = new System.Drawing.Size(496, 513);
			this.groupBox1.TabIndex = 4;
			this.groupBox1.TabStop = false;
			// 
			// buttonPublish
			// 
			this.buttonPublish.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonPublish.Location = new System.Drawing.Point(415, 484);
			this.buttonPublish.Name = "buttonPublish";
			this.buttonPublish.Size = new System.Drawing.Size(75, 23);
			this.buttonPublish.TabIndex = 5;
			this.buttonPublish.Text = "PUBLISH";
			this.buttonPublish.UseVisualStyleBackColor = true;
			this.buttonPublish.Click += new System.EventHandler(this.buttonPublish_Click);
			// 
			// buttonRefresh
			// 
			this.buttonRefresh.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.buttonRefresh.Location = new System.Drawing.Point(6, 484);
			this.buttonRefresh.Name = "buttonRefresh";
			this.buttonRefresh.Size = new System.Drawing.Size(75, 23);
			this.buttonRefresh.TabIndex = 4;
			this.buttonRefresh.Text = "Refresh";
			this.buttonRefresh.UseVisualStyleBackColor = true;
			this.buttonRefresh.Click += new System.EventHandler(this.buttonRefresh_Click);
			// 
			// roboTree
			// 
			this.roboTree.CheckBoxes = true;
			this.roboTree.Dock = System.Windows.Forms.DockStyle.Fill;
			this.roboTree.DrawMode = System.Windows.Forms.TreeViewDrawMode.OwnerDrawText;
			this.roboTree.HideSelection = false;
			this.roboTree.Location = new System.Drawing.Point(0, 0);
			this.roboTree.Name = "roboTree";
			this.roboTree.Size = new System.Drawing.Size(484, 383);
			this.roboTree.TabIndex = 0;
			this.roboTree.AfterCheck += new System.Windows.Forms.TreeViewEventHandler(this.roboTree_AfterCheck);
			this.roboTree.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.roboTree_AfterSelect);
			// 
			// RoboPanel
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.Controls.Add(this.groupBox1);
			this.MinimumSize = new System.Drawing.Size(150, 0);
			this.Name = "RoboPanel";
			this.Size = new System.Drawing.Size(496, 513);
			this.Load += new System.EventHandler(this.RoboPanel_Load);
			this.toolStripContainer1.BottomToolStripPanel.ResumeLayout(false);
			this.toolStripContainer1.BottomToolStripPanel.PerformLayout();
			this.toolStripContainer1.ContentPanel.ResumeLayout(false);
			this.toolStripContainer1.TopToolStripPanel.ResumeLayout(false);
			this.toolStripContainer1.TopToolStripPanel.PerformLayout();
			this.toolStripContainer1.ResumeLayout(false);
			this.toolStripContainer1.PerformLayout();
			this.toolStrip2.ResumeLayout(false);
			this.toolStrip2.PerformLayout();
			this.panelParseProgress.ResumeLayout(false);
			this.panelParseProgress.PerformLayout();
			this.panel_Feedback.ResumeLayout(false);
			this.groupBox_ThreadFeedback.ResumeLayout(false);
			this.toolStrip1.ResumeLayout(false);
			this.toolStrip1.PerformLayout();
			this.panelHeading.ResumeLayout(false);
			this.groupBox1.ResumeLayout(false);
			this.ResumeLayout(false);

		}

		#endregion

		private RoboTree roboTree;
		private System.Windows.Forms.ToolStrip toolStrip1;
		private System.Windows.Forms.Label labelName;
		private System.Windows.Forms.Panel panelHeading;
		private System.Windows.Forms.GroupBox groupBox1;
		private System.Windows.Forms.ToolStripButton toolStripButton_SourceNewerOrDestMissing;
		private System.Windows.Forms.ToolStripButton toolStripButton_FilesAreEqual;
		private System.Windows.Forms.Button buttonRefresh;
		private System.Windows.Forms.ToolStripButton toolStripButton_DestFileNewer;
		private System.Windows.Forms.ToolStripButton toolStripButton_SelectAll;
		private System.Windows.Forms.Button buttonPublish;
		private System.Windows.Forms.Panel panel_Feedback;
		private System.Windows.Forms.GroupBox groupBox_ThreadFeedback;
		private System.Windows.Forms.ProgressBar progressBar_Publish;
		private System.Windows.Forms.RichTextBox richTextBox_Publish;
		private System.Windows.Forms.Button buttonAbort;
		private System.Windows.Forms.Button buttonOk;
		private System.Windows.Forms.ToolStripButton toolStripButton1_SelectDestMissing;
		private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
		private System.Windows.Forms.ToolStripButton toolStripButton_ViewChecked;
		private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
		internal System.Windows.Forms.ToolStripButton toolStripButtonClose;
		private System.Windows.Forms.Panel panelParseProgress;
		private System.Windows.Forms.Label labelParseProgress;
		private System.Windows.Forms.ToolStripDropDownButton toolStripDropDownButtonFilterSKU;
		private System.Windows.Forms.Label labelPleaseWait;
		private System.Windows.Forms.ToolStripButton toolStripButtonDeleteSelection;
		private System.Windows.Forms.ToolStripSeparator toolStripSeparator3;
		private System.Windows.Forms.ToolStripButton toolStripButtonConfigureSorcery;
		private System.Windows.Forms.ToolStripButton toolStripButtonSorceryQuery;
		private System.Windows.Forms.ProgressBar progressBar1;
		private System.Windows.Forms.ToolStripContainer toolStripContainer1;
		public System.Windows.Forms.ToolStrip toolStrip2;
	}
}
