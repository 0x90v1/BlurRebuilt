<?php

//$sqlconnect = "farmserver";
$page = 2;

include('farmcommon.php');

$today = $_GET['day'];
if ($today == "")
    $today = date("Y-n-j");
    
$result = mysql_query("SELECT * FROM workers where time>='$today 00:00:00' and time<='$today 23:59:59'")
or die(mysql_error());

echo ("<center>Screensaver availability list for $today<br><br></center>");

$empty = true;

$inuse = 0;

// collect each entry into a table
while(($row = mysql_fetch_array( $result )) && ($i < $pagesize))
{
    foreach ($row as $key => $value)
    {
        if (is_string($key) && $key != "time")
        {
			if ($key == "inuse")
			    $inuse += $value;
			else
			{
	            $totals[$key] += $value;
    	        $empty = false;
   			}
        }
    }
}

if ($empty)
{
    $totals['total'] = 0;
}

// sort the entries, highest first
arsort($totals);

?>
<td class="indexo">
<CENTER>
<div style="overflow: auto; height: 300px; width: auto; ">
<TABLE class="indexnewslinks" BORDER=0 CELLPADDING=2>
<TR><TH align=left>Computer</TH><TH>Time</TH>
<TH align=center>% of Today</TH></TR>
<?
// print a table with all the entries in
foreach ($totals as $key => $value)
{
  $barwidth = $value / (60*4);
  $percentage = sprintf("%02.2f%%", 100 * ($value / (60*60*24)));
  $time = ConvertSecondsToTimeString($value);

  if ($key == "total")
  {
        $barwidth = 16*24;
        if ($value > 0)
	        $percentage = sprintf("%2.0f%% in use", 100*$inuse/$value);
		else
		    $percentage = "";
  }

  if ($i == 0)
  {
    $i = 1; $rowname = "normal";
  }
  else
  {
    $i = 0; $rowname = "normal2";
  }

    echo ("<tr class=\"$rowname\" onmouseover=\"this.className='hover'\"");
    echo (" onmouseout=\"this.className='$rowname'\" ");
    echo (" onclick=\"location.href='saverusage.php?name=$key&day=$today'\" ");
    echo (">");

  echo("<TD align=left>$key</TD><TD align=center>$time</TD><TD valign=middle>
   <TABLE align=left><TR><TD bgcolor=#48ba8f><IMG SRC='images/s.png' width=$barwidth height=5></TD>
     <TD><FONT SIZE=1>$percentage</FONT></TD></TR>
   </TABLE></TD></TR>
 </TD></TR>");

}

?>
</TABLE>
</div>
</td>


<?
    // ---- previous day, next day code
    $oneday = 24*60*60;
    $nextday = date("Y-n-j", strtotime($today) + $oneday);
    $previousday = date("Y-n-j", strtotime($today) - $oneday);
    
    printf("<td class=\"indexo\"><table class=\"indexnewslinks\"><tr><td>");
    printf("<a href=\"savertopten.php?day=%s\">Previous day</a>", $previousday);
    printf("</td>");
    printf("<td>/</td>");
    printf("<td><a href=\"saverusage.php?day=$today\">this day's graph</a></td>");
    printf("<td>/</td>");
    printf("<td class=\"tdright\">");
    printf("<a href=\"savertopten.php?day=%s\">Next day</a>", $nextday);
    printf("</td></tr></table></td>");
?>

<div>
<?include('footer.php')?>


