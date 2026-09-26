// alienbrain_wrapper.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include <stdio.h>
#include <winbase.h>

#pragma warning(push)
	#pragma warning(disable: 4267)
	#include <NxNIntegratorSDK.h>
	#include <NxNSmartIntegrator.h>
#pragma warning(pop)

#define null NULL

#ifdef _MANAGED
#pragma managed(push, off)
#endif

#include <atlstr.h>

#define PATH_SIZE 2048

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
    return TRUE;
}


// My code

CNxNSmartIntegrator *m_pSmartIntegrator = NULL;
CNxNWorkspace *m_pWorkspace = NULL;
CNxNProject* m_pProject = NULL;
sNxNBasicNodeInfo *m_basicNodeInfoArray = NULL;

extern "C" __declspec(dllexport) BOOL InitialiseAB(void)
{
	m_pSmartIntegrator = new CNxNSmartIntegrator();

	if (m_pSmartIntegrator == NULL || !m_pSmartIntegrator->InitInstance(NXN_CONNECT_HIDE_DIALOG_AND_CONNECT, "Hello"))
    {
        printf("Could not initialize the integrator object.\n");
        delete m_pSmartIntegrator;
        return FALSE;
    }

	m_pWorkspace = m_pSmartIntegrator->GetIntegrator()->CreateWorkspace();
//	m_pWorkspace = m_pIntegrator->CreateWorkspace();
    if (m_pWorkspace == NULL)
    {
        printf("Could not create workspace.\n");
        m_pSmartIntegrator->ExitInstance();
        delete m_pSmartIntegrator;
		m_pSmartIntegrator = NULL;
        return -1;
    }

    return TRUE;
}

// Returns the length of the local path, or 0.
extern "C" __declspec(dllexport) int LoadProjectAB(char *strProject, char * strLocalPath, int localPathMaxLen)
{
	try
	{
		printf("Loading project...\n");

		if(strProject == NULL)
		{
			m_pProject = m_pWorkspace->BrowseForProject();
			//if(m_pProject != NULL)
			//{
			//	m_pWorkspace->LoadProject(L"Gotham");
			//	//m_pProject->Connect();
			//}
			//m_pProject = m_pWorkspace->LoadProject(m_pProject->GetNamespacePath());
		}
		else
		{
			m_pProject = m_pWorkspace->LoadProject(strProject);
		}

		if (m_pProject == NULL)
		{
			return 0;
	  //      printf("Could not create project.\n");
			//m_pSmartIntegrator->GetIntegrator()->CloseWorkspace(m_pWorkspace);
	  //      m_pSmartIntegrator->ExitInstance();
	  //      delete m_pSmartIntegrator;
	  //      return NULL;
		}

		//CNxNString nxnString = m_pProject->GetLocalPath();
		CNxNString nxnString = m_pProject->GetNamespacePath();
		CStringA sFile((const char*)nxnString);

		//strcpy(sBuffer, sFile.GetBuffer());
		//strncpy(strLocalPath, sFile.GetBuffer(), localPathMaxLen);
		strncpy_s(strLocalPath, PATH_SIZE, sFile.GetBuffer(), localPathMaxLen);
		return strlen(sFile.GetBuffer());
	}
	catch(...)
	{
		return 0;
	}
}


extern "C" __declspec(dllexport) BOOL DeInitialiseAB(void)
{
	try
	{
		if(m_pSmartIntegrator != NULL)
		{
			m_pSmartIntegrator->ExitInstance();
			delete m_pSmartIntegrator;
			m_pSmartIntegrator = NULL;
			return true;
		}
	}
	catch(...)
	{
		return FALSE;
	}
	return FALSE;
}




extern "C" __declspec(dllexport) BOOL UndoCheckoutAB(char *strLocalFile)
{
	try
	{
		if(m_pProject != NULL)
		{
			CNxNString sNamespacePath;

			if (m_pSmartIntegrator->MapManagedPath(strLocalFile, sNamespacePath))
			{
				CNxNFile *pFile = (CNxNFile*)m_pProject->FindNode(sNamespacePath);

				if(pFile != NULL)
				{
					bool retval = pFile->UndoCheckOut(false, false);
					return retval;
				}
				else
				{
					return FALSE;
				}
			}
		}

		return FALSE;
	}
	catch(...)
	{
		return FALSE;
	}
}


