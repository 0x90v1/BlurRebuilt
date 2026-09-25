using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using System.Threading;

using System.Runtime.InteropServices;

using System.Xml;

using LogSystem;

using ClientLibrary;

// client \\UKLPLSLAVE01\luntbuild-work\Amax_Proto_GAME\360.xml
// builder 0

// TODO: Need to split work up into a) Copy to, b) Build, c) Copy back - then we can monitor timing and interupt in stages

// TODO: Handle doing a clean build if we have got out of step with everyone else

namespace MyClient
{
    class Program
    {
        static void Help()
        {
            System.Console.WriteLine("Hello. Client command line help.");
            System.Console.WriteLine();
            System.Console.WriteLine("\t-builder");
            System.Console.WriteLine("\t-client");
            System.Console.WriteLine("\t-xml");
            System.Console.WriteLine("\t-verbose");
            System.Console.WriteLine();
            System.Console.WriteLine("e.g. BuildClient -builder 2 -server uklplslave01");
        }

        [DllImport("Kernel32")]
        public static extern bool SetConsoleCtrlHandler(HandlerRoutine Handler, bool Add);

        // A delegate type to be used as the handler routine for SetConsoleCtrlHandler.
        public delegate bool HandlerRoutine(CtrlTypes CtrlType);

        // An enumerated type for the control messages sent to the handler routine.
        public enum CtrlTypes
        {
            CTRL_C_EVENT = 0,
            CTRL_BREAK_EVENT,
            CTRL_CLOSE_EVENT,
            CTRL_LOGOFF_EVENT = 5,
            CTRL_SHUTDOWN_EVENT
        }

        static HandlerRoutine copyOfHandleToPreventGarbageCollection = null;

        private static bool ConsoleCtrlCheck(CtrlTypes ctrlType)
        {
            ClientLibrary.ClientClass.HandleShutdown();
            return true;
        }

        static void Main(string[] args)
        {
            ClientClass.ClientType clientType = ClientClass.ClientType.Unknown;
            int builderID = -1;
            string serverName = "";
            string xmlFilename = "";

            bool problem = false;

            SetConsoleCtrlHandler(copyOfHandleToPreventGarbageCollection = new HandlerRoutine(ConsoleCtrlCheck), true);

            #region Parameter handling

            for (int iParam = 0; iParam < args.Length; iParam++)
            {
                string param = args[iParam].ToLower();

                switch (param)
                {
                    case "-help":
                        problem = true;
                        break;

                    case "-builder":
                        {
                            if (clientType != ClientClass.ClientType.Unknown || iParam == args.Length - 1 || !Int32.TryParse(args[iParam + 1], out builderID))
                            {
                                problem = true;
                                break;
                            }

                            clientType = ClientClass.ClientType.Builder;

                            if (builderID < 0)
                                problem = true;
                        }
                        break;

                    case "-client":
                        {
                            if (clientType != ClientClass.ClientType.Unknown)
                            {
                                problem = true;
                                break;
                            }

                            clientType = ClientClass.ClientType.Client;
                        }
                        break;

                    case "-server":
                        {
                            if (iParam == args.Length - 1)
                            {
                                problem = true;
                                break;
                            }

                            serverName = args[iParam + 1];
                        }
                        break;

                    case "-xml":
                        {
                            if (iParam == args.Length - 1)
                            {
                                problem = true;
                                break;
                            }

                            xmlFilename = args[iParam + 1];
                        }
                        break;

                    case "-verbose":
                        ClientClass.Verbose = true;
                        break;
                }
            }

            if (problem)
            {
                Help();
                return;
            }

            #endregion

            switch (clientType)
            {
                case ClientClass.ClientType.Builder:
                    ClientClass.RunAsBuilder(serverName, builderID);
                    break;

                case ClientClass.ClientType.Client:
                    ClientClass.RunAsClient(serverName, xmlFilename);
                    break;
            }
        }    
    }
}
