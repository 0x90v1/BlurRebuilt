#include "stdafx.h"

#include "display/device/Device.h"
#include "display/device/DeviceContext.h"
#include "display/context/Context_Xenon.h"
#include <system/scheduler/jobqueue.h>
#include "display/context/XenonPatchVertexShader.h"
#include "display/shaders/psConstants.h"
#include "display/shaders/vsConstants.h"
#include "display/buffer/CommandBuffer.h"
#include "Display/memory/d3d9//D3D9RingBuffer.h"

EXTERN_SHADER(vsBasicRenderColoured3D);
EXTERN_SHADER(psBasicRenderColoured);

namespace Display
{

DEFINE_CONTEXT(Context, C_Context);

Debug::Bool g_PatchVertexShaders("Graphics.Display.Context.PatchVertexShaders", true);
Debug::Bool g_SetStateCommandBufferStart("Graphics.Display.Context.SetStateCommandBufferStart", true);
Debug::Bool g_DrawDummyPrimitiveCommandBufferStart("Graphics.Display.Context.DrawDummyPrimitiveCommandBufferStart", false);
Debug::Bool g_DrawDummyPrimitiveCommandBufferEnd("Graphics.Display.Context.DrawDummyPrimitiveCommandBufferEnd", false);
Debug::Bool g_DrawDummyPrimitiveBeforeRunningCommandBuffer("Graphics.Display.Context.DrawDummyPrimitiveBeforeRunningCommandBuffer", true);
Debug::Int g_AlphaToMaskOffsets("Graphics.Display.Context.AlphaToMaskOffsets", -1, Debug::Range(-1, 0xff));

Debug::Bool g_UseSharedContextFunctions("Graphics.Display.Context.UseSharedContextFunctions", false);

void C_SamplerState::SetMaxAnisotropy(S8 x)
{
	m_MaxAnisotropy = x;
}

/// constructor
C_Context::C_Context()
: m_pD3DDevice(NULL)
{
#if ASSERT_ON_VALID_INTREFCOUNT
	for(U32 i=0; i<E_Sampler_Count; i++)
		m_pTextures[i] = NULL;

	for(U32 i=0; i<E_VertexSampler_Count; i++)
		m_pVertexTextures[i] = NULL;
#endif
}

/// destructor
C_Context::~C_Context()
{
	if (m_pD3DDevice)
	{
		m_pD3DDevice->AcquireThreadOwnership();
		m_pD3DDevice->Release();
	}
}

/// one-time init. not needed for contexts that just hold state. only needed for contexts that perform actual rendering
void C_Context::Initialise()
{
	Display::Device()->AcquireThreadOwnership();
	// not dependant on any other contexts
	m_Flags = m_Id;

	g_ContextManager.Initialise();

	if (m_pD3DDevice == NULL)
	{
		Display::Device()->GetD3DObject()->CreateDevice(0, D3DDEVTYPE_COMMAND_BUFFER, NULL, 0, NULL, &m_pD3DDevice );

		// HACK: temporarily making the main device use the command buffer device so we can reset the states on it
		// TODO: InvalidateState and DefaultStates should be called on the C_Context
		// Then we probably wouldn't need the AcquireThreadOwnership and ReleaseThreadOwnership on the main device in this function
		IDirect3DDevice9 *pTmp = Display::Device()->GetD3DDevice();
		Display::Device()->m_pd3dDevice = m_pD3DDevice;
		Display::Device()->InvalidateState();
		Display::Device()->DefaultStates();
		Display::Device()->m_pd3dDevice = pTmp;

		m_pD3DDevice->ReleaseThreadOwnership();
	}

	Display::Device()->ReleaseThreadOwnership();
}

/// clone the state of another context into this context
void C_Context::CloneState(const I_Context *pContext)
{
	PERF_EVENT("C_Context::CloneState");
	if (pContext == NULL)
	{
		m_pDevice = Display::Device();
		m_CullMode = m_pDevice->GetCullModeDefault();
	}
	else
	{
		Display::C_Context *pDisplay = Display::GetContext(pContext);
		m_pDevice = pDisplay->m_pDevice;
		m_CullMode = pDisplay->m_CullMode;
		m_WindowIndex = pDisplay->m_WindowIndex;
		m_pColourSurface = pDisplay->m_pColourSurface;
		m_pDepthStencilSurface = pDisplay->m_pDepthStencilSurface;
		m_pColourTexture = pDisplay->m_pColourTexture;
		m_pDepthStencilTexture = pDisplay->m_pDepthStencilTexture;
		XMemCpy(m_pView, pDisplay->m_pView, sizeof(Display::C_View));
	}
}

/// invalidate the state of this context
void C_Context::InvalidateState()
{

}

/// begin rendering to the comand buffer
void C_Context::BeginCommandBuffer(Display::C_CommandBuffer *pCommandBuffer)
{
	if (pCommandBuffer)
	{
		PERF_EVENT("Display.BeginCommandBuffer");

		Display::RingBuffer()->CreateCommandBuffer(pCommandBuffer);

		D3DTAGCOLLECTION tags;
		D3DTagCollection_SetAll(&tags);

		D3DRECT* pSrcRect = NULL;
		DWORD tileCount = 0;
		DWORD flags = D3DBEGINCB_OVERWRITE_INHERITED_STATE | D3DBEGINCB_RECORD_ALL_SET_STATE;

		m_bMultiThreaded = true;
		m_pD3DCBDevice = m_pD3DDevice;

		m_pD3DCBDevice->AcquireThreadOwnership();

		if(m_WindowIndex != -1)
		{
			C_Window* pWindow = m_pDevice->GetWindow(m_WindowIndex);
			pSrcRect = &pWindow->m_TileSetup.m_Tiles[0];
			tileCount = pWindow->m_TileSetup.m_TileCount;
			flags |= D3DBEGINCB_TILING_PREDICATE_COMPONENTS;
		}
		
		if(m_pColourSurface)
		{
			m_pD3DCBDevice->SetRenderTarget(0, (IDirect3DSurface9*)&m_pColourSurface->m_d3dSurface);
			m_pD3DCBDevice->SetDepthStencilSurface((IDirect3DSurface9*)&m_pDepthStencilSurface->m_d3dSurface);
		}
		else
		{
			m_pD3DCBDevice->SetRenderTarget(0, NULL);
			m_pD3DCBDevice->SetDepthStencilSurface(g_ContextManager.GetDepthStencilSurface());
		}
		
		if(g_SetStateCommandBufferStart)
		{
			m_pD3DCBDevice->SetRenderState(D3DRS_HIZENABLE, D3DHIZ_ENABLE);
			m_pD3DCBDevice->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, FALSE);
			m_pD3DCBDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			m_pD3DCBDevice->SetRenderState(D3DRS_ALPHATOMASKENABLE, FALSE);
			m_pD3DCBDevice->SetRenderState(D3DRS_PRIMITIVERESETENABLE, TRUE);
			m_pD3DCBDevice->SetRenderState(D3DRS_PRIMITIVERESETINDEX, 0xFFFF);
			m_pD3DCBDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
		}

		m_pD3DCBDevice->BeginCommandBuffer(pCommandBuffer->GetD3DCommandBuffer(), flags, &tags, &tags, pSrcRect, tileCount);
		
		if(g_DrawDummyPrimitiveCommandBufferStart)
			DrawDummyPrimitive();

		m_pCommandBuffer = pCommandBuffer;
	}
	else
	{
		m_bMultiThreaded = false;
		m_pD3DCBDevice = Display::Device()->GetD3DDevice();
		m_pCommandBuffer = NULL;
	}