CNxNFolder m_pFolder = NULL;

extern "C" __declspec(dllexport) BOOL DoTestAB(char *strFileOrFolder)
{
	try
	{
		if(m_pProject != NULL)
		{
			m_pProject->GetNamespace();
			
			// this is a temporary variable for the namespace path
			CNxNString sNamespacePath;

			// map the local file path to a corresponding namespace path
			if (m_pSmartIntegrator->MapManagedPath(strFileOrFolder, sNamespacePath))
			{
				OutputDebugString(sNamespacePath.GetBSTR());
				OutputDebugString(L"\n");
	//			CNxNNode *pChild = m_pProject->GetFolder(sNamespacePath);
	//			CNxNNode *pChild = m_pProject->GetFolder("\\Workspace\\PGR4\\TextureToolTest\\");
				CNxNNode *pParent = m_pProject->FindNode(sNamespacePath);
				CNxNNode *pChild = pParent->GetFirstChild();
				//CNxNNode *pChild = m_pProject->GetFirstChild();

				

				sNxNBasicNodeInfo basicNodeInfo;

				CNxNProperty propProperty;

				while(pChild != NULL)
				{
					pChild->GetNodeInfo(&basicNodeInfo);

					if (basicNodeInfo.basicType == NXN_BASICTYPE_FOLDER)
					{
						CNxNFolder *pFolder = (CNxNFolder *)pChild;

						CString sThing((const char*)pChild->GetName());

						pChild->GetProperty(propProperty);

						OutputDebugString(L"NXN_BASICTYPE_FOLDER: ");
						OutputDebugString(sThing.GetBuffer());
						OutputDebugString(L"\n");
					}
					else if (basicNodeInfo.basicType == NXN_BASICTYPE_FILE)
					{
						CNxNFile *pFile = (CNxNFile *)pChild;

						CString sThing((const char*)pChild->GetName());

						OutputDebugString(L"NXN_BASICTYPE_FILE: ");
						OutputDebugString(sThing.GetBuffer());
						OutputDebugString(L"\n");
					}
					else if (basicNodeInfo.basicType == NXN_BASICTYPE_DISKFOLDER)
					{
						CNxNFolder *pFolder = (CNxNFolder *)pChild;

						CString sThing((const char*)pChild->GetName());

						pChild->GetProperty(propProperty);

						OutputDebugString(L"NXN_BASICTYPE_DISKFOLDER: ");
						OutputDebugString(sThing.GetBuffer());
						OutputDebugString(L"\n");
					}
					else if (basicNodeInfo.basicType == NXN_BASICTYPE_DISKFILE)
					{
						CNxNFile *pFile = (CNxNFile *)pChild;

						CString sThing((const char*)pChild->GetName());

						OutputDebugString(L"NXN_BASICTYPE_DISKFILE: ");
						OutputDebugString(sThing.GetBuffer());
						OutputDebugString(L"\n");


					}

										CNxNProperty propProperty;
						CString sThing2((const char*)(pChild->GetStringProperty("SFSZ", NXNCONST_GETPROP_DIRECT)));
						OutputDebugString(L" ");
						OutputDebugString(sThing2.GetBuffer());
						OutputDebugString(L"\n");


					pChild = pParent->GetNextChild();
					//pChild = m_pProject->GetNextChild();
				}
				OutputDebugString(L"\n");
				OutputDebugString(L"\n");
				OutputDebugString(L"\n");
			}
		}

		return FALSE;
	}
	catch(...)
	{
		return FALSE;
	}
}



