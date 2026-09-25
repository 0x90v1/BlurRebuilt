/*
 * This script sets up the paths needed to build tools code
 *
 * Jan van Valburg 2006
 */

// check that we are running from the commandline
if (WScript.FullName.toLowerCase().lastIndexOf("cscript.exe") == -1)
{
	WScript.Echo("This script must be run using CSCRIPT.\n");
	WScript.Quit();
}

var Shell = new ActiveXObject("WScript.shell");
var Env = Shell.Environment("PROCESS");
var g_logfiles = Array();
var g_lastlogfile;
var g_iErrors = 0;
var g_bUseEnv = true;
var g_sOrgPath = Env("PATH");

// setup environment
var Src = Env("SRC_ROOT");
if (!Src)
	Src = Env("SRC_ROOT") = "z:\\devroot";
	
var Dest = Env("DEST_ROOT");

Env("SRC_ROOT_CYG") = ConvertToCyg( Src );

Src = Src + "\\";

var BuildTools = "z:\\tools\\code_tools\\build_tools";

// get visual studio 2003 path
try
{
	var vc7path = "";
	try { vc7path = Shell.RegRead("HKLM\\SOFTWARE\\Microsoft\\VisualStudio\\7.1\\Installdir"); }
	catch (e)
	{ vc7path = Shell.RegRead("HKLM\\SOFTWARE\\Wow6432Node\\Microsoft\\VisualStudio\\7.1\\Installdir"); }

vc7path = RemoveTrailingSlash(vc7path);
vc7path = RemoveLastDirectoryFromPath(vc7path);
vc7path = RemoveLastDirectoryFromPath(vc7path);

// set visual studio 2003 path to environment item
Env("VISUAL_STUDIO") = vc7path;
}
catch(e)
{
	WScript.Echo("*** ERROR: Visual Studio .NET 2003 has not been installed.");
}

// get visual studio 2005 path
try
{
	var vc8path = "";
	try { vc8path = Shell.RegRead("HKLM\\SOFTWARE\\Microsoft\\VisualStudio\\8.0\\Installdir"); }
	catch (e)
	{ vc8path = Shell.RegRead("HKLM\\SOFTWARE\\Wow6432Node\\Microsoft\\VisualStudio\\8.0\\Installdir"); }

vc8path = RemoveTrailingSlash(vc8path);
vc8path = RemoveLastDirectoryFromPath(vc8path);
vc8path = RemoveLastDirectoryFromPath(vc8path);

// set visual studio 2005 path to environment item
Env("VISUAL_STUDIO_8") = vc8path;
}
catch(e)
{
	WScript.Echo("*** WARNING: Visual Studio .NET 2005 has not been installed.");
}

// set PATH
SetVC7Path();

Env("BUILDTOOLS") = BuildTools;
 
// set LIB (this is to stop dependency failures by MSVC)
var DotNetVC = vc7path + "\\vc7";

Env("LIB") = Src + "\\vendor\\dx9sdk\\current\\lib\\x86;" +
			 DotNetVC + "\\lib;" + 
			 DotNetVC + "\\atlmfc\\lib;" +
			 DotNetVC + "\\platformSDK\\lib;";

Env("INCLUDE") =	DotNetVC + "\\include;" +
					DotNetVC + "\\atlmfc\\include;" + 
					DotNetVC + ";" + 
					DotNetVC + "\\PlatformSDK\\Include;" + 
					DotNetVC + "\\atlmfc\\include;";

// display a windows popup dialog
//Shell.Popup(vc7path, 15, "Windows Script Host", 36);

WScript.DisconnectObject(Shell);

/*
function GetLocation(location)
{
	// get latest code from subversion
	Shell.Run("svn checkout http://svn/" + location + " z:/" + location);
}

function BuildLocation(location)
{
	// build
	Shell.Run(location + "/build.bat", 9, true);
}
*/

// remove the everything after the last \
function RemoveLastDirectoryFromPath(path)
{
	var i = path.lastIndexOf("\\");
	if (i == 0)
		return path;
	
	return path.slice(0, i);
}

// remove trailing \ if present
function RemoveTrailingSlash(path)
{
	var i = path.lastIndexOf("\\");
	if (i < path.length - 1)
		return path;
		
	return path.slice(0, i);	
}

function Build(target, config)
{
	var useenvstring = "";
	if (g_bUseEnv)
		useenvstring = "/useenv";

	var exitcode = DOS("devenv " + useenvstring + " \"" + target + "\" /build \"" + config + "\"", 9, true);
	if (exitcode == 0)
		return;

	g_iErrors++; 	// count the errors

	
//	Shell.Run(g_lastlogfile);
//	WScript.Quit();
}

function Clean(target, config)
{
	DOS("devenv /useenv \"" + target + "\" /clean \"" + config + "\"", 9, true);
}

function DisplayBuildLogs()
{
	for (var line in g_logfiles)
	{
		WScript.Echo(g_logfiles[line]);
	}
}

// run a DOS command
function DOS(command)
{
	var WshShell = new ActiveXObject("WScript.Shell");
	WScript.Echo("running: " + command + "\n");
	var oExec    = WshShell.Exec("cmd /c " + command);
	var output = "";
	var se = "Build log was saved at \"file://";

	// run the command and display it's stdout
	while (oExec.Status == 0 && (!oExec.StdOut.AtEndOfStream))
	{
		while (!oExec.StdOut.AtEndOfStream)
		{
			var line = oExec.StdOut.ReadLine();
			
			// check whether this line specifies the build log
			var r = line.match(se);
			if (r)
			{
				var logfile = "\"" + line.substr(r.lastIndex);
				g_lastlogfile = logfile;
				g_logfiles.push(logfile);	// remember the build log
			}
		
			// display the output on screen
			WScript.Echo(line);
		}
		
		WScript.Sleep(100);
	}
	
	return oExec.ExitCode;
}

function ConvertToCyg(text)
{
	var out = "/cygdrive/" + Src.substr(0,1) + Src.substr(2);
	var re = /\\/g;

	out = out.replace(re, "/");

	return out;
}

function Finish()
{
	if (Dest)
	{
		fso = new ActiveXObject("Scripting.FileSystemObject");
		fso.CopyFolder (Src + "\\Bin\\export_pipeline",Dest + "\\export_pipeline");
	}

	if (g_iErrors == 0)
		WScript.Echo("BUILD SUCCESSFUL\n");
	else
		WScript.Echo("BUILD FAILED\n");
}

function SetVC8Path()
{
	SetPath(true);
}

function SetVC7Path()
{
	SetPath(false);
}

function SetPath(bUseVC8)
{
	var path = 
	BuildTools + ";" +						// code_tools
	BuildTools + "\\gcc_wrapper;" +			// code_tools/gcc_wrapper
	g_sOrgPath + ";" +						// %PATH%
	"z:\\tools\\src\\vendor\\dx9sdk\\current\\utilities";		// vendor/dx9sdk

	if (bUseVC8)
	{
		// put VC8 in the path
		path = path + ";" +
		vc8path + "\\bin;" +				// vc/bin
		vc8path + "\\common7\\ide";			// vc/common7/ide
	}
	else
	{
		// put VC7 in the path
		path = path + ";" +
		vc7path + "\\bin;" +					// vc7/bin
		vc7path + "\\common7\\ide";			// vc7/common7/ide
	}

	Env("PATH") = path
}

