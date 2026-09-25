#ifndef DISPLAY_DEVICE_D3D9_D3D9_DEVICESTATES_H
#define DISPLAY_DEVICE_D3D9_D3D9_DEVICESTATES_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Device/DeviceTypes.h"

namespace Display
{
namespace D3D9
{

struct C_DeviceStates
{
	void Get(IDirect3DDevice9* pDevice);
	void Set(IDirect3DDevice9* pDevice);

	struct C_Sampler
	{
		IDirect3DBaseTexture9*		m_pTexture;
		DWORD						m_TextureAddressU;
		DWORD						m_TextureAddressV;
		DWORD						m_TextureAddressW;
		DWORD						m_TextureFilterMag;
		DWORD						m_TextureFilterMin;
		DWORD						m_TextureFilterMip;
		DWORD						m_TextureBorderColour;
	};
	
	struct C_Stream
	{
		IDirect3DVertexBuffer9*		m_pVertexBuffer;
		UINT						m_VertexOffset;
		UINT						m_VertexStride;
	};

	DWORD							m_AlphaTestEnable;
	DWORD							m_AlphaTestFunc;
	DWORD							m_AlphaTestRef;
	DWORD							m_AlphaToMaskEnable;
	DWORD							m_BlendEnable;
	DWORD							m_BlendSrc;
	DWORD							m_BlendDst;
	DWORD							m_BlendOp;
	DWORD							m_BlendSrcAlpha;
	DWORD							m_BlendDstAlpha;
	DWORD							m_BlendOpAlpha;
	DWORD							m_ColourWriteEnable;
	DWORD							m_CullMode;
	DWORD							m_DepthBias;
	DWORD							m_DepthEnable;
	DWORD							m_DepthFunc;
	DWORD							m_DepthWriteEnable;
	DWORD							m_FillMode;
	
	D3DVIEWPORT9					m_Viewport;
	IDirect3DVertexDeclaration9*	m_pVertexDeclaration;
	C_Stream						m_Streams[E_Stream_Count];
	IDirect3DIndexBuffer9*			m_pIndexBuffer;
	IDirect3DVertexShader9*			m_pVertexShader;
	IDirect3DPixelShader9*			m_pPixelShader;
	C_Sampler						m_Samplers[E_Sampler_Count];

	BOOL							m_VertexShaderConstantB[128];
	BOOL							m_PixelShaderConstantB[128];
	int								m_VertexShaderConstantI[16 * 4];
	int								m_PixelShaderConstantI[16 * 4];
	float							m_VertexShaderConstantF[256 * 4];
	float							m_PixelShaderConstantF[256 * 4];
};

}; // namespace D3D9
}; // namespace Display

#endif // DISPLAY_DEVICE_D3D9_D3D9_DEVICESTATES_H
