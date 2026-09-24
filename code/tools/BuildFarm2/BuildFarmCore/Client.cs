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
    
    public class Client
    {
        private bool AllowTimeout = false;

        private int TimeoutPeriod = 30 * 1000;

        private Connection consoleConnection;

        static Bizarre.Misc.LogSystem.DailyLogger log = null;

        public Client(bool outputToConsole)
        {
            string localApplicationData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);

            string logFolder = Path.Combine(localApplicationData, "BizBuildFarm");

            log = new Bizarre.Misc.LogSystem.DailyLogger(logFolder, "client", outputToConsole);

            log.WriteLine("Client created.");
        }

        public void ProcessArteryMessagesCallback(Connection conn, object context)
        {
            log.WriteLine("ArteryCallback started.");

            if (conn.IsConnected())
            {
                WaitContext ctx = new WaitContext();
                log.WriteLine("Server connection established.");

                //MessageBuilder b = new Bizarre.Artery.MessageBuilder();

                //b.Write("Client");

                //Message msg = new Message();
                //if (b.Build(msg))
                //{
                //    conn.Send(msg);
                //}

                while (conn.IsConnected())
                {
                    bool waitRet;

                    if (AllowTimeout)
                    {
                        waitRet = conn.Wait(ctx, TimeoutPeriod);
                    }
                    else
                    {
                        waitRet = conn.Wait(ctx, 10 * 60 * 1000);    // 10 Mins
                    }

                    // Have we received a message in the timeout period?
                    if (waitRet)
                    {
                    }
                    else
                    {
                        // Timeout occured
                        log.WriteLine("Server connection timeout.");
                        break;
                    }
                }
            }

            log.WriteLine("ArteryCallback ending.");
        }

        public bool Initialise()
        {
            // Initialise artery to allow others to connect to us
            if (!ArteryManager.Startup(false, "BuildFarmServer"))
            {
                log.WriteLine("[Error] A problem occured trying to start Artery.");
                return false;
            }

            // TODO: Connect to the server and make sure we can talk

            consoleConnection = new Connection();

            for (int tryCount = 0; tryCount < 10; tryCount++)
            {
                if (!this.consoleConnection.IsConnected())
                {
                    IPHostEntry he = Dns.GetHostEntry("rogperwsv");

                    foreach (var ipAddress in he.AddressList)
                    {
                        if (ipAddress.AddressFamily == System.Net.Sockets.AddressFamily.InterNetwork)
                        {
                            uint addr = (uint)IPAddress.NetworkToHostOrder((int)IPAddress.Parse(ipAddress.ToString()).Address);

                            Bizarre.Artery.SockAddr target = new Bizarre.Artery.SockAddr(addr, 6317);

                            Bizarre.Artery.ArteryManager.Connect(this.consoleConnection, target, "BuildFarmServer", this.ProcessArteryMessagesCallback, 0);

                            // Wait for a bit then try again...
                            System.Threading.Thread.Sleep(TimeSpan.FromSeconds(3));
                            break;
                        }
                    }
                }

                // Are we connected now?
                if (this.consoleConnection.IsConnected())
                {
                    // Jolly good...
                    return true;
                }
            }

            return false;
        }

        public bool Shutdown()
        {
            if (this.consoleConnection != null)
            {
                this.consoleConnection.Close();
                this.consoleConnection.Dispose();
                this.consoleConnection = null;
            }

            return true;
        }

        public bool Process()
        {
            log.WriteLine("Client: Process");

            // TODO: Send work to the server

            // TODO: Wait for work to be finished

            System.Threading.Thread.Sleep(10 * 1000);

            log.WriteLine("Client: Process - End");

            return false;
        }
    }
}
