namespace Executer
{
	partial class XmlViewer
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
			this.webBrowser1 = new System.Windows.Forms.WebBrowser();
			this.buttonSaveAs = new System.Windows.Forms.Button();
			this.SuspendLayout();
			// 
			// webBrowser1
			// 
			this.webBrowser1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
						| System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.webBrowser1.Location = new System.Drawing.Point(12, 41);
			this.webBrowser1.MinimumSize = new System.Drawing.Size(20, 20);
			this.webBrowser1.Name = "webBrowser1";
			this.webBrowser1.Size = new System.Drawing.Size(750, 606);
			this.webBrowser1.TabIndex = 0;
			// 
			// buttonSaveAs
			// 
			this.buttonSaveAs.Location = new System.Drawing.Point(12, 12);
			this.buttonSaveAs.Name = "buttonSaveAs";
			this.buttonSaveAs.Size = new System.Drawing.Size(75, 23);
			this.buttonSaveAs.TabIndex = 1;
			this.buttonSaveAs.Text = "Save As ...";
			this.buttonSaveAs.UseVisualStyleBackColor = true;
			this.buttonSaveAs.Click += new System.EventHandler(this.button_SaveAs_Click);
			// 
			// XmlViewer
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(774, 659);
			this.Controls.Add(this.buttonSaveAs);
			this.Controls.Add(this.webBrowser1);
			this.Name = "XmlViewer";
			this.Text = "XmlViewer";
			this.Load += new System.EventHandler(this.XmlViewer_Load);
			this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.WebBrowser webBrowser1;
		private System.Windows.Forms.Button buttonSaveAs;
	}
}