/*
CNxNFolder* GetFolderNode(const CNxNString& Folder)
{
	CString sFolder((const char*)Folder);

	CNxNString sNamespacePath;

	// map the local file path to a concerning namespace path, successfully mapped?
	if (m_pSmartIntegrator->MapManagedPath(sFolder.GetBuffer(), sNamespacePath))
    {
		// successfully mapped, get the concerning node
		CNxNFolder* pFolder = (CNxNFolder*)m_pSmartIntegrator->GetManagedNode(sNamespacePath, CNxNFolder::GetType());

		if(pFolder == NULL)
			pFolder = (CNxNFolder*)m_pSmartIntegrator->GetManagedNode(sNamespacePath, CNxNProject::GetType());

		// valid file? => return it!
		if (pFolder != NULL)
        {
			return pFolder;
		}
	}

	// if we reach this pointer, it was not found or the name was invalid
	return NULL;
}
*/

CNxNNode *m_pParentOfGetFirstChild = NULL;


bool GetNodeDetails(CNxNNode *pChild, long *flags, int *type, char *local_path, char *name, char *namespace_path, char *smot, char *cmot)
{
	try
	{
		sNxNBasicNodeInfo basicNodeInfo;

		//CNxNProperty propProperty;
		pChild->GetNodeInfo(&basicNodeInfo);

		*type = basicNodeInfo.basicType;
		*flags = basicNodeInfo.lFlags;
		strncpy_s(local_path, PATH_SIZE, basicNodeInfo.sLocalPath, _TRUNCATE);
		strncpy_s(name, PATH_SIZE, basicNodeInfo.sName, _TRUNCATE);
		strncpy_s(namespace_path, PATH_SIZE, basicNodeInfo.sNamespacePath,	_TRUNCATE);
	/*
		const char *cmot_result = (const char*)(pChild->GetStringProperty("CMOT", NXNCONST_GETPROP_DIRECT));
		if(cmot_result != NULL)
			strncpy_s(cmot, PATH_SIZE, cmot, _TRUNCATE);

		const char *smot_result = (const char*)(pChild->GetStringProperty("SMOT", NXNCONST_GETPROP_DIRECT));
		if(smot_result != NULL)
			strncpy_s(smot, PATH_SIZE, smot, _TRUNCATE);
	*/
		return TRUE;
	}
	catch(...)
	{
		return FALSE;
	}
}


extern "C" __declspec(dllexport) BOOL GetNodeDetailsAB(char *strPath, long *flags, int *type, char *local_path, char *name, char *namespace_path, char *smot, char *cmot, bool bPathIsNamespace)
{
	try
	{
		if(m_pProject != NULL)
		{
			//CNxNProject *pProject;
			CNxNNode *pNode = NULL;

			if(bPathIsNamespace == false)
			{
				//char local_path[PATH_SIZE];
				CNxNString cString;
				if (m_pSmartIntegrator->MapManagedPath(strPath, cString))
					//pProject  = (CNxNProject *)m_pSmartIntegrator->GetManagedNode(local_path, CNxNProject::GetType());
					pNode = m_pSmartIntegrator->GetMapper()->GetManagedNode(cString);

			}
			else
			{
				pNode  = m_pSmartIntegrator->GetManagedNode(strPath, CNxNProject::GetType());
			}

			if(pNode != NULL)
			{
				bool retval = GetNodeDetails(pNode, flags, type, local_path, name, namespace_path, smot, cmot);

				if(retval == true && *type == NXN_BASICTYPE_UNKNOWN)
					retval = false;

				return retval;
			}
		}

		return FALSE;
	}
	catch(...)
	{
		return FALSE;
	}

}

/*
extern "C" __declspec(dllexport) BOOL GetFirstChildAB(char *strPath, long *flags, int *type, char *local_path, char *name, char *namespace_path, char *smot, char *cmot)
{
	if(m_pProject != NULL)
	{
		//m_pProject->GetNamespace();
		
		//m_pParentOfGetFirstChild = GetFolderNode(strPath);

		m_pParentOfGetFirstChild  = (CNxNFolder*)m_pSmartIntegrator->GetManagedNode(strPath, CNxNFolder::GetType());

		if(m_pParentOfGetFirstChild == NULL)
			m_pParentOfGetFirstChild = (CNxNFolder*)m_pSmartIntegrator->GetManagedNode(strPath, CNxNProject::GetType());





		if(m_pParentOfGetFirstChild != NULL)
		{
			CNxNNode *pChild = m_pParentOfGetFirstChild->GetFirstChild();

			if(pChild != NULL)
			{
				return GetNodeDetails(pChild, flags, type, local_path, name, namespace_path, smot, cmot);
			}
		}
	}
	return FALSE;
}
*/


