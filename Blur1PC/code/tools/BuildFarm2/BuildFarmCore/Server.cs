
/*
 * Set up server
 * 
 * Allow builders and clients to connect
 * 
 * Manage list of builders
 * 
 * Manage list of work to do
 * 
 * 
 */

namespace BuildFarmCore
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    //using System.Linq;
    using System.Text;
    using System.Threading;

    using Bizarre.Artery;
    using Bizarre.Misc;

    public class Server
    {
        private static ServerContext serverContext = new ServerContext();

        private static bool AllowTimeout = true;

        /// <summary>
        /// A logging system in which we keep a record of everything that goes on to help diagnose issues
        /// </summary>
        static Bizarre.Misc.LogSystem.DailyLogger log = null;

        public Server(bool outputToConsole)
        {
            string localApplicationData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);

            string logFolder = Path.Combine(localApplicationData, "BizBuildFarm");

            log = new Bizarre.Misc.LogSystem.DailyLogger(logFolder, "server", outputToConsole);

            log.WriteLine("Server created.");
        }

        /// <summary>
        /// This function is called whenever anyone contacts the server - either a builder or a client wanting work done
        /// </summary>
        /// <param name="conn">An artery connection</param>
        /// <param name="context">A context to help identify this connection</param>
        static void ServerArteryCallback(Connection conn, object sacontext)
        {
            ServerContext context = sacontext as ServerContext;

            log.WriteLine("ServerCallback: Connection established");

            WaitContext ctx = new WaitContext();

            try
            {
                while (conn.IsConnected())
                {
                    // Wait for us to receive some data to process - how long we wait depends if we're debugging or not
                    bool waitRet = conn.Wait(ctx, AllowTimeout ? 30 * 1000 : 10 * 60 * 1000);

                    // If this is true then data was received before we timed out
                    if (waitRet)
                    {
                        // Extract the data we received
                        Message msg = new Message();

                        // Keep going while we get more data
                        while (conn.TryReceive(msg))
                        {
                            MessageReader msgReader = new MessageReader(msg);

                            string msgType;

                            if (msgReader.Read(out msgType))
                            {
                                log.WriteLine("Message received: " + msgType);

                                switch (msgType)
                                {
                                        // Keep alive message to stop us from timing out
                                    case "PING":
                                        break;

                                    case "Client":
                                        log.WriteLine("ServerCallback: Client connection");
                                        break;

                                    case "Builder":
                                        log.WriteLine("ServerCallback: Builder connection");
                                        break;
                                }
                            }
                        }
                    }
                }
            }
            catch
            {
            }

            log.WriteLine("ServerCallback: Connection shutdown");
        }

        private static void ServerThread(object state)
        {
            ServerContext context = state as ServerContext;

            // Wait for the thread to be shut down
            context.shutdownEvent.WaitOne();
        }

        public static bool Initialise()
        {
            // Initialise artery to allow others to connect to us
            if (!ArteryManager.StartupEx(true, "BuildFarmServer", 100))
            {
                log.WriteLine("[Error] A problem occured trying to start Artery.");
                return false;
            }

            ArteryManager.Register("BuildFarmServer", ServerArteryCallback, serverContext);

            log.WriteLine("Artery has been started.");

            // Create the server thread
            Thread jobThread = new Thread(new ParameterizedThreadStart(ServerThread));

            jobThread.Name = "Server";
            jobThread.Start(serverContext);

            return true;
        }

        public static void Shutdown()
        {
            log.WriteLine("Shutting down.");

            // Tell thread to close
            serverContext.shutdownEvent.Set();

            log.Close();
            log = null;
        }

        public void Process()
        {
            // Wait for something to try and shut us down
            shutdownEvent.WaitOne();
        }

        void AddBuilder()
        {
        }

        void RemoveBuilder()
        {
        }

        void AddClient()
        {
        }

        void RemoveClient()
        {
        }

        void AddJob()
        {
        }

        void RemoveJob()
        {
        }

        /// <summary>
        /// A job can contain different build configurations that need to be executed
        /// </summary>
        class Job
        {
            //uint id;

            /// <summary>
            /// A single chunk of work to do, i.e. XBox
            /// </summary>
            public class WorkItem
            {
                // Split into Copy-Build-Copy
            }
        }

        class Builder
        {
            enum eState
            {
                kIdle,
                kBuilding,
            }

            public string Name { get; set; }
            public uint Id { get; set; }

            // TODO: Store connection info
            // TODO: Current status
        }

        class Client
        {
            // TODO: Job number
        }

        protected class ServerContext
        {
            public ManualResetEvent shutdownEvent = new ManualResetEvent(false);

        }
    }
}
