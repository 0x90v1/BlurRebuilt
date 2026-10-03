// [Blur1PCDX11] Retained views must match fresh views across resource/gamma changes.
#include "Display/Device/D3D11/D3D11Device.h"
#include "Display/Device/D3D11/D3D11Effect.h"
#include "Display/Device/D3D11/D3D11Formats.h"
#include <d3d11sdklayers.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using Microsoft::WRL::ComPtr;

static bool CheckViews(Display::D3D11::C_D3D11Device& device, unsigned kind)
{
    const char* types[] = {"Texture2D", "Texture3D", "TextureCube", "Texture2DArray", "TextureCubeArray", "Texture2D"};
    const char* coords[] = {"float2(.5,.5)", "float3(.5,.5,.5)", "float3(1,0,0)", "float3(.5,.5,0)", "float4(1,0,0,0)", "float2(.5,.5)"};
    std::string source = std::string(types[kind]) + " sampleTexture;\n" + R"(
SamplerState pointSampler { Filter=MIN_MAG_MIP_POINT; AddressU=Clamp; AddressV=Clamp; AddressW=Clamp; };
float4 VS(uint id:SV_VertexID):SV_Position { return float4(id==2?3:-1,id==1?3:-1,0,1); }
float4 PS():SV_Target { return sampleTexture.Sample(pointSampler,)" + coords[kind] + R"(); }
technique11 Material { pass p0 {
 SetVertexShader(CompileShader(vs_5_0,VS())); SetPixelShader(CompileShader(ps_5_0,PS()));
} }
)";
    ComPtr<ID3DBlob> bytecode, errors;
    if (FAILED(D3DCompile(source.data(),source.size(),nullptr,nullptr,nullptr,nullptr,"fx_5_0",0,0,&bytecode,&errors)))
    { if (errors) std::fprintf(stderr,"%s",static_cast<const char*>(errors->GetBufferPointer())); return false; }
    Display::D3D11::C_D3D11Effect effect;
    if (FAILED(effect.LoadMemory(device.Device(),bytecode->GetBufferPointer(),bytecode->GetBufferSize()))) return false;
    ComPtr<ID3D11Resource> textures[2];
    const unsigned char pixels[2][4] = {{128,64,240,255},{24,80,176,255}};
    const float scalarPixels[] = {.25f,.75f};
    for (unsigned i=0;i<2;++i)
    {
        D3D11_SUBRESOURCE_DATA data[12] = {};
        for (auto& face : data) { face.pSysMem=kind==5?static_cast<const void*>(&scalarPixels[i]):pixels[i]; face.SysMemPitch=4; face.SysMemSlicePitch=4; }
        if (kind==1)
        {
            D3D11_TEXTURE3D_DESC desc = {};
            desc.Width=desc.Height=desc.Depth=desc.MipLevels=1;
            desc.Format=DXGI_FORMAT_R8G8B8A8_TYPELESS; desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
            ComPtr<ID3D11Texture3D> texture;
            if (FAILED(device.Device()->CreateTexture3D(&desc,data,&texture))) return false;
            textures[i]=texture;
        }
        else
        {
            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width=desc.Height=desc.MipLevels=desc.SampleDesc.Count=1;
            desc.ArraySize=kind==2?6:kind==3?2:kind==4?12:1;
            desc.Format=kind==5?DXGI_FORMAT_R32_FLOAT:DXGI_FORMAT_R8G8B8A8_TYPELESS; desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
            desc.MiscFlags=kind==2||kind==4?D3D11_RESOURCE_MISC_TEXTURECUBE:0;
            ComPtr<ID3D11Texture2D> texture;
            if (FAILED(device.Device()->CreateTexture2D(&desc,data,&texture))) return false;
            textures[i]=texture;
        }
    }
    ComPtr<ID3D11Texture2D> target, staging;
    ComPtr<ID3D11RenderTargetView> rtv;
    ComPtr<ID3D11ShaderResourceView> targetView;
    if (FAILED(device.CreateRenderTexture2D(1,1,1,1,DXGI_FORMAT_R32G32B32A32_FLOAT,target,rtv,targetView))) return false;
    D3D11_TEXTURE2D_DESC targetDesc = {};
    target->GetDesc(&targetDesc); targetDesc.Usage=D3D11_USAGE_STAGING;
    targetDesc.BindFlags=0; targetDesc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    if (FAILED(device.Device()->CreateTexture2D(&targetDesc,nullptr,&staging))) return false;
    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode=D3D11_FILL_SOLID; rasterDesc.CullMode=D3D11_CULL_NONE;
    ComPtr<ID3D11RasterizerState> raster;
    if (FAILED(device.Device()->CreateRasterizerState(&rasterDesc,&raster))) return false;
    auto* context=device.Context();
    const D3D11_VIEWPORT viewport={0,0,1,1,0,1};
    auto render = [&](ID3D11ShaderResourceView* view, float* pixel) {
        if (FAILED(effect.SetTexture("sampleTexture",view)) || FAILED(effect.Apply("Material",0,context))) return false;
        context->RSSetState(raster.Get()); context->RSSetViewports(1,&viewport);
        auto* raw=rtv.Get(); context->OMSetRenderTargets(1,&raw,nullptr);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->Draw(3,0); context->CopyResource(staging.Get(),target.Get());
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (FAILED(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped))) return false;
        std::memcpy(pixel,mapped.pData,4*sizeof(float)); context->Unmap(staging.Get(),0);
        return true;
    };
    context->ClearState();
    const unsigned resources[]={0,0,0,0,1,1,1,1,0,0,0,0,0,0};
    const bool gamma[]={false,false,true,true,true,true,false,false,false,false,false,false,false,false};
    for (unsigned step=0;step<14;++step)
    {
        if (step==10 && FAILED(effect.SetTexture("sampleTexture",nullptr))) return false;
        if (step==12 && FAILED(effect.LoadMemory(device.Device(),bytecode->GetBufferPointer(),bytecode->GetBufferSize()))) return false;
        ComPtr<ID3D11ShaderResourceView> view, fresh;
        const HRESULT result=effect.CreateTextureView("sampleTexture",device.Device(),textures[resources[step]].Get(),gamma[step],&view);
        if (result != (step%2 || (kind==5 && (step==2 || step==6)) ? S_FALSE : S_OK))
        {
            std::fprintf(stderr,"%s step%u unexpected reuse result %08x\n",types[kind],step,static_cast<unsigned>(result));
            return false;
        }
        if (FAILED(Display::D3D11::CreateTextureView(device.Device(),textures[resources[step]].Get(),gamma[step],&fresh))) return false;
        float reused[4], expected[4];
        if (!render(view.Get(),reused) || !render(fresh.Get(),expected) || std::memcmp(reused,expected,sizeof(reused))!=0) return false;
    }
    context->ClearState();
    std::printf("%s: 14 retained/fresh GPU view comparisons passed\n",types[kind]);
    return true;
}

int main()
{
    Display::D3D11::C_D3D11Device device;
    if (FAILED(device.Initialise())) return 1;
    ComPtr<ID3D11InfoQueue> queue;
    device.Device()->QueryInterface(IID_PPV_ARGS(&queue));
    if (queue) queue->ClearStoredMessages();
    for (unsigned kind=0;kind<6;++kind) if (!CheckViews(device,kind)) return 1;
    if (queue) for (UINT64 i=0;i<queue->GetNumStoredMessages();++i)
    {
        SIZE_T size=0; queue->GetMessage(i,nullptr,&size);
        std::vector<unsigned char> bytes(size);
        auto* message=reinterpret_cast<D3D11_MESSAGE*>(bytes.data());
        if (SUCCEEDED(queue->GetMessage(i,message,&size)) && message->Severity<=D3D11_MESSAGE_SEVERITY_ERROR)
        { std::fprintf(stderr,"Texture view GPU error: %s\n",message->pDescription); return 1; }
    }
    return 0;
}
