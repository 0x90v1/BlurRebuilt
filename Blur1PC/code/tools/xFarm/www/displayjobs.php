<?php

$sqlconnect = "farmserver";
session_start();

if ($HTTP_SESSION_VARS["name"] == "")
{
	//Echo('<HTML><A HREF="index.htm">click here to go to the farmserver</A></HTML>');
	Echo('<HTML><META HTTP-EQUIV="Refresh" CONTENT="0; URL=index.htm"></HTML>');
	exit();
}

$filter = $_GET['filter'];
if ($filter)
	$page = 0;
else
	$page = 1;

include('farmcommon.php');
include('listjobs.php');
include('ldap.php');

$userinfo = GetUserInfo($HTTP_SESSION_VARS["name"]);

// Get all the data from the "example" table
$first = $_GET['first'];
if ($first == "")
    $first = 0;
$last = $first + $pagesize + 2;

if ($page == 0)
{
	$result = mysql_query("SELECT * FROM jobs WHERE user='BIZARRE2/" . $HTTP_SESSION_VARS["name"] . "' ORDER BY id DESC LIMIT $first,$last")
	or die(mysql_error());
}
else
{
	$result = mysql_query("SELECT * FROM jobs WHERE " . GetJobFilterString() . " ORDER BY id DESC LIMIT $first,$last ")
	or die(mysql_error());
}

if ($page == 0)
{
	echo("<center>Job List for " . $userinfo[0]['displayname'][0] . "</center><br>");
}
else
{
	echo("<center>Farm Job List</center><br>");
}
	

//echo("Hello " . $userinfo[0]['displayname'][0] . "<br>");

if ($page == 0)
{
	// filter out jobs that are not assigned to the current user
	echo ListJobs($result, $first, $last, $pagesize, false);
}
else
{
	echo ListJobs($result, $first, $last, $pagesize, false);
}


# if there was another result at the end, display a "next" button
$row = mysql_fetch_array( $result );

# DISPLAY NEXT/PREVIOUS PAGE BUTTONS

?>
   <td class="indexo">
    <table align=center class="indexnewslinks">
     <tr>
      <td>
<?
if ($first >= 10)
    printf("<a href=\"displayjobs.php?first=%d&filter=" . $filter . "\">Previous page</a>", $first-10);
else
    echo("<font color=#808080>Previous page</font>");
?>
      </td><td>/</td>
      <td class="tdright">
<?
if ($row['id'] != 0)
    printf("<a href=\"displayjobs.php?first=%d&filter=" . $filter . "\">Next page</a>", $first+10);
else
    echo("<font color=#808080>Next page</font>");
?>
      </td>
     </tr>
    </table>
   </td>



<?include('footer.php')?>


