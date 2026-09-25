#include "stdafx.h"

#include "display/device/Device.h"
#include "display/context/Context_Xenon.h"
#include <system/scheduler/jobqueue.h>
#include <core/threads/AtomicMutex.h>
#include "display/context/XenonPatchVertexShader.h"

#include "btl/containers/openhashmap.h"
#include "btl/utility/pair.h"

IDirect3DVertexShader9* CreateVertexShader(LPCVOID pFunction)
{
	IDirect3DVertexShader9 *pShader;
	Display::GetD3DDevice()->CreateVertexShader((DWORD*)pFunction, &pShader);
	return pShader;
}

int GetVertexDataSize(const _D3DDECLTYPE& dataType)
{
	switch (dataType)
	{
		case D3DDECLTYPE_D3DCOLOR : return 4;
		case D3DDECLTYPE_FLOAT1: return 4;
		case D3DDECLTYPE_UBYTE4: return 4;
		case D3DDECLTYPE_SHORT2: return 4;
		case D3DDECLTYPE_SHORT4: return 8;
		case D3DDECLTYPE_FLOAT2: return 8;
		case D3DDECLTYPE_FLOAT3: return 12;
		case D3DDECLTYPE_FLOAT4: return 16;
		case D3DDECLTYPE_UDEC3: return 4;
		case D3DDECLTYPE_FLOAT16_2: return 4;
		case D3DDECLTYPE_FLOAT16_4: return 8;
		case D3DDECLTYPE_UBYTE4N: return 4;
		case D3DDECLTYPE_SHORT2N: return 4;
		case D3DDECLTYPE_SHORT4N: return 8;
		case D3DDECLTYPE_USHORT2N: return 4;
		case D3DDECLTYPE_USHORT4N: return 8;
		case D3DDECLTYPE_DEC3N: return 4;
		case D3DDECLTYPE_BYTE4: return 4;
		case D3DDECLTYPE_BYTE4N: return 4;
		case D3DDECLTYPE_HEND3N: return 4;
		case D3DDECLTYPE_UINT1: return 4;
		default: assert(0); return 0;
	};
}

void GetVertexStreamSizes(LPDIRECT3DVERTEXDECLARATION9 pVertexDeclaration, Btl::Vector<U32>& sizeVector)
{
	// get the vertex declaration data
	D3DVERTEXELEMENT9 decl[256];
	U32 numElements = 256;
	pVertexDeclaration->GetDeclaration(decl, &numElements);

	// get the number of streams
	int nStreams = 0;
	for (U32 i = 0; i < numElements - 1; i++)
		nStreams = Maths::Max(nStreams, decl[i].Stream + 1);

	sizeVector.Resize(nStreams);

	for (int i = 0; i < nStreams; i++)
		sizeVector[i] = 0;

	for (U32 i = 0; i < numElements - 1; i++)
		sizeVector[decl[i].Stream] += GetVertexDataSize((_D3DDECLTYPE)decl[i].Type);
}

LPDIRECT3DVERTEXSHADER9 CopyShader(LPDIRECT3DVERTEXSHADER9 vIn)
{
	Display2::C_VertexShader vertexShader;

	U32 functionSize;
	vIn->GetFunction(0, &functionSize);
	U8* pFunctionData = new U8[functionSize];
	vIn->GetFunction(pFunctionData, &functionSize);

	Display::Device()->m_Display2.CreateVertexShader(functionSize, (U8*)pFunctionData, Display2::E_Flags_DoNotTrack, &vertexShader);
	LPDIRECT3DVERTEXSHADER9 newShader = vertexShader.m_pd3dVertexShader;
	vertexShader.m_pd3dVertexShader = NULL;

	delete [] pFunctionData;

	return newShader;
}

namespace Btl
{
	U32 Hash(const Btl::Pair<U64, LPDIRECT3DVERTEXDECLARATION9>& v)
	{
		return Hash((U32&) v.first, Hash(v.second));
	}
}

typedef Btl::ChainedHashMap<Btl::Pair<U64, LPDIRECT3DVERTEXDECLARATION9>, LPDIRECT3DVERTEXSHADER9> VertexShaderMap;
VertexShaderMap g_patchedVertexShaders;

static MT::AtomicLock g_PatchingMutex;

LPDIRECT3DVERTEXSHADER9 PatchVertexShader(LPDIRECT3DVERTEXSHADER9 pVertexShader, LPDIRECT3DPIXELSHADER9 pPixelShader, LPDIRECT3DVERTEXDECLARATION9 pVertexDeclaration)
{
	g_PatchingMutex.Wait();
	U64 shader = (((U64)pVertexShader) << 32) + (U32)pPixelShader;
	VertexShaderMap::Iterator patchedShader = g_patchedVertexShaders.Find(Btl::MakePair(shader, pVertexDeclaration));

	if (patchedShader == g_patchedVertexShaders.End())
	{
		// patch it and insert
		patchedShader = g_patchedVertexShaders.Insert(Btl::MakePair(shader, pVertexDeclaration)).iterator;

		patchedShader->GetValue() = CopyShader(pVertexShader);

		Btl::Vector<U32> streamSizes;
		GetVertexStreamSizes(pVertexDeclaration, streamSizes);

		patchedShader->GetValue()->Bind(0, pVertexDeclaration, (const DWORD*)&streamSizes[0], pPixelShader);
	}

	LPDIRECT3DVERTEXSHADER9 pRtn = patchedShader->GetValue();
	
	g_PatchingMutex.Release();

	return pRtn;
}

void FreePatchedVertexShader(D3DVertexShader* pd3dVertexShader)
{
	g_PatchingMutex.Wait();

	VertexShaderMap::Iterator i = g_patchedVertexShaders.Begin();
	while(i != g_patchedVertexShaders.End())
	{
		if((LPDIRECT3DVERTEXSHADER9)(i->GetKey().first >> 32) == pd3dVertexShader)
		{
			VertexShaderMap::Iterator j = i;
			i++;

			Display2::C_VertexShader vertexShader;
			vertexShader.m_pd3dVertexShader = j->GetValue();
			Display::Device()->m_Display2.DestroyVertexShader(&vertexShader);

			g_patchedVertexShaders.Erase(j);
		}
		else
		{
			i++;
		}
	}

	g_PatchingMutex.Release();
}

void FreePatchedVertexShaders()
{
	g_PatchingMutex.Wait();

	Display2::C_VertexShader vertexShader;

	for (VertexShaderMap::Iterator i = g_patchedVertexShaders.Begin(); i != g_patchedVertexShaders.End(); ++i)
	{
		vertexShader.m_pd3dVertexShader = i->GetValue();
		Display::Device()->m_Display2.DestroyVertexShader(&vertexShader);
	}
	
	g_patchedVertexShaders.Clear();

	g_PatchingMutex.Release();
}
