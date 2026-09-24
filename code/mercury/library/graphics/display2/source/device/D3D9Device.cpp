#include "StdAfx.h"
#include "Display2/Device/D3D9Device.h"

namespace Display2
{

// C_D3D9DeviceParams

C_D3D9DeviceParams::C_D3D9DeviceParams()
{
	m_pd3dDevice				= NULL;
	m_pd3dObject				= NULL;
}

C_D3D9DeviceParams::~C_D3D9DeviceParams()
{
}

// C_D3D9Device

C_D3D9Device::C_D3D9Device()
{
	m_Created					= 0;

	m_LastError					= GPU_ERROR_SUCCESS;

	m_AllowOutOfMemory			= false;

	m_pd3dDevice				= NULL;
	m_pd3dObject				= NULL;
}

C_D3D9Device::~C_D3D9Device()
{
}

U32 C_D3D9Device::GetLastError() const
{
	return m_LastError;
}

void C_D3D9Device::SetLastError(const U32 error)
{
	m_LastError = error;
}

void C_D3D9Device::AllowOutOfMemory(const bool allowOutOfMemory)
{
	m_AllowOutOfMemory = allowOutOfMemory;
}

C_VertexDeclaration* C_D3D9Device::FindVertexDeclaration(const S32 vertexElementCount, const C_VertexElement* pVertexElements) const
{
	BIZ_ASSERT(vertexElementCount > 0);
	BIZ_ASSERT(vertexElementCount <= C_VertexDeclaration::m_MaxElementCount); // If you hit this increase m_MaxElementCount
	BIZ_ASSERT(pVertexElements);

	for(S32 i=0; i<m_VertexDeclarationList.Size(); i++)
	{
		C_VertexDeclaration* pOtherVertexDeclaration = m_VertexDeclarationList[i];

		if(vertexElementCount != pOtherVertexDeclaration->GetElementCount())
			continue;

		static C_VertexElement pOtherVertexElements[C_VertexDeclaration::m_MaxElementCount+1]; // Make this static to save a bunch of allocs every search

		if(pOtherVertexDeclaration->GetElements(vertexElementCount, pOtherVertexElements))
		{
			bool equal = true;

			for(S32 j=0; j<vertexElementCount; j++)
			{
				const C_VertexElement& vertexElement = pVertexElements[j];
				const C_VertexElement& otherVertexElement = pOtherVertexElements[j];

				if(vertexElement != otherVertexElement)
				{
					equal = false;
					break;
				}
			}

			if(equal)
			{
				return pOtherVertexDeclaration;
			}
		}
	}

	return NULL;
}

C_VertexDeclaration* C_D3D9Device::CreateVertexDeclaration(const C_VertexFormat& vertexFormat)
{
	return CreateVertexDeclaration(vertexFormat.m_Elements);
}

bool C_D3D9Device::CreateVertexDeclaration(const C_VertexFormat& vertexFormat, C_VertexDeclaration* pVertexDeclaration)
{
	return CreateVertexDeclaration(vertexFormat.m_Elements, pVertexDeclaration);
}

C_VertexDeclaration* C_D3D9Device::CreateVertexDeclaration(const C_VertexElement* pVertexElements)
{
	m_Mutex.Wait();

	U32 vertexElementCount = 0;

	while(pVertexElements[vertexElementCount].GetStream() != UCHAR_MAX)
	{
		vertexElementCount++;

		BIZ_ASSERT(vertexElementCount <= C_VertexDeclaration::m_MaxElementCount);
	}

	C_VertexDeclaration* pVertexDeclaration = FindVertexDeclaration(vertexElementCount, pVertexElements);

	if(pVertexDeclaration)
	{
		m_Mutex.Release();

		return pVertexDeclaration;
	}
	else
	{
		pVertexDeclaration = new C_VertexDeclaration();

		if(CreateVertexDeclaration(pVertexElements, pVertexDeclaration))
		{
			m_VertexDeclarationList.PushBack(pVertexDeclaration);

			m_Mutex.Release();

			return pVertexDeclaration;
		}
		else
		{
			SAFE_DELETE_PTR(pVertexDeclaration);

			m_Mutex.Release();

			return NULL;
		}
	}
}

bool C_D3D9Device::CreateVertexDeclaration(const C_VertexElement* pVertexElements, C_VertexDeclaration* pVertexDeclaration)
{
	BIZ_ASSERT(pVertexDeclaration);
	BIZ_ASSERT(!pVertexDeclaration->IsCreated());

	BIZ_ASSERT(m_pd3dDevice);
	HRESULT hr = m_pd3dDevice->CreateVertexDeclaration(pVertexElements, &pVertexDeclaration->m_pd3dVertexDeclaration);
	if(FAILED(hr))
	{
		LOGERROR(g_Log, "Failed to create vertex declaration\n");
		return false;
	}

	return true;
}

void C_D3D9Device::DestroyVertexDeclaration(C_VertexDeclaration* pVertexDeclaration)
{
	BIZ_ASSERT(pVertexDeclaration);
	if(pVertexDeclaration->IsCreated())
	{
		const ULONG refCount = pVertexDeclaration->m_pd3dVertexDeclaration->Release();
		BIZ_ASSERT(refCount == 0);
		pVertexDeclaration->m_pd3dVertexDeclaration = NULL;
	}
}

void C_D3D9Device::DestroyVertexDeclarations()
{
	m_Mutex.Wait();

	for(S32 i=0; i<m_VertexDeclarationList.Size(); i++)
	{
		DestroyVertexDeclaration(m_VertexDeclarationList[i]);
		SAFE_DELETE_PTR(m_VertexDeclarationList[i]);
	}
	m_VertexDeclarationList.Clear();

	m_Mutex.Release();
}

// C_D3D9DeviceManagerParams

C_D3D9DeviceManagerParams::C_D3D9DeviceManagerParams()
{
}

C_D3D9DeviceManagerParams::~C_D3D9DeviceManagerParams()
{
}

// C_D3D9DeviceManager

C_D3D9DeviceManager::C_D3D9DeviceManager()
{
	m_pd3dObject = NULL;
}

C_D3D9DeviceManager::~C_D3D9DeviceManager()
{
}

bool C_D3D9DeviceManager::CreateObject()
{
	if(!m_pd3dObject)
	{
		m_pd3dObject = Direct3DCreate9(D3D_SDK_VERSION);	
		if(!m_pd3dObject)
		{
			LOGERROR(g_Log, "Failed to create d3d9\n");
			return false;
		}
	}

	return true;
}

void C_D3D9DeviceManager::DestroyObject()
{
	if(m_pd3dObject)
	{
		const ULONG refCount = m_pd3dObject->Release();
		#if DEVICE_D3D9_PC
		BIZ_ASSERT(refCount == 0);
		#endif
		m_pd3dObject = NULL;
	}
}

} // namespace Display2
