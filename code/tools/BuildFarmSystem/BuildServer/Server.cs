
using System;
using System.ServiceModel;
using System.ServiceModel.Description;

using System.Runtime.Serialization;

using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;

using System.Threading;

using System.Timers;

using LogSystem;

/*
 * Server:
 *      Add/Remove builders from the system
 *      Receive work lists from clients
 *      Allocate work-list out to available builders (respecting specific build machine requests)
 *      
 * Builder:
 *      Register with the server
 *      Receive work
 *      Tell server when work done and result
 *      
 * Client:
 *      Ask server to do some work
 *      Wait for work to be done
 * 
 */

// TODO: If clean build done, then another builder asked to do a build that hasn't done one for a while - wipe or no wipe?

// TODO: Builder killed (disappears) while processing a job... need to add back to pool

// TODO: When handling missing builder, need to possibly do work in JobID order (else if busy stuff won't get done ever)

namespace ServerWCF
{
    public class BuilderInfo
    {
        public INotify Callback
        {
            get;
            set;
        }
        public string Name
        {
            get;
            set;
        }
    }

    [DataContract]
    public class JobRequest
    {
        [DataMember]
        public int JobID
        {
            get;
            set;
        }

        [DataMember] 
        public int BuilderID  // We want to be able to try and run the build on the same PC each time
        {
            get;
            set;
        }

        [DataMember]
        public string Executable
        {
            get;
            set;
        }

        [DataMember]
        public string Arguments
        {
            get;
            set;
        }

        [DataMember]
        public string Logfile
        {
            get;
            set;
        }

        [DataMember]
        public string Comment
        {
            get;
            set;
        }

        [DataMember]
        public TimeSpan Duration
        {
            get;
            set;
        }
    }

    public enum JobStatus { Pending, Fail, Done, InProgress, Killed };

    public class Job
    {
        public Job()
        {
            Status = JobStatus.Pending;
            ActualBuilderID = -1;
        }
        public Job(JobRequest jr)
        {
            Request = jr;
            Status = JobStatus.Pending;
            ActualBuilderID = -1;
        }
        public Job(JobRequest jr, INotify cb) : this(jr)
        {
            Callback = cb;
        }

        public JobRequest Request
        {
            get;
            set;
        }
        public JobStatus Status
        {
            get;
            set;
        }
        public int ActualBuilderID
        {
            get;
            set;
        }       // Who is actually processing this job

        public INotify Callback
        {
            get;
            set;
        }
    }

    public interface INotify
    {
    // Builders that do work
        [OperationContract(IsOneWay = true)]
        void WorkAvailable();                           // Notify builders that new work is available for processing

        [OperationContract(IsOneWay = true)]
        void TerminateJob(int ID);                      // We want to stop processing this job please

    // Clients that request work
        [OperationContract(IsOneWay = true)]
        void JobComplete(int ID, TimeSpan duration, int exitCode, string builderName);

    // All
        [OperationContract(IsOneWay = true)]
        void PingClient();

        [OperationContract(IsOneWay = true)]
        void ServerShutdown();
    }

    // TODO: We should keep a tally of just how much work each builder has (so we can possibly shift work around when overloaded?)

    class JobListHandler
    {
        private Dictionary<int, Job> jobList = new Dictionary<int, Job>();
        private Dictionary<int, Job> pendingList = new Dictionary<int, Job>();

        private readonly object syncJobList = new object();                         // Used to synchronise

        private HelloService helloService = null;

        public void SetHelloService(HelloService hs)
        {
            helloService = hs;
        }

        public Job Kill(int ID)
        {
            HelloService.AppLog.WriteLine("Kill: Job " + ID);

            lock (syncJobList)
            {
                Job job;

                if (jobList.TryGetValue(ID, out job))
                {
                    bool pending = (job.Status == JobStatus.Pending);

                    HelloService.AppLog.WriteLine(String.Format("Kill: Job {0} found. Pending({1}) Status({2})", ID, pending, job.Status));

                    job.Status = JobStatus.Killed;

                    // Is it farmed out yet? Kill before it gets the chance if possible
                    if (pending)
                    {
                        pendingList.Remove(ID);
                        return null;
                    }

                    return job;
                }
            }

            return null;
        }

        public bool Add(Job job)
        {
            lock (syncJobList)
            {
                jobList.Add(job.Request.JobID, job);
                pendingList.Add(job.Request.JobID, job);
            }

            return true;
        }

        public Job Find(int ID)
        {
            lock (syncJobList)
            {
                Job job;

                if (jobList.TryGetValue(ID, out job))
                {
                    return job;
                }
            }

            return null;
        }

