#ifndef __DISPLAY_CONTEXT_XENON_H__
#define __DISPLAY_CONTEXT_XENON_H__

#include "Display/Buffer/CommandBuffer.h"
#include "Display/Buffer/IndexBuffer.h"
#include "Display/Buffer/VertexBuffer.h"
#include "Display/Buffer/VertexDeclarations.h"
#include "Display/Shader/VertexShader.h"
#include "Display/Shader/PixelShader.h"
#include "Display/Device/DeviceContext.h"
#include "Display/View/View.h"
#include "Display/Context/Context_D3D9.h"

#define NUM_CTXMANAGER_DEVICES 7

#if !CFG_GOLD
//#define ASSERT_ON_VALID_INTREFCOUNT 1
#endif

namespace Display
{

class C_Device;
class C_ContextManager;
struct C_StencilState;

DECLARE_CONTEXT(Context, C_Context);

/// thread-safe rendering context
class C_Context : public C_ContextD3D9
{
	friend class C_ContextManager;
public:
	/// constructor
	C_Context();

	/// destructor
	~C_Context();

	/// one-time initialisation
	void Initialise();

	/// allocate a new instance of this class
	I_Context *New() { return new C_Context; }

	/// clone the state of another context into this context
	void CloneState(const I_Context *pContext);

	/// invalidate the state of this context
	void InvalidateState();

	/// draws a dummy primitive which forces D3D to write previously set states into command buffer
	void DrawDummyPrimitive();

	/// setting states before we run command buffer
	void SetColourDepthStencilSurface(const Display::C_Surface* pColourSurface, const Display::C_Surface* pDepthStencilSurface) { m_pColourSurface = pColourSurface; m_pDepthStencilSurface = pDepthStencilSurface; }
	void SetWindowIndex(const S32 windowIndex) { m_WindowIndex = windowIndex; }

	/// begin rendering to the comand buffer
	void BeginCommandBuffer(Display::C_CommandBuffer *pCommandBuffer);

	/// stop rendering to the command buffer
	void EndCommandBuffer();

	/// set vertex declaration
	void SetVertexDeclaration(const Display2::C_VertexDeclaration* pVertexDeclaration);
	
	/// set vertex buffer
	void SetVertexBuffer(const U32 stream, const Display2::C_VertexBuffer* pVertexBuffer, const U32 offset, const U32 stride);
	
	/// set index buffer
	void SetIndexBuffer(const Display2::C_IndexBuffer* pIndexBuffer);
	
	/// draw primtive
	void DrawPrimitive(const Display2::E_PrimitiveType primitiveType, const U32 startVertex, const U32 vertexCount);
	
	/// draw indexed primtive
	void DrawIndexedPrimitive(const Display2::E_PrimitiveType primitiveType, const U32 baseVertex, const U32 minIndex, const U32 vertexCount, const U32 startIndex, const U32 indexCount);
	
	/// begin/end vertices
	void* BeginVertices(const E_PrimitiveType primitiveType, const U32 vertexCount, const U32 vertexStride, const C_VertexDeclaration* pVertexDeclaration);
	void EndVertices();
	
	// set alpha test function
	void SetAlphaTestFunc(const E_CmpFunc alphaTestFunc);

	/// set blend high precision enable
	void SetBlendHighPrecisionEnable(const E_Bool blendHighPrecisionEnable);

	/// set blend enable
	void SetBlendEnable(const E_Bool blendEnable);

	/// set cull mode
	void SetCullMode(const E_CullMode eCullMode);

	/// set depth enable
	void SetDepthEnable(const E_Bool depthEnable);

	/// set depthwrite enable
	void SetDepthWriteEnable(const E_Bool depthWriteEnable);

	/// set depth func
	void SetDepthFunc(const E_CmpFunc depthFunc);

	/// set wireframe / solid
	void SetFillMode(const E_FillMode fillMode);

	/// change the current vertex shader
	void SetVertexShader(const Display2::C_VertexShader *pShader);
	/// change the current pixel shader
	void SetPixelShader(const Display2::C_PixelShader *pShader);
	
