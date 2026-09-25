<?php

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
  global $defaultlevel;

  if ($defaultlevel == "")
	$defaultlevel = "4";
	
  sscanf($line, "%d %d %s", $level, $time, $logger);

  // check this logger's level
  $dlevel =  $_GET[strtr($logger, '.', '_')];
  if ($dlevel == "")
    $dlevel = "5";
  if ($level > $dlevel)
    return;
  
  $pos = strpos($line, ' ') + 1;
  $pos = strpos($line, ' ', $pos) + 1;
  $pos = strpos($line, ' ', $pos) + 1;
  $message = substr($line, $pos);

  echo("<tr>" .
		"<td align=\"left\"> " . $message . "</td>" .
		"</tr>");
}

?>

