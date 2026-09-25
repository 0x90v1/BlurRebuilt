using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Collections.ObjectModel;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

using System.ServiceModel;
using System.ServiceModel.Description;

using System.Windows.Threading;

namespace WpfApplication1
{
    /// <summary>
    /// Interaction logic for Window1.xaml
    /// </summary>
    public partial class Window1 : Window
    {
        public delegate void MyDelegateType();

        ServiceHost host = null;
        ServerWCF.HelloService mySingleton = null;

        class BuilderInfo
        {
            public int ID
            {
                get;
                set;
            }
            public string name
            {
                get;
                set;
            }
        }

        class Hmmm
        {
            public string Name { get; set; }
            public Int32 ID { get; set; }
        }

        ObservableCollection<Hmmm> jobStuff = new ObservableCollection<Hmmm>();

        void BuildersChanged(bool add, int builderID, string name)
        {
            if (add)
                AddBuilder(builderID, name);
            else
                RemoveBuilder(builderID);
        }

        public Window1()
        {
            InitializeComponent();

            // We did not separate contract from implementation.
            // Therefor service and contract are the same in this example.
            Type serviceType = typeof(ServerWCF.HelloService);

            mySingleton = new ServerWCF.HelloService();

            // ServiceHost
            //host = new ServiceHost(serviceType, new Uri[] { new Uri("http://localhost:8080/") });
            host = new ServiceHost(mySingleton, new Uri[] { new Uri("http://" + System.Environment.MachineName + ":8080/") });

            mySingleton.builderChanged += BuildersChanged;

            // Add behavior for our MEX endpoint
            ServiceMetadataBehavior behavior = new ServiceMetadataBehavior();
            behavior.HttpGetEnabled = true;
            host.Description.Behaviors.Add(behavior);

            ServiceDebugBehavior behaviorDebug = host.Description.Behaviors.Find<ServiceDebugBehavior>();
            behaviorDebug.IncludeExceptionDetailInFaults = true;

            WSDualHttpBinding wsdhb = new WSDualHttpBinding();
            wsdhb.Security.Mode = WSDualHttpSecurityMode.None;

            // Create basicHttpBinding endpoint at http://localhost:8080/HelloService/
            host.AddServiceEndpoint(serviceType, wsdhb, "HelloService");

            // Add MEX endpoint at http://localhost:8080/MEX/
            host.AddServiceEndpoint(typeof(IMetadataExchange), new WSDualHttpBinding(), "MEX");

            try
            {
                host.Open();
            }
            catch (Exception ex)
            {
                int mrf = 10;
            }

            //jobStuff.Add(new Hmmm() { ID = 147, Name = "Mr Tickle" });

            jobList.DataContext = jobStuff;
        }

        protected override void OnClosing(System.ComponentModel.CancelEventArgs e)
        {
            base.OnClosing(e);

            mySingleton.Shutdown();

            host.Abort();
            host.Close();
            host = null;
        }

        public void AddBuilder(int builderID, string name)
        {
            BuilderInfo bi = new BuilderInfo();
            bi.ID = builderID;
            bi.name = name;

            MyDelegateType addBuilder = delegate
            {
                clientList.Items.Add(bi);
            };
            this.Dispatcher.BeginInvoke(DispatcherPriority.Normal, addBuilder);
        }
        
        public void RemoveBuilder(int builderID)
        {
            try
            {
                foreach (BuilderInfo bi in clientList.Items)
                {
                    if (bi.ID == builderID)
                    {
                        MyDelegateType removeBuilder = delegate
                        {
                            clientList.Items.Remove(bi);
                            clientList.InvalidateVisual();
                        };
                        this.Dispatcher.BeginInvoke(DispatcherPriority.Normal, removeBuilder);

                        break;
                    }
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine("RemoveBuilder Exception: " + ex);
            }
        }

        private void Button_Click_1(object sender, RoutedEventArgs e)
        {
            mySingleton.PingClients();
        }
    }
}
