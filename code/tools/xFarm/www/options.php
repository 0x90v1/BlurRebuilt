<?php

$page = 4;
session_start();

if ($HTTP_SESSION_VARS["name"] == "")
{
	//Echo('<HTML><A HREF="index.htm">click here to go to the farmserver</A></HTML>');
	Echo('<HTML><META HTTP-EQUIV="Refresh" CONTENT="0; URL=index.htm"></HTML>');
	exit();
}

include('farmcommon.php');
include('ldap.php');

function EmitCheckbox($name, $text, $bool)
{
	Echo('<tr><td class="left"><input type="checkbox" name="'.$name.'"');
	if ($bool)
		Echo(' checked="1" ');
	Echo('/>'.$text.'</td></tr>');
}

function EmitSQLSetCheckBox($name)
{
	global $_POST;

	if ($_POST[$name] == "on")
		return $name." = '1'";
	 else
		return $name." = '0'";
}

$userinfo = GetUserInfo($HTTP_SESSION_VARS["name"]);
$username = $HTTP_SESSION_VARS["name"];

// get the user's e-mail address
$userinfo = GetUserInfo($username);
$email = $userinfo[0]['mail'][0];

// ensure the the user exists in the database
$result = mysql_query("SELECT * FROM users WHERE user='" . $username . "'");

$row = mysql_fetch_array( $result );
if ($row['user'] != $username)
{
	// create new entry for this user
	$query = "insert into users values('".$username."', '".$email."', '0', '0', '0',
				'1', '1', '1', '1');";
	mysql_query($query);
}

// set the user's options into the database
if ($_REQUEST['set'] == "1")
{
	// setting values for this user
	$query = "UPDATE users SET " .
				EmitSQLSetCheckBox("mycomplete") . ", " .
	            EmitSQLSetCheckBox("myfail") . ", " .
	            EmitSQLSetCheckBox("mycrash") . ", " .
				EmitSQLSetCheckBox("showgothamcar") . ", " .
	            EmitSQLSetCheckBox("showgothamcity") . ", " .
	            EmitSQLSetCheckBox("showtheclub") . ", " .
	            EmitSQLSetCheckBox("showunknown") . ", " .
	            "email = '" . $email . "' " .
				" WHERE user = '".$username."' LIMIT 1;";
				
	mysql_query($query);
}

// get the user's options from the database
$result = mysql_query("SELECT * FROM users WHERE user='" . $username . "'");

$row = mysql_fetch_array( $result );
if ($row['user'] != $username)
{
	Echo("something went wrong. better tell Jan<br>");
}

# display checkboxes
?>
<center>Options for <?Echo($userinfo[0]['displayname'][0]);?>
<br>
<div id="stats"><center>
<div style="overflow: auto; height: 260px; width: auto; "><center><br>
<form action="options.php?set=1" method="post"><center>
<table><tr><td>
<table>
<?
EmitCheckbox("mycomplete", "Send me Email when my jobs complete", $row['mycomplete']);
EmitCheckbox("myfail", "Send me Email when my jobs fail", $row['myfail']);
EmitCheckbox("mycrash", "Send me Email when my jobs crash", $row['mycrash']);
Echo("</table></td><td><table>");
EmitCheckbox("showgothamcar", "Display Gotham Car exports", $row['showgothamcar']);
EmitCheckbox("showgothamcity", "Display Gotham City exports", $row['showgothamcity']);
EmitCheckbox("showtheclub", "Display The Club exports", $row['showtheclub']);
EmitCheckbox("showunknown", "Display other jobs", $row['showunknown']);
?>
</td></tr></table>
</table>
<input type="submit" value="Apply Changes"/>
</form>
<table align=center><tr><td class="test">
<center>
<?
if ($_REQUEST['set'] == "1")
	Echo("changes applied<br><br>");
else
	Echo("<br><br>");
?>
<a href="http://farmserver">http://farmserver</a><br>
Bizarre Creations Farm Web Manager &copy;2006 Bizarre Creations<br>
by Jan van Valburg / <a href="mailto:janv@bizarrecreations.com">janv@bizarrecreations.com</a>
</td></tr></table></div>
<br>

<?


# PRINT FOOTER
include('footer.php')?>
