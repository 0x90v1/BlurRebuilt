<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "DTD/xhtml1-transitional.dtd">
<html xml:lang="en" lang="en">
<head>
<title>Farm Manager</title>
<link rel="stylesheet" type="text/css" href="style.css" />
</head>

<!-- Page heading -->
<body>
<div class="bodydiv">


<!-- Content table -->
  <div id="contentdiv">
<!-- Page content -->
<h1 class="h1tab">Farm Manager</h1>
<div id="tabs">
 <ul id="primary">
<?
    if ($page == 0)
        echo("<li><span>My Jobs</span></li>");
    else
        echo("<li><a href=\"displayjobs.php?filter=1\">My Jobs</a></li>");
    if ($page == 1)
        echo("<li><span>List Jobs</span></li>");
    else
        echo("<li><a href=\"displayjobs.php\">List Jobs</a></li>");
	if ($page == 5)
		echo("<li><span>Search</span></li>");
	else
		echo("<li><a href=\"search.php\">Search</a></li>");
    if ($page == 2)
        echo("<li><span>Farm Stats</span></li>");
    else
        echo("<li><a href=\"savertopten.php\">Farm Stats</a></li>");
    if ($page == 3)
        echo("<li><span>Farm Chart</span></li>");
    else
        echo("<li><a href=\"saverusage.php\">Farm Chart</a></li>");
	if ($page == 4)
		echo("<li><span>Options</span></li>");
	else
		echo("<li><a href=\"options.php\">Options</a></li>");
?>
 </ul>
</div>
<div id="tabpage">
 <div id="tabcontents">
<div class="indexpage">
<center>
