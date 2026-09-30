// [Blur1PCDX11] Verifies actual D3D11 device creation, shaders, draw and readback.
#include "Display/Device/D3D11/D3D11Device.h"
#include "Display/Device/D3D11/D3D11Effect.h"
#include "Display/Device/D3D11/D3D11EffectLibrary.h"
#include "Display/Device/D3D11/D3D11InputLayout.h"
#include "Display/Device/D3D11/D3D11States.h"
#include <cstdio>
#include <string>
#include <cmath>
#include <vector>
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include "polygon_font_checks.h"

// [Blur1PCDX11] D3D11 stage linkage requires register/component agreement,
// even when the original D3D9 stages shared matching semantic names.
static bool CheckBuiltinShaderLinks()
{
    std::vector<std::pair<std::wstring, std::wstring>> pairs;
    for (const wchar_t* suffix : {L"", L"R", L"G", L"B", L"A", L"HDR"})
        pairs.emplace_back(L"render/vsScreenBlobShadows", std::wstring(L"render/psScreenBlobShadows") + suffix);
    pairs.emplace_back(L"ui/vsMultiTexture1", L"ui/psAMLoading");
    for (const wchar_t* suffix : {L"", L"Projected", L"Volume"})
        pairs.emplace_back(L"deferred/vsSpotlightPrepass", std::wstring(L"deferred/psSpotlightPrepass") + suffix);
    for (unsigned count = 1; count <= 8; ++count)
        for (const wchar_t* suffix : {L"", L"HDR"})
            pairs.emplace_back(L"dropshadows/vsDropShadowsViewNormal",
                L"dropshadows/psDropShadowsViewNormal" + std::to_wstring(count) + suffix);
    for (const wchar_t* prefix : {L"deferred/psDynamicLightsPrepass", L"deferred/psDynamicLightsPrepass2xMSAA"})
        for (unsigned count = 1; count <= 8; ++count)
            pairs.emplace_back(L"deferred/vsDynamicLightsPrepass", std::wstring(prefix) + std::to_wstring(count));
    for (const auto& pair : pairs)
    {
        Microsoft::WRL::ComPtr<ID3DBlob> vb, pb;
        const std::wstring root = L"_work/build/gen/dx11_builtins/";
        if (FAILED(D3DReadFileToBlob((root + pair.first + L".cso").c_str(), &vb)) ||
            FAILED(D3DReadFileToBlob((root + pair.second + L".cso").c_str(), &pb))) return false;
        Microsoft::WRL::ComPtr<ID3D11ShaderReflection> vs, ps;
        if (FAILED(D3DReflect(vb->GetBufferPointer(), vb->GetBufferSize(), IID_PPV_ARGS(&vs))) ||
            FAILED(D3DReflect(pb->GetBufferPointer(), pb->GetBufferSize(), IID_PPV_ARGS(&ps)))) return false;
        D3D11_SHADER_DESC vd = {}, pd = {};
        vs->GetDesc(&vd); ps->GetDesc(&pd);
        for (UINT i = 0; i < pd.InputParameters; ++i)
        {
            D3D11_SIGNATURE_PARAMETER_DESC input = {};
            ps->GetInputParameterDesc(i, &input);
            if (!input.ReadWriteMask) continue;
            bool matched = false;
            for (UINT j = 0; j < vd.OutputParameters; ++j)
            {
                D3D11_SIGNATURE_PARAMETER_DESC output = {};
                vs->GetOutputParameterDesc(j, &output);
                if (_stricmp(input.SemanticName, output.SemanticName) == 0 &&
                    input.SemanticIndex == output.SemanticIndex && input.Register == output.Register &&
                    input.ComponentType == output.ComponentType &&
                    (output.Mask & input.ReadWriteMask) == input.ReadWriteMask) matched = true;
            }
            if (!matched)
            {
                std::fprintf(stderr, "Shader linkage failed: %ls / %ls, %s%u register %u\n",
                    pair.first.c_str(), pair.second.c_str(), input.SemanticName, input.SemanticIndex, input.Register);
                return false;
            }
        }
    }
    std::printf("Built-in shader linkage: %zu stage pairs checked\n", pairs.size());
    return true;
}

