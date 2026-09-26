namespace TestBuildFarm
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;

    using Bizarre.Misc;
    using BuildFarmCore;

    class Program
    {
        static void Main(string[] args)
        {
            Arguments argParser = new Arguments(args);

            // Has a kit file been specificed on the command line, if so, we use the kits from this, else we use the default kits from this PC
            if (argParser["client"] != null)
            {
                Client client = new Client(true);

                bool connected = client.Initialise();

                if (connected)
                {
                    client.Process();
                    client.Shutdown();
                }
            }
            else if (argParser["server"] != null)
            {
                if (Server.Initialise(true))
                {
                    Server.Process();
                    Server.Shutdown();
                }
            }
            else if (argParser["builder"] != null)
            {
                Builder builder = new Builder(true);

                builder.Initialise();

                while (true)
                {
                    System.Threading.Thread.Sleep(2000);
                }

                builder.Shutdown();
            }
        }
    }
}