        // Handles giving out work to the builders - allocates specific work to builders and deals with missing builders (and specific jobs)
        public JobRequest GetWork(int builderID)
        {
            // Look for work for this specific builder
            lock (syncJobList)
            {
                HelloService.AppLog.WriteLine(String.Format("GetWork: Builder({0}) - Pending({1})", builderID, pendingList.Count));

                foreach (KeyValuePair<int, Job> kvp in pendingList)
                {
                    if (kvp.Value.Request.BuilderID == builderID)
                    {
                        kvp.Value.Status = JobStatus.InProgress;
                        kvp.Value.ActualBuilderID = builderID;

                        pendingList.Remove(kvp.Key);

                        HelloService.AppLog.WriteLine(String.Format("GetWork: Builder({0}) - Builder match to Job {1}", builderID, kvp.Key));

                        return kvp.Value.Request;
                    }
                }
            }

            // We need to ask the host for a list of available builders - probably need a reference to it to help
            List<int> biList = helloService.BuilderIDList();

            lock (syncJobList)
            {
                foreach (KeyValuePair<int, Job> kvp in pendingList)
                {
                    // We only do work that is for builders that aren't currently online
                    if (biList.Contains(kvp.Value.Request.BuilderID) == false)
                    {
                        kvp.Value.Status = JobStatus.InProgress;
                        kvp.Value.ActualBuilderID = builderID;

                        pendingList.Remove(kvp.Key);

                        HelloService.AppLog.WriteLine(String.Format("GetWork: Builder({0}) - Doing work for Builder {1} - Job {2}", builderID, kvp.Value.Request.BuilderID, kvp.Key));

                        return kvp.Value.Request;
                    }
                }
            }

            HelloService.AppLog.WriteLine(String.Format("GetWork: Builder({0}) - No work found", builderID));

            return null;
        }
    };

    [ServiceBehavior(InstanceContextMode = InstanceContextMode.Single)] 
    [ServiceContract(CallbackContract = typeof(INotify), SessionMode = SessionMode.Required)]
    class HelloService
    {
        static protected ManualResetEvent shutdownEvent = new ManualResetEvent(false);

        static protected AutoResetEvent jobListChangedEvent = new AutoResetEvent(false);

        public delegate void BuilderListChanged(bool add, int builderID, string name);

        public BuilderListChanged builderChanged = null;

        static DailyLogger appLog;

        protected System.Timers.Timer timmyTheTimer = new System.Timers.Timer();

        #region Job Stuff

        protected int uniqueJobID = 0;

        public JobListHandler workList = new JobListHandler();

        #endregion

        #region Builder stuff

        Dictionary<int, BuilderInfo> builderDict = new Dictionary<int, BuilderInfo>();
        private readonly object syncBuilderDict = new object();
           
        public HelloService()
        {
            appLog = new DailyLogger("logs", "server", true);

            workList.SetHelloService(this);

            timmyTheTimer.Elapsed += new ElapsedEventHandler(_timer_Elapsed);
            timmyTheTimer.Interval = 6000;
            timmyTheTimer.Start();
        }

        static public DailyLogger AppLog
        {
            get
            {
                return appLog;
            }
        }

        public void Shutdown()
        {
            // Set an event to let us internally know we're going down
            shutdownEvent.Set();

            // Let's tell all the clients the server is going down
            lock (syncBuilderDict)
            {
                foreach (KeyValuePair<int, BuilderInfo> kvp in builderDict)
                {
                    kvp.Value.Callback.ServerShutdown();
                }
            }

            timmyTheTimer.Stop();
        }

        private void _timer_Elapsed(object sender, ElapsedEventArgs e)
        {
            // Drop out early if we're shutting down
            if (shutdownEvent.WaitOne(0, true) == true)
                return;

            //appLog.WriteLine("SERVER PING!!!");

            List<BuilderInfo> tempList = new List<BuilderInfo>();

            lock (syncBuilderDict)
            {
                foreach (KeyValuePair<int, BuilderInfo> kvp in builderDict)
                {
                    tempList.Add(kvp.Value);
                }
            }

            foreach (BuilderInfo bi in tempList)
            {
                try
                {
                    bi.Callback.PingClient();
                }
                catch (Exception ex)
                {
                    appLog.WriteLine(String.Format("PING failed to {0} ({1})", bi.Name, ex.Message));
                    RemoveBuilder(bi);
                }
            }
        }

        [OperationContract]
        int AddBuilder(int builderID, string clientName)
        {
            // Drop out early if we're shutting down
            if (shutdownEvent.WaitOne(0, true) == true)
                return 2;

            try
            {
                appLog.WriteLine("AddBuilder: " + clientName);

                BuilderInfo bi = new BuilderInfo();

                bi.Name = clientName;
                bi.Callback = OperationContext.Current.GetCallbackChannel<INotify>();

                lock (syncBuilderDict)
                {
                    builderDict.Add(builderID, bi);
                }

                if (builderChanged != null)
                    builderChanged(true, builderID, clientName);

                return 0;
            }
            catch (Exception ex)
            {
                appLog.WriteLine("AddBuilder Exception: " + ex);
                return 1;
            }
        }