	m_Context.m_pd3dDevice = m_pD3DCBDevice;

	m_pVtxDecl = NULL;
	m_bVertexShaderChanged = false;
	m_pVtxDecl = NULL;
	m_pVertexShader = NULL;
	m_bEmptyCommandBuffer = true;

	m_Context.SetD3DDevice(m_pD3DCBDevice);

#if ASSERT_ON_VALID_INTREFCOUNT
	for(U32 i=0; i<E_Sampler_Count; i++)
		m_pTextures[i] = NULL;
	
	for(U32 i=0; i<E_VertexSampler_Count; i++)
		m_pVertexTextures[i] = NULL;
#endif
}

/// stop rendering to the command buffer
void C_Context::EndCommandBuffer()
{
	PERF_EVENT("Display.EndCommandBuffer");

#if ASSERT_ON_VALID_INTREFCOUNT
	for(U32 i=0; i<E_Sampler_Count; i++)
	{
		SetTexture(i, NULL);
		m_pTextures[i] = NULL;
	}

	for(U32 i=0; i<E_VertexSampler_Count; i++)
	{
		SetVertexTexture(i, NULL);
		m_pVertexTextures[i] = NULL;
	}
#endif

	if (m_bMultiThreaded)
	{
		// we need to draw a dummy primitive here or we can get gpu hangs etc
		if(m_pPixelShader == 0 || g_DrawDummyPrimitiveCommandBufferEnd)
			DrawDummyPrimitive();

		//Unbind(); // Unbind the resources from the device. If we don't do this we get d3d asserts firing in a Debug build.
		
		m_pD3DCBDevice->EndCommandBuffer();
		m_pD3DCBDevice->ReleaseThreadOwnership();
		if (m_bEmptyCommandBuffer && m_pCommandBuffer)
			m_pCommandBuffer->SetEmptyCommandBuffer(true);
	}
	else
	{
		Unbind(); // Unbind the resources from the device. If we don't do this we get d3d asserts firing in a Debug build.
	}

	m_Context.SetD3DDevice(NULL);
}

