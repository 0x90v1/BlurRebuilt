#include "StdAfx.h"
#include "Farm.h"
#include "FatalEmail.h"
#include <core\Core.h>
#include <core\threads.h>
#include <system\network.h>
#include <system\serialise\c_filesystem.h>
#include <system\network\smtpclient.h>

#if defined(PLATFORM_PC) && defined(COMPILER_VC)

// run a commandline and wait for it to complete before returning
int RunCommandline(const Util::C_String &cmdline)
{
      DWORD iExitCode = 0;
 
      PROCESS_INFORMATION     Pi;
      STARTUPINFO             Si;
 
      ZeroMemory(&Pi, sizeof(Pi));
      ZeroMemory(&Si, sizeof(Si));
 
      Si.cb = sizeof(Si);
 
      char Buffer[10000];
      strcpy(Buffer, cmdline.Chars());
 
      // create process that runs the worker command
      if (CreateProcess(0, Buffer, 0, 0, TRUE, 0, 0, 0, &Si, &Pi) != FALSE)
      {
			// wait no more than 10 seconds
            if (WaitForSingleObject( Pi.hProcess, 10 * 1000 ) == WAIT_TIMEOUT)
				TerminateProcess(Pi.hProcess, -1);

            // propagate bad exit codes
            DWORD dwExitCode = -1;
            BOOL bOk = GetExitCodeProcess( Pi.hProcess, &dwExitCode );
 
			CloseHandle(Pi.hProcess);
            return dwExitCode;
      }
 
      return -1;
}

bool GetEMailXMLFromJobID(int iJobID, int iType, int iLogLine, char *pszTemp)
{
	Util::C_String cmdline;
	cmdline.Printf("cmd.exe /c \""
		"\"c:\\program files\\wamp\\php\\php.exe\" -c \"c:\\program files\\wamp\\apache\\php.ini\" c:\\www\\jobinfo_email.php %d %d %d > %s\"", iJobID, iType, iLogLine, pszTemp);

	return RunCommandline(cmdline) == 0;
}

bool SendEmail(int iJobID, int iType, int iLogLine, const Ut::Vector<std::string> &recipients, std::string subject)
{
	char pszTempPath[_MAX_PATH];
	if (!GetTempPath(_MAX_PATH, pszTempPath))
		return false;

	char pszTemp[_MAX_PATH];
	if (!GetTempFileName(pszTempPath, "email", 0, pszTemp))
		return false;

	Sys::C_FileSystem& fs = Sys::C_FileSystem::Instance();

	if (!GetEMailXMLFromJobID(iJobID, iType, iLogLine, pszTemp))
		return false;
	Sys::I_File *pFile = fs.Open(pszTemp);
	if (!pFile)
		return false;

	int c = pFile->Length();
	char *pszFileText = new char[c+1];
	pFile->Read(pszFileText, c);
	pszFileText[c] = 0;
	delete pFile;

	//-initialize OS socket resources!
	WSADATA	wsaData;

	WSAStartup(MAKEWORD(2, 2), &wsaData);

	// send the email to all the recipients
	Network::SMTPClient smtp;
	smtp.Connect("uklplex01.activision.com");
	smtp.Login("farm", "Password_1");
	for (int i=0; i<recipients.Size(); i++)
	{
		smtp.SendEMailHeaderHTML("farmserver@bizarre2.local", recipients[i].c_str(), subject.c_str(), "Farm Server");

		smtp.SendEMailBody(pszFileText);
		smtp.FinishEMail();
	}

	delete pszFileText;

	fs.DeleteFile(pszTemp);

	return true;
}

#else

bool SendEmail(int iJobID, int iType, int iLogLine, const Ut::Vector<std::string> &recipients, std::string subject)
{
}

#endif
