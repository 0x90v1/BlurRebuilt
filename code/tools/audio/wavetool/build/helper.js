
var g_iErrors = 0;

var Shell = new ActiveXObject("WScript.shell");
var Env = Shell.Environment("PROCESS");

var ROBOCOPY_PATH = Env("SRC_ROOT") + "\\bin\\build_tools\\robocopy.exe /np /nfl /ndl ";

WScript.Echo("SRC_ROOT: '" + Env("SRC_ROOT") + "'\n");

var SOURCE_PATH = Env("SRC_ROOT");

Env("INCREDIBUILD_PATH") = "C:\\Program Files\\Xoreax\\IncrediBuild";
Env("SUBVERSION_PATH") = "C:\\Program Files\\Subversion\\bin";

function SetEnvPC(localPath, srcLoc, platform, version)
{
	Env("VSINSTALLDIR") = "C:\\Program Files\\Microsoft Visual Studio .NET 2003\\Common7\\IDE";
	Env("VCINSTALLDIR") = "C:\\Program Files\\Microsoft Visual Studio .NET 2003";
	Env("FrameworkDir") = "C:\\WINDOWS\\Microsoft.NET\\Framework";
	Env("FrameworkVersion") = "v1.1.4322";
	Env("FrameworkSDKDir") = "C:\\Program Files\\Microsoft Visual Studio .NET 2003\\SDK\\v1.1";
	Env("DevEnvDir") = Env("VSINSTALLDIR");
	Env("MSVCDir") = Env("VCINSTALLDIR") + "\\VC7";

	Env("PATH") = Env("DevEnvDir")+";"+
					Env("MSVCDir")+"\\BIN;"+
					Env("VCINSTALLDIR")+"\\Common7\\Tools;"+
					Env("VCINSTALLDIR")+"\\Common7\\Tools\\bin\\prerelease;"+
					Env("VCINSTALLDIR")+"\\Common7\\Tools\\bin;"+
					Env("FrameworkSDKDir")+"\\bin;"+
					Env("FrameworkSDKDir")+"\\"+Env("FrameworkVersion")+";"+
					Env("INCREDIBUILD_PATH") + ";" +
					Env("SUBVERSION_PATH") + ";" +
					Env("PATH");

	Env("INCLUDE") = 
			Env("DXSDK_DIR")+"\\Include;"+
			Env("MSVCDir")+"\\INCLUDE;"+
			Env("MSVCDir")+"\\atlmfc\\INCLUDE;"+
			Env("MSVCDir")+"\\PlatformSDK\\include\\prerelease;"+
			Env("MSVCDir")+"\\PlatformSDK\\include;"+
			Env("FrameworkSDKDir")+"\\include;"+
			Env("INCLUDE");
	
	Env("LIB") = 
			Env("DXSDK_DIR")+"\\Lib;"+
			Env("MSVCDir")+"\\LIB;"+
			Env("MSVCDir")+"\\ATLMFC\\LIB;"+
			Env("MSVCDir")+"\\PlatformSDK\\lib\\prerelease;"+
			Env("MSVCDir")+"\\PlatformSDK\\lib;"+
			Env("FrameworkSDKDir")+"\\lib;"+
			Env("LIB");
}

function SetEnvPC_2005(localPath, srcLoc, platform, version)
{
	Env("VSINSTALLDIR") = "C:\\Program Files\\Microsoft Visual Studio 8";
	Env("VCINSTALLDIR") = "C:\\Program Files\\Microsoft Visual Studio 8\\VC";
	Env("FrameworkDir") = "C:\\WINDOWS\\Microsoft.NET\\Framework";
	Env("FrameworkVersion") = "v2.0.50727";
	Env("FrameworkSDKDir") = "C:\\Program Files\\Microsoft Visual Studio 8\\SDK\\v2.0";
	Env("DevEnvDir") = Env("VSINSTALLDIR") + "\\Common7\\IDE";
	Env("VS80COMNTOOLS") = Env("VSINSTALLDIR") + "\\Common7\\Tools";
	
	Env("INCLUDE") = 
			Env("VCINSTALLDIR")+"\\ATLMFC\\INCLUDE;"+
			Env("VCINSTALLDIR")+"\\INCLUDE;"+
			Env("VCINSTALLDIR")+"\\PlatformSDK\\include;"+
			Env("VSINSTALLDIR")+"\\SDK\\v2.0\\include;"+
			Env("INCLUDE")+";"+
			Env("XEDK")+"\\include\\win32;"+
			Env("XEDK")+"\\include\\win32\\vs2005;";

	Env("LIB") = 
			Env("VCINSTALLDIR")+"\\ATLMFC\\LIB;"+
			Env("VCINSTALLDIR")+"\\LIB;"+
			Env("VCINSTALLDIR")+"\\PlatformSDK\\lib;"+
			Env("VCINSTALLDIR")+"\\SDK\\v2.0\\lib;"+
			Env("LIB")+";"+
			Env("XEDK")+"\\lib\\win32\\vs2005;";
			
	Env("PATH") = 	Env("DevEnvDir") + ";" + 
					Env("VCINSTALLDIR") + "\\bin;" +
					"c:\\windows;c:\\windows\\system32;" + 
					Env("SUBVERSION_PATH") + ";" +
					Env("INCREDIBUILD_PATH") + ";";
}