void C_Context::Unbind()
{
	BIZ_ASSERT(m_pD3DCBDevice);

	m_pD3DCBDevice->SetVertexShader(NULL);
	m_pD3DCBDevice->SetPixelShader(NULL);
	m_pD3DCBDevice->SetVertexDeclaration(NULL);
	m_pD3DCBDevice->SetIndices(NULL);

	for(U32 stream=0; stream<E_Stream_Count; stream++)
		m_pD3DCBDevice->SetStreamSource(stream, NULL, 0, 0);
	for(U32 sampler=0; sampler<E_Sampler_Count; sampler++)
		m_pD3DCBDevice->SetTexture(sampler, NULL);
	for(U32 vertexSampler = 0; vertexSampler<E_VertexSampler_Count; vertexSampler++)
		m_pD3DCBDevice->SetTexture(D3DVERTEXTEXTURESAMPLER0+vertexSampler, NULL);
}

/// draw a dummy primitive to force states to get set
void C_Context::DrawDummyPrimitive()
{
	m_pD3DCBDevice->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, FALSE);
	m_pD3DCBDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

	m_pD3DCBDevice->SetStreamSource(0, g_ContextManager.m_pVertexBuffer, 0, 0);
	m_pD3DCBDevice->SetIndices(g_ContextManager.m_pIndexBuffer);
	
	m_pD3DCBDevice->SetVertexDeclaration(NULL);
	Display::C_Matrix identityMatrix;
	Display::MatrixSetIdentity(&identityMatrix);
	Display::MatrixSetTranslation(&identityMatrix, 0.f, -10000.f, 0.f);
	m_pD3DCBDevice->SetVertexShaderConstantF(0, (F32*)&identityMatrix, 4);
	m_pD3DCBDevice->SetVertexShader(g_ContextManager.m_VertexShader.m_pd3dVertexShader);
	m_pD3DCBDevice->SetPixelShader(g_ContextManager.m_PixelShader.m_pd3dPixelShader);

	m_pD3DCBDevice->DrawIndexedVertices(D3DPT_POINTLIST, 0, 0, 1);
}

/// set vertex declaration
void C_Context::SetVertexDeclaration(const Display2::C_VertexDeclaration* pVertexDeclaration)
{
	m_pVtxDecl = pVertexDeclaration->m_pd3dVertexDeclaration;
	m_bVertexShaderChanged = true;
}

/// set vertex buffer
void C_Context::SetVertexBuffer(const U32 stream, const Display2::C_VertexBuffer* pVertexBuffer, const U32 offset, const U32 stride)
{
#if 0
	m_Context.SetVertexBuffer(stream, pVertexBuffer, offset, g_PatchVertexShaders ? 0 : stride);
#else
	m_pD3DCBDevice->SetStreamSource(stream, pVertexBuffer?(const_cast<D3DVertexBuffer*>(&pVertexBuffer->m_d3dVertexBuffer)):NULL, offset, g_PatchVertexShaders ? 0 : stride);
#endif
}

/// set index buffer
void C_Context::SetIndexBuffer(const Display2::C_IndexBuffer* pIndexBuffer)
{
#if 0
	m_Context.SetIndexBuffer(pIndexBuffer);
#else
	m_pD3DCBDevice->SetIndices(pIndexBuffer?(const_cast<D3DIndexBuffer*>(&pIndexBuffer->m_d3dIndexBuffer)):NULL);
#endif
}

