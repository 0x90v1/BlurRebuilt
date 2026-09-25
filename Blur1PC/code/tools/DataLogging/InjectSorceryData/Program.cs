using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using XtInterop;

using MySql.Data;
using MySql.Data.MySqlClient;

using Bizarre.MySqlHelper;
using Bizarre.Misc;

namespace InjectSorceryData
{
    class Program
    {
        static int UploadRecords(BizSqlConnection dbConnection, XtDb d, string type, string field)
        {
            var records = from r in d.Records where r.Type == type select r;

            foreach (var r in records)
            {
                SqlValuesBuilder ivb = new SqlValuesBuilder();

                string value = ((XtField)r.Root.Data[field]).Value;

//                System.Diagnostics.Debug.WriteLine(string.Format("Upload: {0}: {1:X8} - {2}", type, r.Id, value));

                // Table - id, name
                ivb.AddColumnValuePair(eDlType.kDlU32, "_id", r.Id.ToString());
                ivb.AddColumnValuePair(eDlType.kDlString, "name", value);

                dbConnection.UpdateDataInTable("_strings", ivb);
            }

            return records.Count();
        }

        static int UploadLoc(BizSqlConnection dbConnection, XtDb d)
        {
            var locRecords = from r in d.Records where r.Type == "XtLocTable" select r;

            int recCount = 0;
            int locRCount = 1;

            foreach (var lr in locRecords)
            {
                XtArray stringArray = lr.Root.Data["strings"] as XtArray;

                System.Console.WriteLine(string.Format("- Processing loc record {0} of {1} - {2} strings", locRCount, locRecords.Count(), stringArray.Count));

                for (int i = 0; i < stringArray.Count; i++)
                {
                    XtBlock block = stringArray[i] as XtBlock;

                    uint id = stringArray.GetIdByIndex(i);

                    XtField name = block["name"] as XtField;

                    if (block.Keys.Contains("english"))
                    {
                        XtField english = block["english"] as XtField;

                        SqlValuesBuilder ivb = new SqlValuesBuilder();

                        ivb.AddColumnValuePair(eDlType.kDlU32, "_id", id.ToString());
                        ivb.AddColumnValuePair(eDlType.kDlString, "name", name.Value);
                        ivb.AddColumnValuePair(eDlType.kDlString, "english", english.Value);

                        dbConnection.UpdateDataInTable("_locstrings", ivb);

                        recCount++;
                    }

                    //System.Diagnostics.Debug.WriteLine(string.Format("{0:X8} - {1} : {2}", id, name.Value, english.Value));
                }

                locRCount++;
            }

            return recCount;
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

        public class Output
        {
            public static void WriteLine(string s)
            {
                System.Console.WriteLine(s);
                System.Diagnostics.Debug.WriteLine(s);
            }
        }

        static void Main(string[] args)
        {
            Arguments arguments = new Arguments(args);

            if (arguments["sorcery"] == null)
            {
                Output.WriteLine("Need to specify path to Sorcery database (-sorcery)");
                return;
            }

            if (arguments["database"] == null)
            {
                Output.WriteLine("Need to specify name of the database (-database)");
                return;
            }

            if (arguments["values"] == null)
            {
                Output.WriteLine("Need to specify values (e.g. -values \"SorceryRecordType:Field\")");
                return;
            }

            XtDb d = new XtDb();

            bool ok = d.Open(arguments["sorcery"]);
            if (!ok)
            {
                Output.WriteLine(string.Format("XtDb.Open of '{0}' failed '{1}'", arguments["sorcery"], d.GetErrorString()));
                return;
            }
           
            BizSqlConnection dbConnection = new BizSqlConnection();

            string dbConnectString = BuildConnectionString(true);

            if (dbConnection.Connect(dbConnectString))
            {
                System.Console.WriteLine("Connection to MySQL established.");

                string databaseName = arguments["database"];

                // Create database - if it doesn't already exist
                dbConnection.CreateDatabase(databaseName);
                dbConnection.UseDatabase(databaseName);

                // Create record name table
                SqlTableDefinition stringsTd = new SqlTableDefinition();
                stringsTd.Name = "_strings";
                stringsTd.Columns.Add(new SqlTableDefinition.Column() { Name = "name", Type = eDlType.kDlString, CanBeNull = false });
                dbConnection.CreateTable(stringsTd);

                // Create loc string table
                SqlTableDefinition locStringTd = new SqlTableDefinition();
                locStringTd.Name = "_locstrings";
                locStringTd.Columns.Add(new SqlTableDefinition.Column() { Name = "name", Type = eDlType.kDlString, CanBeNull = false });
                locStringTd.Columns.Add(new SqlTableDefinition.Column() { Name = "english", Type = eDlType.kDlString, CanBeNull = false });
                dbConnection.CreateTable(locStringTd);

                string[] values = arguments["values"].Split(' ');

                foreach (var value in values)
                {
                    string[] pair = value.Split(':');

                    int count = UploadRecords(dbConnection, d, pair[0], pair[1]);

                    Output.WriteLine(string.Format("Updated {0} values for Record({1}) Field({2})", count, pair[0], pair[1]));
                }

                int locCount = UploadLoc(dbConnection, d);
                Output.WriteLine(string.Format("Updated {0} values for Loc Strings", locCount));

                // Write the data we want
                //UploadRecords(dbConnection, d, "AmaxVehicle", "Name");
                //UploadRecords(dbConnection, d, "Route", "Name");
                //UploadRecords(dbConnection, d, "AmaxEvent", "Name");

                // Close database again
                dbConnection.Disconnect();
            }
        }
    }
}
