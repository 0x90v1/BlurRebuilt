using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using MySql.Data;
using MySql.Data.MySqlClient;

using Bizarre.Artery;

using Bizarre.MySqlHelper;

namespace ArteryServer
{
    class Program
    {
        static bool Verbose = false;

        static bool AllowTimeout = true;

        static int TimeoutPeriod = 30 * 1000;

        public class ConnectionInfo
        {
            public Connection connection;

            public BizSqlConnection dbConnection;

            public string dbConnectionString;
            public string databaseName;

            public long sessionId = long.MaxValue;
            public long subSessionId = long.MaxValue;
        }

        static void HandleMessage_Session(ConnectionInfo connectionInfo, MessageReader msgReader)
        {
            SqlValuesBuilder ivb = new SqlValuesBuilder();

            int version;
            msgReader.Read(out version);

            int platformType;
            msgReader.Read(out platformType);

            string platformName = "Unknown";

            switch (platformType)
            {
                case 1:
                    platformName = "360";
                    break;
                case 2:
                    platformName = "PS3";
                    break;
                case 3:
                    platformName = "PC";
                    break;
            }

            ivb.AddColumnValuePair(eDlType.kDlString, "platform", platformName);

            string name;
            msgReader.Read(out name);

            connectionInfo.databaseName = name;

            SetupDatabase(connectionInfo.dbConnection, connectionInfo.databaseName);

            string tag;
            while (msgReader.Read(out tag))
            {
                switch (tag)
                {
                    case "KITNAME":
                        string kn;
                        msgReader.Read(out kn);
                        //System.Diagnostics.Debug.WriteLine("SESSION: KIT: " + kn);

                        ivb.AddColumnValuePair(eDlType.kDlString, "kit", kn);

                        System.Diagnostics.Debug.WriteLine("   - Kit    : " + kn + " (" + platformName + ")");
                        System.Console.WriteLine("   - Kit    : " + kn + " (" + platformName + ")");

                        break;

                    case "CONFIG":
                        string config;
                        msgReader.Read(out config);
                        //System.Diagnostics.Debug.WriteLine("SESSION: CONFIG: " + config);

                        ivb.AddColumnValuePair(eDlType.kDlString, "config", config);

                        System.Diagnostics.Debug.WriteLine("   - Config : " + config + " (" + platformName + ")");
                        System.Console.WriteLine("   - Config : " + config + " (" + platformName + ")");

                        break;

                    case "VERSION":
                        string ver;
                        msgReader.Read(out ver);
                        //System.Diagnostics.Debug.WriteLine("SESSION: VERSION: " + ver);

                        ivb.AddColumnValuePair(eDlType.kDlString, "version", ver);

                        System.Diagnostics.Debug.WriteLine("   - Version: " + ver + " (" + platformName + ")");
                        System.Console.WriteLine("   - Version: " + ver + " (" + platformName + ")");

                        break;

                    case "REFERENCE":
                        string reference;
                        msgReader.Read(out reference);
                        //System.Diagnostics.Debug.WriteLine("SESSION: REFERENCE: " + reference);

                        ivb.AddColumnValuePair(eDlType.kDlString, "ref", reference);

                        System.Diagnostics.Debug.WriteLine("   - Ref    : " + reference);
                        System.Console.WriteLine("   - Ref    : " + reference);

                        break;

                    default:
                        string errmsg = string.Format("ERROR: SESSION: Unknown tag '{0}'", tag);
                        System.Diagnostics.Debug.WriteLine(errmsg);
                        System.Console.WriteLine(errmsg);
                        break;
                }
            }

            // We need to create an entry in the session table now - store down the Session Id so we can reference in other tables
            {
                string insertData = "INSERT INTO `_sessions` (" + ivb.Columns + ") VALUES (" + ivb.Values + ")";

                MySqlCommand myCommand = new MySqlCommand(insertData, connectionInfo.dbConnection.SqlConnection);

                myCommand.ExecuteNonQuery();

                connectionInfo.sessionId = myCommand.LastInsertedId;
            }
        }

