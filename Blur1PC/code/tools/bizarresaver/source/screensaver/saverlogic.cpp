
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <assert.h>
#include <lm.h>

using namespace std;

bool RunningProgram(char *psz)
{
	// check all running processes - see whether we have maya or photoshop running
	bool bBannedProgram = false;

	HANDLE hSnapShot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);

	PROCESSENTRY32* processInfo=new PROCESSENTRY32;

	processInfo->dwSize=sizeof(PROCESSENTRY32);
	int index=0;

	while(Process32Next(hSnapShot,processInfo)!=FALSE)
	{
		if (stricmp(processInfo->szExeFile, psz) == 0)
			bBannedProgram = true;
	}

	delete processInfo;
	return bBannedProgram;
}

bool MemberOfGroup(char *pszGroup, char* pszServer)
{
	WCHAR wbuffer[512];
	WCHAR wpszGroup[512], wpszServer[512];
	DWORD c = 512;
	GetUserNameW(wbuffer,&c);

//MultiByteToWideChar( CP_ACP, 0, buffer, strlen(buffer)+1, wbuffer, sizeof(buffer[0])/sizeof(wbuffer[0]) );
	MultiByteToWideChar( CP_ACP, 0, pszGroup, strlen(pszGroup)+1, wpszGroup, 512 );
	MultiByteToWideChar( CP_ACP, 0, pszServer, strlen(pszServer)+1, wpszServer, 512 );

	LPGROUP_USERS_INFO_0 pBuf = NULL;
	DWORD dwLevel = 0;
	DWORD dwPrefMaxLen = MAX_PREFERRED_LENGTH;
	DWORD dwEntriesRead = 0;
	DWORD dwTotalEntries = 0;
	NET_API_STATUS nStatus;

	nStatus = NetUserGetGroups(wpszServer, wbuffer, dwLevel, (LPBYTE*)&pBuf, dwPrefMaxLen, &dwEntriesRead, &dwTotalEntries);

	// couldn't read groups, so can't determine whether we're part of a particular group
	if (nStatus != NERR_Success)
		return false;

	LPGROUP_USERS_INFO_0 pTmpBuf;
	DWORD i;
	DWORD dwTotalCount = 0;

	if ((pTmpBuf = pBuf) != NULL)
	{
		for (i = 0; i < dwEntriesRead; i++)
		{
			if (_wcsicmp(pTmpBuf[i].grui0_name,wpszGroup) == 0)
				return true;
		}
	}


	return false;
}

// this function will check various settings and decide whether the screensaver is allowed to run on this PC
bool OkToRunScreenSaver()
{
	TCHAR server[512];

	//look for ini file in the same directory
	TCHAR path[512];
	{
		::GetModuleFileName(NULL, path, 512);

		TCHAR *pCurrent = path;
		TCHAR *pLast = path;
		while (*pCurrent)
		{
			if (*pCurrent == '/' || *pCurrent== '\\')
				pLast = pCurrent+1;
			pCurrent++;
		}

		sprintf_s(pLast, path+512 - pLast, "screensaver.ini");
	}

	GetPrivateProfileString("server", "name", "uklpldc01.activision.com", server, 512, path);
	//the native windows ini parser doesn't allow duplicate sections. As a workaround, we'll assume our sections are named consecutively "exclude%d"

	for (int i=0; i<16; ++i)
	{
		TCHAR section[512];
		TCHAR option[2048];
		sprintf_s(section, 512, "exclude%d", i);

		int nConditions = 0;
		int nConditionsPassed = 0;
		if (GetPrivateProfileString(section, "group", "", option, 2048, path)>0)
		{
			nConditions++;
			if (MemberOfGroup(option, server))
				nConditionsPassed++;
		}

		if (GetPrivateProfileString(section, "programs", "", option, 2048, path)>0)
		{
			nConditions++;
			TCHAR *pStart = option, *pEnd = option;
			while (*pEnd)
			{
				if (*pEnd == ';')
				{
					*pEnd = '\0';
					if (RunningProgram(pStart))
					{
						pStart = 0;
						nConditionsPassed++;
						break;
					}

					pEnd++;
					pStart = pEnd;
				}
				else
					pEnd++;
			}

			if (pStart && RunningProgram(pStart))
				nConditionsPassed++;
		}

		if (GetPrivateProfileString(section, "machines", "", option, 2048, path)>0)
		{
			char computerName[MAX_COMPUTERNAME_LENGTH + 1];
			DWORD bufferSize = MAX_COMPUTERNAME_LENGTH;
			BOOL success = GetComputerName(computerName, &bufferSize);

			if (success)
			{
				nConditions++;

				TCHAR *pStart = option, *pEnd = option;
				while (*pEnd)
				{
					if (*pEnd == ';')
					{
						*pEnd = '\0';
						if (_stricmp(computerName, pStart)==0)
						{
							pStart = 0;
							nConditionsPassed++;
							break;
						}

						pEnd++;
						pStart = pEnd;
					}
					else
						pEnd++;
				}

				if (pStart && _stricmp(computerName, pStart)==0)
					nConditionsPassed++;
			}
		}

		if (nConditions>0 && nConditions==nConditionsPassed)
			return false;
	}

	return true;
}
