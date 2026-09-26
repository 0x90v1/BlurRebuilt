
/*
 * Connect to server
 * Listen for work
 * Keep connection alive
 * Process work requests
 * 
 */

namespace BuildFarmCore
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Net;
    //using System.Linq;
    using System.Text;

    using Bizarre.Artery;
    using Bizarre.Misc;

    public class Builder
    {
        static Bizarre.Misc.LogSystem.DailyLogger log = null;

        public Builder(bool outputToConsole)
        {
            string localApplicationData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);

            string logFolder = Path.Combine(localApplicationData, "BizBuildFarm");

            log = new Bizarre.Misc.LogSystem.DailyLogger(logFolder, "builder", outputToConsole);

            log.WriteLine("Builder created.");
        }

        public bool Initialise()
        {
            return false;
        }

        public bool Shutdown()
        {
            // Close the connection
            return false;
        }
    }
}