int m_nodeIndex = 0;
int m_numNodes = NULL;

int PrepareToEnumerateChildren(char *strPath)
{
	if(m_pProject != NULL)
	{
		m_pParentOfGetFirstChild = m_pSmartIntegrator->GetMapper()->GetManagedNode(strPath);

		if(m_pParentOfGetFirstChild != NULL)
		{
			if(m_basicNodeInfoArray != NULL)
				delete[] m_basicNodeInfoArray;

			m_nodeIndex = 0;

			m_numNodes = m_pParentOfGetFirstChild->GetChildCount();
			if(m_numNodes > 0)					// \Workspace\PGR4\TextureToolTest\textures 4\Art\Cars\Saleen_SR7_Test\Backup\Saleen_S7_15th_April_B2 returns '-1' children.
			{
				m_basicNodeInfoArray = (sNxNBasicNodeInfo*) new sNxNBasicNodeInfo[m_numNodes];

				m_pParentOfGetFirstChild->GetNextNodeInfos(true, m_numNodes, m_basicNodeInfoArray);
			}
			return m_numNodes;
		}
	}
	return -1;
}

extern "C" __declspec(dllexport) BOOL GetLatestFileAB(char *strFile)
{
	try
	{
		if(m_pProject != NULL)
		{
			CNxNString sNamespacePath;

			if (m_pSmartIntegrator->MapManagedPath(strFile, sNamespacePath))
			{
				CNxNFile *pFile = (CNxNFile*)m_pProject->FindNode(sNamespacePath);

				if(pFile != NULL)
				{


					bool retval = pFile->GetLatest(false, false, false, false, false, "", true, false, NXN_GETLATEST_OVERWRITE_WRITABLE_DEFAULT, NXN_GETLATEST_OVERWRITE_CHECKEDOUT_DEFAULT, NXN_GETLATEST_FILETIME_DEFAULT);
					return retval;
				}
				else
				{
					return NULL;
				}
			}
			//m_pProject->SetWorkingPath(strFileOrFolder);
			//return m_pProject->GetLatest(true, false, false, false, false, "", false, false);
		}
	}
	catch(...)
	{
		OutputDebugString(L"Unhandled exception in GetNextChildAB.");
	}

	return FALSE;
}

extern "C" __declspec(dllexport) BOOL GetLatestFolderAB(char *strFolder)
{
	if(m_basicNodeInfoArray != NULL)		// Do not even try if there is an enumeration underway.
		return false;

	try
	{
		PrepareToEnumerateChildren(strFolder);

		if(m_pProject != NULL)
		{
			CNxNString sNamespaceFolder;

			if (m_pSmartIntegrator->MapManagedPath(strFolder, sNamespaceFolder))
			{
				CNxNFolder *pFolder = (CNxNFolder*)m_pProject->FindNode(sNamespaceFolder);

				if(pFolder != NULL)
				{
					bool retval = pFolder->GetLatest(false, false, false, false, false, "", false, false, NXN_GETLATEST_OVERWRITE_WRITABLE_DEFAULT, NXN_GETLATEST_OVERWRITE_CHECKEDOUT_DEFAULT, NXN_GETLATEST_FILETIME_DEFAULT);
					return retval;
				}
				else
				{
					return NULL;
				}
			}
			//m_pProject->SetWorkingPath(strFileOrFolder);
			//return m_pProject->GetLatest(true, false, false, false, false, "", false, false);
		}
	}
	catch(...)
	{
		OutputDebugString(L"Unhandled exception in GetNextChildAB.");
	}

	return FALSE;
}
extern "C" __declspec(dllexport) int PrepareToEnumerateChildrenAB(char *strPath)
{
	try
	{
		return PrepareToEnumerateChildren(strPath);
	}
	catch(...)
	{
		return 0;
	}
}