function SetEnv360(localPath, srcLoc, platform, version)
{
	Env("VSINSTALLDIR") = "C:\\Program Files\\Microsoft Visual Studio .NET 2003\\Common7\\IDE";
	Env("VCINSTALLDIR") = "C:\\Program Files\\Microsoft Visual Studio .NET 2003";
	Env("FrameworkDir") = "C:\\WINDOWS\\Microsoft.NET\\Framework";
	Env("FrameworkVersion") = "v1.1.4322";
	Env("FrameworkSDKDir") = "C:\\Program Files\\Microsoft Visual Studio .NET 2003\\SDK\\v1.1";
	Env("DevEnvDir") = Env("VSINSTALLDIR");
	Env("MSVCDir") = Env("VCINSTALLDIR") + "\\VC7";

	Env("INCLUDE") = "C:\\Program Files\\Microsoft Xbox 360 SDK\\Include\\Xbox";
	Env("LIB") = "C:\\Program Files\\Microsoft Xbox 360 SDK\\Lib\\Xbox";
	Env("PATH") = "C:\\Program Files\\Microsoft Xbox 360 SDK\\Bin\\Win32" + ";" +
					Env("INCREDIBUILD_PATH") + ";" + 
					Env("SUBVERSION_PATH") + ";" +
					Env("DevEnvDir") + ";";
}

function SetEnv360_2005(localPath, srcLoc, platform, version)
{
	Env("VSINSTALLDIR") = "C:\\Program Files\\Microsoft Visual Studio 8";
	Env("VCINSTALLDIR") = "C:\\Program Files\\Microsoft Visual Studio 8\\VC";
	Env("FrameworkDir") = "C:\\WINDOWS\\Microsoft.NET\\Framework";
	Env("FrameworkVersion") = "v2.0.50727";
	Env("FrameworkSDKDir") = "C:\\Program Files\\Microsoft Visual Studio 8\\SDK\\v2.0";
	Env("DevEnvDir") = Env("VSINSTALLDIR") + "\\Common7\\IDE";
	Env("VS80COMNTOOLS") = Env("VSINSTALLDIR") + "\\Common7\\Tools";
	
	Env("INCLUDE") = "C:\\Program Files\\Microsoft Xbox 360 SDK\\Include\\Xbox";
	Env("LIB") = "C:\\Program Files\\Microsoft Xbox 360 SDK\\Lib\\Xbox";
	Env("PATH") = "C:\\Program Files\\Microsoft Xbox 360 SDK\\Bin\\Win32" + ";" +
					Env("DevEnvDir") + ";" + 
					Env("VCINSTALLDIR") + "\\bin;" +
					 "c:\\windows;c:\\windows\\system32;" + 
					Env("SUBVERSION_PATH") + ";" +
					 Env("INCREDIBUILD_PATH") + ";";
}

function ShowEnv(localPath, srcLoc, platform, version)
{
	WScript.Echo("PATH   : " + Env("PATH") + "\n");
	WScript.Echo("INCLUDE: " + Env("INCLUDE") + "\n");
	WScript.Echo("LIB    : " + Env("LIB") + "\n");
}

// Given the name of the project (i.e. GetMeInGame4) this generates the full path to the symbol server
// e.g. \\gothambitch2\Tools_src_sym\x86\GetMeInGame3\2006.06.15.4\ 
function GenerateSymbolServerPath(symbolServer, name)
{
	var platform = Env("buildPlatform");
	var version = Env("buildVersion");

	return (symbolServer + platform + "\\" + name + "\\" + version + "\\");
}

function GenerateSubversionRevision(folder, path)
{
	// c:\dev\Gotham3\ > c:\dev\Gotham3\Game\source_version.txt
	var exitcode = DOS("svnversion -n \"" + SOURCE_PATH + "\\" + folder + "\" > \"" + SOURCE_PATH + "\\" + path + "\"");
}

function RevertSourceMap(localPath)
{
	var sourceMapFilename = SOURCE_PATH + localPath + "source_map.txt";
	
	var exitcode = DOS("svn revert \"" + sourceMapFilename + "\"");
}

