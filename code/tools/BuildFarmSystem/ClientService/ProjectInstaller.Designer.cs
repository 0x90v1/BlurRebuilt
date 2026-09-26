
using Microsoft.Win32;

namespace ClientService
{
    partial class ProjectInstaller
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
            if (disposing && (components != null))
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
            this.serviceProcessInstaller1 = new System.ServiceProcess.ServiceProcessInstaller();
            this.serviceInstaller1 = new System.ServiceProcess.ServiceInstaller();
            // 
            // serviceProcessInstaller1
            // 
            this.serviceProcessInstaller1.Password = "Biztech_1";
            this.serviceProcessInstaller1.Username = "ATVI\\luntbuild";
            // 
            // serviceInstaller1
            // 
            this.serviceInstaller1.Description = "Builds code as part of a farm network of builders";
            this.serviceInstaller1.DisplayName = "CodeBuildFarm";
            this.serviceInstaller1.ServiceName = "CodeBuildFarm";
            // 
            // ProjectInstaller
            // 
            this.Installers.AddRange(new System.Configuration.Install.Installer[] {
            this.serviceProcessInstaller1,
            this.serviceInstaller1});
       }

        public override void Install(System.Collections.IDictionary stateSaver)
        {
            try
            {
                string path = System.IO.Path.Combine(System.Environment.GetFolderPath(System.Environment.SpecialFolder.CommonApplicationData), "Bizarre Creations\\Code Build Farm");

                System.IO.Directory.CreateDirectory(path);

                System.IO.StreamWriter sw = new System.IO.StreamWriter(path + "\\config.ini");

                if (this.Context.Parameters.ContainsKey("servername"))
                    sw.WriteLine("serverName=" + this.Context.Parameters["servername"]);

                if (this.Context.Parameters.ContainsKey("builderid"))
                    sw.WriteLine("builderID=" + this.Context.Parameters["builderid"]);

                sw.Close();
            }
            catch (System.Exception ex)
            {
                System.Diagnostics.Debug.Assert(false, ex.Message);
            }

            base.Install(stateSaver);
        }

        #endregion

        private System.ServiceProcess.ServiceProcessInstaller serviceProcessInstaller1;
        private System.ServiceProcess.ServiceInstaller serviceInstaller1;
    }
}