extern "C" __declspec(dllexport) BOOL GetNextEnumeratedChildAB(long *flags, int *type, char *local_path, char *name, char *namespace_path)
{
	try
	{
		if(m_basicNodeInfoArray != null)
		{
			sNxNBasicNodeInfo *pBasicNodeInfo = &m_basicNodeInfoArray[m_nodeIndex];

			*type = pBasicNodeInfo->basicType;
			*flags = pBasicNodeInfo->lFlags;
			strncpy_s(local_path, PATH_SIZE, pBasicNodeInfo->sLocalPath, _TRUNCATE);
			strncpy_s(name, PATH_SIZE, pBasicNodeInfo->sName, _TRUNCATE);

			strncpy_s(namespace_path, PATH_SIZE, pBasicNodeInfo->sNamespacePath, _TRUNCATE);

			m_nodeIndex++;

			if(m_nodeIndex == m_numNodes)
			{
				delete[] m_basicNodeInfoArray;
				m_basicNodeInfoArray = null;
				m_nodeIndex = 0;
				m_numNodes = 0;
			}

			return true;
		}
		else
			return false;
	}
	catch(...)
	{
		return FALSE;
	}
}



extern "C" __declspec(dllexport) BOOL GetFirstChildAB(char *strPath, long *flags, int *type, char *local_path, char *name, char *namespace_path, char *smot, char *cmot)
{
	try
	{
		if(m_pProject != NULL)
		{
			m_pParentOfGetFirstChild = m_pSmartIntegrator->GetMapper()->GetManagedNode(strPath);

			if(m_pParentOfGetFirstChild != NULL)
			{
				m_numNodes = m_pParentOfGetFirstChild->GetChildCount();
				m_basicNodeInfoArray = (sNxNBasicNodeInfo*) new sNxNBasicNodeInfo[m_numNodes];// malloc(sizeof(sNxNBasicNodeInfo) * m_numNodes * 2);

				m_pParentOfGetFirstChild->GetNextNodeInfos(true, m_numNodes, m_basicNodeInfoArray);

				delete[] m_basicNodeInfoArray;
				CNxNNode *pChild = m_pParentOfGetFirstChild->GetFirstChild();

				if(pChild != NULL)
				{
					return GetNodeDetails(pChild, flags, type, local_path, name, namespace_path, smot, cmot);
				}
			}
		}
		return FALSE;
	}
	catch(...)
	{
		return FALSE;
	}

}


extern "C" __declspec(dllexport) BOOL GetFirstFolderAB(char *strPath, long *flags, int *type, char *local_path, char *name, char *namespace_path, char *smot, char *cmot)
{
	try
	{
		if(m_pProject != NULL)
		{
			m_pParentOfGetFirstChild = m_pSmartIntegrator->GetMapper()->GetManagedNode(strPath);

			if(m_pParentOfGetFirstChild != NULL)
			{
				CNxNNode *pChild = m_pParentOfGetFirstChild->GetFirstChild();
				while(pChild)
				{
					sNxNBasicNodeInfo basicNodeInfo;

					CNxNProperty propProperty;
					pChild->GetNodeInfo(&basicNodeInfo);

					if((basicNodeInfo.basicType == NXN_BASICTYPE_FOLDER) || (basicNodeInfo.basicType == NXN_BASICTYPE_DISKFOLDER))
						return GetNodeDetails(pChild, flags, type, local_path, name, namespace_path, smot, cmot);

					pChild = m_pParentOfGetFirstChild->GetNextChild();
					
				}
			}
		}
		return FALSE;
	}
	catch(...)
	{
		return FALSE;
	}
}



int bGetNextChildTest = 0;