        static void HandleMessage_SubSession(ConnectionInfo connectionInfo, MessageReader msgReader)
        {
            SqlValuesBuilder ivb = new SqlValuesBuilder();

            string tag;
            while (msgReader.Read(out tag))
            {
                switch (tag)
                {
                    case "GAMERTAG":
                        string gt;
                        msgReader.Read(out gt);

                        ivb.AddColumnValuePair(eDlType.kDlString, "gamertag", gt);

                        break;

                    default:
                        string errmsg = string.Format("ERROR: SUBSESSION: Unknown tag '{0}'", tag);
                        System.Diagnostics.Debug.WriteLine(errmsg);
                        System.Console.WriteLine(errmsg);
                        break;
                }
            }

            ivb.AddColumnValuePair(eDlType.kDlU32, "_sessionid", connectionInfo.sessionId.ToString());

            long insertId = connectionInfo.dbConnection.InsertDataIntoTable("_subsessions", ivb);

            if (insertId != -1)
            {
                connectionInfo.subSessionId = insertId;
            }
        }

        static void MyCallback(Connection conn, object context)
        {
            WaitContext ctx = new WaitContext();

            ConnectionInfo connectionInfo = new ConnectionInfo();

            connectionInfo.connection = conn;
            connectionInfo.dbConnectionString = BuildConnectionString(false);
            connectionInfo.dbConnection = new BizSqlConnection();

            try
            {
                if (connectionInfo.dbConnection.Connect(connectionInfo.dbConnectionString))
                {

                    System.Diagnostics.Debug.WriteLine("Connection established");
                    System.Console.WriteLine("Connection established");

                    Dictionary<int, SqlTableDefinition> tableDictionary = new Dictionary<int, SqlTableDefinition>();

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

                        if (waitRet)
                        {
                            Message msg = new Message();

                            while (conn.TryReceive(msg))
                            {
                                MessageReader msgReader = new Bizarre.Artery.MessageReader(msg);

                                string s;

                                bool r = msgReader.Read(out s);
                                if (r)
                                {
                                    switch (s)
                                    {
                                        case "PING":
                                            break;

                                        case "SESSION":
                                            HandleMessage_Session(connectionInfo, msgReader);
                                            break;

                                        case "SUBSESSION":
                                            // We need to create an entry in the session table now - store down the Session Id so we can reference in other tables
                                            HandleMessage_SubSession(connectionInfo, msgReader);
                                            break;

                                        case "ENDSUBSESSION":
                                            // Update sub-session with duration information
                                            if (connectionInfo.subSessionId != long.MaxValue)
                                            {
                                                string insertData = "UPDATE `" + connectionInfo.databaseName + "`.`_subsessions` SET `end`=CURRENT_TIMESTAMP WHERE `_id`='" + connectionInfo.subSessionId + "'";

                                                MySqlCommand myCommand = new MySqlCommand(insertData, connectionInfo.dbConnection.SqlConnection);

                                                myCommand.ExecuteNonQuery();

                                                // Invalidate the SubSession ID
                                                connectionInfo.subSessionId = long.MaxValue;
                                            }
                                            break;

                                        case "TABLE":
                                            {
                                                int version;
                                                msgReader.Read(out version);

                                                int ist;
                                                msgReader.Read(out ist);
                                                bool isLinkedToSubSession = (ist == 1);

                                                string tableName;
                                                msgReader.Read(out tableName);

                                                int tableId;
                                                msgReader.Read(out tableId);

                                                if (!tableDictionary.ContainsKey(tableId))
                                                {
                                                    SqlTableDefinition td = new SqlTableDefinition() { Name = tableName, LinkedToSubSession = isLinkedToSubSession };

                                                    int variableCount;
                                                    msgReader.Read(out variableCount);

                                                    if (Verbose)
                                                        System.Diagnostics.Debug.WriteLine(string.Format("Table: {0} - Id({1}) VC({2})", tableName, tableId, variableCount));

                                                    // If the table is per session, we need to store the session ID in the records
                                                    if (isLinkedToSubSession)
                                                    {
                                                        td.Columns.Add(new SqlTableDefinition.Column() { Name = "_subsessionid", Type = eDlType.kKeyReference });
                                                        td.Indexes.Add(new SqlTableDefinition.Index() { Name = "ssIndex", Columns = { "_subsessionid" } } );
                                                        td.ForeignKeys.Add(new SqlTableDefinition.ForeignKey() { Column = "_subsessionid", ForeignTable = "_subsessions", ForeignColumn = "_id" } );
                                                    }
                                                    else
                                                    {
                                                        td.Columns.Add(new SqlTableDefinition.Column() { Name = "_sessionid", Type = eDlType.kKeyReference });
                                                        td.Indexes.Add(new SqlTableDefinition.Index() { Name = "sIndex", Columns = { "_sessionid" } });
                                                        td.ForeignKeys.Add(new SqlTableDefinition.ForeignKey() { Column = "_sessionid", ForeignTable = "_sessions", ForeignColumn = "_id" } );
                                                    }

                                                    // Special table with some default values of our own - start and end time of the subsession for now
                                                    if (tableName == "_subsessions")
                                                    {
                                                        System.Diagnostics.Debug.Assert(isLinkedToSubSession == false);
                                                        td.Columns.Add(new SqlTableDefinition.Column() { Name = "start", Type = eDlType.kAutoTimestamp });
                                                        td.Columns.Add(new SqlTableDefinition.Column() { Name = "end", Type = eDlType.kTimestamp });
                                                    }

                                                    for (int i = 0; i < variableCount; i++)
                                                    {
                                                        string name;
                                                        int type;

                                                        msgReader.Read(out name);
                                                        msgReader.Read(out type);

                                                        SqlTableDefinition.Column tdc = new SqlTableDefinition.Column() { Name = name, Type = (eDlType)type };

                                                        td.Columns.Add(tdc);

                                                        if (Verbose)
                                                            System.Diagnostics.Debug.WriteLine("   Field: " + name + " - " + type);
                                                    }

                                                    connectionInfo.dbConnection.CreateTable(td);

                                                    tableDictionary.Add(tableId, td);
                                                }
                                            }
                                            break;

                                        case "EVENT":
                                            {
                                                int version;
                                                msgReader.Read(out version);

                                                int tableId;
                                                msgReader.Read(out tableId);

                                                SqlTableDefinition td = tableDictionary[tableId];

                                                if (td.LinkedToSubSession && connectionInfo.subSessionId == long.MaxValue)
                                                {
                                                    if (Verbose)
                                                        System.Diagnostics.Debug.WriteLine(string.Format("Event: {0} - SessionId not set", td.Name));
                                                    break;
                                                }

                                                if (Verbose)
                                                    System.Diagnostics.Debug.WriteLine(string.Format("Event: {0}", td.Name));

                                                SqlValuesBuilder ivb = new SqlValuesBuilder();

                                                for (int i = 0; i < td.Columns.Count; i++)
                                                {
                                                    SqlTableDefinition.Column tdc = td.Columns[i];

                                                    string valueString;

                                                    switch (tdc.Type)
                                                    {
                                                        // TODO: Is Bool implemented properly?
                                                        case eDlType.kDlBool:
                                                            {
                                                                int value;
                                                                msgReader.Read(out value);
                                                                valueString = (value == 1) ? "1" : "0";
                                                            }
                                                            break;

                                                        case eDlType.kDlS8:
                                                            {
                                                                sbyte value;
                                                                msgReader.Read(out value);
                                                                valueString = value.ToString();
                                                            }
                                                            break;

                                                        case eDlType.kDlU8:
                                                            {
                                                                byte value;
                                                                msgReader.Read(out value);
                                                                valueString = value.ToString();
                                                            }
                                                            break;

                                                        case eDlType.kDlS32:
                                                            {
                                                                int value;
                                                                msgReader.Read(out value);
                                                                valueString = value.ToString();
                                                            }
                                                            break;

                                                        case eDlType.kDlU32:
                                                            {
                                                                uint value;
                                                                msgReader.Read(out value);
                                                                valueString = value.ToString();
                                                            }
                                                            break;

                                                        case eDlType.kDlF32:
                                                            {
                                                                float value;
                                                                msgReader.Read(out value);
                                                                valueString = value.ToString();
                                                            }
                                                            break;


                                                        case eDlType.kDlS64:
                                                            {
                                                                long value;
                                                                msgReader.Read(out value);
                                                                valueString = value.ToString();
                                                            }
                                                            break;

                                                        case eDlType.kDlString:
                                                            {
                                                                string value;
                                                                msgReader.Read(out value);
                                                                valueString = value;
                                                            }
                                                            break;

                                                        case eDlType.kTimestamp:
                                                            // TODO: Fix up timestamp entry
                                                            continue;

                                                        // Skip this field as it's internal to the server
                                                        case eDlType.kKeyReference:
                                                        case eDlType.kAutoTimestamp:
                                                            continue;

                                                        default:
                                                            System.Diagnostics.Debug.WriteLine("Type not recognised: " + tdc.Type);
                                                            return;
                                                    }

                                                    ivb.AddColumnValuePair(tdc.Type, tdc.Name, valueString);

                                                    if (Verbose)
                                                        System.Diagnostics.Debug.WriteLine("   Field: " + tdc.Name + ": " + valueString);
                                                }

                                                if (td.LinkedToSubSession)
                                                {
                                                    ivb.AddColumnValuePair(eDlType.kDlS32, "_subsessionid", connectionInfo.subSessionId.ToString());
                                                }
                                                else
                                                {
                                                    ivb.AddColumnValuePair(eDlType.kDlS32, "_sessionid", connectionInfo.sessionId.ToString());
                                                }

                                                long insertId = connectionInfo.dbConnection.InsertDataIntoTable(td.Name, ivb);

                                                // Special table
                                                if (insertId != -1 && td.Name == "_subsessions")
                                                {
                                                    connectionInfo.subSessionId = insertId;
                                                }
                                            }
                                            break;
                                    }
                                }
                            }
                        }
                        else
                        {
                            // Nothing for the timeout period - we assume the connection is dropped
                            System.Console.WriteLine("Timeout occured");
                            break;
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                string message = "Exception: MyCallback: " + ex.Message;
                System.Diagnostics.Debug.WriteLine(message);
                System.Console.WriteLine(message);
            }
            finally
            {
                connectionInfo.dbConnection.Disconnect();
            }

            System.Diagnostics.Debug.WriteLine("Connection closed");
            System.Console.WriteLine("Connection closed");
        }

        static void SetupDatabase(BizSqlConnection dbConnection, string databaseName)
        {
            string query = "CREATE DATABASE IF NOT EXISTS `" + databaseName + "`";

            MySqlCommand cmd = new MySqlCommand(query, dbConnection.SqlConnection);

            cmd.ExecuteNonQuery();

            cmd = new MySqlCommand("USE `" + databaseName + "`", dbConnection.SqlConnection);

            cmd.ExecuteNonQuery();

            // Now setup the session and subsession tables
            SqlTableDefinition sessionTd = new SqlTableDefinition();
            sessionTd.Name = "_sessions";
            sessionTd.Columns.Add(new SqlTableDefinition.Column() { Name = "kit", Type = eDlType.kDlString });
            sessionTd.Columns.Add(new SqlTableDefinition.Column() { Name = "config", Type = eDlType.kDlString });
            sessionTd.Columns.Add(new SqlTableDefinition.Column() { Name = "version", Type = eDlType.kDlString });
            sessionTd.Columns.Add(new SqlTableDefinition.Column() { Name = "platform", Type = eDlType.kDlString });
            sessionTd.Columns.Add(new SqlTableDefinition.Column() { Name = "time", Type = eDlType.kAutoTimestamp });
            sessionTd.Columns.Add(new SqlTableDefinition.Column() { Name = "ref", Type = eDlType.kDlString });
            dbConnection.CreateTable(sessionTd);

            SqlTableDefinition subSessionTd = new SqlTableDefinition();
            subSessionTd.Name = "_subsessions";
            subSessionTd.Columns.Add(new SqlTableDefinition.Column() { Name = "_sessionid", Type = eDlType.kKeyReference });
            subSessionTd.Columns.Add(new SqlTableDefinition.Column() { Name = "start", Type = eDlType.kAutoTimestamp });
            subSessionTd.Columns.Add(new SqlTableDefinition.Column() { Name = "end", Type = eDlType.kTimestamp });
            subSessionTd.Columns.Add(new SqlTableDefinition.Column() { Name = "gamertag", Type = eDlType.kDlString });
            subSessionTd.Indexes.Add(new SqlTableDefinition.Index() { Name = "sIndex", Columns = { "_sessionid" } });
            subSessionTd.ForeignKeys.Add(new SqlTableDefinition.ForeignKey() { Column = "_sessionid", ForeignTable = "_sessions", ForeignColumn = "_id" });
            dbConnection.CreateTable(subSessionTd);
        }

        static string BuildConnectionString(bool displayConnectionInfo)
        {
            string ipAddress = "127.0.0.1";
            string username = "datalog";
            string password = "datalog";

            if (System.IO.File.Exists("server.ini"))
            {
                System.IO.StreamReader sr = new System.IO.StreamReader("server.ini");

                while (!sr.EndOfStream)
                {
                    string line = sr.ReadLine();

                    string[] items = line.Split('=');

                    if (items.Length == 2)
                    {
                        switch (items[0].ToLower())
                        {
                            case "ip":
                                ipAddress = items[1].Trim();
                                break;
                            case "username":
                                username = items[1].Trim();
                                break;
                            case "password":
                                password = items[1].Trim();
                                break;
                        }
                    }
                }
            }
            else
            {
                System.Console.WriteLine("'server.ini' not found.");
            }

            if (displayConnectionInfo)
            {
                System.Console.WriteLine("Trying to connect to:");
                System.Console.WriteLine("\tServer  : " + ipAddress);
                System.Console.WriteLine("\tUsername: " + username);
            }

            // Default string to connect to localhost with default user/pwd
            //return "Database=mysql;Data Source=127.0.0.1;User Id=datalog;Password=datalog";
            //return string.Format("Database=mysql;Data Source={0};User Id={1};Password={2}", ipAddress, username, password);
            return string.Format("Database=information_schema;Data Source={0};User Id={1};Password={2}", ipAddress, username, password);
        }

        static void Main(string[] args)
        {
            string databaseConnection = BuildConnectionString(true);

            try
            {
                MySqlConnection dbConnection = new MySqlConnection(databaseConnection);

                dbConnection.Open();
                dbConnection.Close();

                System.Console.WriteLine("Connection to MySQL established.");

                ArteryManager.StartupEx(true, "DataLogServer", 100);

                ArteryManager.Register("datalog", MyCallback, null);

                while (true)
                {
                    System.Threading.Thread.Sleep(1000);
                }
            }
            catch (MySql.Data.MySqlClient.MySqlException ex)
            {
                switch (ex.Number)
                {
                    case 1042:
                        System.Console.WriteLine("ERROR: Database not possibly running at expected location.");
                        break;

                    case 1044:
                        System.Console.WriteLine("ERROR: Access denied.");
                        break;

                    case 1045:
                        System.Console.WriteLine("ERROR: Problem logging in with username/password.");
                        break;
                }
                
                System.Console.WriteLine("Exception: " + ex.Message);
            }
        }
    }
}
