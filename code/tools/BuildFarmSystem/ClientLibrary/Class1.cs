using System;
using System.Collections.Generic;
//using System.Linq;
using System.Text;

using System.ServiceModel;

using System.Threading;

using System.Runtime.InteropServices;
using System.Xml;

using LogSystem;

namespace ClientLibrary
{
    public class ClientClass
    {
        public delegate void CtrlCallback();

        public static CtrlCallback ctrlCallback = null;

        public enum ClientType { Unknown, Builder, Client };

        static DailyLogger appLog;

        static bool anyFail = false;
        static bool builderAdded = false;

        static bool verbose = false;

        static public bool Verbose { get { return verbose; } set { verbose = value; } }

        static ServiceReference1.HelloServiceClient proxy = null;
        static ServiceReference1.JobRequest[] jobRequestList = null;

        static Dictionary<Int32, bool> jobList = new Dictionary<int, bool>();

        static System.Threading.ManualResetEvent jobDoneEvent = new System.Threading.ManualResetEvent(false);
        static System.Threading.AutoResetEvent workComingIn = new System.Threading.AutoResetEvent(true);
        static protected ManualResetEvent shutdownEvent = new ManualResetEvent(false);
        static protected AutoResetEvent serverShutdownEvent = new AutoResetEvent(false);

        static public void HandleShutdown()
        {
            if (proxy != null)
            {
                try
                {
                    if (builderAdded)
                        proxy.RemoveBuilder();

                    if (ctrlCallback != null)
                        ctrlCallback();

                    proxy.Abort();
                }
                catch
                {
                }
            }

            shutdownEvent.Set();
        }

        static string MakeNiceTimeSpanString(System.TimeSpan ts)
        {
            string output = "";

            if (ts.Days > 0)
                output = String.Format("{0} {1}, ", ts.Days, (ts.Days != 0) ? "days" : "day");
            if (ts.Hours > 0)
                output += String.Format("{0} {1}, ", ts.Hours, (ts.Hours != 0) ? "hrs" : "hr");
            if (ts.Minutes > 0)
                output += String.Format("{0} {1}, ", ts.Minutes, (ts.Minutes != 0) ? "mins" : "min");
            if (ts.Seconds > 0 || output.Length == 0)
                output += String.Format("{0} {1}", ts.Seconds, (ts.Seconds != 0) ? "secs" : "sec");

            return output;
        }


        #region Builder Keep-Alive thread
        static AutoResetEvent keepConnectionAliveEvent = new AutoResetEvent(false);

        public static void KeepConnectionAlive(object val)
        {
            ServiceReference1.HelloServiceClient proxy = val as ServiceReference1.HelloServiceClient;

            System.Diagnostics.Debug.Assert(keepConnectionAliveEvent != null);

            while (!keepConnectionAliveEvent.WaitOne(TimeSpan.FromSeconds(10)))
            {
                proxy.PingServer();
            }
        }
        #endregion

