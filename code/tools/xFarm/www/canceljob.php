<?php

// server
system("c:/pgr3/tools/xfarm.exe -a kill " . $_GET['id'], $retval);
// jantest
//system("g:/alienbrainwork/devroot/bin/export_pipeline/win32/xfarm/xfarm.exe -p 1234 kill " . $_GET['id'], $retval);

Echo('<HTML><META HTTP-EQUIV="Refresh" CONTENT="0; URL=javascript:history.go(-2)"></HTML>');

?>
