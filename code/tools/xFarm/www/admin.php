<?php

$page = 3;

include('farmcommon.php');

# PRINT COMMANDS
?>
<center>Admin Options</center>
<br>
<div id="stats">
<table class="rowod">
<tr><th><a href="admin.php?command=1">Reset Database</a></th></tr>
</table>
<br>
<?
# INTERPRET COMMANDS AND PRINT RESULTS

// Get all the data from the "example" table
$item = $_GET['command'];
if ($item == "1")
{
	# delete entire jobs table + contents
    mysql_query("DROP TABLE jobs");

	# delete entire workers table + contents
    mysql_query("DROP TABLE workers");

	# delete entire users table + contents
    mysql_query("DROP TABLE users");

	# create a new jobs table
	mysql_query("CREATE TABLE jobs( ".
		"id INT NOT NULL AUTO_INCREMENT, ".
		"user TEXT NOT NULL, ".
		"commandline TEXT NOT NULL, ".
		"comment TEXT NOT NULL, ".
		"project TEXT NOT NULL, ".
		"log TEXT NOT NULL, ".
		"status INT NOT NULL, ".
		"created DATETIME NOT NULL, ".
		"start DATETIME NOT NULL, ".
		"end DATETIME NOT NULL, ".
		"progressv INT NOT NULL, " .
		"progressd TEXT NOT NULL, " .
		"worker TEXT NOT NULL, ".
		"PRIMARY KEY(id) )");
		
    # create a new workers table
    mysql_query("CREATE TABLE workers( ".
        "time DATETIME NOT NULL, ".
        "PRIMARY KEY(time) )");

    # create a new users table
    mysql_query("CREATE TABLE users( ".
		"user TEXT, ".
		"email TEXT, ".
		"mycomplete BOOL, ".
		"myfail BOOL, ".
		"mycrash BOOL, ".
		"showgothamcar BOOL, ".
		"showgothamcity BOOL, ".
		"showtheclub BOOL, ".
		"showunknown BOOL, ".
		"PRIMARY KEY(user( 100 ) ) )");

?>
    <table><tr><td class="test">
    The database was reset
    </td></tr></table>
<?
}


# PRINT FOOTER
include('footer.php')?>
