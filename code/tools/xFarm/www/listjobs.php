<?php

function GetJobFilterString()
{
	global $HTTP_SESSION_VARS;

	// get username and retrieve this user's settings from the database
	$username = $HTTP_SESSION_VARS["name"];
	$result = mysql_query("SELECT * FROM users WHERE user='" . $username . "'");
	$row = mysql_fetch_array( $result );

	$rtn = "";
	if ($row['showgothamcar'])
	    $rtn .= "project = 'Gotham Car' OR ";
	if ($row['showgothamcity'])
	    $rtn .= "project = 'Gotham City' OR ";
	if ($row['showtheclub'])
	    $rtn .= "project = 'The Club' OR ";
	if ($row['showunknown'])
	    $rtn .= "project = 'unknown' OR project = '' OR ";
	$rtn .= "0";
	
	return "( " . $rtn . " )";
}

function ListJobs($result, $first, $last, $pagesize, $alternate)
{
	# TABLE HEADER HTML
	$out = 
	"<center><div id = \"stats\" style=\"overflow: auto; height: 300px; width: auto; \">" .
	"<table align=center>" .
	"<tr align=center>";
	if ($alternate)
		$out .= "<th>Worker</th>";
	else
	    $out .= "<th>Created</th>";
	$out .= "<th>User</th>" .
			"<th>Comment</th>" .
			"<th>Output</th>" .
			"<th>Status</th>" .
			"</tr>";

	# TABLE PRINTING CODE
    $iRow = 0;
    $i = 0;
    // keeps getting the next row until there are no more to get
    while(($row = mysql_fetch_array( $result )) && ($i < $pagesize))
    {
		// Print out the contents of each row into a table
		if ($iRow == 0)
		{
			$out .= EchoRow($row, "normal", $alternate);
			$iRow = 1;
		}
		else
		{
			$out .=  EchoRow($row, "normal2", $alternate);
			$iRow = 0;
		}
		$i++;
    }
    $out .= "</table>";

    if ($i == "0")
    {
		if ($alternate)
		    $out = "";
		else
		    $out .= "<br>there are no results to display<br><br>";
		$i++;
	}
	
	while ($i < $pagesize)
	{
	    $out .= "<br>";
	    $i++;
	}

	return $out;
}



?>