/// draw primtive
void C_Context::DrawPrimitive(const Display2::E_PrimitiveType primitiveType, const U32 startVertex, const U32 vertexCount)
{
	PatchVertexShader();
	m_pD3DCBDevice->DrawVertices((D3DPRIMITIVETYPE)primitiveType, startVertex, vertexCount);
	m_bEmptyCommandBuffer = false;
}

/// draw indexed primtive
void C_Context::DrawIndexedPrimitive(const Display2::E_PrimitiveType primitiveType, const U32 baseVertex, const U32 minIndex, const U32 vertexCount, const U32 startIndex, const U32 indexCount)
{
	PatchVertexShader();
	m_pD3DCBDevice->DrawIndexedVertices((D3DPRIMITIVETYPE)primitiveType, baseVertex, startIndex, indexCount);
	m_bEmptyCommandBuffer = false;
}

/// begin/end vertices
void* C_Context::BeginVertices(const E_PrimitiveType primitiveType, const U32 vertexCount, const U32 vertexStride, const C_VertexDeclaration* pVertexDeclaration)
{
	PatchVertexShader();
	void* pVertices = NULL;
	m_pD3DCBDevice->BeginVertices(D3D9PrimitiveType(primitiveType), vertexCount, vertexStride, &pVertices);
	m_bEmptyCommandBuffer = false;
	
	return pVertices;
}

void C_Context::EndVertices()
{
	m_pD3DCBDevice->EndVertices();
}

// set alpha test function
void C_Context::SetAlphaTestFunc(const E_CmpFunc alphaTestFunc)
{
	if(g_UseSharedContextFunctions)
	{
		m_Context.SetAlphaTestFunc((Display2::E_CmpFunc)alphaTestFunc);
		return;
	}
	
	m_pD3DCBDevice->SetRenderState(D3DRS_ALPHAFUNC, D3D9CmpFunc(alphaTestFunc));
}

// set wireframe / solid
void C_Context::SetFillMode(const E_FillMode fillMode)
{
	if(g_UseSharedContextFunctions)
	{
		C_ContextBase::SetFillMode(fillMode);
		return;
	}

#if 0
	m_Context.SetFillMode((Display2::E_FillMode)fillMode);
#else
	m_pD3DCBDevice->SetRenderState(D3DRS_FILLMODE, D3D9FillMode(fillMode));
#endif
}

/// set blend high precision enable
void C_Context::SetBlendHighPrecisionEnable(const E_Bool blendHighPrecisionEnable)
{
	m_pD3DCBDevice->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, blendHighPrecisionEnable == E_Bool_True ? TRUE : FALSE);
}

/// set blend enable
void C_Context::SetBlendEnable(const E_Bool blendEnable)
{
	if(g_UseSharedContextFunctions)
	{
		C_ContextBase::SetBlendEnable(blendEnable);
		return;
	}

#if 0
	m_Context.SetBlendEnable((Display2::E_Bool)blendEnable);
#else
	m_pD3DCBDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, D3D9Bool(blendEnable));
#endif
}

/// set cull mode
void C_Context::SetCullMode(const E_CullMode cullMode)
{
	if(g_UseSharedContextFunctions)
	{
		C_ContextBase::SetCullMode(cullMode);
		return;
	}

	switch (cullMode)
	{
#if 0
	case Display::E_CullMode_Cw:
	case Display::E_CullMode_Reversed:
		m_Context.SetCullMode(Display2::E_CullMode_Cw);
		break;
	case Display::E_CullMode_Ccw:
	case Display::E_CullMode_Default:
		m_Context.SetCullMode(Display2::E_CullMode_Ccw);
		break;
	case Display::E_CullMode_None:
		m_Context.SetCullMode(Display2::E_CullMode_None);
		break;
#else
	case Display::E_CullMode_Default: m_pD3DCBDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW); break;
	case Display::E_CullMode_Reversed: m_pD3DCBDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW); break;
	case Display::E_CullMode_Ccw: m_pD3DCBDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW); break;
	case Display::E_CullMode_Cw: m_pD3DCBDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW); break;
	case Display::E_CullMode_None: m_pD3DCBDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE); break;
#endif
	}
}

/// set depth enable
void C_Context::SetDepthEnable(const E_Bool depthEnable)
{
	if(g_UseSharedContextFunctions)
	{
		C_ContextBase::SetDepthEnable(depthEnable);
		return;
	}
	
	m_pD3DCBDevice->SetRenderState(D3DRS_ZENABLE, D3D9Bool(depthEnable));
	m_pD3DCBDevice->SetRenderState(D3DRS_HIZENABLE, D3D9Bool(depthEnable));
}

