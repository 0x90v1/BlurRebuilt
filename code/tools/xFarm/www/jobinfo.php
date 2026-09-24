<?php

$sqlconnect = "farmserver";
session_start();

include('farmcommon.php');
include('ldap.php');
$page = 0;

// Get all the data from the "example" table
$item = $_GET['item'];
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
<center>Job Information<br><br>
<div id="stats"><center>
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

$allowcancel = 0;
if ($row['status'] <= 2)
{
	if ((strcasecmp($pieces[1], $HTTP_SESSION_VARS["name"]) == 0)
		|| $HTTP_SESSION_VARS["name"] == "janv"
		|| $HTTP_SESSION_VARS["name"] == "rogerp"
		)
		$allowcancel = 1;

	if ( $HTTP_SESSION_VARS["name"] == "janv" ||
		 $HTTP_SESSION_VARS["name"] == "rogerp" ||
		 $HTTP_SESSION_VARS["name"] == "kunalp"
		)
		$allowprioritise = 1;
}

?></tr>
<tr><th>Progress</th>           <? echo(EchoCell(EchoJobProgressBar( $row['progressv'], $row['progressd'], $row['status'] ), "")); ?> </tr>
</table>

   <td class="indexo">
    <table class="indexnewslinks">
     <tr>
      <td>
        <a href="javascript:history.go(-1);">Return to menu</a>
      </td>
      <? if ($allowcancel == 1)
			Echo('<td><a href="canceljob?id=' . $row['id'] . ';">Cancel job</a></td>');
      ?>
      <? if ($allowprioritise == 1)
			Echo('<td><a href="prioritisejob?id=' . $row['id'] . ';">Prioritise job</a></td>');
      ?>
     </tr>
    </table>
   </td>

<?include('footer.php')?>

