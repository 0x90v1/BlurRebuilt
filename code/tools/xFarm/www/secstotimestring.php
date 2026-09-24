<?php

// all our times are in GMT.
// before printing any times, pass them through here to adjust for daylight savings
function AdjustForDaylightSaving($intime)
{
	// Set these variables for your use:
	$calc_adjust = 1800; // set time to make change
	$time_regulr = 0; // set regular time offset
	$time_DSTadj = 3600; // set DST time offset
	$varble_name = "adjust_PT"; // set varialbe name
	
	$mnth_server = date("n", $intime + $calc_adjust);
	$dayw_server = date("w", $intime + $calc_adjust);
	$date_server = date("j", $intime + $calc_adjust);
	
	if ($mnth_server >= 4 && $mnth_server <= 9) {
	  $$varble_name = 3600; // DST
	} else {
	  if ($mnth_server <= 2 || $mnth_server >= 11) {
		$$varble_name = 0; // Regular Time
	  } else {
		if (($mnth_server == 3 &&
		$date_server - $dayw_server >= 25 ) 
		|| ($mnth_server == 10 &&
		$date_server - $dayw_server < 25 )) {
			$$varble_name = 3600; // DST
		  } else {
			$$varble_name = 0; // Regular Time
		  }
	  }
	}

	return $intime - $adjust_PT;
}

function ConvertSecondsToTimeString($value)
{
	$value = $value;

    $minutes = (int)($value / 60);
    $hours = (int)($minutes / 60);
    $minutes -= (int)($hours * 60);
    $days = (int)($hours / 24);
    $hours -= (int)($days * 24);

    if ($days == 0)
        return sprintf("%02d:%02d", $hours, $minutes);
    else
        return sprintf("%dd %02d:%02d", $days, $hours, $minutes);
}

function ConvertSecondsToRelativeDate($seconds)
{
	// is it the same day as today?
	if (date("j-n-Y", time()) == date("j-n-Y", $seconds))
	{
		// yes, return just the time
		return date("G:i", $seconds);
	}

	// is it less than 6 days ago?
	if (time() - $seconds < 6*24*60*60)
	{
		// yes, return day of the week + time
		return date("D G:i", $seconds);
	}
	
	// otherwise, return date only
	return date("j-n-y", $seconds);
}

?>