/// set depthwrite enable
void C_Context::SetDepthWriteEnable(const E_Bool depthWriteEnable)
{
	if(g_UseSharedContextFunctions)
	{
		C_ContextBase::SetDepthWriteEnable(depthWriteEnable);
		return;
	}

#if 0
	m_Context.SetDepthWriteEnable((Display2::E_Bool)depthWriteEnable);
#else
	m_pD3DCBDevice->SetRenderState(D3DRS_ZWRITEENABLE, D3D9Bool(depthWriteEnable));
#endif
}

/// set depth func
void C_Context::SetDepthFunc(const E_CmpFunc depthFunc)
{
	if(g_UseSharedContextFunctions)
	{
		C_ContextBase::SetDepthFunc(depthFunc);
		return;
	}

#if 0
	m_Context.SetDepthFunc((Display2::E_CmpFunc)depthFunc);
#else
	m_pD3DCBDevice->SetRenderState(D3DRS_ZFUNC, D3D9CmpFunc(depthFunc));
#endif

	m_pD3DCBDevice->SetRenderState(D3DRS_HIZENABLE, depthFunc == E_CmpFunc_GreaterEqual ? D3DHIZ_ENABLE: D3DHIZ_AUTOMATIC);
}

/// change the current vertex shader
void C_Context::SetVertexShader(const Display2::C_VertexShader *pShader)
{
	m_bVertexShaderChanged = true;
	m_pVertexShader = pShader->m_pd3dVertexShader;
}

/// change the current pixel shader
void C_Context::SetPixelShader(const Display2::C_PixelShader *pShader)
{
	m_bVertexShaderChanged = true;

	if(pShader)
		m_pPixelShader = pShader->m_pd3dPixelShader;
	else
		m_pPixelShader = 0;
}

/// patch this combination of vertex shader and vertex declaration
void C_Context::PatchVertexShader()
{
	if (!m_bVertexShaderChanged)
		return;
	m_bVertexShaderChanged = false;

	if (g_PatchVertexShaders)
	{
		m_pD3DCBDevice->SetVertexDeclaration(NULL);
		m_pD3DCBDevice->SetVertexShader(::PatchVertexShader(m_pVertexShader, m_pPixelShader, m_pVtxDecl));
	}
	else
	{
		m_pD3DCBDevice->SetVertexDeclaration(m_pVtxDecl);
		m_pD3DCBDevice->SetVertexShader(m_pVertexShader);
	}

	m_pD3DCBDevice->SetPixelShader(m_pPixelShader);
}

void C_Context::SetAlphaTestState(const Display::E_Bool alphaTestEnable, const U32 alphaRef)
{
	if(g_UseSharedContextFunctions)
	{
		C_ContextBase::SetAlphaTestState(alphaTestEnable, alphaRef);
		return;
	}

#if 1
	m_Context.SetAlphaTestEnable((Display2::E_Bool)alphaTestEnable);
	m_Context.SetAlphaTestRef(alphaRef);
#else
	m_pD3DCBDevice->SetRenderState(D3DRS_ALPHAREF, alphaRef);
	m_pD3DCBDevice->SetRenderState(D3DRS_ALPHATESTENABLE, D3D9Bool(alphaTestEnable));
#endif
}

void C_Context::SetAlphaToMaskEnable(const Display::E_Bool alphaToMaskEnable)
{
#if 1
	m_Context.SetAlphaToMaskEnable((Display2::E_Bool)alphaToMaskEnable);
	if (alphaToMaskEnable == E_Bool_True && g_AlphaToMaskOffsets >= 0)
		m_Context.SetAlphaToMaskOffsets(g_AlphaToMaskOffsets);
#else
	m_pD3DCBDevice->SetRenderState(D3DRS_ALPHATOMASKENABLE, D3D9Bool(alphaToMaskEnable));
	if (alphaToMaskEnable && g_AlphaToMaskOffsets >= 0)
		m_pD3DCBDevice->SetRenderState(D3DRS_ALPHATOMASKOFFSETS, g_AlphaToMaskOffsets);
#endif
}