static bool CheckNativeAlphaTest(Display::D3D11::C_D3D11Device& device)
{
    struct AlphaState { UINT enabled, function; float reference; UINT padding; };
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(AlphaState);
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
    if (FAILED(device.Device()->CreateBuffer(&desc, nullptr, &buffer))) return false;
    const unsigned char alphas[] = {0, 127, 128, 129, 255};
    const unsigned expectedMasks[] = {0x00, 0x03, 0x04, 0x07, 0x18, 0x1b, 0x1c, 0x1f, 0x1f};
    for (UINT test = 0; test < 9; ++test)
        for (UINT pixel = 0; pixel < 5; ++pixel)
        {
            const AlphaState state = {test == 8 ? 0U : 1U, test == 8 ? 1U : test + 1, 128.f / 255.f, 0};
            device.Context()->UpdateSubresource(buffer.Get(), 0, nullptr, &state, 0, 0);
            ID3D11Buffer* raw = buffer.Get();
            device.Context()->PSSetConstantBuffers(13, 1, &raw);
            const unsigned char texel[] = {200, 150, 20, alphas[pixel]};
            const HRESULT hr = device.DrawTexturedQuad(
                L"_work/build/gen/dx11_builtins/display/vsBasicRenderTextured2D.cso",
                L"_work/build/gen/dx11_builtins/display/psBasicRenderTextured.cso", texel);
            unsigned char rgba[4] = {};
            if (FAILED(hr) || FAILED(device.ReadPixel(32, 32, rgba))) return false;
            const bool expected = (expectedMasks[test] & (1U << pixel)) != 0;
            if (rgba[0] != (expected ? 200 : 0))
            {
                std::fprintf(stderr, "Alpha test failed: enabled=%u func=%u alpha=%u red=%u expectedPass=%u\n",
                    state.enabled, state.function, alphas[pixel], rgba[0], expected);
                return false;
            }
        }
    std::printf("Native alpha test: 45 GPU pixel checks passed (all comparisons, threshold, disabled)\n");
    return true;
}

static bool CheckAlphaBlendFactors(Display::D3D11::C_D3D11Device& device)
{
    const D3DBLEND factors[] = {D3DBLEND_SRCCOLOR, D3DBLEND_INVSRCCOLOR,
        D3DBLEND_DESTCOLOR, D3DBLEND_INVDESTCOLOR, D3DBLEND_SRCALPHASAT};
    const unsigned char expected[][4] = {{157,39,10,64}, {43,61,40,64},
        {0,0,0,128}, {200,100,50,0}, {0,0,0,128}};
    const unsigned char texel[] = {200,100,50,128};
    for (unsigned test = 0; test < 5; ++test)
    {
        D3D11_BLEND_DESC desc = {};
        if (!Display::D3D11::MakeBlendDesc(factors[test], D3DBLEND_ZERO, D3DBLENDOP_ADD,
            factors[test], D3DBLEND_ZERO, D3DBLENDOP_ADD, true, 15, &desc)) return false;
        Microsoft::WRL::ComPtr<ID3D11BlendState> blend;
        if (FAILED(device.Device()->CreateBlendState(&desc, &blend))) return false;
        const float scale[] = {1,1,1,1};
        device.Context()->OMSetBlendState(blend.Get(), scale, 0xffffffff);
        if (FAILED(device.DrawTexturedQuad(
            L"_work/build/gen/dx11_builtins/display/vsBasicRenderTextured2D.cso",
            L"_work/build/gen/dx11_builtins/display/psBasicRenderTextured.cso", texel))) return false;
        unsigned char actual[4] = {};
        if (FAILED(device.ReadPixel(32, 32, actual))) return false;
        for (unsigned channel = 0; channel < 4; ++channel)
            if (std::abs(int(actual[channel]) - int(expected[test][channel])) > 1)
            {
                std::fprintf(stderr, "Blend factor %u channel %u: %u expected %u\n",
                    factors[test], channel, actual[channel], expected[test][channel]);
                return false;
            }
    }
    std::puts("PC alpha blend factors: 5 GPU RGBA checks passed");
    return true;
}

