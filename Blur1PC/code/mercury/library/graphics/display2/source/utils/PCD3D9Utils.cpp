#include "StdAfx.h"
#include "Display2/Utils/Utils.h"
#include "Display2/Device/Device.h"

namespace Display2
{

Util::C_String g_VertexShaderFileExt = "pcvsh";

Util::C_String g_PixelShaderFileExt = "pcpsh";

bool D3DXCreateTexture2DFromFile(const Util::C_String& fileName, C_Texture* pTexture)
{
	HRESULT hr = E_FAIL;

	assert(pTexture);

	if(pTexture->IsCreated())
	{
		LOGERROR(g_Log, "Failed to create the 2d texture as its already created %s\n", fileName.Chars());
		return false;
	}

	U32 fileSize = 0;
	U8* pFileData = NULL;
	if(!GetFileData(fileName, fileSize, &pFileData))
	{
		LOGERROR(g_Log, "Failed to open file %s\n", fileName.Chars());
		return false;
	}

	IDirect3DTexture9* pd3dTexture = NULL;
	hr = D3DXCreateTextureFromFileInMemory(Device()->GetD3DDevice(), pFileData, fileSize, &pd3dTexture);

	SAFE_DELETE_ARY(pFileData);

	if(FAILURE(hr))
	{
		LOGERROR(g_Log, "Failed to create the 2d texture from file in memory %s\n", fileName.Chars());
		return false;
	}

	pTexture->m_pd3dTexture = pd3dTexture;

	return true;
}

bool D3DXCreateTexture3DFromFile(const Util::C_String& fileName, C_Texture* pTexture)
{
	HRESULT hr = E_FAIL;

	assert(pTexture);

	if(pTexture->IsCreated())
	{
		LOGERROR(g_Log, "Failed to create the 3d texture as its already created %s\n", fileName.Chars());
		return false;
	}

	U32 fileSize = 0;
	U8* pFileData = NULL;
	if(!GetFileData(fileName, fileSize, &pFileData))
	{
		LOGERROR(g_Log, "Failed to open file %s\n", fileName.Chars());
		return false;
	}

	IDirect3DVolumeTexture9* pd3dTexture = NULL;
	hr = D3DXCreateVolumeTextureFromFileInMemory(Device()->GetD3DDevice(), pFileData, fileSize, &pd3dTexture);

	SAFE_DELETE_ARY(pFileData);

	if(FAILURE(hr))
	{
		LOGERROR(g_Log, "Failed to create the 3d texture from file in memory %s\n", fileName.Chars());
		return false;
	}

	pTexture->m_pd3dTexture = pd3dTexture;

	return true;
}

bool D3DXCreateTextureCubeFromFile(const Util::C_String& fileName, C_Texture* pTexture)
{
	HRESULT hr = E_FAIL;

	assert(pTexture);

	if(pTexture->IsCreated())
	{
		LOGERROR(g_Log, "Failed to create the cube texture as its already created %s\n", fileName.Chars());
		return false;
	}

	U32 fileSize = 0;
	U8* pFileData = NULL;
	if(!GetFileData(fileName, fileSize, &pFileData))
	{
		LOGERROR(g_Log, "Failed to open file %s\n", fileName.Chars());
		return false;
	}

	IDirect3DCubeTexture9* pd3dTexture = NULL;
	hr = D3DXCreateCubeTextureFromFileInMemory(Device()->GetD3DDevice(), pFileData, fileSize, &pd3dTexture);

	SAFE_DELETE_ARY(pFileData);

	if(FAILURE(hr))
	{
		LOGERROR(g_Log, "Failed to create the cube texture from file in memory %s\n", fileName.Chars());
		return false;
	}

	pTexture->m_pd3dTexture = pd3dTexture;

	return true;
}

} // namespace Display2