/// set a pixel shader constant register
void C_Context::SetPixelShaderConstantF(const U32 startRegister, const F32* pData, const U32 registerCount)
{
#if 0
	m_Context.SetPixelShaderConstantF(startRegister, pData, registerCount);
#else
	m_pD3DCBDevice->SetPixelShaderConstantF(startRegister, pData, registerCount);
#endif
}

/// set a vertex shader constant register
void C_Context::SetVertexShaderConstantF(const U32 startRegister, const F32* pData, const U32 registerCount)
{
	if(g_UseSharedContextFunctions)
	{
		C_ContextBase::SetVertexShaderConstantF(startRegister, pData, registerCount);
		return;
	}

#if 0
	m_Context.SetVertexShaderConstantF(startRegister, pData, registerCount);
#else
	m_pD3DCBDevice->SetVertexShaderConstantF(startRegister, pData, registerCount);
#endif
}

/// set pixelshader parameter
void C_Context::SetPixelShaderParameterF(Display::C_PixelShaderParameter handle, const F32 *src)
{
	if(handle.m_RegisterIndex != UINT_MAX && handle.m_RegisterCount != 0)
		SetPixelShaderConstantF(handle.m_RegisterIndex, src, 1);
}

#if ASSERT_ON_VALID_INTREFCOUNT
bool C_Context::CheckSetTexture(const void* pTexture)
{
	const Display2::GPUTexture* pgpuTexture = (Display2::GPUTexture*)pTexture;

	BIZ_ASSERT_SPEED(pgpuTexture);
	BIZ_ASSERT_SPEED(Display2::DebugCheckGPUTexture(pgpuTexture));

	U32 refCount = GPUCOMMON_DEBUG_INTREFCOUNT(pgpuTexture->Common);
	BIZ_ASSERT_MSG_SPEED(refCount != 0, "refCount %i", refCount);

	return true;
}

bool C_Context::CheckUnsetTexture(const void* pTexture)
{
	const Display2::GPUTexture* pgpuTexture = (Display2::GPUTexture*)pTexture;
	
	BIZ_ASSERT_SPEED(pgpuTexture);
	BIZ_ASSERT_SPEED(Display2::DebugCheckGPUTexture(pgpuTexture));

	return true;
}

#if !defined(_DEBUG)
void C_Context::IncTextureIntRefCount(void* pTexture)
{
	if(!pTexture)
		return;

	D3DBaseTexture* pd3dTexture = (D3DBaseTexture*)pTexture;
	DWORD newCommon = -1;
	DWORD oldCommon = -1;
	do
	{
		oldCommon = pd3dTexture->Common;
		DWORD intRefCount1 = ((oldCommon & GPUCOMMON_DEBUG_INTREFCOUNT_MASK) >> GPUCOMMON_DEBUG_INTREFCOUNT_SHIFT);
		intRefCount1++;
		newCommon = (oldCommon & ~GPUCOMMON_DEBUG_INTREFCOUNT_MASK) | (intRefCount1 << GPUCOMMON_DEBUG_INTREFCOUNT_SHIFT);
		DWORD intRefCount2 = ((newCommon & GPUCOMMON_DEBUG_INTREFCOUNT_MASK) >> GPUCOMMON_DEBUG_INTREFCOUNT_SHIFT);
		BIZ_ASSERT_MSG_SPEED(intRefCount1 == intRefCount2, "intRefCount1 %i intRefCount2 %i", intRefCount1, intRefCount2);

	} while(!MT::AtomicCompareAndSwap((U32*)&pd3dTexture->Common, (U32)newCommon, (U32)oldCommon));
}

void C_Context::DecTextureIntRefCount(void* pTexture)
{
	if(!pTexture)
		return;

	D3DBaseTexture* pd3dTexture = (D3DBaseTexture*)pTexture;
	DWORD newCommon = -1;
	DWORD oldCommon = -1;
	do
	{
		oldCommon = pd3dTexture->Common;
		DWORD intRefCount1 = ((oldCommon & GPUCOMMON_DEBUG_INTREFCOUNT_MASK) >> GPUCOMMON_DEBUG_INTREFCOUNT_SHIFT);
		BIZ_ASSERT_MSG_SPEED(intRefCount1 > 0, "intRefCount1 %i", intRefCount1);
		intRefCount1--;
		newCommon = (oldCommon & ~GPUCOMMON_DEBUG_INTREFCOUNT_MASK) | (intRefCount1 << GPUCOMMON_DEBUG_INTREFCOUNT_SHIFT);
		DWORD intRefCount2 = ((newCommon & GPUCOMMON_DEBUG_INTREFCOUNT_MASK) >> GPUCOMMON_DEBUG_INTREFCOUNT_SHIFT);
		BIZ_ASSERT_MSG_SPEED(intRefCount1 == intRefCount2, "intRefCount1 %i intRefCount2 %i", intRefCount1, intRefCount2);

	} while(!MT::AtomicCompareAndSwap((U32*)&pd3dTexture->Common, (U32)newCommon, (U32)oldCommon));
}
#endif
#endif