extern "C" __declspec(dllexport) BOOL GetNextChildAB(long *flags, int *type, char *local_path, char *name, char *namespace_path, char *smot, char *cmot)
{
	try
	{
		bGetNextChildTest++;

		if(bGetNextChildTest != 1)
		{
			OutputDebugString(L"\nIn 1 XXXXXXXXXXXBROKENXXXXXXXXXXX\n");
		}
		else
		{
			try
			{
				if(m_pProject != NULL)
				{
					if(m_pParentOfGetFirstChild == NULL)
					{
						OutputDebugString(L"\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
						OutputDebugString(L"BROKEN BROKEN BROKEN BROKEN BROKEN.");
						OutputDebugString(L"\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
					}

					CNxNNode *pChild = m_pParentOfGetFirstChild->GetNextChild();

					if(pChild != NULL)
					{
						bool bRetval = GetNodeDetails(pChild, flags, type, local_path, name, namespace_path, smot, cmot);
						bGetNextChildTest--;
						return bRetval;
					}
				}
			}
			catch(...)
			{
				OutputDebugString(L"Unhandled exception in GetNextChildAB.");
			}
		}
		bGetNextChildTest--;
		return FALSE;
	}
	catch(...)
	{
		return FALSE;
	}
}


#ifdef smeg
extern "C" __declspec(dllexport) BOOL ImportAB(char *local_path)
{

	if(m_pProject != NULL)
	{
		m_pProject->GetNamespace();
		
        // this is a temporary variable for the namespace path
        CNxNString sNamespacePath;

        // map the local file path to a corresponding namespace path
        if (m_pSmartIntegrator->MapManagedPath(local_path, sNamespacePath))
        {
			CNxNFile *cf = (CNxNFile *)(m_pProject->FindNode(sNamespacePath));
			CNxNFolder *cfold = (CNxNFolder *)cf.GetParent();
			OutputDebugString(sNamespacePath.GetBSTR());
			OutputDebugString(L"\n");

			bool retval = cfold.Import(local_path);
			//bool retval = m_pProject->Import(local_path);
			//bool retval = m_pProject->Import(sNamespacePath);
			OutputDebugString(L"m_pProject->GetLastErrorMessage()");
			OutputDebugString(m_pProject->GetLastErrorMessage());

			return retval;
		}
	}
		return false;

		/*


//			CNxNNode *pChild = m_pProject->GetFolder(sNamespacePath);
//			CNxNNode *pChild = m_pProject->GetFolder("\\Workspace\\PGR4\\TextureToolTest\\");
			CNxNNode *pParent = m_pProject->FindNode(sNamespacePath);
			CNxNNode *pChild = pParent->GetFirstChild();
			//CNxNNode *pChild = m_pProject->GetFirstChild();

			sNxNBasicNodeInfo basicNodeInfo;

			CNxNProperty propProperty;

			while(pChild != NULL)
			{
				pChild->GetNodeInfo(&basicNodeInfo);

				if (basicNodeInfo.basicType == NXN_BASICTYPE_FOLDER)
				{
					CNxNFolder *pFolder = (CNxNFolder *)pChild;

					CString sThing((const char*)pChild->GetName());





	if(m_pProject != NULL)
	{
		//CNxNDiskItem diskItem = new CNxNDiskItem(


		bool retval = m_pProject->ImportDiskItem(local_path, true, L"TextureTagManagerAutoImport", L"", false, true, false, L"", false, false, false);
		OutputDebugString(L"m_pProject->GetLastErrorMessage()");
		OutputDebugString(m_pProject->GetLastErrorMessage());

		return retval;
	}

	return FALSE;
	*/
}

#endif


extern "C" __declspec(dllexport) BOOL ImportAB(char *local_path_file)
{
	try
	{
		if(m_pProject != NULL)
		{
			// this is a temporary variable for the namespace path.
			CNxNString sNamespacePath;

			char local_path[PATH_SIZE];

			int c;
			for(c = strlen(local_path_file); c > 0; c--)
			{
				if(local_path_file[c] == '\\')
					break;
			}
			strncpy_s(local_path, PATH_SIZE, local_path_file, c);

			// map the local file path to a corresponding namespace path
			if (m_pSmartIntegrator->MapManagedPath(local_path, sNamespacePath))
			{
				CNxNFolder *pFolder = (CNxNFolder*)m_pProject->FindTypedNode( sNamespacePath, CNxNFolder::GetType() );		// DANGER?

				if(pFolder == NULL)
				{
					pFolder = (CNxNFolder*)m_pProject->FindTypedNode( sNamespacePath, CNxNProject::GetType() );		// DANGER?
				}

				bool retval = false;
				
				if(pFolder != NULL)
				{	retval = pFolder->ImportDbNode( local_path_file, false, "Automatically imported by TextureTagManager", "TextureTagManager", false, true, false, "", false, true, false );
	//				retval = pFolder->ImportDiskItem( local_path_file, false, "Automatically imported by TextureTagManager", "TextureTagManager", false, true, false, "", false, true, false );

					//pFolder->Flush(true, true);
					pFolder->Flush(false, false);
					//m_pProject->Flush(false, true);
				}

				//bool retval = m_pProject->Import(local_path);
				//bool retval = m_pProject->Import(sNamespacePath);
				//bool retval = m_pProject->ImportDbNode(local_path);

				OutputDebugString(L"m_pProject->GetLastErrorMessage() : ");
				OutputDebugString(m_pProject->GetLastErrorMessage());

				return retval;
			}
		}
		return false;
	}
	catch(...)
	{
		return FALSE;
	}
}



extern "C" __declspec(dllexport) BOOL CheckoutAB(char *local_path_file)
{
	try
	{
		if(m_pProject != NULL)
		{
			// this is a temporary variable for the namespace path.
			CNxNString sNamespacePath;

			if (m_pSmartIntegrator->MapManagedPath(local_path_file, sNamespacePath))
			{
				//CNxNFolder *pFolder = (CNxNFolder*)m_pProject->FindTypedNode( local_path, CNxNFolder::GetType() );
				//CNxNNode *pNode = (CNxNNode*)m_pProject->FindTypedNode( local_path_file, CNxNNode::GetType() );

				//CNxNFile *pFile = (CNxNFile*)m_pProject->FindTypedNode( local_path_file, CNxNFile::GetType() );
				CNxNFile *pFile = (CNxNFile*)m_pProject->FindTypedNode( sNamespacePath, CNxNFile::GetType() );
				
				bool retval = false;
				
				if(pFile != NULL)
					//											   ||||
					retval = pFile->CheckOut(false, "", false, /**/true, false, "", false, /**/false/**/, NXN_GETLATEST_FILETIME_DEFAULT);
					//											   ||||

				OutputDebugString(L"m_pProject->GetLastErrorMessage() : ");
				OutputDebugString(m_pProject->GetLastErrorMessage());
				return retval;
			}
		}
		return false;
	}
	catch(...)
	{
		return FALSE;
	}
}


extern "C" __declspec(dllexport) BOOL CheckinAB(char *local_path_file)
{
	try
	{
		if(m_pProject != NULL)
		{
			// this is a temporary variable for the namespace path.
			CNxNString sNamespacePath;

			if (m_pSmartIntegrator->MapManagedPath(local_path_file, sNamespacePath))
			{
				//CNxNFolder *pFolder = (CNxNFolder*)m_pProject->FindTypedNode( local_path, CNxNFolder::GetType() );
				CNxNFile *pFile = (CNxNFile*)m_pProject->FindTypedNode( local_path_file, CNxNFile::GetType() );
				
				bool retval = false;
				
				if(pFile != NULL)
					//											   ||||
					retval = pFile->CheckIn(false, "", "Checked in by the Texture Tag Manager", false, false, false, false, NXN_CHECKIN_CREATE_VERSION_DEFAULT, false, false);
					//											   ||||

				OutputDebugString(L"m_pProject->GetLastErrorMessage() : ");
				OutputDebugString(m_pProject->GetLastErrorMessage());
				return retval;
			}
		}
		return false;
	}
	catch(...)
	{
		return FALSE;
	}
}











/*
    // find the typed node
    CNxNNode* pNode = pIntegrator->FindTypedNode(_STR("\\Workspace\\MyProject\\File.txt"), CNxNFile::GetType());

    // typed node found?
    if (pNode != NULL)
    {
        // typed node found, use it
        CNxNFile* pFile = (CNxNFile*)pDbNode;

        // do something...
        pFile->CheckOut();

        ...
    }
    else
    {
        // node not found or is not of correct type
        ...
    }





BOOL OpenProject(CNxNString strProject)
{
	//Assert(m_pWorkspace != NULL);

	m_pSmartIntegrator->OpenWorkspace(

	 return true;
}
*/





// End my code.






#ifdef _MANAGED
#pragma managed(pop)
#endif

