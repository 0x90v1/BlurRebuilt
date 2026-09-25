<?
if ($email=="1")
{
	include('emailheader.php');
}
else
{
	include('header.php');
}
include('secstotimestring.php');

putenv("TZ=Britain (UK)/London Great Britain");

static $pagesize = 10;
$status[0]="Waiting";
$status[1]="Pending";
$status[2]="In Progress";
$status[3]="Killed";
$status[4]="Requeue";
$status[5]="Completed";
$status[6]="Failed";

function EchoTime($t)
{
    if ($t != "0000-00-00 00:00:00")
    {
		return date("j-n-Y G:i", strtotime($t));
	}
    else
        return "-";
}

function EchoCell($data, $rowname)
{
	return "<td class=\"$rowname\">" . $data . "</td>";
}

function ClampString($string, $maxlen)
{
    if (strlen($string) < $maxlen)
        return $string;

    $array = str_split( $string, $maxlen );

    return $array[0] . "...";
}

function EchoLog($data, $rowname)
{
    if ($data)
    {
		if ($email == "1")
			$logstring = "<a href=\"http://farmserver/logframes?file=logs\\" . $data . "\">view log</a>";
		else
			$logstring = "<a href=\"logframes?file=logs\\" . $data . "\">view log</a>";
    }
    else
    {
        $logstring = "-";
    }
    return EchoCell( $logstring, $rowname );
}

function TinyText($text)
{
	return	"<td class=\"indexo\">" .
    		"<table class=\"indexnewslinks\">" .
    		"<tr><td>$text</td></tr>" .
		    "</table>" .
		    "</td>";
}

function EchoRow($row, $rowname, $alternate)
{
    global $status;
    
    $id = $row['id'];

	$out =
	"<tr class=\"$rowname\" onmouseover=\"this.className='hover'\"" .
	"    onmouseout=\"this.className='$rowname'\" " .
    "    onclick=\"location.href='jobinfo.php?item=$id'\" " .
    ">";

	if ($alternate)
		$out .= EchoCell( $row['worker'], "" );
	else
	{
		$id = "<center>" . ConvertSecondsToRelativeDate(strtotime($row['created'])) . "</center>";
	    $out .= EchoCell( $id, "" );
	}
    
	$user =  explode("/", $row['user']);
	if ($user[1])
	    $out .= EchoCell( $user[1], "" );
	else
		$out .= EchoCell( $user[0], "" );
	$out .= EchoCell( ClampString( $row['comment'], 60 ), "comment" );
    $out .= "<center>";
    $out .= EchoLog( $row['log'], "" );
//	$out .= EchoCell( $status[ $row['status'] ], "" );
	$out .= EchoCell(EchoSmallJobProgressBar( $row['progressv'], $row['status'] ), "");
    $out .= "</center>" .
			"</tr>\n";
			
	return $out;
}

function EchoProgressBar($barfull, $barwidth, $barheight, $description)
{
	$out = "<TABLE align=left><TR><TD align=\"left\" bgcolor=#48ba8f width = $barwidth><IMG SRC='images/s.png' width=$barfull height=$barheight></TD>";
	if ($description)
		$out .= "<TD><FONT SIZE=1>$description</FONT></TD></TR>";
	$out .= "</TABLE>";
	
	return $out;
}

function EchoJobProgressBar($progress)
{
	return EchoProgressBar($progress, 100, 15, sprintf("%2d%%", $progress) );		
}

function EchoSmallJobProgressBar($progress, $statusid)
{
	global $status;

	if ($statusid != 2)
		return $status[$statusid];
	
	return EchoProgressBar($progress/2, 50, 7, "");		
}

# CONNECT TO SQL DATABASE

// Make a MySQL Connection
if (!$sqlconnect)
    $sqlconnect="localhost";

mysql_connect($sqlconnect, "farmaccess", "") or die(mysql_error());
mysql_select_db("farm") or die(mysql_error());

?>