/// set a texture. this function takes a pointer to a GcmTexture or a D3DTexture
void C_Context::SetTexture(const U32 sampler, const void* pTexture)
{
#if 0
	m_Context.GetD3DDevice()->SetTexture(sampler, (Display2::GPUTexture*)pTexture);
#else
#if ASSERT_ON_VALID_INTREFCOUNT
	BIZ_ASSERT_SPEED(sampler < E_Sampler_Count);

	if(m_pTextures[sampler])
	{
		BIZ_ASSERT_SPEED(CheckSetTexture(m_pTextures[sampler]));

		#if !defined(_DEBUG)
		DecTextureIntRefCount((void*)m_pTextures[sampler]);
		#endif
	}

	if(pTexture)
	{
		BIZ_ASSERT_SPEED(CheckUnsetTexture(pTexture));
	}
#endif

	m_pD3DCBDevice->SetTexture(sampler, (D3DBaseTexture*)pTexture);

#if ASSERT_ON_VALID_INTREFCOUNT
	#if !defined(_DEBUG)
	IncTextureIntRefCount((void*)pTexture);
	#endif

	m_pTextures[sampler] = pTexture;
#endif
#endif
}

/// set a vertex texture. this function takes a pointer to a GcmTexture or a D3DTexture
void C_Context::SetVertexTexture(const U32 sampler, const void* pTexture)
{
#if 0
	m_Context.GetD3DDevice()->SetTexture(sampler + D3DVERTEXTEXTURESAMPLER0, (Display2::GPUTexture*)pTexture);
#else
#if ASSERT_ON_VALID_INTREFCOUNT
	BIZ_ASSERT_SPEED(sampler < E_VertexSampler_Count);

	if(m_pVertexTextures[sampler])
	{
		BIZ_ASSERT_SPEED(CheckSetTexture(m_pVertexTextures[sampler]));

		#if !defined(_DEBUG)
		DecTextureIntRefCount((void*)m_pVertexTextures[sampler]);
		#endif
	}

	if(pTexture)
	{
		BIZ_ASSERT_SPEED(CheckUnsetTexture(pTexture));
	}
#endif

	m_pD3DCBDevice->SetTexture(sampler + D3DVERTEXTEXTURESAMPLER0, (D3DBaseTexture*)pTexture);

#if ASSERT_ON_VALID_INTREFCOUNT
	#if !defined(_DEBUG)
	IncTextureIntRefCount((void*)pTexture);
	#endif

	m_pVertexTextures[sampler] = pTexture;
#endif
#endif
}

/// set stencil states
void C_Context::SetStencilState(const Display::C_StencilState& stencilState)
{
	if(g_UseSharedContextFunctions)
	{
		C_ContextBase::SetStencilState(stencilState);
		return;
	}

#if 0
	m_Context.SetStencilEnable((Display2::E_Bool)stencilState.m_StencilEnable);
	m_Context.SetStencilFail((Display2::E_StencilOp)stencilState.m_StencilFail);
	m_Context.SetStencilDepthFail((Display2::E_StencilOp)stencilState.m_StencilDepthFail);
	m_Context.SetStencilPass((Display2::E_StencilOp)stencilState.m_StencilDepthPass);
	m_Context.SetStencilFunc((Display2::E_CmpFunc)stencilState.m_StencilFunc);
	m_Context.SetStencilRef(stencilState.m_StencilRef);
	m_Context.SetStencilMask(stencilState.m_StencilMask);
	m_Context.SetStencilWriteMask(stencilState.m_StencilWriteMask);
#else
	m_pD3DCBDevice->SetRenderState(D3DRS_STENCILENABLE, D3D9Bool(stencilState.m_StencilEnable));
	m_pD3DCBDevice->SetRenderState(D3DRS_STENCILFAIL, D3D9StencilOp(stencilState.m_StencilFail));
	m_pD3DCBDevice->SetRenderState(D3DRS_STENCILZFAIL, D3D9StencilOp(stencilState.m_StencilDepthFail));
	m_pD3DCBDevice->SetRenderState(D3DRS_STENCILPASS, D3D9StencilOp(stencilState.m_StencilDepthPass));
	m_pD3DCBDevice->SetRenderState(D3DRS_STENCILFUNC, D3D9CmpFunc(stencilState.m_StencilFunc));
	m_pD3DCBDevice->SetRenderState(D3DRS_STENCILREF, stencilState.m_StencilRef);
	m_pD3DCBDevice->SetRenderState(D3DRS_STENCILMASK, stencilState.m_StencilMask);
	m_pD3DCBDevice->SetRenderState(D3DRS_STENCILWRITEMASK, stencilState.m_StencilWriteMask);
#endif
}

