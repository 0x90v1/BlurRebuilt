#include "stdafx.h"

#include <input\DeviceManager.h>
#include <input\Pad360.h>
#include <input\PadPS3.h>
#include <input\KeyboardPC.h>
#include <input\MousePC.h>
#ifndef CFG_GOLD
#include <input\PadVirtual.h>
#endif //CFG_GOLD

namespace Input
{

C_DeviceManager* g_pDeviceManager = NULL;

// C_DeviceManager

C_DeviceManager::C_DeviceManager( const C_DeviceManagerCreateParam& param, const S32 userIndexCount )
	: m_UserIndexCount( userIndexCount )
{
	assert( g_pDeviceManager == NULL );
	g_pDeviceManager = this;
}

C_DeviceManager::~C_DeviceManager()
{
	assert((m_DeviceList.Size() == 0) && "Device list not empty");
	
	assert(g_pDeviceManager != NULL);
	g_pDeviceManager = NULL;
}
	
C_DeviceManager* C_DeviceManager::GetInstance()
{
	return g_pDeviceManager;
}

void C_DeviceManager::Update()
{
	PlatformUpdate();

	const S32 deviceCount = m_DeviceList.Size();
	for(S32 i=0; i<deviceCount; i++)
	{
		m_DeviceList[i]->Update();
	}
}

S32 C_DeviceManager::GetUserIndexCount()
{
	return m_UserIndexCount;
}

void C_DeviceManager::CreateDevice( const S32 userIndex, const E_Device deviceType, C_Device** ppDevice )
{
	assert(( userIndex >= 0 ) && ( userIndex < m_UserIndexCount));

	C_Device* pNewDevice = NULL;
	switch(deviceType)
	{
		default:
		break;

		#if defined(PLATFORM_PC) || defined(PLATFORM_XENON)
		case E_Device_Pad360:
			pNewDevice = (C_Device*)new C_Pad360( this, userIndex );
		break;
		#ifndef CFG_GOLD
		case E_Device_PadVirtual360:
			pNewDevice = (C_Device*)new C_PadVirtual360( this, userIndex );
		break;
		#endif //CFG_GOLD
		#endif

		#if defined(PLATFORM_PS3)
		case E_Device_PadPS3:
			pNewDevice = (C_Device*)new C_PadPS3( this, userIndex );
		break;
		#ifndef CFG_GOLD
		case E_Device_PadVirtualPS3:
			pNewDevice = (C_Device*)new C_PadVirtualPS3( this, userIndex );
		break;
		#endif //CFG_GOLD
		#endif

		#if defined(PLATFORM_PC)
		case E_Device_Keyboard:
			pNewDevice = (C_Device*)new C_KeyboardPC( this, userIndex );
		break;
		
		case E_Device_Mouse:
			pNewDevice = new C_MousePC( this, userIndex );
		break;
		#endif
	}

	*ppDevice = pNewDevice;

	if(!pNewDevice)
	{
		LOGWARN(g_Log, "Input::C_DeviceManager Device not created\n");
		return;
	}

	// add pointer to list of devices
	m_DeviceList.PushBack(pNewDevice);
}

void C_DeviceManager::RemoveDevice( C_Device* pDevice )
{
	// remove pointer from list of devices
	const S32 deviceCount = m_DeviceList.Size();
	for(S32 i=0; i<deviceCount; i++)
	{
		if(m_DeviceList[i] == pDevice)
		{
			m_DeviceList.EraseByIndex(i);
			break;
		}
	}
}

C_Device* C_DeviceManager::GetDevice( const S32 deviceNo ) const
{
	return m_DeviceList[deviceNo];
}

void C_DeviceManager::EnableAllDevices()
{
	const S32 deviceCount = m_DeviceList.Size();
	for(S32 i=0; i<deviceCount; i++)
	{
		m_DeviceList[i]->Enable();
	}
}

void C_DeviceManager::DisableAllDevices()
{
	const S32 deviceCount = m_DeviceList.Size();
	for(S32 i=0; i<deviceCount; i++)
	{
		m_DeviceList[i]->Disable();
	}
}

void DestroyDeviceManager( C_DeviceManager* pDeviceManger )
{
	C_DeviceManager* pDeviceManager = GetDeviceManager();
	assert(pDeviceManager);
	SAFE_DELETE_PTR(pDeviceManager); 
}

C_DeviceManager* GetDeviceManager()
{
	return C_DeviceManager::GetInstance();
}

} // namespace Input


