// [Blur1PCDX11] Native Win32 D3D11 game device and DXGI swapchain.
#include "stdafx.h"
#include "Display/Device/D3D11/PCD3D11GameDevice.h"
#include "Display/Device/D3D11/D3D11Formats.h"
#include <system/debug/Blur2PCAutomation.h>
#include "Display/Render/Render.h"
#include "Display/Utils/Screenshot.h"
#include <d3d11sdklayers.h>
#include <d3d11_4.h>
#include <vector>
#include <string>
namespace Display { namespace D3D11 {
namespace
{
ID3D11Device* g_ExceptionDevice = nullptr;

// [Blur1PCDX11] The D3D11 debug layer raises 0x87D on the corruption it finds and the
// message text only exists inside its queue, so log it before the crash handler runs.
LONG CALLBACK LogNativeException(EXCEPTION_POINTERS* info)
{
    if (!g_ExceptionDevice || !info || !info->ExceptionRecord) return EXCEPTION_CONTINUE_SEARCH;
    // [Blur1PCDX11] Report distinct failures; one repeating shader pair must
    // not consume the diagnostic budget before the race starts rendering.
    static std::vector<std::string> reported;
    ID3D11DeviceContext* context = nullptr;
    g_ExceptionDevice->GetImmediateContext(&context);
    ID3D11VertexShader* vertex = nullptr;
    ID3D11PixelShader* pixel = nullptr;
    if (context) { context->VSGetShader(&vertex, nullptr, nullptr); context->PSGetShader(&pixel, nullptr, nullptr); }
    ID3D11InfoQueue* queue = nullptr;
    if (SUCCEEDED(g_ExceptionDevice->QueryInterface(__uuidof(ID3D11InfoQueue), reinterpret_cast<void**>(&queue))) && queue)
    {
        const UINT64 count = queue->GetNumStoredMessages();
        for (UINT64 cursor = 0; cursor < count && reported.size() < 40; ++cursor)
        {
            SIZE_T size = 0;
            queue->GetMessage(cursor, nullptr, &size);
            std::vector<U8> bytes(size);
            D3D11_MESSAGE* message = reinterpret_cast<D3D11_MESSAGE*>(bytes.data());
            if (SUCCEEDED(queue->GetMessage(cursor, message, &size)))
            {
                char prefix[80] = {};
                sprintf_s(prefix, "%p/%p/%u/", vertex, pixel, static_cast<unsigned>(message->ID));
                const std::string key = std::string(prefix) + message->pDescription;
                bool duplicate = false;
                for (const auto& previous : reported) duplicate |= previous == key;
                if (duplicate) continue;
                Blur2PCAutomation::LogF("DX11 queued sev=%u id=%d VS=%p PS=%p: %s",
                    static_cast<unsigned>(message->Severity), static_cast<int>(message->ID),
                    vertex, pixel, message->pDescription);
                reported.push_back(key);
            }
        }
        queue->ClearStoredMessages();
        queue->Release();
    }
    if (context) { if (vertex) vertex->Release(); if (pixel) pixel->Release(); context->Release(); }
    return EXCEPTION_CONTINUE_SEARCH;
}
}
C_PCD3D11GameDevice::C_PCD3D11GameDevice() : m_Lost(false),m_Valid(false),m_pSwapChain(nullptr),
    m_BeginVerticesUsage(0),m_MaxBeginVerticesUsage(0),m_UserBeginVerticesUsage(0),
    m_UserMaxBeginVerticesUsage(0),m_BeginVerticesLocked(false)
{
#ifndef CFG_GOLD
    m_BeginVerticesStart=nullptr;m_BeginVerticesSize=0;
#endif
    SetDepthFlip(true,false);
}
C_PCD3D11GameDevice::~C_PCD3D11GameDevice() {}
void C_PCD3D11GameDevice::SetPresentParamsD3D11()
{
    ZeroMemory(&m_d3dPresentParams,sizeof(m_d3dPresentParams));
    m_d3dPresentParams.BufferDesc.Width=m_DeviceParams.m_Width;
    m_d3dPresentParams.BufferDesc.Height=m_DeviceParams.m_Height;
    m_d3dPresentParams.BufferDesc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;
    m_d3dPresentParams.SampleDesc.Count=1;
    m_d3dPresentParams.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;
    m_d3dPresentParams.BufferCount=1;
    m_d3dPresentParams.OutputWindow=m_DeviceParams.m_hWnd;
    m_d3dPresentParams.Windowed=m_DeviceParams.m_Windowed;
    m_d3dPresentParams.SwapEffect=DXGI_SWAP_EFFECT_DISCARD;
}
bool C_PCD3D11GameDevice::CreateD3D11()
{
    SetPresentParamsD3D11();
    char debugSetting[2] = {};
    UINT flags=D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    if (GetEnvironmentVariableA("BLUR_DX11_DEBUG", debugSetting, sizeof(debugSetting)) &&
        debugSetting[0] == '1') flags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL features[]={D3D_FEATURE_LEVEL_11_1,D3D_FEATURE_LEVEL_11_0,D3D_FEATURE_LEVEL_10_1,D3D_FEATURE_LEVEL_10_0};
    D3D_FEATURE_LEVEL selected={};ID3D11DeviceContext* context=nullptr;
    char warpSetting[2] = {};
    const bool forceWarp = GetEnvironmentVariableA("BLUR_DX11_WARP", warpSetting, sizeof(warpSetting)) &&
        warpSetting[0] == '1';
    D3D_DRIVER_TYPE driver=(m_DeviceParams.m_UseREF || forceWarp) ? D3D_DRIVER_TYPE_WARP : D3D_DRIVER_TYPE_HARDWARE;
    HRESULT hr=D3D11CreateDeviceAndSwapChain(nullptr,driver,nullptr,flags,features,4,D3D11_SDK_VERSION,
        &m_d3dPresentParams,&m_pSwapChain,&m_pd3dDevice,&selected,&context);
    if (hr == DXGI_ERROR_SDK_COMPONENT_MISSING)
    {
        flags &= ~D3D11_CREATE_DEVICE_DEBUG;
        hr=D3D11CreateDeviceAndSwapChain(nullptr,driver,nullptr,flags,features,4,D3D11_SDK_VERSION,
            &m_d3dPresentParams,&m_pSwapChain,&m_pd3dDevice,&selected,&context);
    }
    if(hr==E_INVALIDARG)
        hr=D3D11CreateDeviceAndSwapChain(nullptr,driver,nullptr,flags,features+1,3,D3D11_SDK_VERSION,
            &m_d3dPresentParams,&m_pSwapChain,&m_pd3dDevice,&selected,&context);
    if(FAILED(hr) && driver==D3D_DRIVER_TYPE_HARDWARE)
        hr=D3D11CreateDeviceAndSwapChain(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,flags,features+1,3,D3D11_SDK_VERSION,
            &m_d3dPresentParams,&m_pSwapChain,&m_pd3dDevice,&selected,&context);
    // [Blur1PCDX11] The engine uploads textures and buffers from its loading thread while the
    // main thread renders; D3D9 serialised that internally and D3D11 needs the same guarantee.
    if (m_pd3dDevice)
    {
        ID3D11DeviceContext* immediate = nullptr;
        m_pd3dDevice->GetImmediateContext(&immediate);
        if (immediate)
        {
            ID3D11Multithread* multithread = nullptr;
            if (SUCCEEDED(immediate->QueryInterface(__uuidof(ID3D11Multithread),
                reinterpret_cast<void**>(&multithread))) && multithread)
            {
                multithread->SetMultithreadProtected(TRUE);
                multithread->Release();
            }
            immediate->Release();
        }
    }
    if(context)context->Release();
    if(FAILED(hr)) {LOGERROR(g_Log,"D3D11CreateDeviceAndSwapChain failed 0x%08x\n",hr);return false;}
    Blur2PCAutomation::LogF("DX11 device created debug=%u warp=%u feature=%x", (flags & D3D11_CREATE_DEVICE_DEBUG) != 0,
        driver == D3D_DRIVER_TYPE_WARP, static_cast<unsigned>(selected));
    g_ExceptionDevice = m_pd3dDevice;
    AddVectoredExceptionHandler(1, LogNativeException);
    if (flags & D3D11_CREATE_DEVICE_DEBUG)
    {
        // [Blur1PCDX11] Keep errors and corruption; warnings such as an unbound render
        // target flood the queue and each one raises under a debugger's default settings.
        ID3D11InfoQueue* queue = nullptr;
        if (SUCCEEDED(m_pd3dDevice->QueryInterface(__uuidof(ID3D11InfoQueue), reinterpret_cast<void**>(&queue))) && queue)
        {
            D3D11_MESSAGE_SEVERITY denied[] = {D3D11_MESSAGE_SEVERITY_INFO,
                D3D11_MESSAGE_SEVERITY_MESSAGE, D3D11_MESSAGE_SEVERITY_WARNING};
            D3D11_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumSeverities = 3;
            filter.DenyList.pSeverityList = denied;
            queue->AddStorageFilterEntries(&filter);
            queue->Release();
        }
    }
    m_pd3dObject=m_DeviceParams.m_pd3dObject;
    if(m_pd3dObject)m_pd3dObject->AddRef();
    return true;
}
bool C_PCD3D11GameDevice::CreateDevice()
{
    if(!CreateD3D11())return false;
    Display2::Device(&m_Display2);
    Display2::C_DeviceParams params;params.m_pd3dDevice=m_pd3dDevice;params.m_pd3dObject=m_pd3dObject;
    m_Display2.SetParams(params);
    if(!m_Display2.Create() || !m_Display2.CreateVolatile())return false;
    C_BackBuffer* back=new C_BackBuffer();
    back->m_Width=m_DeviceParams.m_Width;back->m_Height=m_DeviceParams.m_Height;
    back->m_ColourFormat=m_DeviceParams.m_ColourFormat;
    back->m_DepthStencilFormat=m_DeviceParams.m_DepthStencilFormat;
    back->m_Multisample=E_Multisample_None;
    if(FAILED(m_pSwapChain->GetBuffer(0,__uuidof(ID3D11Texture2D),
        reinterpret_cast<void**>(&back->m_ColourSurface.m_pd3dSurface)))) {delete back;return false;}
    m_BackBuffers.PushBack(back);
    if(!m_Display2.CreateDepthStencilSurface(back->m_Width,back->m_Height,E_Usage_DepthStencil,
        static_cast<Display2::E_SurfaceFormat>(back->m_DepthStencilFormat),Display2::E_Multisample_None,nullptr,
        &back->m_DepthStencilSurface))return false;
    InitialiseBuffers();
    if(!C_D3D11GameDevice::CreateDevice())return false;
    m_ShaderProfile=E_ShaderProfile_3_0;
    m_VertexShaderProfile=E_VertexShaderProfile_3_0;
    m_PixelShaderProfile=E_PixelShaderProfile_3_0;
    if(!m_DynamicVertexBuffer.Create(m_DeviceParams.m_BeginVerticesSize))return false;
    m_MaxBeginVerticesUsage=m_UserMaxBeginVerticesUsage=
        m_DeviceParams.m_BeginVerticesSize-m_DeviceParams.m_FreeBeginVerticesSize;
    SetColourDepthStencilSurface();InvalidateState();DefaultStates();ResizeWindows();
    m_Valid=true;return true;
}
bool C_PCD3D11GameDevice::CreateDeviceVolatile() {return true;}
void C_PCD3D11GameDevice::DestroyDeviceVolatile() {DestroyWindows();}
void C_PCD3D11GameDevice::DestroyDevice()
{
    ClearWindows();m_DynamicVertexBuffer.Destroy();
    for(S32 i=0;i<m_BackBuffers.Size();++i)
    {m_Display2.DestroySurface(&m_BackBuffers[i]->m_DepthStencilSurface);
     m_Display2.DestroySurface(&m_BackBuffers[i]->m_ColourSurface);delete m_BackBuffers[i];}
    m_BackBuffers.Clear();m_Display2.Destroy();
    if(m_pSwapChain){m_pSwapChain->Release();m_pSwapChain=nullptr;}
    if(m_pd3dDevice){m_pd3dDevice->Release();m_pd3dDevice=nullptr;}
    if(m_pd3dObject){m_pd3dObject->Release();m_pd3dObject=nullptr;}
}
extern unsigned g_dx11TraceFrame; // [Blur1PCDX11] Temporary, defined in D3D11GameDeviceNative.cpp.
bool C_PCD3D11GameDevice::Present()
{
    if(!m_pSwapChain)return false;
    {
        ID3D11InfoQueue* queue = nullptr;
        if (SUCCEEDED(m_pd3dDevice->QueryInterface(__uuidof(ID3D11InfoQueue),
            reinterpret_cast<void**>(&queue))))
        {
            static UINT64 nextMessage = 0;
            static unsigned messagesLogged = 0;
            const UINT64 count = queue->GetNumStoredMessages();
            for (; nextMessage < count && messagesLogged < 25; ++nextMessage)
            {
                SIZE_T size = 0;
                queue->GetMessage(nextMessage, nullptr, &size);
                std::vector<U8> bytes(size);
                D3D11_MESSAGE* message = reinterpret_cast<D3D11_MESSAGE*>(bytes.data());
                if (SUCCEEDED(queue->GetMessage(nextMessage, message, &size)) &&
                    message->Severity <= D3D11_MESSAGE_SEVERITY_WARNING)
                {
                    Blur2PCAutomation::LogF("DX11 validation %u: %s",
                        static_cast<unsigned>(message->Severity), message->pDescription);
                    ++messagesLogged;
                }
            }
            queue->Release();
        }
    }
    if (const char* shot = Blur2PCAutomation::TakeShotRequest())
    {
        ID3D11Texture2D* back = nullptr;
        const HRESULT shotResult = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
            reinterpret_cast<void**>(&back));
        if (SUCCEEDED(shotResult))
        {
            TakeD3D11BackBufferScreenshot(shot, back);
            back->Release();
        }
		else Blur2PCAutomation::LogF("DX11 shot GetBuffer failed %08x", static_cast<unsigned>(shotResult));
		if (m_Windows.Size() > 0)
		{
			ID3D11Texture2D* windowTexture = m_Windows[0]->m_ColourRenderTexture.GetTexture().GetGPUTexture2D();
			if (windowTexture)
			{
				std::string windowShot(shot);
				windowShot += ".window.tga";
				TakeD3D11BackBufferScreenshot(windowShot.c_str(), windowTexture);
			}
		}
    }
    {
        // [Blur1PCDX11] Temporary: trace every draw of one frame after BLUR_DX11_TRACE_AT seconds.
        static int traceAt = -1;
        if (traceAt < 0)
        {
            char value[16] = {};
            traceAt = GetEnvironmentVariableA("BLUR_DX11_TRACE_AT", value, sizeof(value)) ? atoi(value) : 0;
        }
        ID3D11InfoQueue* traceQueue = nullptr;
        m_pd3dDevice->QueryInterface(__uuidof(ID3D11InfoQueue), reinterpret_cast<void**>(&traceQueue));
        if (g_dx11TraceFrame)
        {
            g_dx11TraceFrame = 0;
            if (traceQueue)
            {
                const UINT64 stored = traceQueue->GetNumStoredMessages();
                for (UINT64 i = 0; i < stored && i < 80; ++i)
                {
                    SIZE_T size = 0;
                    traceQueue->GetMessage(i, nullptr, &size);
                    std::vector<U8> bytes(size);
                    D3D11_MESSAGE* message = reinterpret_cast<D3D11_MESSAGE*>(bytes.data());
                    if (SUCCEEDED(traceQueue->GetMessage(i, message, &size)))
                        Blur2PCAutomation::LogF("DX11 trace message %u: %s", static_cast<unsigned>(message->Severity), message->pDescription);
                }
                traceQueue->PopStorageFilter();
                traceQueue->ClearStoredMessages();
            }
            Blur2PCAutomation::LogF("DX11 trace end");
        }
        else if (traceAt > 0 && Blur2PCAutomation::Now() > traceAt)
        {
            traceAt = 0; g_dx11TraceFrame = 1;
            if (traceQueue) { traceQueue->ClearStoredMessages(); traceQueue->PushEmptyStorageFilter(); traceQueue->SetMessageCountLimit(100000); }
            Blur2PCAutomation::LogF("DX11 trace begin queue=%p", traceQueue);
        }
        if (traceQueue) traceQueue->Release();
    }
    HRESULT hr=m_pSwapChain->Present(m_PresentInterval==E_PresentInterval_Immediate ? 0 : 1,0);
    if (FAILED(hr))
    {
        static unsigned presentFailures = 0;
        if (presentFailures++ < 4)
            Blur2PCAutomation::LogF("DX11 Present failed %08x, removed reason %08x",
                static_cast<unsigned>(hr), static_cast<unsigned>(m_pd3dDevice->GetDeviceRemovedReason()));
    }
    m_Valid=SUCCEEDED(hr);m_Lost=hr==DXGI_ERROR_DEVICE_REMOVED || hr==DXGI_ERROR_DEVICE_RESET;
    m_DynamicVertexBuffer.Swap();m_BeginVerticesUsage=m_UserBeginVerticesUsage=0;
    return !m_Lost && m_Valid;
}
bool C_PCD3D11GameDevice::GetLost(){return m_Lost;}
bool C_PCD3D11GameDevice::GetValid(){return m_Valid;}
bool C_PCD3D11GameDevice::CanReset(){return !m_Lost;}
bool C_PCD3D11GameDevice::CanRender(){return m_Valid && !m_Lost;}
const bool C_PCD3D11GameDevice::BeginVerticesLocked() const{return m_BeginVerticesLocked;}
void* C_PCD3D11GameDevice::BeginVertices(E_PrimitiveType type,U32 count,U32 stride,C_VertexDeclaration* decl,bool checkUsage)
{
    if(m_BeginVerticesLocked)return nullptr;
    U32 size=count*stride;
    if(checkUsage && (m_BeginVerticesUsage+size>m_MaxBeginVerticesUsage ||
       m_UserBeginVerticesUsage+size>m_UserMaxBeginVerticesUsage))return nullptr;
    void* result=m_DynamicVertexBuffer.Begin(type,count,stride,decl);
    if(result){m_BeginVerticesLocked=true;m_BeginVerticesUsage+=size;m_UserBeginVerticesUsage+=size;
#ifndef CFG_GOLD
        m_BeginVerticesStart=static_cast<U8*>(result);m_BeginVerticesSize=size;
#endif
    }
    return result;
}
void C_PCD3D11GameDevice::EndVertices(void* vertex)
{
    if(!m_BeginVerticesLocked)return;
    m_DynamicVertexBuffer.End();
    SetVertexBuffer(0,m_DynamicVertexBuffer.GetVertexBuffer(),
        m_DynamicVertexBuffer.GetOffset()-m_DynamicVertexBuffer.GetVertexSize(),m_DynamicVertexBuffer.GetVertexStride());
    SetVertexDeclaration(m_DynamicVertexBuffer.GetVertexDeclaration());
    DrawPrimitive(m_DynamicVertexBuffer.GetPrimitiveType(),0,m_DynamicVertexBuffer.GetVertexCount());
    m_BeginVerticesLocked=false;
}
void C_PCD3D11GameDevice::FakeEndVertices(void* vertex)
{if(!m_BeginVerticesLocked)return;m_DynamicVertexBuffer.End();m_BeginVerticesLocked=false;}
void C_PCD3D11GameDevice::SetMaxBeginVerticesUsage(U32 size){m_MaxBeginVerticesUsage=size;}
void C_PCD3D11GameDevice::SetUserMaxBeginVerticesUsage(F32 scale)
{m_UserMaxBeginVerticesUsage=static_cast<U32>(m_MaxBeginVerticesUsage*scale);}
U32 C_PCD3D11GameDevice::GetFreeResourceMemory(){return 0;}
bool C_PCD3D11GameDevice::AllocMemory(E_ResourceType type,U32 size,U8** address,S32 id){return true;}
void C_PCD3D11GameDevice::FreeMemory(E_ResourceType type,U32 size,U8* address,S32 id){}
bool C_PCD3D11GameDevice::SupportForRenderTarget(E_ResourceType type,E_Format format)
{C_FormatLayout layout={};return GetFormatLayout(static_cast<D3DFORMAT>(format),&layout);}

bool C_PCD3D11GameDevice::CreateRenderTexture2D(U32 width,U32 height,U32 levels,U32 flags,E_Format format,
    E_Multisample samples,const C_SurfaceParams* params,C_RenderTexture2D* target,U8* address,U32 addressSize)
{
    if(!target || target->IsCreated() || !width || !height || !levels)return false;
    flags|=E_Usage_Colour|E_Usage_Video;
    if(samples!=E_Multisample_None)flags&=~E_Usage_ReadWrite;
    target->m_Flags=flags;target->m_Multisample=samples;
    const U32 buffers=flags&E_Usage_ReadWrite ? 2 : 1;
    for(U32 index=0;index<buffers;++index)
    {
        C_Texture& texture=target->m_Texture[index];
        if(!CreateTexture2D(width,height,levels,flags,format,&texture,address,addressSize))
        {DestroyRenderTexture2D(target);return false;}
        if(samples!=E_Multisample_None)
        {
            target->m_Surfaces[index].Resize(1);
            if(!CreateColourSurface(width,height,flags,format,samples,params,&target->m_Surfaces[index][0]))
            {DestroyRenderTexture2D(target);return false;}
            target->m_ResolveSurfaces[index].Resize(1);
            target->m_ResolveSurfaces[index][0].m_pd3dSurface=texture.GetGPUTexture2D();
            texture.GetGPUTexture2D()->AddRef();
        }
        else
        {
            target->m_Surfaces[index].Resize(levels);
            for(U32 level=0;level<levels;++level)
            {target->m_Surfaces[index][level].m_pd3dSurface=texture.GetGPUTexture2D();
             target->m_Surfaces[index][level].m_Subresource=level;
             texture.GetGPUTexture2D()->AddRef();}
        }
    }
    target->m_SurfaceIndex=0;target->m_TextureIndex=buffers==2 ? 1 : 0;
    return true;
}
void C_PCD3D11GameDevice::DestroyRenderTexture2D(C_RenderTexture2D* target)
{
    if(!target)return;
    for(U32 index=0;index<2;++index)
    {
        for(S32 level=0;level<target->m_Surfaces[index].Size();++level)
            m_Display2.DestroySurface(&target->m_Surfaces[index][level]);
        target->m_Surfaces[index].Clear();
        for(S32 level=0;level<target->m_ResolveSurfaces[index].Size();++level)
            m_Display2.DestroySurface(&target->m_ResolveSurfaces[index][level]);
        target->m_ResolveSurfaces[index].Clear();
        DestroyTexture2D(&target->m_Texture[index]);
    }
    target->m_Flags=0;target->m_Multisample=0;
    target->m_SurfaceIndex=target->m_TextureIndex=0;
}
void C_RenderTexture2D::Resolve(U32 level) const
{
    if(m_Flags&E_Usage_ReadWrite)
    {m_SurfaceIndex=(m_SurfaceIndex+1)&1;m_TextureIndex=(m_TextureIndex+1)&1;}
    if(m_Multisample==E_Multisample_None || !m_Surfaces[0].Size() || !m_ResolveSurfaces[0].Size())return;
    ID3D11Texture2D* source=m_Surfaces[0][0].m_pd3dSurface;
    ID3D11Texture2D* dest=m_ResolveSurfaces[0][0].m_pd3dSurface;
    if(!source || !dest || source==dest)return;
    ID3D11Device* device=nullptr;source->GetDevice(&device);if(!device)return;
    ID3D11DeviceContext* context=nullptr;device->GetImmediateContext(&context);
    D3D11_TEXTURE2D_DESC desc={};source->GetDesc(&desc);
    if(context){context->ResolveSubresource(dest,level,source,0,desc.Format);context->Release();}
    device->Release();
}

void C_PCD3D11GameDevice::ClearWindows()
{ DestroyWindows();for(S32 i=0;i<m_Windows.Size();++i)delete m_Windows[i];m_Windows.Clear(); }
void C_PCD3D11GameDevice::ResizeWindows()
{
    DestroyWindows();
    for(S32 i=0;i<m_Windows.Size();++i)
        m_Windows[i]->Resize(m_DeviceParams.m_WindowBufferWidth,m_DeviceParams.m_WindowBufferHeight,
            m_DeviceParams.m_WindowBufferMultisample,m_DeviceParams.m_ScreenWidth,m_DeviceParams.m_ScreenHeight,
            m_DeviceParams.m_WindowBufferColourFormat,m_DeviceParams.m_WindowBufferDepthStencilFormat);
    CreateWindows();
}
bool C_PCD3D11GameDevice::CreateWindows()
{
    for(S32 i=0;i<m_Windows.Size();++i)
    {
        C_Window* window=m_Windows[i];window->m_Multisample=m_DeviceParams.m_WindowBufferMultisample;
        if(!CreateRenderTexture2D(window->m_Width,window->m_Height,1,E_Usage_Colour,
            window->m_ColourFormat,window->m_Multisample,nullptr,&window->m_ColourRenderTexture))return false;
        if(!CreateDepthStencilSurface(window->m_Width,window->m_Height,E_Usage_DepthStencil,
            window->m_DepthStencilFormat,window->m_Multisample,nullptr,&window->m_DepthStencilSurface))return false;
        if(!CreateRenderTexture2D(window->m_Width,window->m_Height,1,E_Usage_Colour,
            E_Format_A8R8G8B8,window->m_Multisample,nullptr,&window->m_MaskRenderTexture))return false;
        window->m_ClearDepthStencil=true;window->m_MaskEnable=true;
    }
    return true;
}
void C_PCD3D11GameDevice::DestroyWindows()
{
    for(S32 i=0;i<m_Windows.Size();++i)
    {
        C_Window* window=m_Windows[i];
        if(window->m_ColourRenderTexture.IsCreated())DestroyRenderTexture2D(&window->m_ColourRenderTexture);
        if(window->m_MaskRenderTexture.IsCreated())DestroyRenderTexture2D(&window->m_MaskRenderTexture);
        DestroyDepthStencilSurface(&window->m_DepthStencilSurface);
    }
}
void C_PCD3D11GameDevice::SetWindow(S32 index,bool aa)
{
    if(index>=0 && index<m_Windows.Size())
    {
        C_Window* window=m_Windows[index];
        SetColourDepthStencilSurface(&window->m_ColourRenderTexture.GetSurface(),&window->m_DepthStencilSurface);
        const U32 flags=window->m_ClearDepthStencil ? E_Clear_Colour|E_Clear_Depth|E_Clear_Stencil : E_Clear_Colour;
        Clear(flags,Maths::RGBA(0,0,0,255));
    }
    m_CurrentWindow=index;
}
void C_PCD3D11GameDevice::EndWindow(){m_CurrentWindow=-1;}
void C_PCD3D11GameDevice::RestoreWindow(S32 index,U32 colourWrite)
{
    if(index<0 || index>=m_Windows.Size())return;
    m_CurrentWindow=index;C_Window* window=m_Windows[index];
    window->m_ColourRenderTexture.Resolve();
    SetBlendMode(E_BlendMode_Solid);SetTextureGamma(0,E_Bool_False);
    ID3D11DeviceContext* context = nullptr;
    m_pd3dDevice->GetImmediateContext(&context);
    ID3D11RenderTargetView* target = nullptr;
    if (context) context->OMGetRenderTargets(1, &target, nullptr);
    if (target) target->Release();
    if (context) context->Release();
    static bool quadProbed = false;
    char probe[2] = {};
    if (!quadProbed && Blur2PCAutomation::Now() > 5.0 &&
        GetEnvironmentVariableA("BLUR_DX11_TEST_QUAD", probe, sizeof(probe)) && probe[0] == '1')
    {
        quadProbed = true;
        SetColourDepthStencilSurface();
        Quad::DrawFullScreenColour(Maths::C_RGBA_F32(1.0f, 0.0f, 1.0f, 1.0f));
        ID3D11DeviceContext* traceContext = nullptr;
        m_pd3dDevice->GetImmediateContext(&traceContext);
        if (traceContext)
        {
            ID3D11VertexShader* vertex = nullptr;
            ID3D11PixelShader* pixel = nullptr;
            ID3D11InputLayout* layout = nullptr;
            ID3D11RenderTargetView* colour = nullptr;
            ID3D11Buffer* buffer = nullptr;
            ID3D11BlendState* blend = nullptr;
            ID3D11DepthStencilState* depth = nullptr;
            UINT stride = 0, offset = 0, sampleMask = 0, stencilRef = 0;
            float blendFactor[4] = {};
            D3D11_VIEWPORT viewport = {};
            UINT viewportCount = 1;
            traceContext->VSGetShader(&vertex, nullptr, nullptr);
            traceContext->PSGetShader(&pixel, nullptr, nullptr);
            traceContext->IAGetInputLayout(&layout);
            traceContext->OMGetRenderTargets(1, &colour, nullptr);
            traceContext->IAGetVertexBuffers(0, 1, &buffer, &stride, &offset);
            traceContext->OMGetBlendState(&blend, blendFactor, &sampleMask);
            traceContext->OMGetDepthStencilState(&depth, &stencilRef);
            traceContext->RSGetViewports(&viewportCount, &viewport);
            D3D11_BLEND_DESC blendDesc = {};
            D3D11_DEPTH_STENCIL_DESC depthDesc = {};
            if (blend) blend->GetDesc(&blendDesc);
            if (depth) depth->GetDesc(&depthDesc);
            Blur2PCAutomation::LogF("DX11 restore quad VS=%p PS=%p IA=%p RTV=%p VB=%p stride=%u offset=%u VP=%.0fx%.0f depth=%u blend=%u src=%u dst=%u write=%u",
                vertex, pixel, layout, colour, buffer, stride, offset,
                viewport.Width, viewport.Height, depthDesc.DepthEnable,
                blendDesc.RenderTarget[0].BlendEnable,
                blendDesc.RenderTarget[0].SrcBlend,
                blendDesc.RenderTarget[0].DestBlend,
                blendDesc.RenderTarget[0].RenderTargetWriteMask);
            if (vertex) vertex->Release(); if (pixel) pixel->Release();
            if (layout) layout->Release(); if (colour) colour->Release();
            if (buffer) buffer->Release(); if (blend) blend->Release();
            if (depth) depth->Release(); traceContext->Release();
        }
        TakeD3D11BackBufferScreenshot("C:\\AIProjects\\Blur2PC\\Blur1PCDX11\\_work\\run\\scripts\\quadcolour.tga",
            m_BackBuffers[0]->m_ColourSurface.m_pd3dSurface);
    }
    // [Blur1PCDX11] Retail branches on the engine's current colour surface, not on what is bound:
    // onto the back buffer the window is a textured quad in the current viewport, onto any other
    // surface it is a copy (D3D9: StretchRect).
    {
        static unsigned mismatch = 0;
        const C_Surface* backSurface = &GetBackBuffer().m_ColourSurface;
        if (m_RenderTarget.m_pColourSurface != backSurface && mismatch++ < 4)
            Blur2PCAutomation::LogF("DX11 RestoreWindow surface mismatch engine=%p (tex=%p) back=%p (tex=%p) windows=%d first=%p",
                m_RenderTarget.m_pColourSurface,
                m_RenderTarget.m_pColourSurface ? m_RenderTarget.m_pColourSurface->m_pd3dSurface : nullptr,
                backSurface, backSurface->m_pd3dSurface, m_Windows.Size(),
                m_Windows.Size() > 0 ? &m_Windows[0]->m_ColourRenderTexture.GetSurface() : nullptr);
    }
    if (m_RenderTarget.m_pColourSurface == &GetBackBuffer().m_ColourSurface)
    {
        static unsigned quadPaths = 0;
        if (quadPaths++ % 60 == 0)
            Blur2PCAutomation::LogF("DX11 RestoreWindow quad paths=%u t=%.1f", quadPaths, Blur2PCAutomation::Now());
        Quad::DrawFullScreen(&window->m_ColourRenderTexture.GetTexture());
        return;
    }
    ID3D11Texture2D* source = window->m_ColourRenderTexture.GetTexture().GetGPUTexture2D();
    ID3D11Texture2D* destination = m_RenderTarget.m_pColourSurface ?
        m_RenderTarget.m_pColourSurface->m_pd3dSurface : nullptr;
    {
        static unsigned copyPaths = 0;
        if (copyPaths++ % 60 == 0)
            Blur2PCAutomation::LogF("DX11 RestoreWindow copy paths=%u source=%p destination=%p t=%.1f", copyPaths, source, destination, Blur2PCAutomation::Now());
    }
    if (!source || !destination || source == destination) return;
    D3D11_TEXTURE2D_DESC sourceDesc = {}, destinationDesc = {};
    source->GetDesc(&sourceDesc);
    destination->GetDesc(&destinationDesc);
    ID3D11DeviceContext* copyContext = nullptr;
    m_pd3dDevice->GetImmediateContext(&copyContext);
    if (!copyContext) return;
    if (sourceDesc.Width == destinationDesc.Width && sourceDesc.Height == destinationDesc.Height &&
        sourceDesc.Format == destinationDesc.Format)
        copyContext->CopyResource(destination, source);
    else
        Quad::DrawFullScreen(&window->m_ColourRenderTexture.GetTexture());
    copyContext->Release();
    {
        static unsigned reported = 0;
        if (reported < 4 && Blur2PCAutomation::Now() > 30.0)
        {
            ReportTextureStats("restore-source", source, reported);
            ReportTextureStats("restore-destination", destination, reported);
            ++reported;
        }
    }
}
void C_PCD3D11GameDevice::RenderWindowMask()
{
    if(m_CurrentWindow<0 || m_CurrentWindow>=m_Windows.Size())return;
    C_Window* window=m_Windows[m_CurrentWindow];window->m_ColourRenderTexture.Resolve();
    if(!window->m_MaskEnable)return;
    SetColourSurface(0,&window->m_MaskRenderTexture.GetSurface());
    Clear(E_Clear_Colour,Maths::RGBA(0,0,0,0));SetBlendMode(E_BlendMode_Solid);
    C_StencilState stencil;stencil.m_StencilEnable=E_Bool_True;
    stencil.m_StencilFunc=E_CmpFunc_Equal;stencil.m_StencilRef=0xff;
    stencil.m_StencilMask=0xff;stencil.m_StencilWriteMask=0;
    SetStencilState(stencil);SetDepthEnable(E_Bool_False);
    SetDepthWriteEnable(E_Bool_False);SetCullMode(E_CullMode_None);
    Quad::DrawVertexColour(Maths::RGBA(255,255,255,255));
    stencil.m_StencilEnable=E_Bool_False;SetStencilState(stencil);
    SetCullMode(E_CullMode_Default);SetDepthWriteEnable(E_Bool_True);SetDepthEnable(E_Bool_True);
    window->m_MaskRenderTexture.Resolve();
    SetColourSurface(0,&window->m_ColourRenderTexture.GetSurface());
}
} }

