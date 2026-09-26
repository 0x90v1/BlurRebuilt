#include "stdafx.h"

#include <Game\dlc\DLC_Controller.h>

#include <ui\UiManager.h>
#include <ui\UiInterface.h>

#include <GameApp.h>
#include <GameLogic/GameData/GameData.h>
#include <GameLogic/GameData/MultiplayerData.h>
#include <GameLogic/GameData/GameVehicleData.h>

#include <GameLogic/Profile/GameProfile.h>
#include <GameLogic/Profile/GameProfileManager.h>

Debug::Logger &g_DLCLogger( Debug::GetLogger("DLC.Controller") );
Debug::Bool g_DebugLicenseAllDLC( "DLC.LicenseAllDLC", false, Debug::Persist() );
Debug::Int g_DebugNumItemsToEnumerate( "DLC.NumItemsToEnumerate", 10, Debug::Persist() );

namespace UI
{
	DLCController DLCController::s_instance;

	void DLCController::ThreadMain(void)
	{
		ClearDLC();
		Enumerate();
		LoadAndLicenseContent();

		// Re-cache the MP data now we have the DLC loaded, as it may have changed.
		C_MultiplayerData::Instance().PreCacheData();

		Game::C_GameProfile* pProf = Game::C_GameProfileManager::Instance().GetPrimaryProfile();
		if(pProf)
			pProf->CacheDLCData();

#ifdef COMPILER_VC
		Close();
#endif
	}

	// ------------------------------------------------------------------------------------------------------------------------------------

	void DLCController::ClearDLC()
	{
		C_GameData::Instance().mDLCPacks.DestroyAll();
		mFilesToLoad.Destroy();

		for( int i = 0; i < mPacksOpen.Size(); i++ )
			XtPackClose(C_GameApp::GetInstance()->GetGlobalXtDatabase(), mPacksOpen[i]);

		mPacksOpen.Clear();

		// Cheat: I want all the DLC in the world ever. ALL OF IT.
		if(g_DebugLicenseAllDLC)
			mLicense = 0xffffffff;
		else
			mLicense = 0x00000000;

		// Close all our existing locations
		for(Btl::Set<U32>::Iterator it = mDLCPacksInstalled.Begin(); it != mDLCPacksInstalled.End(); it++)
		{
			Util::C_String32 location;
			location.Printf("DLC%u", *it);
			XContentClose(location.Chars(), NULL);
		}
		mDLCPacksInstalled.Clear();

		mCorruptFiles.Clear();
		mCorruptLocIds.Clear();
	}

	// ------------------------------------------------------------------------------------------------------------------------------------

	void DLCController::LoadAndLicenseContent()
	{
		for( int i = mFilesToLoad.Size() - 1; i >= 0; i-- )
		{
			// This should use the new file system stuff eventually.
			Sys::C_FileSystem& fs = Sys::C_FileSystem::Instance();
			Sys::I_File* pFile = fs.Open(mFilesToLoad[i].Chars(), Sys::E_FileAccess_Read);

			if(pFile)
			{
				const U32 uBufferSize = (U32)pFile->Length();

				// Read and process
				U8* pBuffer = new U8[ uBufferSize ];	// use fixed data size
				memset( pBuffer, 0xfd, uBufferSize );	// debug clear

				bool success = pFile->Read( pBuffer, uBufferSize );

				if(success)
				{
					XtPack* xtPack = XtPackOpen(C_GameApp::GetInstance()->GetGlobalXtDatabase(), pBuffer, uBufferSize);

					if(xtPack)
					{
						bool licensedSomeFiles = false;
						mPacksOpen.PushBack(xtPack);

						int num = C_GameData::Instance().mCheatPacks.Number();

						for( U32 k = 0; k < xtPack->count; k++ )
						{
							if(xtPack->records[k].record->strongRef)
							{
								// We recursively search for the base class.
								for( U32 j = 0; j < xtPack->records[k].record->type->baseCount; j++ )
								{
									if(IsGameDataObject(xtPack->records[k].record->type->bases[j]))
									{
										XtHandle<AmaxDLCBase> objHandle(xtPack->records[k].record);

										if((!objHandle->dlcPack || !objHandle->dlcLicenseBit) && mLicense != 0 ) // Always available.
											objHandle->dlcLicensed = true;
										else if (objHandle->dlcLicenseBit > 0) // Bit is valid.
											objHandle->dlcLicensed = mLicense&(1<<(objHandle->dlcLicenseBit - 1)) ? true : false; 
										else
											objHandle->dlcLicensed = false;

										if(objHandle->dlcLicensed)
											licensedSomeFiles = true;

										break;
									}
								}
							}
						}

						if(licensedSomeFiles)
						{
							mCorruptFiles.Erase(mCorruptFiles.Begin() + i);
							mCorruptLocIds.Erase(mCorruptLocIds.Begin() + i);
						}
						else
						{
							mCorruptLocIds[i] = UI::C_TextStringID("POP_DLC_UNLICENSED").Value();
						}
					}
				}

				SAFE_DELETE_ARY( pBuffer );
			}

			SAFE_DELETE_PTR( pFile );
		}
	}

