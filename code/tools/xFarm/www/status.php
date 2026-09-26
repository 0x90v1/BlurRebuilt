<?php

$sqlconnect = "farmserver";

# get last visited date cookie
$lastvisited = $HTTP_COOKIE_VARS["lastvisited"];
if ($lastvisited == 0)
	$lastvisited = "1-1-1900";

# update the cookie
$now = date("Y-m-j H:i:s");
setcookie("lastvisited",$now);

include('farmcommon.php');
include('listjobs.php');
$page = 0;

// Get all the data from the "example" table
$first = 0;
$last = 6;

echo("<center>Welcome</center><br>");

$result = mysql_query("SELECT * FROM jobs WHERE end>'$lastvisited' ORDER BY id DESC LIMIT $first,8 ")
or die(mysql_error());

$table = ListJobs($result, $first, $last, $last, 1);
if ($table)
{
	echo(TinyText("<center>New jobs have been completed since your last visit:</center>"));
	echo($table);
}
else
	echo(TinyText("<center>No New Jobs Completed Since Your Last Visit</center><br>"));
	
$row = mysql_fetch_array( $result );
if ($row != 0)
	echo(TinyText("more.."));


echo("<br>");

$result = mysql_query("SELECT * FROM jobs WHERE STATUS=2 ORDER BY user DESC LIMIT $first,$last ")
or die(mysql_error());

echo("<center>Currently Active Jobs</center><br>");

echo ListJobs($result, $first, $last, $last, 1);

echo("</div>");

include('footer.php')
?>