        static int Client_Builder(int builderID, ServiceReference1.HelloServiceClient proxy)
        {
            for (; ; )
            {
                WaitHandle[] waitList = new WaitHandle[] { shutdownEvent, serverShutdownEvent, workComingIn };

                int eventIndex = WaitHandle.WaitAny(waitList, TimeSpan.FromSeconds(10), false);

                // Drop out if we're being closed or the server has been nuked
                if (eventIndex == 0)
                    return 1;

                if (eventIndex == 1)
                    return 2;

                if (eventIndex == WaitHandle.WaitTimeout)
                {
                    proxy.PingServer();
                    continue;
                }

                while (true)
                {
                    if (verbose)
                    {
                        appLog.WriteLine("-");
                        appLog.WriteLine("Checking for work...");
                        appLog.WriteLine("-");
                    }

                    ServiceReference1.JobRequest jr = proxy.RequestWork(builderID);

                    if (jr == null)
                        break;

                    System.DateTime startTime = System.DateTime.Now;

                    // Give us some output so we know what this builder is currently doing
                    {
                        appLog.WriteLine("\n--------------------------------");
                        appLog.WriteLine(String.Format("Job       : {0}", jr.JobID));
                        appLog.WriteLine(String.Format("Start     : {0}", startTime.ToString()));
                        appLog.WriteLine("Comment   : " + jr.Comment);
                        appLog.WriteLine("");
                        appLog.WriteLine("Logfile   : " + jr.Logfile);
                        appLog.WriteLine("Executable: " + jr.Executable);
                        appLog.WriteLine("Arguments : " + jr.Arguments);
                    }

                    System.Diagnostics.Process p = new System.Diagnostics.Process();

                    p.StartInfo.Arguments = jr.Arguments;
                    p.StartInfo.FileName = jr.Executable;

                    p.StartInfo.WindowStyle = System.Diagnostics.ProcessWindowStyle.Minimized;

                    int exitCode = -1;

                    for (int iTry = 0; iTry < 10; iTry++)
                    {
                        try
                        {
                            p.StartInfo.UseShellExecute = false;
                            p.StartInfo.RedirectStandardOutput = true;

                            p.Start();

                            Thread backgroundThread = new Thread(KeepConnectionAlive);
                            backgroundThread.Start(proxy);

                            string stdOut = p.StandardOutput.ReadToEnd();

                            LogSystem.Logger lf = new LogSystem.Logger(jr.Logfile);

                            lf.WriteLine(stdOut);
                            lf.Close();

                            exitCode = p.ExitCode;

                            break;
                        }
                        catch (Exception ex)
                        {
                            appLog.WriteLine("Exception: " + ex.Message);
                            appLog.WriteLine("Exception Stack: " + ex.StackTrace);
                            exitCode = -1;
                        }

                        System.Threading.Thread.Sleep(TimeSpan.FromSeconds(3));
                    }

                    // Kill our keep-alive thread now
                    keepConnectionAliveEvent.Set();

                    jr.Duration = System.DateTime.Now - startTime;

                    proxy.WorkDone(jr.JobID, jr.Duration, exitCode);

                    appLog.WriteLine("");
                    appLog.WriteLine("Duration  : " + MakeNiceTimeSpanString(jr.Duration));
                    appLog.WriteLine("");
                    appLog.WriteLine("Completed : " + ((exitCode == 0) ? "Success" : String.Format("Fail ({0})", ConvertExitCode(exitCode))));
                }
            }
        }

        static bool foundDescription = false;

        static void ProcessRoot(XmlTextReader reader, Dictionary<string, string> xmlValues, List<string> builder_description)
        {
            string xmlName = "";
            string xmlValue = "";

            while (reader.Read())
            {
                switch (reader.NodeType)
                {
                    case XmlNodeType.Element: // The node is an element.
                        xmlName = reader.Name;
                        break;

                    case XmlNodeType.Text: //Display the text in each element.
                        xmlValue = reader.Value;
                        break;

                    case XmlNodeType.EndElement: //Display the end of the element.
                        if (reader.Name == "root")
                            return;

                        if (reader.Name == xmlName)
                        {
                            if (xmlName.StartsWith("description"))
                            {
                                builder_description.Add(xmlValue);
                                foundDescription = true;
                            }
                            else if (xmlName.StartsWith("param") && !foundDescription)
                            {
                                builder_description.Add(xmlValue);
                            }
                            else
                            {
                                xmlValues.Add(xmlName, xmlValue);

                                xmlName = "";
                                xmlValue = "";
                            }
                        }
                        break;
                }
            }
        }

        static bool ProcessXMLFile(string filename, Dictionary<string, string> xmlValues, List<string> builder_description)
        {
            try
            {
                XmlTextReader reader = new XmlTextReader(filename);

                while (reader.Read())
                {
                    switch (reader.NodeType)
                    {
                        case XmlNodeType.Element: // The node is an element.
                            if (reader.Name == "root")
                                ProcessRoot(reader, xmlValues, builder_description);
                            break;

                        //case XmlNodeType.Text: //Display the text in each element.
                        //    break;
                        //case XmlNodeType.EndElement: //Display the end of the element.
                        //    break;
                    }
                }

                return true;
            }
            catch (Exception ex)
            {
                if (appLog != null)
                    appLog.WriteLine("ProcessXMLFile: Exception: " + ex.Message);

                return false;
            }
        } 

