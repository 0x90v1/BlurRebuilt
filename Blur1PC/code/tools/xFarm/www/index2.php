<?php

session_start();			// This connects to the existing session
session_register ("name");	// Create a session variable called name
$HTTP_SESSION_VARS ["name"] = $_GET['user'];

# CONNECT TO SQL DATABASE

// Make a MySQL Connection
if (!$sqlconnect)
    $sqlconnect="localhost";

// check whether any jobs exist that this user started
mysql_connect($sqlconnect, "farmaccess", "") or die(mysql_error());
mysql_select_db("farm") or die(mysql_error());

$result = mysql_query("SELECT * FROM jobs WHERE user='BIZARRE2/" . $HTTP_SESSION_VARS["name"] . "' ORDER BY id DESC LIMIT 0,10")
or die(mysql_error());

$count = 0;
while($row = mysql_fetch_array( $result ))
{
	$count++;
}

// go to 'my jobs' if there are any
if ($count > 0)
{
	Echo('<HTML><META HTTP-EQUIV="Refresh" CONTENT="0; URL=\displayjobs.php?filter=1"></HTML>');
}
else
// otherwise go to 'list jobs'
{
	Echo('<HTML><META HTTP-EQUIV="Refresh" CONTENT="0; URL=\displayjobs.php"></HTML>');
}

?>
