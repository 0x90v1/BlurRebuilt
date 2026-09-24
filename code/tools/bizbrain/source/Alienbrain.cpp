#include "stdafx.h"

#include "AlienBrain.h"

#include <system/shell/shellexec.h>
#include <system/serialise/C_FileSystem.h>


C_AlienBrainWrapper::C_AlienBrainWrapper()
{
	m_pIntegrator = 0;
	m_pProject = 0;
}

bool C_AlienBrainWrapper::Initialise(
   const Btl::String& username, 
   const Btl::String& password, 
   const Btl::String& project,
   const Btl::String& server)
{
	if (m_pIntegrator)
	{
		BIZ_ASSERT(!"Alienbrain Wrapper already initialised!");
		return false;
	}

	if (!m_pIntegrator)
	{
		m_pIntegrator = new CNxNSmartIntegrator();
		
		if (!m_pIntegrator)
			return false;
		
		if (!m_pIntegrator->InitInstance(false))
		{
			delete m_pIntegrator;
			m_pIntegrator = NULL;
			return false;
		}
	}

	CNxNWorkspace* pWorkspace = m_pIntegrator->GetIntegrator()->OpenWorkspace();
	m_pProject = pWorkspace->GetProject(project.Chars());
	if (!m_pProject)
	{
		m_pProject = pWorkspace->InsertProject(
			"mercurygame", 
			"kenm",
			"kenm",
			"bigboy");
	}
/*	
	m_pProject = pWorkspace->LoadProjectEx(
		"MercuryGame",
		"kenm",
		"kenm",
		"bigboy",
		true,
		NXN_TIMESYNC_POLICY_NEVER);
*/
/*
    CNxNString sParentPath = m_pIntegrator->GetIntegrator()->GetNamespacePath();
	CNxNCommand cmd("ProjectLoadEx", sParentPath, 1, 0);
    cmd.SetAt(0, "MercuryGame");
	cmd.SetAt(1, "kenm");
	cmd.SetAt(2, "kenm");
	cmd.SetAt(3, "Bigboy");
	cmd.SetAt(4, "0");
	cmd.SetAt(5, "Never");
    if (!m_pIntegrator->GetIntegrator()->RunCommand(cmd))
*/
	CNxNVarCommand cmd("ProjectLoadEx");
	cmd.SetIn("Name", "MercuryGame");
	cmd.SetIn("Username", "kenm");
	cmd.SetIn("Password", "kenm");
	cmd.SetIn("Hostname", "Bigboy");
	cmd.SetIn("ShowDialog", true);
	cmd.SetIn("TimeSyncPolicy", "Never");
	bool test = m_pIntegrator->GetIntegrator()->RunCommand(cmd);
	if (cmd.WasSuccessful())
    {
        // if we reach this point an error occurred and the command could
        // not be executed successfully
		int win = 0;
    }
    else
    {
        // the command execution was successful.
		int fail = 0;
    }

/*
var oXMLParam = tAPP_GetNxNXMLParamObject();

oXMLParam.Command = "ProjectLoadEx";
oXMLParam.ParamIn( "Name" ) = "Dune";
oXMLParam.ParamIn( "Username" ) = "Karlos";
oXMLParam.ParamIn( "Password" ) = "du873kgf";
oXMLParam.ParamIn( "Hostname" ) = "SceneServer";
oXMLParam.ParamIn( "ShowDialog" ) = "0";
oXMLParam.ParamIn( "TimeSyncPolicy" ) = "Always";
oXMLParam.VerboseLevel = 1;

oXMLParam.XML = oWorkspace.RunCommand( "ProjectLoadEx", oXMLParam.XML, false );
if (oXMLParam.WasSuccessful != true)
{
  window.external.alert( "Project load failed!" );
}

{
	CNxNVarCommand cmd("ProjectLoadEx", 
}
*/
	bool result = true;
	result &= m_pProject->SetUserName("robot");
	result &= m_pProject->SetPassword("kenm");
	result &= m_pProject->SetHostName("bigboy");

	if (!m_pProject->Connect(
		"",
		"",
		"",
		true, 
		NXN_TIMESYNC_POLICY_NEVER))
		return false;

	return true;
}

void C_AlienBrainWrapper::Shutdown()
{
	if (m_pProject)
		m_pProject->Disconnect();
//	SAFE_DELETE_PTR(m_pProject);
	if (m_pIntegrator)
		m_pIntegrator->ExitInstance();
	SAFE_DELETE_PTR(m_pIntegrator);
}

// release alienbrain
C_AlienBrainWrapper::~C_AlienBrainWrapper()
{
	BIZ_ASSERT(!m_pIntegrator);
}

