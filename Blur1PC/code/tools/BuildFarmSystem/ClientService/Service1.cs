using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Linq;
using System.ServiceProcess;
using System.Text;

namespace ClientService
{
    public partial class Service1 : ServiceBase
    {
        public Service1()
        {
            InitializeComponent();
        }

        protected override void OnStart(string[] args)
        {
            try
            {
                string serverName = "";
                int builderID = -1;

                string path = System.IO.Path.Combine(System.Environment.GetFolderPath(System.Environment.SpecialFolder.CommonApplicationData), "Bizarre Creations\\Code Build Farm");

                System.IO.StreamReader sr = new System.IO.StreamReader(path + "\\config.ini");

                while (!sr.EndOfStream)
                {
                    string line = sr.ReadLine();

                    if (line.StartsWith("serverName="))
                    {
                        serverName = line.Substring(11).Trim();
                    }
                    else if (line.StartsWith("builderID="))
                    {
                        builderID = Int32.Parse(line.Substring(10).Trim());
                    }

                    //if (this.Context.Parameters.ContainsKey("servername"))
                    //    sw.WriteLine("serverName=" + this.Context.Parameters["servername"]);

                    //if (this.Context.Parameters.ContainsKey("builderid"))
                    //    sw.WriteLine("builderID=" + this.Context.Parameters["builderid"]);
                }

                sr.Close();

                eventLog1.WriteEntry(String.Format("OnStart - ServerName({0}) BuilderID({1})", serverName, builderID));
            }
            catch
            {
            }
        }

        protected override void OnStop()
        {
            eventLog1.WriteEntry("OnStop");
        }
    }
}
