<?php

//$sqlconnect = "farmserver";
session_start();

if ($HTTP_SESSION_VARS["name"] == "")
{
	//Echo('<HTML><A HREF="index.htm">click here to go to the farmserver</A></HTML>');
	Echo('<HTML><META HTTP-EQUIV="Refresh" CONTENT="0; URL=index.htm"></HTML>');
	exit();
}

$page = 5;

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
	$result = mysql_query("SELECT * FROM jobs ORDER BY id DESC LIMIT $first,$last ")
	or die(mysql_error());
}

$search = $_POST['search'];
if ($search == "")
	$search = $_GET['search'];

if ($search)
{
// display the job list that we searched for
$first = $_GET['first'];
if ($first == "")
    $first = 0;
$last = $first + $pagesize + 2;

	$result = mysql_query("SELECT * FROM jobs WHERE comment LIKE '%".$search."%' ORDER BY id DESC LIMIT $first,$last ")
	or die(mysql_error());

	echo("<center>Farm Search Results</center><br>");

	echo ListJobs($result, $first, $last, $pagesize, false);


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
    printf("<a href=\"search.php?first=%d&search=%s\">Previous page</a>", $first-10, $search);
else
    echo("<font color=#808080>Previous page</font>");
?>
      </td><td>/</td>
      
      <td>
      <a href="search.php">search again</a>
      </td>
      
      <td>/</td>
      
      <td class="tdright">
<?
if ($row['id'] != 0)
    printf("<a href=\"search.php?first=%d&search=%s\">Next page</a>", $first+10, $search);
else
    echo("<font color=#808080>Next page</font>");
?>
      </td>
     </tr>
    </table>
   </td>
<?
}
else
{
	echo("<center>Search Job List</center><br>");
?>
   <td class="indexo">
    <table align=center class="indexnewslinks">
     <tr>
      <td>
      <br><br><br><br><br><br>
	  Enter job name to search for:
<?
// display the search form
Echo('<form action="search.php" method="post">');
?>
<input type="text" name="search"/>
<input type="submit" value="Search"/>
</form>
    <br><br><br><br><br><br>

      </td>
     </tr>
    </table>
   </td>
<div id="stats">
<?
}
?>


<?include('footer.php')?>