/// set colour write enable
void C_Context::SetColourWriteEnable(const U32 flags)
{
	if(g_UseSharedContextFunctions)
	{
		C_ContextBase::SetColourWriteEnable(flags);
		return;
	}

#if 0
	m_Context.SetColourWriteEnable(flags);
#else
	m_pD3DCBDevice->SetRenderState(D3DRS_COLORWRITEENABLE, flags);
#endif
}

/// set predication
void C_Context::SetPredication(const U32 predication)
{
	if(m_bMultiThreaded)
		m_pD3DCBDevice->SetCommandBufferPredication(predication, 0);
	else
		m_pD3DCBDevice->SetPredication(predication);
}

/// sets half pixel offset
void C_Context::SetHalfPixelOffset(const E_Bool enable)
{
	m_pD3DCBDevice->SetRenderState(D3DRS_HALFPIXELOFFSET, (BOOL)enable);
}

#if USE_GPU_TIMINGS2
void C_Context::InsertCallback(D3DCALLBACK pCallback, DWORD context)
{
	m_pD3DCBDevice->InsertCallback(D3DCALLBACK_IMMEDIATE, pCallback, context);
}
#endif

/// manager

/// constructor
C_ContextManager::C_ContextManager()
{
	m_bInitialised = false;
}

/// initialise helper surfaces
void C_ContextManager::Initialise()
{
	if (m_bInitialised)
		return;

	m_bInitialised = true;

	Display::Device()->GetD3DDevice()->CreateDepthStencilSurface(16, 16, D3DFMT_D24FS8, D3DMULTISAMPLE_NONE, 0, FALSE, &m_pDepthStencilSurface, NULL);

	Display::Device()->GetD3DDevice()->CreateVertexBuffer(sizeof(Display::Vertex_XYZ_C), 0, 0, 0, &m_pVertexBuffer, NULL);
	void* pData;
	m_pVertexBuffer->Lock(0, 0, &pData, 0);
	Display::Vertex_XYZ_C* pVertex = (Display::Vertex_XYZ_C*)pData;
	pVertex->x = 0.0f;
	pVertex->y = 0.0f;
	pVertex->z = 0.0f;
	pVertex->c = 0xFFFFFFFF;
	m_pVertexBuffer->Unlock();
	
	Display::Device()->GetD3DDevice()->CreateIndexBuffer(sizeof(U16), 0, D3DFMT_INDEX16, 0, &m_pIndexBuffer, NULL);
	m_pIndexBuffer->Lock(0, 0, &pData, 0);
	U16* pIndex = (U16*)pData;
	*pIndex = 0;
	m_pIndexBuffer->Unlock();

	Display::CreateVertexShaderFromFunc("Shaders/vsBasicRenderColoured3D", 0, &m_VertexShader, REF_SHADER(vsBasicRenderColoured3D));
	Display::CreatePixelShaderFromFunc("Shaders/psBasicRenderColoured", 0, &m_PixelShader, REF_SHADER(psBasicRenderColoured));

	DWORD stride = 16;
	m_VertexShader.GetGPUVertexShader()->Bind(0, Display::Vertex_XYZ_C::m_pDeclaration->GetGPUVertexDeclaration(), &stride, m_PixelShader.GetGPUPixelShader());
		
}

/// free up memory used by patched vertex shaders
void C_ContextManager::FreePatchedVertexShader(D3DVertexShader* pd3dVertexShaders)
{
	::FreePatchedVertexShader(pd3dVertexShaders);
}

C_ContextManager g_ContextManager;

}	// namespace
