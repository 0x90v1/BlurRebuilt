<?php

//Bizarre settings (don't change these)
$server      = 'saville';
$port	 	 = 389;
$usertree    = 'CN=Users,DC=bizarre2,DC=local';
$grouptree   = 'CN=Users,DC=bizzare2,DC=local';
$userfilter  = '(sAMAccountName=%{user})';
$version     = 3;
$binddn     = 'BIZARRE2\biznetweb';
$bindpw     = 'Monkey_1';

$con = _connectAndBind();	//Connect

function GetUserInfo($userToSearch)
{
	global $server, $port, $usertree, $grouptree, $userfilter, $version, $binddn, $bindpw;
	global $info, $base, $con, $sr, $userinfo, $base;
	
	//Grab the infoC:
	$info['user']   = $userToSearch;
	$info['server'] = $server;
	$base = _makeFilter($usertree, $info);
	$filter = _makeFilter($userfilter, $info);
	$sr = @ldap_search($con, $base, $filter);
	return @ldap_get_entries($con, $sr);
}

//Connect to an Active Directory server and bind as superuser
function _connectAndBind()
{
	global $server, $port, $version, $binddn, $bindpw;
	
	$con = @ldap_connect($server,$port);
	if (!$con) { return false; }
	if (!@ldap_set_option($con, LDAP_OPT_PROTOCOL_VERSION, $version)) { return false; }
	if (!@ldap_bind($con,$binddn,$bindpw)) { return false; }
	
	return $con;
}


//Make a filter for use in info grabbing
function _makeFilter($filter, $placeholders) {
	preg_match_all("/%{([^}]+)/", $filter, $matches, PREG_PATTERN_ORDER);
	//replace each match
	foreach ($matches[1] as $match) {
		//take first element if array
		if(is_array($placeholders[$match])) {
			$value = $placeholders[$match][0];
		} else {
			$value = $placeholders[$match];
		}
		$filter = str_replace('%{'.$match.'}', $value, $filter);
	}
	return $filter;
}

?>