        [OperationContract]
        void RemoveBuilder()
        {
            BuilderInfo biRemove = null;

            lock (syncBuilderDict)
            {
                foreach (KeyValuePair<int, BuilderInfo> kvp in builderDict)
                {
                    if (kvp.Value.Callback == OperationContext.Current.GetCallbackChannel<INotify>())
                    {
                        biRemove = kvp.Value;
                        break;
                    }
                }
            }

            if (biRemove != null)
            {
                appLog.WriteLine(String.Format("Builder {0} has requested to be removed.", biRemove.Name));
                RemoveBuilder(biRemove);
            }
        }

        void RemoveBuilder(BuilderInfo bi)
        {
            int biKey = -1;
            BuilderInfo biRemove = null;

            lock (syncBuilderDict)
            {
                foreach (KeyValuePair<int, BuilderInfo> kvp in builderDict)
                {
                    if (kvp.Value == bi)
                    {
                        builderDict.Remove(kvp.Key);

                        biRemove = kvp.Value;
                        biKey = kvp.Key;
                        break;
                    }
                }
            }

            if (biRemove != null)
            {
                if (builderChanged != null)
                    builderChanged(false, biKey, biRemove.Name);

                appLog.WriteLine("RemoveBuilder: " + biRemove.Name);
            }
        }

        #endregion

        [OperationContract]
        int GetJobID()
        {
            return uniqueJobID++;
        }

        [OperationContract]
        void KillJob(int ID)
        {
            Job job = workList.Kill(ID);

            if (job != null)
            {
                // Notify the builder doing this job to kill it
                {
                    BuilderInfo bi;
                    bool foundBuilder;

                    lock (syncBuilderDict)
                    {
                        foundBuilder = builderDict.TryGetValue(job.ActualBuilderID, out bi);
                    }

                    if (foundBuilder)
                    {
                        bi.Callback.TerminateJob(ID);
                    }
                }
            }
        }

        public BuilderInfo BuilderFromID(int ID)
        {
            lock (syncBuilderDict)
            {
                foreach (KeyValuePair<int, BuilderInfo> kvp in builderDict)
                {
                    if (kvp.Key == ID)
                        return kvp.Value;
                }
            }

            return null;
        }

        public List<int> BuilderIDList()
        {
            List<int> list = new List<int>();

            lock (syncBuilderDict)
            {
                foreach (KeyValuePair<int, BuilderInfo> kvp in builderDict)
                {
                    list.Add(kvp.Key);
                }
            }

            return list;
        }

        private List<BuilderInfo> BuilderList()
        {
            List<BuilderInfo> list = new List<BuilderInfo>();

            lock (syncBuilderDict)
            {
                foreach (KeyValuePair<int, BuilderInfo> kvp in builderDict)
                {
                    list.Add(kvp.Value);
                }
            }

            return list;
        }

        [OperationContract]
        bool SubmitJobs(JobRequest[] workToDo)
        {
            foreach (JobRequest jr in workToDo)
            {
                Job newJob = new Job(jr, OperationContext.Current.GetCallbackChannel<INotify>());

                appLog.WriteLine(String.Format("Added new job {0}", newJob.Request.JobID));

                workList.Add(newJob);
            }

            appLog.WriteLine("Notifying clients about new work");

            jobListChangedEvent.Set();

            // Notify all the builders that new work has come in
            {
                List<BuilderInfo> blist = BuilderList();

                foreach (BuilderInfo bi in blist)
                {
                    bi.Callback.WorkAvailable();
                }
            }

            return true;
        }

        [OperationContract]
        bool PingServer()
        {
            return true;
        }

        // This is called by the client to check if any work is available, and if so, be given work to do
        [OperationContract]
        JobRequest RequestWork(int builderID)
        {
            // Check for specific work for this builder
            JobRequest jr = workList.GetWork(builderID);

            return jr;
        }

        [OperationContract]
        void WorkDone(int jobID, TimeSpan duration, int exitCode)
        {
            appLog.WriteLine(String.Format("WorkDone: Job({0}) exitCode({1})", jobID, exitCode));

            Job job = workList.Find(jobID);

            if (job != null)
            {
                job.Callback.JobComplete(jobID, duration, exitCode, BuilderFromID(job.ActualBuilderID).Name);
            }
            else
            {
                appLog.WriteLine(String.Format("WorkDone: Job({0}) - Didn't find a match in workList", jobID));
            }
        }

        public void PingClients()
        {
            List<BuilderInfo> tempList = new List<BuilderInfo>();

            lock (syncBuilderDict)
            {
                foreach (KeyValuePair<int, BuilderInfo> kvp in builderDict)
                {
                    tempList.Add(kvp.Value);
                }
            }

            foreach (BuilderInfo bi in tempList)
            {
                bi.Callback.PingClient();
            }
        }
    }
}