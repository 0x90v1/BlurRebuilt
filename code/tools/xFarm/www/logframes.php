<?php

// get log filename and pass it into logtree.php
$item = $_GET['file'];
if ($item == "")
die("no log file specified");

Echo('<frameset cols="280,*">');
Echo('<frameset rows="90,*">');
Echo('<frame name="banner" scrolling="no" border="no" src="logbanner.htm">');
Echo('<frame src="logtree.php?file=' . $item . '">');
Echo('</frameset>');
Echo('<frame name="content" src="displaylog.php?file=' . $item . '#bottom">');
Echo('</frameset>');

?>