	// ------------------------------------------------------------------------------------------------------------------------------------

	void DLCController::Enumerate()
	{
		mEnumerationHandle = INVALID_HANDLE_VALUE;
		mEnumeratedDataSizeRequired = 0;
		mpEnumeratedData = NULL;

		DWORD res = XContentCreateEnumerator(XUSER_INDEX_NONE, XCONTENTDEVICE_ANY, XCONTENTTYPE_MARKETPLACE, 0 , g_DebugNumItemsToEnumerate, &mEnumeratedDataSizeRequired, &mEnumerationHandle);
		if (res == ERROR_SUCCESS && mEnumeratedDataSizeRequired && mEnumerationHandle != INVALID_HANDLE_VALUE)
		{
			mpEnumeratedData = new U8[mEnumeratedDataSizeRequired];

			if (mpEnumeratedData != NULL)
			{
				DWORD packsEnumerated = 0;
				res = XEnumerate(mEnumerationHandle, mpEnumeratedData, mEnumeratedDataSizeRequired, &packsEnumerated, NULL);

				if (res == ERROR_SUCCESS)
				{
					for (DWORD i = 0; i < packsEnumerated; i++)
					{
						XCONTENT_DATA *pContent = &((XCONTENT_DATA*)mpEnumeratedData)[i];

						if(pContent)
						{
							DWORD license = 0;

							mCorruptFiles.PushBack(UI::C_WideString128(pContent->szDisplayName));
							mCorruptLocIds.PushBack(UI::C_TextStringID("POP_DLC_CORRUPT").Value());

							res = XContentCreate(XUSER_INDEX_NONE, "DLC", pContent, XCONTENTFLAG_OPENEXISTING, NULL, &license, NULL); 

							if (res == ERROR_SUCCESS)
							{
								// This should use the new file system stuff eventually.
								Sys::C_FileSystem& fs = Sys::C_FileSystem::Instance();
								Sys::I_File* pFile = fs.Open("DLC:\\ToC.bin", Sys::E_FileAccess_Read);

								if(pFile)
								{
									const U32 uBufferSize = (U32)pFile->Length();

									// Read and process
									U8* pBuffer = new U8[ uBufferSize ];	// use fixed data size
									memset( pBuffer, 0xfd, uBufferSize );	// debug clear

									bool success = pFile->Read( pBuffer, uBufferSize );

									if(success)
									{
										// This will automatically load the pack into C_GameData::mDLCPacks via a callback
										XtPack* xtPack = XtPackOpen(C_GameApp::GetInstance()->GetGlobalXtDatabase(), pBuffer, uBufferSize);

										if(xtPack && xtPack->records && xtPack->records->record)
										{
											AmaxDLCPack* dlcPack = (AmaxDLCPack*)xtPack->records->record->object;

											if(dlcPack)
											{
												Btl::Set<U32>::InsertionReturnType returnType = mDLCPacksInstalled.Insert(dlcPack->mPackID);
												mPacksOpen.PushBack(xtPack);

												mLicense |= license;

												if(returnType.inserted)
												{
													Util::C_String32 driveName;
													driveName.Printf("DLC%u", dlcPack->mPackID);

													res = XContentCreate(XUSER_INDEX_NONE, driveName.Chars(), pContent, XCONTENTFLAG_OPENEXISTING, NULL, &license, NULL);

													if (res == ERROR_SUCCESS)
													{
														for( int j = 0; j < dlcPack->mLoadableItems.count; j++ )
														{
															mFilesToLoad.PushBack();
															mFilesToLoad[mFilesToLoad.Size()-1].Printf("%s:\\%s",driveName.Chars(), dlcPack->mLoadableItems[j].Text());
														}	
													}
												}
											}
										}
									}
									SAFE_DELETE_ARY( pBuffer );
								}	
								SAFE_DELETE_PTR( pFile );
							}
							XContentClose("DLC", NULL);
						}
					}
				}
				delete [] mpEnumeratedData;
			}
		}
	}
}