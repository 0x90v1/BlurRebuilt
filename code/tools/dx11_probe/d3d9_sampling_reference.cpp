// [Blur1PCDX11] Standalone PC sampling authority; D3D9 never links into Amax.
#include <windows.h>
#include <d3d9.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <cstdio>
#include <cmath>
#include <cstring>

using Microsoft::WRL::ComPtr;

int main()
{
    HWND window = CreateWindowW(L"STATIC", L"PC sampling reference", WS_OVERLAPPED,
                                0, 0, 64, 64, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    ComPtr<IDirect3D9> api;
    api.Attach(Direct3DCreate9(D3D_SDK_VERSION));
    if (!window || !api) return 1;
    D3DPRESENT_PARAMETERS present = {};
    present.Windowed = TRUE;
    present.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present.hDeviceWindow = window;
    present.BackBufferWidth = present.BackBufferHeight = 4;
    present.BackBufferFormat = D3DFMT_UNKNOWN;
    ComPtr<IDirect3DDevice9> device;
    HRESULT result = api->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE, &present, &device);
    if (FAILED(result)) { std::fprintf(stderr, "CreateDevice %08lx\n", result); return 1; }
    const char* source = "sampler2D image:register(s0); float4 main(float2 uv:TEXCOORD0):COLOR0 { return tex2D(image,uv); }";
    ComPtr<ID3DBlob> code, errors;
    result = D3DCompile(source, std::strlen(source), nullptr, nullptr, nullptr, "main", "ps_3_0",
                         D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY, 0, &code, &errors);
    if (FAILED(result)) return 1;
    ComPtr<IDirect3DPixelShader9> shader;
    if (FAILED(device->CreatePixelShader(static_cast<const DWORD*>(code->GetBufferPointer()), &shader))) return 1;
    ComPtr<IDirect3DSurface9> target, readback;
    if (FAILED(device->CreateRenderTarget(4, 4, D3DFMT_A32B32G32R32F, D3DMULTISAMPLE_NONE, 0, FALSE, &target, nullptr)) ||
        FAILED(device->CreateOffscreenPlainSurface(4, 4, D3DFMT_A32B32G32R32F, D3DPOOL_SYSTEMMEM, &readback, nullptr))) return 1;
    device->SetRenderTarget(0, target.Get());
    device->SetDepthStencilSurface(nullptr);
    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
    device->SetPixelShader(shader.Get());
    device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
    device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    struct Vertex { float x,y,z,w,u,v; };
    const Vertex quad[] = {{-.5f,-.5f,0,1,.5f,.5f},{3.5f,-.5f,0,1,.5f,.5f},
                           {-.5f,3.5f,0,1,.5f,.5f},{3.5f,3.5f,0,1,.5f,.5f}};
    struct Test { D3DFORMAT format; const char* name; unsigned short data; float expected[4]; };
    const Test tests[] = {
        {D3DFMT_L8, "L8", 74, {74.f/255,74.f/255,74.f/255,1}},
        {D3DFMT_A8L8, "A8L8", 74 | (173 << 8), {74.f/255,74.f/255,74.f/255,173.f/255}},
        {D3DFMT_L16, "L16", 19018, {19018.f/65535,19018.f/65535,19018.f/65535,1}},
        {D3DFMT_A8, "A8", 173, {0,0,0,173.f/255}},
    };
    unsigned checked = 0;
    for (const auto& test : tests)
    {
        ComPtr<IDirect3DTexture9> texture;
        if (FAILED(device->CreateTexture(1,1,1,0,test.format,D3DPOOL_MANAGED,&texture,nullptr))) return 1;
        D3DLOCKED_RECT lock = {};
        if (FAILED(texture->LockRect(0,&lock,nullptr,0))) return 1;
        std::memcpy(lock.pBits,&test.data,test.format == D3DFMT_L16 || test.format == D3DFMT_A8L8 ? 2 : 1);
        texture->UnlockRect(0);
        device->SetTexture(0,texture.Get());
        for (unsigned gamma = 0; gamma != 2; ++gamma)
        {
            device->SetSamplerState(0,D3DSAMP_SRGBTEXTURE,gamma);
            if (FAILED(device->BeginScene())) return 1;
            result = device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,quad,sizeof(Vertex));
            device->EndScene();
            if (FAILED(result) || FAILED(device->GetRenderTargetData(target.Get(),readback.Get())) ||
                FAILED(readback->LockRect(&lock,nullptr,D3DLOCK_READONLY))) return 1;
            const float* rgba = static_cast<const float*>(lock.pBits);
            std::printf("D3D9 %s SRGB=%u RGBA %.8f %.8f %.8f %.8f\n",test.name,gamma,rgba[0],rgba[1],rgba[2],rgba[3]);
            bool valid = true;
            for (unsigned channel=0; channel<4; ++channel)
            {
                float expected = test.expected[channel];
                if (gamma && channel < 3 && (test.format == D3DFMT_L8 || test.format == D3DFMT_A8L8))
                    expected = expected <= .04045f ? expected/12.92f : std::pow((expected+.055f)/1.055f,2.4f);
                valid &= std::abs(rgba[channel]-expected) < .001f;
            }
            readback->UnlockRect();
            if (!valid) { std::fprintf(stderr,"Unexpected PC sampling contract\n"); return 1; }
            ++checked;
        }
    }
    // Hardware must decode each texel before interpolating; decoding an
    // already-filtered red channel would reproduce neither the PC RGB nor sRGB.
    const D3DFORMAT filteredFormats[] = {D3DFMT_L8,D3DFMT_A8L8};
    for (D3DFORMAT format : filteredFormats)
    {
        ComPtr<IDirect3DTexture9> texture;
        if (FAILED(device->CreateTexture(2,1,1,0,format,D3DPOOL_MANAGED,&texture,nullptr))) return 1;
        D3DLOCKED_RECT lock = {};
        if (FAILED(texture->LockRect(0,&lock,nullptr,0))) return 1;
        const unsigned char luma[] = {64,192}, alpha[] = {91,173};
        unsigned char* bytes = static_cast<unsigned char*>(lock.pBits);
        for (unsigned x=0; x<2; ++x)
        {
            bytes[x*(format==D3DFMT_L8 ? 1 : 2)] = luma[x];
            if (format==D3DFMT_A8L8) bytes[2*x+1] = alpha[x];
        }
        texture->UnlockRect(0);
        device->SetTexture(0,texture.Get());
        device->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);
        device->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
        for (unsigned gamma=0; gamma<2; ++gamma)
        {
            device->SetSamplerState(0,D3DSAMP_SRGBTEXTURE,gamma);
            if (FAILED(device->BeginScene())) return 1;
            result=device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,quad,sizeof(Vertex));
            device->EndScene();
            if (FAILED(result) || FAILED(device->GetRenderTargetData(target.Get(),readback.Get())) ||
                FAILED(readback->LockRect(&lock,nullptr,D3DLOCK_READONLY))) return 1;
            const float* rgba=static_cast<const float*>(lock.pBits);
            const float expected=gamma ? (std::pow((64.f/255+.055f)/1.055f,2.4f)+std::pow((192.f/255+.055f)/1.055f,2.4f))*.5f : 128.f/255;
            const float expectedAlpha=format==D3DFMT_L8 ? 1 : 132.f/255;
            std::printf("D3D9 filtered %u SRGB=%u RGBA %.8f %.8f %.8f %.8f\n",format,gamma,rgba[0],rgba[1],rgba[2],rgba[3]);
            bool valid=true;
            for (unsigned c=0; c<3; ++c) valid &= std::abs(rgba[c]-expected)<.001f;
            valid &= std::abs(rgba[3]-expectedAlpha)<.001f;
            readback->UnlockRect();
            if (!valid) return 1;
            ++checked;
        }
    }
    // Independent HAL authority for the Display2 encoded LOD and border state.
    ComPtr<IDirect3DTexture9> mipTexture;
    if (FAILED(device->CreateTexture(8,8,4,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&mipTexture,nullptr))) return 1;
    for (unsigned mip=0; mip<4; ++mip)
    {
        D3DLOCKED_RECT lock = {};
        if (FAILED(mipTexture->LockRect(mip,&lock,nullptr,0))) return 1;
        const unsigned edge=8>>mip;
        for (unsigned y=0; y<edge; ++y)
            for (unsigned x=0; x<edge; ++x)
                reinterpret_cast<DWORD*>(static_cast<unsigned char*>(lock.pBits)+y*lock.Pitch)[x] =
                    mip==0 ? (x<4 ? 0xffff0000 : 0xff0000ff) :
                    mip==1 ? 0xff00ff00 : mip==2 ? 0xff0000ff : 0xffffffff;
        mipTexture->UnlockRect(mip);
    }
    device->SetTexture(0,mipTexture.Get());
    device->SetSamplerState(0,D3DSAMP_SRGBTEXTURE,0);
    device->SetSamplerState(0,D3DSAMP_BORDERCOLOR,0x80402010);
    device->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
    struct SamplerTest { const char* name; D3DTEXTUREADDRESS address;
        D3DTEXTUREFILTERTYPE filter,mip; int bias; bool minify; float u; float expected[4]; };
    const SamplerTest samplerTests[] = {
        {"wrap",D3DTADDRESS_WRAP,D3DTEXF_POINT,D3DTEXF_POINT,0,false,1.25f,{1,0,0,1}},
        {"clamp",D3DTADDRESS_CLAMP,D3DTEXF_POINT,D3DTEXF_POINT,0,false,1.25f,{0,0,1,1}},
        {"mirror",D3DTADDRESS_MIRROR,D3DTEXF_POINT,D3DTEXF_POINT,0,false,1.25f,{0,0,1,1}},
        {"mirror once",D3DTADDRESS_MIRRORONCE,D3DTEXF_POINT,D3DTEXF_POINT,0,false,-.25f,{1,0,0,1}},
        {"border",D3DTADDRESS_BORDER,D3DTEXF_POINT,D3DTEXF_POINT,0,false,1.25f,{64.f/255,32.f/255,16.f/255,128.f/255}},
        {"linear mag",D3DTADDRESS_WRAP,D3DTEXF_LINEAR,D3DTEXF_POINT,0,false,.5f,{.5f,0,.5f,1}},
        {"mip one",D3DTADDRESS_WRAP,D3DTEXF_POINT,D3DTEXF_POINT,0,true,0,{0,1,0,1}},
        {"bias +16",D3DTADDRESS_WRAP,D3DTEXF_POINT,D3DTEXF_POINT,16,true,0,{0,0,1,1}},
        {"bias -16",D3DTADDRESS_WRAP,D3DTEXF_POINT,D3DTEXF_POINT,-16,true,0,{1,0,0,1}},
        {"bias +8 trilinear",D3DTADDRESS_WRAP,D3DTEXF_POINT,D3DTEXF_LINEAR,8,true,0,{0,.5f,.5f,1}},
        {"mips disabled",D3DTADDRESS_WRAP,D3DTEXF_POINT,D3DTEXF_NONE,32,true,0,{1,0,0,1}},
    };
    for (const auto& test:samplerTests)
    {
        device->SetSamplerState(0,D3DSAMP_ADDRESSU,test.address);
        device->SetSamplerState(0,D3DSAMP_MINFILTER,test.filter);
        device->SetSamplerState(0,D3DSAMP_MAGFILTER,test.filter);
        device->SetSamplerState(0,D3DSAMP_MIPFILTER,test.mip);
        const float bias=(static_cast<float>(test.bias)/128.f)*8.f;
        DWORD bits; std::memcpy(&bits,&bias,sizeof(bits));
        device->SetSamplerState(0,D3DSAMP_MIPMAPLODBIAS,bits);
        Vertex vertices[4]; std::memcpy(vertices,quad,sizeof(vertices));
        for (unsigned i=0;i<4;++i)
        {
            vertices[i].u=test.minify ? float(i&1) : test.u;
            vertices[i].v=test.minify ? float(i>>1) : .5f;
        }
        if (FAILED(device->BeginScene())) return 1;
        result=device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,vertices,sizeof(Vertex));
        device->EndScene();
        D3DLOCKED_RECT lock = {};
        if (FAILED(result) || FAILED(device->GetRenderTargetData(target.Get(),readback.Get())) ||
            FAILED(readback->LockRect(&lock,nullptr,D3DLOCK_READONLY))) return 1;
        const float* rgba=static_cast<const float*>(lock.pBits);
        std::printf("D3D9 sampler %s RGBA %.6f %.6f %.6f %.6f\n",test.name,rgba[0],rgba[1],rgba[2],rgba[3]);
        bool valid=true;
        for (unsigned c=0;c<4;++c) valid &= std::abs(rgba[c]-test.expected[c])<.002f;
        readback->UnlockRect();
        if (!valid) return 1;
        ++checked;
    }
    device->SetTexture(0,nullptr);
    // [Blur1PCDX11] The loaded foliage prepass performs rcp(0), then mul(0).
    // Measure the PC arithmetic separately from the SM5 NaN rules.
    const char* depthSource = "float4 value:register(c0); float4 main():COLOR0 { return value.xxzz * (1 / value.yyww); }";
    code.Reset(); errors.Reset(); shader.Reset();
    result = D3DCompile(depthSource, std::strlen(depthSource), nullptr, nullptr, nullptr,
        "main", "ps_3_0", D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY, 0, &code, &errors);
    if (FAILED(result) || FAILED(device->CreatePixelShader(static_cast<const DWORD*>(code->GetBufferPointer()),&shader))) return 1;
    ComPtr<ID3DBlob> assembly;
    if (FAILED(D3DDisassemble(code->GetBufferPointer(),code->GetBufferSize(),0,nullptr,&assembly))) return 1;
    const char* instructions = static_cast<const char*>(assembly->GetBufferPointer());
    if (!std::strstr(instructions,"rcp") || !std::strstr(instructions,"mul")) return 1;
    std::printf("D3D9 depth reference shader:\n%s\n",instructions);
    device->SetPixelShader(shader.Get());
    const float operands[][4] = {{0,0,.25f,2},{0,1,.25f,1}};
    for (const auto& values : operands)
    {
        if (FAILED(device->SetPixelShaderConstantF(0,values,1)) || FAILED(device->BeginScene())) return 1;
        result = device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,quad,sizeof(Vertex));
        device->EndScene();
        D3DLOCKED_RECT lock = {};
        if (FAILED(result) || FAILED(device->GetRenderTargetData(target.Get(),readback.Get())) ||
            FAILED(readback->LockRect(&lock,nullptr,D3DLOCK_READONLY))) return 1;
        const float* rgba = static_cast<const float*>(lock.pBits);
        const float expected[] = {0,0,values[2]/values[3],values[2]/values[3]};
        bool valid = true;
        for (unsigned c=0;c<4;++c) valid &= std::isfinite(rgba[c]) && std::abs(rgba[c]-expected[c])<1e-6f;
        std::printf("D3D9 depth %g * rcp(%g): RGBA %.8g %.8g %.8g %.8g\n",values[0],values[1],rgba[0],rgba[1],rgba[2],rgba[3]);
        readback->UnlockRect();
        if (!valid) return 1;
        ++checked;
    }
    // [Blur1PCDX11] Retail ScrollFade declares a cube but can retain the
    // FX system's 2D default. PC returns zero RGBA for that dimension mismatch.
    const char* cubeSource = "samplerCUBE image:register(s0); float4 direction:register(c0); float4 main():COLOR0 { return texCUBEbias(image,direction); }";
    code.Reset(); errors.Reset(); shader.Reset();
    result = D3DCompile(cubeSource, std::strlen(cubeSource), nullptr, nullptr, nullptr,
        "main", "ps_3_0", D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY, 0, &code, &errors);
    if (FAILED(result) || FAILED(device->CreatePixelShader(static_cast<const DWORD*>(code->GetBufferPointer()),&shader))) return 1;
    device->SetPixelShader(shader.Get());
    const DWORD values[] = {0,0xffffffff,0x99663322};
    for(unsigned kind=0;kind<3;++kind) for(DWORD value:values)
    {
        if (kind == 0 && value != 0) continue;
        ComPtr<IDirect3DTexture9> image;
        ComPtr<IDirect3DCubeTexture9> cube;
        D3DLOCKED_RECT lock={};
        if(kind==1) {
            if(FAILED(device->CreateTexture(1,1,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&image,nullptr)))return 1;
            image->LockRect(0,&lock,nullptr,0); memcpy(lock.pBits,&value,4);image->UnlockRect(0);
        }
        if(kind==2) {
            if(FAILED(device->CreateCubeTexture(1,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&cube,nullptr)))return 1;
            for(unsigned face=0;face<6;++face){cube->LockRect((D3DCUBEMAP_FACES)face,0,&lock,nullptr,0);memcpy(lock.pBits,&value,4);cube->UnlockRect((D3DCUBEMAP_FACES)face,0);}
        }
        HRESULT bound=device->SetTexture(0,kind==1?static_cast<IDirect3DBaseTexture9*>(image.Get()):static_cast<IDirect3DBaseTexture9*>(cube.Get()));
        for(unsigned direction=0;direction<6;++direction) {
            float vector[4]={0,0,0,0};vector[direction/2]=direction%2?-1.f:1.f;
            device->SetPixelShaderConstantF(0,vector,1);
            device->Clear(0,nullptr,D3DCLEAR_TARGET,0xffff00ff,1,0);
            if(FAILED(device->BeginScene()))return 1;
            HRESULT drawn=device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,quad,sizeof(Vertex));device->EndScene();
            if(FAILED(device->GetRenderTargetData(target.Get(),readback.Get())) || FAILED(readback->LockRect(&lock,nullptr,D3DLOCK_READONLY))) return 1;
            auto p=static_cast<const float*>(lock.pBits);
            printf("kind=%u value=%08lx direction=%u bind=%08lx draw=%08lx RGBA=%.9g %.9g %.9g %.9g\n",kind,value,direction,bound,drawn,p[0],p[1],p[2],p[3]);
            bool valid = SUCCEEDED(bound) && SUCCEEDED(drawn);
            const float expected[] = {kind == 2 ? ((value >> 16) & 255)/255.f : 0,
                kind == 2 ? ((value >> 8) & 255)/255.f : 0,
                kind == 2 ? (value & 255)/255.f : 0,
                kind == 2 ? ((value >> 24) & 255)/255.f : kind == 0 ? 1.f : 0.f};
            for (unsigned channel=0;channel<4;++channel) valid &= std::abs(p[channel]-expected[channel])<1e-6f;
            readback->UnlockRect();
            if (!valid) return 1;
            ++checked;
        }
    }
    device.Reset();
    DestroyWindow(window);
    std::printf("D3D9 sampling reference: %u GPU checks passed\n",checked);
    return 0;
}
