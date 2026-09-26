<?php

// server
system("c:/pgr3/tools/xfarm.exe -a prioritise " . $_GET['id'], $retval);
// jantest
//system("c:/pgr3/tools/xfarm.exe -p 1235 prioritise " . $_GET['id'], $retval);

Echo('<HTML><META HTTP-EQUIV="Refresh" CONTENT="0; URL=javascript:history.go(-2)"></HTML>');

?>
