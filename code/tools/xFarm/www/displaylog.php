<?php

$page = 4;

include('secstotimestring.php');

$sqlconnect = "localhost";

$levelstrings = array(
  "none",
  "fatal",
  "error",
  "warn",
  "info",
  "debug",
  );

function FormatLogLine($line)
{
  global $levelstrings;

  sscanf($line, "%d %d %s", $level, $time, $logger);

  // check this logger's level
  $dlevel =  $_GET[strtr($logger, '.', '_')];
  if ($dlevel == "")
    $dlevel = "4";
  if ($level > $dlevel)
    return;
  
  $pos = strpos($line, ' ') + 1;
  $pos = strpos($line, ' ', $pos) + 1;
  $pos = strpos($line, ' ', $pos) + 1;
  $message = substr($line, $pos);

  echo("<tr>" .
		"<td>" . strftime("%H:%M:%S", AdjustForDaylightSaving($time)) . "</td>" .
		"<td align=\"left\"> " . $message . "</td>" .
		"</tr><br>");

//		"<td>" . $levelstrings[$level] . "</td>" .
//		"<td>" . $logger . "</td>" .
//		"<td>" . strftime("%b %d %Y %H:%M:%S", $time) . "</td>" .
}

$page = 0;

// Get all the data from the "example" table
$item = $_GET['file'];
if ($item == "")
die("no log file specified");

$lines = file($item);

?>
<xdiv id = "stats" style="overflow: auto; height: 300px; width: auto; ">
<table>
<tr><th>Time</th><th width="500">Message</th></tr><br>
</table>
<?

foreach ($lines as &$line)
{
  FormatLogLine($line);
}

?>
</table>
</div>

   <td class="indexo">
    <table class="indexnewslinks">
     <tr>
      <td>
        <a target="_parent" href="">back to menu</a>
      </td>
     </tr>
    </table>
   </td>
   
<a name="bottom" id="bottom"></a>