        static ServiceReference1.HelloServiceClient CreateProxy(string serverName)
        {
            InstanceContext context = new InstanceContext(new ClientCallback());

            ServiceReference1.HelloServiceClient newProxy = new ServiceReference1.HelloServiceClient(context);

            //if (false)
            {
                System.ServiceModel.Channels.AddressHeader addressHeader1 = System.ServiceModel.Channels.AddressHeader.CreateAddressHeader("HelloService", "http://localhost:8000/HelloService", 1);

                System.ServiceModel.Channels.AddressHeader[] addressHeaders = new System.ServiceModel.Channels.AddressHeader[1] { addressHeader1 };

                System.ServiceModel.Description.ServiceEndpoint sp = newProxy.Endpoint;

                string serviceLocation = "http://" + serverName + ":8080/HelloService";

                // sp.Address
                EndpointAddress epa = new EndpointAddress(new Uri(serviceLocation), sp.Address.Identity, addressHeaders);

                sp.Address = epa;

                sp.ListenUri = new Uri("http://" + serverName + ":8080/HelloService");
            }

            {
                WSDualHttpBinding binding = (WSDualHttpBinding)newProxy.Endpoint.Binding;

                binding.ClientBaseAddress = new Uri("http://" + System.Environment.MachineName + ":8129/HelloService" + Guid.NewGuid().ToString());
            }

            return newProxy;
        }

        static List<int> jobIDList = null;

        static void KillJobsOnShutdown()
        {
            if (jobIDList != null)
            {
                foreach (int id in jobIDList)
                {
                    bool isComplete;

                    if (jobList.TryGetValue(id, out isComplete))
                    {
                        if (isComplete == false)
                        {
                            appLog.WriteLine(String.Format("Job {0,6}: Kill", id));
                            proxy.KillJob(id);
                        }
                    }
                }
            }
        }

        static string MakeSafeFilename(string filename)
        {
            return filename.Replace("|", "_");
        }

        static string ConvertExitCode(int exitCode)
        {
            switch (exitCode)
            {
                case -1073741510: // 0xc000013a - STATUS_CONTROL_C_EXIT:
                    return "0xc000013a - STATUS_CONTROL_C_EXIT";

                default:
                    return exitCode.ToString();
            }
        }

        public class ClientCallback : ServiceReference1.HelloServiceCallback
        {
            public void JobComplete(int ID, TimeSpan duration, int exitCode, string builderName)
            {
                //appLog.WriteLine(System.Environment.MachineName + ": JobComplete: " + ID);
                foreach (var j in jobRequestList)
                {
                    if (j.JobID == ID)
                    {
                        appLog.WriteLine(String.Format("Job {0,6}: Finished - {1} - {2} ({3}) ({4})", ID, (exitCode == 0 ? "Success" : String.Format("Fail ({0})", ConvertExitCode(exitCode))), MakeNiceTimeSpanString(duration), j.Comment, builderName));
                        break;
                    }
                }

                if (exitCode != 0)
                    anyFail = true;

                bool isComplete;

                if (jobList.TryGetValue(ID, out isComplete))
                {
                    System.Diagnostics.Debug.Assert(isComplete == false);

                    jobList.Remove(ID);
                    jobList.Add(ID, true);  // Mark job as complete

                    bool ok = true;

                    foreach (KeyValuePair<int, bool> kvp in jobList)
                    {
                        if (kvp.Value == false)
                        {
                            ok = false;
                            break;
                        }
                    }

                    if (ok) // Are all jobs done now?
                    {
                        jobDoneEvent.Set();
                    }
                }
                else
                {
                    System.Diagnostics.Debug.Assert(false);
                }
            }

            public void WorkAvailable()
            {
                if (verbose)
                    appLog.WriteLine("Server says work is available");
                
                // Set flag stating work is up on the server
                workComingIn.Set();
            }

            public void TerminateJob(int ID)
            {
                if (verbose)
                    appLog.WriteLine("TerminateJob: ID(" + ID + ") - Not implemented");
                
                // TODO: Hey Mr Builder... kill this job if you can
            }

            // Notification that the server is shutting down
            public void ServerShutdown()
            {
                serverShutdownEvent.Set();
            }

            public void PingClient()
            {
                //System.Diagnostics.Debug.WriteLine(System.Environment.MachineName + ": PING!!!");
            }
        }

