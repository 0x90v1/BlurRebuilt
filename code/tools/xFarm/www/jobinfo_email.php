<?php

//$sqlconnect = "farmserver";
session_start();

$email = "1";

include('farmcommon.php');
include('ldap.php');
include('logcommon.php');
$page = 0;

// Get all the data from the "example" table
$item = $argv[1];
if ($item == "")
    $item=1;

$result = mysql_query("SELECT * FROM jobs WHERE id = $item ORDER BY id DESC")
or die(mysql_error());

# PRINT JOB INFO
$row = mysql_fetch_array( $result );

# calculate duration
$start = $row['start'];
$duration = "-";
if ($start != 0)
{
	$iStart = strtotime($start);
	$end = $row['end'];
	if ($end == 0)
	    $iEnd = time();
	else
	    $iEnd = strtotime($end);

	$duration = ConvertSecondsToTimeString($iEnd - $iStart);
}

$pieces = explode("/", $row['user']);

$userinfo = GetUserInfo($pieces[1]);

?>


<!-- Content table -->
  <div id="contentdiv">
<!-- Page content -->

<div id="tabpage"><div id="tabcontents">
<div class="indexpage">
<center>
<?
if ($argv[2] == "0")
	Echo("Job Completed");
else if ($argv[2] == "1")
	Echo("Job Failed");
else if ($argv[2] == "2")
	Echo("Fatal Error Report");
else
	Echo("Please tell Jan something went wrong");
?>
</center><br>
<div id="stats">
<table class="rowod">
<tr><th>Job Name</th>           <? echo(EchoCell( $row['comment'], "comment" )); ?></tr>
<tr><th>Project</th>            <? echo(EchoCell( $row['project'], "" )); ?></tr>
<tr><th>User</th>               <? echo(EchoCell( $userinfo[0]['displayname'][0] . ' (<A HREF="mailto:' . $userinfo[0]['mail'][0] . '">' . $userinfo[0]['mail'][0] . '</A>)' , "" )); ?></tr>
<tr><th>Created</th>            <? echo(EchoCell( EchoTime($row['created']), "" )); ?></tr>
<tr><th>Worker</th>             <? echo(EchoCell( $row['worker' ], "" )); ?></tr>
<tr><th>Start</rh>              <? echo(EchoCell( EchoTime($row['start']), "" )); ?></tr>
<tr><th>End</th>                <? echo(EchoCell( EchoTime($row['end']), "" )); ?></tr>
<tr><th>Duration</th>           <? echo(EchoCell( $duration, "" )); ?></tr>
<tr><th>Output</th>             <? echo(EchoLog( $row['log'], "" )); ?></tr>
<tr><th>Status</th>             <?
$istat = $row['status'];
if (($istat == 2 || $istat == 6 || $istat == 3) && $row['progressd'])
{
	echo(EchoCell( $status[ $istat ] . " (" . $row['progressd'] . ")", "" ));
}
else
{
	echo(EchoCell( $status[ $istat ], "" ));
}

?></tr>
</table>
<?
// don't show last 10 messages if the job completed successfully
if ($argv[2] != "0")
{
?>
<br>
<br>
<xdiv id = "stats" style="overflow: auto; vertical-align: top; width: 500px; ">
<table class="log">
<tr><th width="500">Last 10 Messages</th></tr>
<?
$item = "c:\\www\\logs\\" . $row['log'];
$lines = file($item);
$defaultlevel = "5";

$n = count($lines);
if ($argv[3] != "-1")
	$n = $argv[3];
foreach ($lines as &$line)
{
	if ($n < 11)
		FormatLogLine($line);
	$n--;
	if ($n < 0)
		break;
}

?></table>
<?
}
?>
</div>
</div>
</div>
<center>
<table><tr><td class="test">
Farm Web Management - <a href="http://farmserver">http://farmserver</a> - &copy;2007 Bizarre Creations
</td></tr></table></div>


