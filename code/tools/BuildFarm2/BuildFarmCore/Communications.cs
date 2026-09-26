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

    public class Communicate
    {
        public static void ArteryCallback(Connection conn, object context)
        {
            CommunicateContext cContext = context as CommunicateContext;

            cContext.log.WriteLine("ArteryCallback started.");

            if (conn.IsConnected())
            {
                WaitContext ctx = new WaitContext();
                cContext.log.WriteLine("Server connection established.");

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

                    if (cContext.AllowTimeout)
                    {
                        waitRet = conn.Wait(ctx, cContext.TimeoutPeriod);
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
                        cContext.log.WriteLine("Server connection timeout.");
                        break;
                    }
                }
            }

            cContext.log.WriteLine("ArteryCallback ending.");
        }

        class CommunicateContext
        {
            public bool AllowTimeout = false;
            public int TimeoutPeriod = 20 * 1000;
            public Bizarre.Misc.LogSystem.DailyLogger log = null;
        }
    }
}