using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Linq;
using System.ServiceProcess;
using System.Text;

namespace BizBuildService
{
    public partial class Service1 : ServiceBase
    {
        static BuildFarmCore.Server server = null;

        public Service1()
        {
            InitializeComponent();
        }

        protected override void OnStart(string[] args)
        {
            EventLog.WriteEntry("BizBuildService service started.");

            foreach (string arg in args)
            {
                EventLog.WriteEntry("BizBuildService - Arg: " + arg);
            }

            server = new BuildFarmCore.Server(false);
            server.Initialise();
        }

        protected override void OnStop()
        {
            if (server != null)
            {
                server.Shutdown();
                server = null;
            }

            EventLog.WriteEntry("BizBuildService service stopped.");
        }
    }
}