int main()
{
    Display::D3D11::C_D3D11Device device;
    HRESULT result = device.Initialise();
    if (FAILED(result)) return 1;
    if (!CheckPackedPolygonFont(device)) return 10;
    UINT packedColourSupport = 0;
    result = device.Device()->CheckFormatSupport(DXGI_FORMAT_B8G8R8A8_UNORM,
                                                  &packedColourSupport);
    if (FAILED(result)) return 1;
    std::printf("BGRA packed vertex colour input: %s\n",
                (packedColourSupport & D3D11_FORMAT_SUPPORT_IA_VERTEX_BUFFER) ?
                "supported" : "requires native upload conversion");

    const unsigned char volumeTexels[32] = {
        0, 0, 0, 255, 32, 0, 0, 255, 64, 0, 0, 255, 96, 0, 0, 255,
        128, 0, 0, 255, 160, 0, 0, 255, 192, 0, 0, 255, 224, 0, 0, 255
    };
    const void* volumeMips[] = { volumeTexels };
    const UINT volumeSizes[] = { sizeof(volumeTexels) };
    Microsoft::WRL::ComPtr<ID3D11Texture3D> volumeTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> volumeView;
    result = device.CreateTexture3DFromPCData(2, 2, 2, 1, D3DFMT_A8R8G8B8,
                                               volumeMips, volumeSizes,
                                               volumeTexture, volumeView);
    if (FAILED(result)) return 1;
    D3D11_SHADER_RESOURCE_VIEW_DESC volumeViewDesc = {};
    volumeView->GetDesc(&volumeViewDesc);
    if (volumeViewDesc.ViewDimension != D3D11_SRV_DIMENSION_TEXTURE3D) return 1;

    const unsigned char cubeFaces[6][4] = {
        { 255, 0, 0, 255 }, { 0, 255, 0, 255 }, { 0, 0, 255, 255 },
        { 255, 255, 0, 255 }, { 255, 0, 255, 255 }, { 0, 255, 255, 255 }
    };
    const void* cubeMips[6] = { cubeFaces[0], cubeFaces[1], cubeFaces[2],
                                 cubeFaces[3], cubeFaces[4], cubeFaces[5] };
    const UINT cubeSizes[6] = { 4, 4, 4, 4, 4, 4 };
    Microsoft::WRL::ComPtr<ID3D11Texture2D> cubeTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cubeView;
    result = device.CreateTextureCubeFromPCData(1, 1, D3DFMT_A8R8G8B8,
                                                 cubeMips, cubeSizes, cubeTexture,
                                                 cubeView);
    if (FAILED(result)) return 1;
    D3D11_SHADER_RESOURCE_VIEW_DESC cubeViewDesc = {};
    cubeView->GetDesc(&cubeViewDesc);
    if (cubeViewDesc.ViewDimension != D3D11_SRV_DIMENSION_TEXTURECUBE) return 1;

    Display::D3D11::C_D3D11EffectLibrary effectLibrary;
    result = effectLibrary.Open(L"_work/build/gen/fx11/effects");
    if (FAILED(result) || effectLibrary.EffectCount() != 1254) return 9;
    std::string failedEffect;
    result = effectLibrary.PreloadAll(device.Device(), &failedEffect);
    if (FAILED(result))
    {
        std::fprintf(stderr, "FX11 effect %s failed to load: 0x%08lx\n",
                     failedEffect.c_str(), static_cast<unsigned long>(result));
        return 9;
    }
    Display::D3D11::C_D3D11Effect* gameEffect = nullptr;
    result = effectLibrary.Get(device.Device(), "ScreenFacingSprite.fx", &gameEffect);
    if (FAILED(result) || !gameEffect) return 9;
    const void* materialSignature = nullptr;
    SIZE_T materialSignatureBytes = 0;
    result = gameEffect->GetPassInputSignature("Material", 0,
                                               &materialSignature, &materialSignatureBytes);
    if (FAILED(result) || !materialSignatureBytes) return 9;
    std::printf("FX11 indexed %zu game effects; Material pass has %zu input-signature bytes\n",
                effectLibrary.EffectCount(), materialSignatureBytes);
    // [Blur1PCDX11] FXB/render stages own fixed-function state. Reapplying FX11
    // for immediate parameters must preserve reverse depth, stencil, blend
    // masks/factors and raster overrides, even for authored ZWriteEnable passes.
    Microsoft::WRL::ComPtr<ID3D11BlendState> gameBlend;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> gameDepth;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> gameRaster;
    D3D11_BLEND_DESC blendDesc = {};
    D3D11_DEPTH_STENCIL_DESC depthDesc = {};
    D3D11_RASTERIZER_DESC rasterDesc = {};
    if (!Display::D3D11::MakeBlendDesc(D3DBLEND_ZERO, D3DBLEND_SRCCOLOR, D3DBLENDOP_ADD,
        D3DBLEND_ONE, D3DBLEND_ZERO, D3DBLENDOP_ADD, true, 5, &blendDesc) ||
        !Display::D3D11::MakeDepthStencilDesc(true, false, D3DCMP_GREATEREQUAL,
        true, D3DCMP_EQUAL, D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE,
        0x7f, 0x3f, &depthDesc) ||
        !Display::D3D11::MakeRasterizerDesc(D3DFILL_WIREFRAME, D3DCULL_CW, true, &rasterDesc) ||
        FAILED(device.Device()->CreateBlendState(&blendDesc, &gameBlend)) ||
        FAILED(device.Device()->CreateDepthStencilState(&depthDesc, &gameDepth)) ||
        FAILED(device.Device()->CreateRasterizerState(&rasterDesc, &gameRaster))) return 9;
    const FLOAT expectedFactor[4] = {.25f, .5f, .75f, 1.f};
    device.Context()->OMSetBlendState(gameBlend.Get(), expectedFactor, 0x1357);
    device.Context()->OMSetDepthStencilState(gameDepth.Get(), 37);
    device.Context()->RSSetState(gameRaster.Get());
    const char* stateEffects[] = {"MeshEffect.fx", "Puddle_LN_NV.fx"};
    const char* stateTechniques[] = {"Material", "DefaultLights"};
    for (unsigned i = 0; i < 2; ++i)
    {
        Display::D3D11::C_D3D11Effect* stateEffect = nullptr;
        if (FAILED(effectLibrary.Get(device.Device(), stateEffects[i], &stateEffect))) return 9;
        for (unsigned repeat = 0; repeat < 2; ++repeat)
        {
            if (FAILED(stateEffect->Apply(stateTechniques[i], 0, device.Context()))) return 9;
            Microsoft::WRL::ComPtr<ID3D11BlendState> actualBlend;
            Microsoft::WRL::ComPtr<ID3D11DepthStencilState> actualDepth;
            Microsoft::WRL::ComPtr<ID3D11RasterizerState> actualRaster;
            FLOAT actualFactor[4] = {};
            UINT actualMask = 0, actualReference = 0;
            device.Context()->OMGetBlendState(&actualBlend, actualFactor, &actualMask);
            device.Context()->OMGetDepthStencilState(&actualDepth, &actualReference);
            device.Context()->RSGetState(&actualRaster);
            if (actualBlend.Get() != gameBlend.Get() || actualDepth.Get() != gameDepth.Get() ||
                actualRaster.Get() != gameRaster.Get() || actualMask != 0x1357 || actualReference != 37 ||
                memcmp(actualFactor, expectedFactor, sizeof(expectedFactor)))
            { std::fprintf(stderr, "FX11 overwrote engine state: %s repeat=%u\n", stateEffects[i], repeat); return 9; }
        }
    }
    std::printf("FX11 engine state preserved across MeshEffect/Puddle and immediate reapply\n");
    device.Context()->ClearState();
    Display::D3D11::C_D3D11Effect* alphaEffect = nullptr;
    result = effectLibrary.Get(device.Device(), "AutoScrollFade.fx", &alphaEffect);
    if (FAILED(result) || !alphaEffect) return 9;
    const void* alphaBytecode = nullptr;
    SIZE_T alphaBytes = 0;
    result = alphaEffect->GetPassPixelShaderBytecode("Material", 0,
                                                     &alphaBytecode, &alphaBytes);
    Microsoft::WRL::ComPtr<ID3DBlob> alphaAssembly;
    if (SUCCEEDED(result)) result = D3DDisassemble(alphaBytecode, alphaBytes, 0,
                                                    nullptr, &alphaAssembly);
    if (FAILED(result) || !alphaAssembly ||
        std::string(static_cast<const char*>(alphaAssembly->GetBufferPointer()),
                    alphaAssembly->GetBufferSize()).find("discard") == std::string::npos)
        return 9;
    if (SUCCEEDED(result)) result = device.CreateOffscreenTarget(64, 64);
    if (FAILED(result)) return 10;

    // Draw through an FX5 pass compiled from the game's authored material.
    const float identity[16] = { 1, 0, 0, 0, 0, 1, 0, 0,
                                 0, 0, 1, 0, 0, 0, 0, 1 };
    const char* matrixNames[] = { "g_mWorld", "g_mView", "g_mViewProj", "g_mProj" };
    for (const char* name : matrixNames)
    {
        if (gameEffect->HasParameter(name))
        {
            result = gameEffect->SetRawParameter(name, identity, sizeof(identity));
            if (FAILED(result)) return 10;
        }
    }
    if (gameEffect->HasParameter("g_lodFade"))
    {
        const float fade[4] = { 1, 1, 1, 1 };
        result = gameEffect->SetRawParameter("g_lodFade", fade, sizeof(fade));
        if (FAILED(result)) return 10;
    }
    const unsigned char whiteBGRA[4] = { 255, 255, 255, 255 };
    const void* whiteMip[] = { whiteBGRA };
    const UINT whiteMipSize[] = { sizeof(whiteBGRA) };
    Microsoft::WRL::ComPtr<ID3D11Texture2D> whiteTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> whiteView;
    bool materialAlbedoSRGB = false;
    result = effectLibrary.TextureUsesSRGB("ScreenFacingSprite.fx", "g_tAlbedo",
                                            &materialAlbedoSRGB);
    if (FAILED(result) || !materialAlbedoSRGB) return 10;
    result = device.CreateTexture2DFromPCData(1, 1, 1, D3DFMT_A8R8G8B8,
                                              whiteMip, whiteMipSize,
                                              whiteTexture, whiteView,
                                              materialAlbedoSRGB);
    if (FAILED(result)) return 10;
    if (FAILED(gameEffect->SetTexture("g_tAlbedo", whiteView.Get()))) return 10;

    const D3DVERTEXELEMENT9 materialElements[] = {
        { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
        { 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        { 0, 32, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
        { 0, 48, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 3 }
    };
    struct MaterialVertex { float x, y, z, nx, ny, nz, u, v, r, g, b, a, px, py; };
    const MaterialVertex materialVertices[] = {
        { 0, 0.8f, 0.2f, 0, 0, 1, 0.5f, 0, 1, 1, 1, 1, 0, 0 },
        { 0.8f, -0.8f, 0.2f, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
        { -0.8f, -0.8f, 0.2f, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 0 }
    };
    const unsigned short materialIndices[] = { 0, 1, 2 };
    Microsoft::WRL::ComPtr<ID3D11Buffer> materialVB, materialIB;
    result = device.CreateVertexBuffer(sizeof(materialVertices), materialVertices, false,
                                       materialVB);
    if (SUCCEEDED(result)) result = device.CreateIndexBuffer(
        sizeof(materialIndices), materialIndices, false, materialIB);
    if (FAILED(result)) return 10;
    D3D11_RASTERIZER_DESC materialRaster = {};
    if (!Display::D3D11::MakeRasterizerDesc(D3DFILL_SOLID, D3DCULL_NONE, false,
                                           &materialRaster)) return 10;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> materialRasterState;
    result = device.Device()->CreateRasterizerState(&materialRaster, &materialRasterState);
    if (FAILED(result)) return 10;
    const float materialClear[4] = { 0, 0, 0, 1 };
    device.Context()->ClearRenderTargetView(device.RenderTargetView(), materialClear);
    device.Context()->ClearDepthStencilView(device.DepthStencilView(),
        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
    ID3D11RenderTargetView* materialTarget = device.RenderTargetView();
    device.Context()->OMSetRenderTargets(1, &materialTarget, device.DepthStencilView());
    D3D11_VIEWPORT materialViewport = { 0, 0, 64, 64, 0, 1 };
    device.Context()->RSSetViewports(1, &materialViewport);
    device.Context()->RSSetState(materialRasterState.Get());
    result = device.DrawGameEffectIndexed(*gameEffect, "Material", 0,
                                          materialElements, 5, materialVB.Get(),
                                          sizeof(MaterialVertex), materialIB.Get(),
                                          DXGI_FORMAT_R16_UINT, 3);
    if (FAILED(result) || device.InputLayoutCount() != 1) return 10;
    result = device.DrawGameEffectIndexed(*gameEffect, "Material", 0,
                                          materialElements, 5, materialVB.Get(),
                                          sizeof(MaterialVertex), materialIB.Get(),
                                          DXGI_FORMAT_R16_UINT, 3);
    if (FAILED(result) || device.InputLayoutCount() != 1) return 10;
    ID3D11SamplerState* boundSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT] = {};
    device.Context()->PSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,
                                    boundSamplers);
    bool linearSamplerBound = false;
    for (ID3D11SamplerState* bound : boundSamplers)
    {
        if (bound)
        {
            D3D11_SAMPLER_DESC samplerDesc = {};
            bound->GetDesc(&samplerDesc);
            linearSamplerBound |= samplerDesc.Filter == D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            bound->Release();
        }
    }
    if (!linearSamplerBound) return 10;
    unsigned char materialPixel[4] = {};
    result = device.ReadPixel(32, 32, materialPixel);
    device.Context()->ClearState();
    if (FAILED(result)) return 10;
    std::printf("FX11 material draw center RGBA %u %u %u %u\n",
                materialPixel[0], materialPixel[1], materialPixel[2], materialPixel[3]);
    if (materialPixel[0] < 200 || materialPixel[1] < 200 || materialPixel[2] < 200)
        return 10;

    // The same generated exit must run for FX11 effects, not just built-ins.
    const UINT alphaEnabled = 1, alphaNever = D3D11_COMPARISON_NEVER;
    if (FAILED(gameEffect->SetRawParameter("BlurPCAlphaEnabled", &alphaEnabled, sizeof(alphaEnabled))) ||
        FAILED(gameEffect->SetRawParameter("BlurPCAlphaFunction", &alphaNever, sizeof(alphaNever)))) return 12;
    device.Context()->ClearRenderTargetView(device.RenderTargetView(), materialClear);
    device.Context()->ClearDepthStencilView(device.DepthStencilView(), D3D11_CLEAR_DEPTH, 1.f, 0);
    device.Context()->OMSetRenderTargets(1, &materialTarget, device.DepthStencilView());
    device.Context()->RSSetViewports(1, &materialViewport);
    device.Context()->RSSetState(materialRasterState.Get());
    result = device.DrawGameEffectIndexed(*gameEffect, "Material", 0,
        materialElements, 5, materialVB.Get(), sizeof(MaterialVertex), materialIB.Get(), DXGI_FORMAT_R16_UINT, 3);
    if (FAILED(result) || FAILED(device.ReadPixel(32, 32, materialPixel)) || materialPixel[0] != 0) return 12;
    device.Context()->ClearState();
    std::printf("FX11 alpha test: previously visible material rejected by Never\n");

    if (!CheckNativeAlphaTest(device) || !CheckBuiltinShaderLinks() || !CheckAlphaBlendFactors(device)) return 12;

    if (SUCCEEDED(result)) result = device.DrawProbeTriangle(
        L"_work/build/gen/dx11_builtins/display/vsBasicRenderColoured2D.cso",
        L"_work/build/gen/dx11_builtins/display/psBasicRenderColoured.cso");
    unsigned char pixel[4] = {};
    if (SUCCEEDED(result)) result = device.ReadPixel(32, 32, pixel);
    unsigned char corner[4] = {};
    if (SUCCEEDED(result)) result = device.ReadPixel(2, 2, corner);
    if (FAILED(result))
    {
        std::fprintf(stderr, "D3D11 probe failed: HRESULT 0x%08lx\n", static_cast<unsigned long>(result));
        return 1;
    }
    std::printf("D3D11 feature level 0x%x, %s device; center RGBA %u %u %u %u; corner RGBA %u %u %u %u\n",
                static_cast<unsigned>(device.FeatureLevel()),
                device.DriverType() == D3D_DRIVER_TYPE_HARDWARE ? "hardware" : "WARP",
                pixel[0], pixel[1], pixel[2], pixel[3], corner[0], corner[1], corner[2], corner[3]);
    // The center must contain the triangle, rather than the cleared background.
    if (!(pixel[3] == 255 && pixel[0] > 20 && pixel[1] > 20 && pixel[2] > 20 &&
          corner[0] == 5 && corner[1] == 10 && corner[2] == 20 && corner[3] == 255))
        return 2;

    const unsigned char texel[4] = { 20, 150, 200, 255 };
    result = device.DrawTexturedQuad(
        L"_work/build/gen/dx11_builtins/display/vsBasicRenderTextured2D.cso",
        L"_work/build/gen/dx11_builtins/display/psBasicRenderTextured.cso",
        texel);
    if (SUCCEEDED(result)) result = device.ReadPixel(32, 32, pixel);
    if (FAILED(result) || pixel[0] != texel[0] || pixel[1] != texel[1] ||
        pixel[2] != texel[2] || pixel[3] != texel[3])
    {
        std::fprintf(stderr, "D3D11 texture path failed: HRESULT 0x%08lx, pixel %u %u %u %u\n",
                     static_cast<unsigned long>(result), pixel[0], pixel[1], pixel[2], pixel[3]);
        return 5;
    }
    const unsigned char srgbTexel[4] = { 128, 128, 128, 255 };
    result = device.DrawTexturedQuad(
        L"_work/build/gen/dx11_builtins/display/vsBasicRenderTextured2D.cso",
        L"_work/build/gen/dx11_builtins/display/psBasicRenderTextured.cso",
        srgbTexel, true);
    if (SUCCEEDED(result)) result = device.ReadPixel(32, 32, pixel);
    if (FAILED(result) || pixel[0] < 54 || pixel[0] > 56 ||
        pixel[1] < 54 || pixel[1] > 56 || pixel[2] < 54 || pixel[2] > 56)
    {
        std::fprintf(stderr, "D3D11 sRGB decode failed: 0x%08lx, pixel %u %u %u %u\n",
                     static_cast<unsigned long>(result), pixel[0], pixel[1],
                     pixel[2], pixel[3]);
        return 5;
    }
    const unsigned char bc1Block[8] = { 0x00, 0xf8, 0, 0, 0, 0, 0, 0 };
    const unsigned char bc2Block[16] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x00, 0xf8, 0, 0, 0, 0, 0, 0
    };
    const unsigned char bc3Block[16] = {
        0xff, 0, 0, 0, 0, 0, 0, 0,
        0x00, 0xf8, 0, 0, 0, 0, 0, 0
    };
    const struct { D3DFORMAT format; const void* bytes; UINT size; } bcCases[] = {
        { D3DFMT_DXT1, bc1Block, sizeof(bc1Block) },
        { D3DFMT_DXT3, bc2Block, sizeof(bc2Block) },
        { D3DFMT_DXT5, bc3Block, sizeof(bc3Block) }
    };
    for (const auto& bc : bcCases)
    {
        const void* mip[] = { bc.bytes };
        const UINT size[] = { bc.size };
        Microsoft::WRL::ComPtr<ID3D11Texture2D> compressedTexture;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> compressedView;
        result = device.CreateTexture2DFromPCData(4, 4, 1, bc.format, mip, size,
                                                   compressedTexture, compressedView);
        if (SUCCEEDED(result)) result = device.DrawTexturedQuadView(
            L"_work/build/gen/dx11_builtins/display/vsBasicRenderTextured2D.cso",
            L"_work/build/gen/dx11_builtins/display/psBasicRenderTextured.cso",
            compressedView.Get());
        if (SUCCEEDED(result)) result = device.ReadPixel(32, 32, pixel);
        if (FAILED(result) || pixel[0] != 255 || pixel[1] != 0 ||
            pixel[2] != 0 || pixel[3] != 255) return 5;
    }

    // Draw with the engine's 3D shader and named world/view-projection matrices.
    const float world[16] = { 1, 0, 0, 0, 0, 1, 0, 0,
                              0, 0, 1, 0, 0, 0, 0, 1 };
    const float viewProj[16] = { 1, 0, 0, 0, 0, 1, 0, 0,
                                 0, 0, 1, 0, 0.65f, 0, 0, 1 };
    struct Vertex3D { float x, y, z, r, g, b, a; };
    const Vertex3D triangle[] = {
        {  0,     0.7f, 0.3f, 0.8f, 0.1f, 0.2f, 1 },
        { -0.25f,-0.5f, 0.3f, 0.8f, 0.1f, 0.2f, 1 },
        {  0.25f,-0.5f, 0.3f, 0.8f, 0.1f, 0.2f, 1 }
    };
    const unsigned short triangleIndices[] = { 0, 1, 2 };
    result = device.DrawColoured3D(
        L"_work/build/gen/dx11_builtins/display/vsBasicRenderColoured3D.cso",
        L"_work/build/gen/dx11_builtins/display/psBasicRenderColoured.cso",
        world, viewProj, triangle, sizeof(triangle), sizeof(Vertex3D),
        triangleIndices, 3);
    unsigned char translated[4] = {};
    if (SUCCEEDED(result)) result = device.ReadPixel(52, 32, translated);
    if (SUCCEEDED(result)) result = device.ReadPixel(32, 32, pixel);
    if (FAILED(result) || translated[0] < 150 || translated[1] < 15 ||
        translated[2] < 30 || pixel[0] != 5 || pixel[1] != 10 || pixel[2] != 20)
    {
        std::fprintf(stderr, "D3D11 3D matrices failed: HRESULT 0x%08lx, translated %u %u %u %u, center %u %u %u %u\n",
                     static_cast<unsigned long>(result), translated[0], translated[1],
                     translated[2], translated[3], pixel[0], pixel[1], pixel[2], pixel[3]);
        return 6;
    }

    // Exercise the render-target/depth views used for game postprocessing.
    Microsoft::WRL::ComPtr<ID3D11Texture2D> msColor, resolvedColor, msDepth;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> msTarget, resolvedTarget;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> msColorView, resolvedView, depthView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> msDepthTarget;
    result = device.CreateRenderTexture2D(64, 64, 1, 4, DXGI_FORMAT_R8G8B8A8_UNORM,
                                          msColor, msTarget, msColorView);
    if (SUCCEEDED(result)) result = device.CreateRenderTexture2D(
        64, 64, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM,
        resolvedColor, resolvedTarget, resolvedView);
    if (SUCCEEDED(result)) result = device.CreateDepthTexture2D(
        64, 64, 4, msDepth, msDepthTarget, depthView);
    if (FAILED(result))
    {
        std::fprintf(stderr, "D3D11 render/depth texture creation failed: 0x%08lx\n",
                     static_cast<unsigned long>(result));
        return 7;
    }
    const float resolveColour[4] = { 0.25f, 0.5f, 0.75f, 1.f };
    device.Context()->ClearRenderTargetView(msTarget.Get(), resolveColour);
    device.Context()->ClearDepthStencilView(msDepthTarget.Get(),
                                             D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
    device.Context()->ResolveSubresource(resolvedColor.Get(), 0, msColor.Get(), 0,
                                         DXGI_FORMAT_R8G8B8A8_UNORM);
    D3D11_TEXTURE2D_DESC stagingDesc = {};
    resolvedColor->GetDesc(&stagingDesc);
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> staging;
    result = device.Device()->CreateTexture2D(&stagingDesc, nullptr, &staging);
    if (SUCCEEDED(result)) device.Context()->CopyResource(staging.Get(), resolvedColor.Get());
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (SUCCEEDED(result)) result = device.Context()->Map(staging.Get(), 0, D3D11_MAP_READ,
                                                          0, &mapped);
    if (SUCCEEDED(result))
    {
        const unsigned char* sample = static_cast<const unsigned char*>(mapped.pData) +
                                      32 * mapped.RowPitch + 32 * 4;
        const bool rightColour = sample[0] >= 63 && sample[0] <= 65 &&
                                 sample[1] >= 127 && sample[1] <= 129 &&
                                 sample[2] >= 190 && sample[2] <= 192 &&
                                 sample[3] == 255;
        device.Context()->Unmap(staging.Get(), 0);
        if (!rightColour) return 8;
    }
    if (FAILED(result)) return 8;

    // Exercise the DXGI presentation and resize path without launching Amax.
    HWND window = CreateWindowExW(0, L"STATIC", L"Blur DX11 probe", WS_OVERLAPPEDWINDOW,
                                  0, 0, 64, 64, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!window) return 3;
    result = device.CreateWindowTarget(window, 64, 64);
    if (SUCCEEDED(result)) result = device.DrawProbeTriangle(
        L"_work/build/gen/dx11_builtins/display/vsBasicRenderColoured2D.cso",
        L"_work/build/gen/dx11_builtins/display/psBasicRenderColoured.cso");
    if (SUCCEEDED(result)) result = device.ReadPixel(32, 32, pixel);
    if (SUCCEEDED(result)) result = device.Present(0);
    if (SUCCEEDED(result)) result = device.ResizeWindowTarget(128, 96);
    if (SUCCEEDED(result)) result = device.DrawProbeTriangle(
        L"_work/build/gen/dx11_builtins/display/vsBasicRenderColoured2D.cso",
        L"_work/build/gen/dx11_builtins/display/psBasicRenderColoured.cso");
    if (SUCCEEDED(result)) result = device.ReadPixel(64, 48, pixel);
    if (SUCCEEDED(result)) result = device.Present(0);
    DestroyWindow(window);
    if (FAILED(result) || pixel[0] < 20 || pixel[1] < 20 || pixel[2] < 20)
    {
        std::fprintf(stderr, "DXGI window path failed: HRESULT 0x%08lx\n", static_cast<unsigned long>(result));
        return 4;
    }
    std::puts("DXGI swap chain create, present and resize passed");
    return 0;
}