function MakeSourceMap(localPath, symbolServerPath)
{
	var sourceMapFilename = SOURCE_PATH + localPath + "source_map.txt";
	var sourceMapText = SOURCE_PATH + "\\ " + symbolServerPath;

	WScript.Echo("echo " + sourceMapText + " > " + sourceMapFilename + "\n");

	var ForReading = 1, ForWriting = 2, ForAppending = 3; 
	var TristateUseDefault = -2, TristateTrue = -1, TristateFalse = 0; 
	var fs, f, ts, s; 

	fs = new ActiveXObject("Scripting.FileSystemObject"); 
	fs.CreateTextFile(sourceMapFilename); // Create a file 
	f = fs.GetFile(sourceMapFilename); 
	ts = f.OpenAsTextStream(ForWriting, TristateUseDefault); 
	ts.Write(sourceMapText); 
	ts.Write("\r\n"); 
	ts.Close(); 
}

function CopyFilesToSymbolServer(relativePathList, symbolServerPath, extensions)
{
	for (i=0; i < relativePathList.length; i++)
	{
		var srcPathFull = "\"" + SOURCE_PATH + "\\" + relativePathList[i] + "\"";
		var dstPathFull = "\"" + symbolServerPath + relativePathList[i] + "\"";
		
		WScript.Echo("CopyFilesToSymbolServer: srcPathFull(" + srcPathFull + ") dstPathFull(" + dstPathFull + ")\n");
		
		// Add /nfl to stop it reporting all the file names
		var exitcode = DOS(ROBOCOPY_PATH + " /s /nfl /ndl " + srcPathFull + " " + dstPathFull + " " + extensions, 9, true);
	}
}

function RegisterSymbolServer(path, title, symbolServer)
{
	var version = Env("buildVersion");
	
	var exitcode = DOS(
		Env("SRC_ROOT") + "\\bin\\build_tools\\symstore.exe add /r /p /f " +
		 path + " /s " + symbolServer + " /t " + title + " /v \"" + version + "\" /c \"Adding Build\"");
}					
					

function Build2003(target, config)
{
	var exitcode = DOS("devenv /useenv \"" + target + "\" /build \"" + config + "\"", 9, true);
//	var exitcode = DOS("devenv \"" + target + "\" /build \"" + config + "\"", 9, true);
	
	if (exitcode == 0)
		return;

	g_iErrors++; 	// count the errors

	
//	Shell.Run(g_lastlogfile);
//	WScript.Quit();
}

function Build2005(target, config, useIncredibuild)
{
	var COMPILER_OPTIONS = Env("compilerOptions");
	var exitcode;
	
	if (useIncredibuild == true)
	{
		var commandLine = "BuildConsole \"" + target + "\"";
		
		if (COMPILER_OPTIONS != "")
		{
			commandLine += " /CL_ADD=\"" + COMPILER_OPTIONS + "\"";
		}
		
		commandLine += " /CFG=\"" + config + "\" /Rebuild /useenv"
	
		exitcode = DOS(commandLine);
	}
	else
	{
//	var exitcode = DOS("devenv.com " + target + " /Build \"Master|Xbox 360\" /useenv");
//	var exitcode = DOS("devenv \"" + target + "\" /build \"" + config + "\"", 9, true);
	
		exitcode = DOS("devenv.com " + target + " /build \"" + config + "\" /useenv");
	}
	
	if (exitcode == 0)
		return;

	g_iErrors++; 	// count the errors
}

// run a DOS command
function DOS(command)
{
	var WshShell = new ActiveXObject("WScript.Shell");
	WScript.Echo("running: " + command + "\n");
	var oExec    = WshShell.Exec("cmd /c " + command);
	var output = "";
	var se = "Build log was saved at \"file://";
	
	//WScript.Echo("PATH: " + Env("PATH") + "\n");

	// run the command and display it's stdout
	// oExec.Status == 0
	while (!oExec.StdOut.AtEndOfStream)
	{
		while (!oExec.StdOut.AtEndOfStream)
		{
			var line = oExec.StdOut.ReadLine();
			
			WScript.Echo(line);
		}
		
		WScript.Sleep(100);
	}
	
	// Any bad output?
	while (!oExec.StdErr.AtEndOfStream)
	{
		var line = oExec.StdErr.ReadLine();
		
		WScript.Echo("Error: " + line);
	}
	
	return oExec.ExitCode;
}

function DeleteFile(filename)
{
	var fso = new ActiveXObject("Scripting.FileSystemObject");
	
	if (fso.FileExists(filename))
	{
		WScript.Echo("Deleting '" + filename + "'");

		fso.DeleteFile(filename);
	}
}

function DeleteFolder(folderspec)
{
   var fso = new ActiveXObject("Scripting.FileSystemObject");

	if (fso.FolderExists(folderspec))
	{
		WScript.Echo("Deleting folder '" + folderspec + "'");

		fso.DeleteFolder(folderspec, true);
	}
}

function CreateFolder(folderspec)
{
   var fso = new ActiveXObject("Scripting.FileSystemObject");

	if (!fso.FolderExists(folderspec))
	{
		WScript.Echo("Creating folder '" + folderspec + "'");

		fso.CreateFolder(folderspec);
	}
}
