<?php

// Get all the data from the "example" table
$logfilename = $_GET['file'];
if ($logfilename == "")
die("no log file specified");

Echo("<html>");
Echo("<!-- (c)2006 Bizarre Creations, written by Jan van Valburg -->");

// emit the javascript code for the menu bar control
$logtreehtm = file('menubar.htm');
foreach ($logtreehtm as &$line)
{
	Echo($line);
}

// emit the javascript code for the tree control
$logtreehtm = file('logtree.htm');
foreach ($logtreehtm as &$line)
{
	Echo($line);
}

// emit the log file name for the javascript tree control
Echo('
function getLogFilename()
{
	return "' . $logfilename . '";
}');

$loggermap = array();

function GetLoggerParent( $logger )
{
	$idot = strrpos($logger, ".");
    if ($idot === false)
		return "root";

	return substr($logger, 0, $idot);
}

function GetLoggerVarName( $logger )
{
	return strtr($logger, '.', '_');
}

// load the log file, and make a map of all the loggers
$logfile = file($logfilename);
foreach ($logfile as &$line)
{
	// read logger
	sscanf($line, "%d %d %s", $level, $time, $logger);
	
	// add this logger and all it's parents
	while ($logger != "root")
	{
		$parentlogger = GetLoggerParent($logger);
		$loggermap[$logger] = $parentlogger;
		$logger = $parentlogger;
	}
}

?>

</script>

<body>

<!-- the PHP generated data for the tree script -->
<script language="JavaScript">
var root = new node("root","root");
root.debugLevel = 4;
<?
// emit javascript code for the tree creation
foreach( $loggermap as $logger => $parent)
{
	Echo('var ' . GetLoggerVarName($logger) . ' = new node("' . $logger . '","' . $logger . '");
');
}
// emit javascript code to link each node into it's parent
foreach( $loggermap as $logger => $parent)
{
	Echo(GetLoggerVarName(GetLoggerParent($logger)) . '.add(' . GetLoggerVarName($logger) . ');
');
}

?>

document.write(root.write());

</script>

<div id="janMenu" class="menu">
<a class="menuItem" TARGET="banner" href="logbanner.htm" onClick="return clickTickButton(5);">Debug</a>
<a class="menuItem" TARGET="banner" href="logbanner.htm" onClick="return clickTickButton(4);">Info</a>
<a class="menuItem" TARGET="banner" href="logbanner.htm" onClick="return clickTickButton(3);">Warn</a>
<a class="menuItem" TARGET="banner" href="logbanner.htm" onClick="return clickTickButton(2);">Error</a>
<a class="menuItem" TARGET="banner" href="logbanner.htm" onClick="return clickTickButton(1);">None</a>
<a class="menuItem" TARGET="banner" href="logbanner.htm" onClick="return clickTickButton(0);">not set</a>
</div>


</body>

<?
Echo("</html>");
?>


