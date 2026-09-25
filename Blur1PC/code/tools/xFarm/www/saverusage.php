<?php

$sqlconnect = "farmserver";
$page = 3;

include('farmcommon.php');

$today = $_GET['day'];
if ($today == "")
    $today = date("Y-n-j");

$name = $_GET['name'];
if ($name == "")
    $name = "total";
    
$result = mysql_query("SELECT time, $name, total, inuse FROM workers where time>='$today 00:00:00' and time<='$today 23:59:59'")
or die(mysql_error());

// collect each entry into a table
while($row = mysql_fetch_array( $result ))
{
    // parse the time for this entry
    $time = sscanf($row[0], "%d-%d-%d %d:%d:%d");
	$i = $time[3];
	$total = $row[$name];
	if ($row['total'] == 0)
	    $row['total'] = 1;
    $percentInUse = $row['inuse'] / $row['total'];
    $inuse[$i] = $total * $percentInUse;
    $notinuse[$i] = $total * (1 - $percentInUse);
    
    if ($name != "total")
    {
		$notinuse[$i] += $inuse[$i];
		$inuse[$i] = 0;
 	}
}

// collect $data into a $urldata string
$i = 0;
while ($i < 24)
{
    if ($i != 0)
    {
        $urldata1 = $urldata1 . ",";
        $urldata2 = $urldata2 . ",";
    }

    $val = $inuse[$i];
    if (!$val) $val = "0";
    $urldata1 = $urldata1 . $val;
    $val = $notinuse[$i];
    if (!$val) $val = "0";
    $urldata2 = $urldata2 . $val;

    $i++;
}

if ($name == "total")
	$mode = "1";

echo ("<center>Screensaver availability chart for $today<br><br>");
?>
<IMG SRC="http://jan/drawchart.php?data1=<?echo($urldata1);?>&data2=<?echo($urldata2);?>&name=Screensaver Availability (<?echo("$name)&mode=$mode")?>">

<?
    // ---- previous day, next day code
    $oneday = 24*60*60;
    $nextday = date("Y-n-j", strtotime($today) + $oneday);
    $previousday = date("Y-n-j", strtotime($today) - $oneday);

    printf("<td class=\"indexo\"><table class=\"indexnewslinks\"><tr><td>");
    printf("<a href=\"saverusage.php?name=%s&day=%s\">Previous day</a>", $name, $previousday);
    printf("</td>");
    printf("<td>/</td>");
    printf("<td><a href=\"savertopten.php?day=$today\">this day's list</a></td>");
    printf("<td>/</td>");
    printf("<td class=\"tdright\">");
    printf("<a href=\"saverusage.php?name=%s&day=%s\">Next day</a>", $name, $nextday);
    printf("</td></tr></table></td>");
?>
<div>

<?include('footer.php')?>