// get the latest version of a particular file
bool C_AlienBrainWrapper::GetLatest(const Btl::String& psz, U32 nRetries)
{
	return true;
	/*
	CNxNString nxnpath;

	if (!m_pIntegrator->MapManagedPath(psz, nxnpath))
	{
		return false; // don't need to get latest on local file
	}

	// First test if we're getting a file
	CNxNFile *pFile = (CNxNFile*)m_pIntegrator->GetManagedNode(nxnpath, CNxNFile::GetType());

	// If this isn't a file, test if its a folder
	if (!pFile)
	{
		CNxNFolder *pFolder = (CNxNFolder*)m_pIntegrator->GetManagedNode(nxnpath, CNxNFolder::GetType());
		if (!pFolder)
			return false;

		bool bSuccess = pFolder->GetLatest(false, false, false, false, false, "", true, false);
		for (U32 i = 0; i < nRetries && !bSuccess; ++i)
		{
			LOGINFO(g_log, "GetLatest Failed... retrying %i/%i\n", i+1, nRetries);
			bSuccess = pFolder->GetLatest(false, false, false, false, false, "", true, false);
		}
		delete pFolder;

		return bSuccess;
	}

	bool bSuccess = pFile->GetLatest(false, false, false, false, false, "", true, false);
	for (U32 i = 0; i < nRetries && !bSuccess; ++i)
	{
		LOGINFO(g_log, "GetLatest Failed... retrying %i/%i\n", i+1, nRetries);
		bSuccess = pFile->GetLatest(false, false, false, false, false, "", true, false);
	}
	delete pFile;
	return bSuccess;
	*/
}

bool C_AlienBrainWrapper::SyncLatest(const Btl::String& psz, U32 nRetries, bool forceOnFailure)
{
	return true;
	/*
	CNxNString nxnpath;

	if (!m_pIntegrator->MapManagedPath(psz, nxnpath))
	{
		return false; // can't sync latest on local file
	}
	
	CNxNFolder *pFolder = (CNxNFolder*)m_pIntegrator->GetManagedNode(nxnpath, CNxNFolder::GetType());
	if (!pFolder)
	{
		//we're only interested in folders for syncing
		return false;
	}

	//now, recurse every file in this folder, and see if it exists in the database. 
	//If not, delete it.

	//flip slashes for windows!
	char buffer[260];
	memset(buffer, 0, sizeof(char)*260);
	strncpy_s(buffer, psz, 260);
	char *cp = buffer;

	while (*cp)
	{
		if (*cp == '/')
		{
			*cp = '\\';
		}	
		cp++;	
	}

	//add trailing slash if not present

	size_t length = strlen(buffer);
	if (buffer[length-1] == '\\')
	{
		RecurseDeleteFilesNotInAb(CNxNString(buffer));
	}
	else
	{
		RecurseDeleteFilesNotInAb(CNxNString(buffer) + "\\");
	}

	//finally, do a get latest on the folder
	bool bSuccess = pFolder->GetLatest(false, false, false, false, false, "", true, false);

	for (U32 i=0; i<nRetries && !bSuccess; ++i)
	{
		LOGINFO(g_log, "ABSyncLatest Failed... retrying %i/%i\n", i+1, nRetries);
		bSuccess = pFolder->GetLatest(false, false, false, false, false, "", true, false);
	}

	delete pFolder;

	if (forceOnFailure && !bSuccess)
	{
		LOGWARN(g_log, "All retries have failed ... performing a GetLatest from scratch\n");

		U32 exitCode = Sys::ShellExec::ExecuteCommandAndWait(
			"cmd",
			Util::C_String("/C rmdir /S /Q \"") + buffer + "\"",
			Sys::C_FileSystem::Instance().GetWorkingDirectory(),
			Sys::ShellExec::E_LaunchState_Minimise);

		if (exitCode != 0)
			LOGWARN(g_log, "Failed to properly delete folder prior to performing GetLatest\n");

		bSuccess = GetLatest(psz, nRetries);
	}

	return bSuccess;
	*/
}

void C_AlienBrainWrapper::RecurseDeleteFilesNotInAb(CNxNString folderName)
{
	//for debugging: ABSyncLatest("z:\\shitstorm\\assets\\levels\\manor2\\")
	WIN32_FIND_DATA fileData;
	
	HANDLE handle = FindFirstFile(folderName + "*.*", &fileData);
	if (handle == INVALID_HANDLE_VALUE)
		return;

	bool bGoing=true;
	while (bGoing)
	{
		CNxNString fileAndPath = folderName + fileData.cFileName;

		bool bDirectory = ((GetFileAttributes(fileAndPath) & FILE_ATTRIBUTE_DIRECTORY) != 0);
		if (bDirectory)
		{
			if(strcmp(".", fileData.cFileName) && strcmp("..", fileData.cFileName))
			{
				RecurseDeleteFilesNotInAb(fileAndPath + "\\");
			}
		}
		else
		{
			//can we map the file in AB? If not, make sure its not readonly and delete
			CNxNString nxnpath;
			m_pIntegrator->MapManagedPath(fileAndPath, nxnpath);
			CNxNFile *pFile = (CNxNFile*)m_pIntegrator->GetManagedNode(nxnpath, CNxNFile::GetType());

			if (!pFile)
			{
				//is it read only?

				DWORD attr = GetFileAttributes(fileAndPath);
				if (attr&FILE_ATTRIBUTE_READONLY)
				{
					SetFileAttributes(fileAndPath, attr&=~FILE_ATTRIBUTE_READONLY);
				}
				
				//delete it

				DeleteFile(fileAndPath);
			}
			else
			{
				delete pFile;
			}
		}
		(FindNextFile(handle, &fileData)==TRUE)?(bGoing = true):(bGoing = false);
	}
}