        public static void RunAsBuilder(string serverName, int builderID)
        {
            appLog = new DailyLogger("logs", "client", true);

            proxy = CreateProxy(serverName);

            while (shutdownEvent.WaitOne(0, true) == false)
            {
                appLog.WriteLine(String.Format("Trying to connect to server '{0}'...", serverName));

                try
                {
                    int retAddBuilder = proxy.AddBuilder(builderID, System.Environment.MachineName);

                    if (retAddBuilder == 0)
                    {
                        workComingIn.Set();

                        appLog.WriteLine("Connection established");

                        builderAdded = true;

                        int retBuilder = Client_Builder(builderID, proxy);

                        // Is the server going?
                        if (retBuilder == 2)
                        {
                            appLog.WriteLine("Server was shut down.");

                            builderAdded = false;

                            // Give server time to actually go away
                            System.Threading.Thread.Sleep(1000);
                        }
                        else
                        {
                            proxy.RemoveBuilder();
                        }
                    }
                    else if (retAddBuilder == 2)    // Server is shutting down
                    {
                        // Do nothing
                    }
                    else
                    {
                        appLog.WriteLine("ERROR: A problem occured trying to add this builder.");
                    }
                }
                catch (System.TimeoutException)
                {
                    appLog.WriteLine(String.Format("Server timed out during communication: " + System.DateTime.Now.ToString()));
                }
                catch (System.ServiceModel.EndpointNotFoundException)
                {
                }
                catch (CommunicationObjectAbortedException)
                {
                }
                catch (Exception ex)
                {
                    //appLog.WriteLine("Failed to establish a connection to the server.");
                    appLog.WriteLine("Exception: " + ex);
                }
                finally
                {
                    builderAdded = false;
                }

                try
                {
                    if (proxy != null)
                        proxy.Close();
                }
                catch
                {
                }

                proxy = CreateProxy(serverName);
            }

            proxy.Close();
        }

        public static void RunAsClient(string serverName, string xmlFilename)
        {
            appLog = new DailyLogger("logs", "client", true);
            
            proxy = CreateProxy(serverName);

            Dictionary<string, string> xmlValues = new Dictionary<string, string>();
            List<string> builder_description = new List<string>();

            if (!ProcessXMLFile(xmlFilename, xmlValues, builder_description))
            {
                appLog.WriteLine("A problem occured trying to open the XML file.");
                Environment.ExitCode = 3;
                return;
            }

            jobRequestList = new ServiceReference1.JobRequest[builder_description.Count];

            string projectName, cleanBuild, exeName, exeArgs, logFolder, job_count;

            xmlValues.TryGetValue("project", out projectName);
            xmlValues.TryGetValue("clean_build", out cleanBuild);

            xmlValues.TryGetValue("exe", out exeName);
            xmlValues.TryGetValue("args", out exeArgs);
            xmlValues.TryGetValue("log_folder", out logFolder);
            xmlValues.TryGetValue("job_count", out job_count);

            int iJobCount;
            Int32.TryParse(job_count, out iJobCount);

            for (int iJob = 0; iJob < iJobCount; iJob++)
            {
                ServiceReference1.JobRequest jr = new ServiceReference1.JobRequest();

                jr.JobID = proxy.GetJobID();
                jr.BuilderID = jobList.Count;
                jr.Executable = exeName;
                jr.Arguments = exeArgs + " " + iJob;
                jr.Comment = builder_description[iJob];

                jr.Logfile = logFolder + "\\" + iJob.ToString() + ".log";

                jobRequestList[jobList.Count] = jr;
                jobList.Add(jr.JobID, false);
            }

            bool retSubJobs = proxy.SubmitJobs(jobRequestList);

            if (retSubJobs)
            {
                jobIDList = new List<int>();

                foreach (ServiceReference1.JobRequest jr in jobRequestList)
                {
                    appLog.WriteLine(String.Format("Job {0,6}: Added - {1}", jr.JobID, jr.Comment));
                    jobIDList.Add(jr.JobID);
                }

                ctrlCallback += KillJobsOnShutdown;

                WaitHandle[] waitList = new WaitHandle[] { shutdownEvent, serverShutdownEvent, jobDoneEvent };

                int eventIndex = WaitHandle.WaitTimeout;

                while (eventIndex == WaitHandle.WaitTimeout)
                {
                    eventIndex = WaitHandle.WaitAny(waitList, TimeSpan.FromSeconds(10), false);

                    if (eventIndex == WaitHandle.WaitTimeout)
                        proxy.PingServer();
                }

                switch (eventIndex)
                {
                    case 0:
                        appLog.WriteLine("Client closing down.");

                        anyFail = true;
                        break;

                    case 1:
                        appLog.WriteLine("The server shut down.");
                        anyFail = true;
                        break;

                    case 2:
                        appLog.WriteLine("Jobs done");
                        break;
                }

                Environment.ExitCode = (anyFail == false) ? 0 : 1;
            }
            else
            {
                appLog.WriteLine("A problem occured adding the jobs.");

                Environment.ExitCode = 2;
            }
            
            proxy.Close();
        }
    }
}