	/// set alpha teststate
	void SetAlphaTestState(Display::E_Bool alphaTestEnable, U32 alphaRef);

	/// set alpha to mask enable
	void SetAlphaToMaskEnable(Display::E_Bool alphaToMaskEnable);

	/// set a pixel shader constant register
	void SetPixelShaderConstantF(const U32 startRegister, const F32* pData, const U32 registerCount);

	/// set a vertex shader constant register
	void SetVertexShaderConstantF(const U32 startRegister, const F32* pData, const U32 registerCount);

	/// set pixelshader parameter
	void SetPixelShaderParameterF(Display::C_PixelShaderParameter handle, const F32 *src);

	/// set a texture. this function takes a pointer to a GcmTexture or a D3DTexture
	void SetTexture(const U32 sampler, const void* pTexture);

	/// set a vertex texture. this function takes a pointer to a GcmTexture or a D3DTexture
	void SetVertexTexture(const U32 sampler, const void* pTexture);

	/// set stencil states
	void SetStencilState(const Display::C_StencilState& stencilState);

	/// set colour write enable
	void SetColourWriteEnable(const U32 flags);

	/// sets predication for tiling
	void SetPredication(const U32 predication);

	/// sets half pixel offset
	void SetHalfPixelOffset(const E_Bool enable);

	#if USE_GPU_TIMINGS2
	void InsertCallback(D3DCALLBACK pCallback, DWORD context);
	#endif

	void Unbind();

private:

#if ASSERT_ON_VALID_INTREFCOUNT
	bool CheckSetTexture(const void* pTexture);
	bool CheckUnsetTexture(const void* pTexture);
	#if !defined(_DEBUG)
	void IncTextureIntRefCount(void* pTexture);
	void DecTextureIntRefCount(void* pTexture);
	#endif
#endif

protected:
	/// patch this combination of vertex shader and vertex declaration
	void PatchVertexShader();

	IDirect3DDevice9*				m_pD3DDevice;
		
	S32								m_WindowIndex;
	const Display::C_Surface*		m_pColourSurface;
	const Display::C_Surface*		m_pDepthStencilSurface;

	IDirect3DPixelShader9*			m_pPixelShader;

	// for vertex shader patching
	IDirect3DVertexDeclaration9*	m_pVtxDecl;
	IDirect3DVertexShader9*			m_pVertexShader;
	bool							m_bVertexShaderChanged;
	bool							m_bMultiThreaded;
	bool							m_bEmptyCommandBuffer;
	Display::C_CommandBuffer*		m_pCommandBuffer;

#if ASSERT_ON_VALID_INTREFCOUNT
	const void*						m_pTextures[E_Sampler_Count];
	const void*						m_pVertexTextures[E_VertexSampler_Count];
#endif

public:
	Display::C_Device *m_pDevice;
};

/// This is a helper class that may eventually disappear
/// creates helper surfaces to make the commandbuffer rendering work
/// also manages pixel shader patching
class C_ContextManager
{
public:
	/// constructor
	C_ContextManager();

	/// free up memory used by patched vertex shaders
	void FreePatchedVertexShader(D3DVertexShader* pd3dVertexShaders);

	IDirect3DSurface9* GetDepthStencilSurface() { return m_pDepthStencilSurface; }

	/// create a device for each worker thread to render with
	void Initialise();

	friend class C_Context;

private:

	/// list of command buffers used to render the current frame
	Btl::Vector<IDirect3DCommandBuffer9*>	m_EndFrameCommandBuffers[2];
	IDirect3DSurface9*						m_pDepthStencilSurface;
	IDirect3DVertexBuffer9*					m_pVertexBuffer;
	IDirect3DIndexBuffer9*					m_pIndexBuffer;
	Display::C_VertexShader					m_VertexShader;
	Display::C_PixelShader					m_PixelShader;

	bool									m_bInitialised;
};

extern C_ContextManager g_ContextManager;

}

#endif
