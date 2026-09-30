// [Blur1PCDX11] Use the game's authored FX5 permutations as native D3D11 effects.
#include "stdafx.h"
#include "fx/NativeFX11.h"
#include "fx/effect.h"
#include "fx/Shaders.h"
#include "fx/Shared.h"
#include "fx/SamplerStates.h"
#include "Display/Device/Device.h"
#include "Display/Device/D3D11/D3D11EffectLibrary.h"
#include "Display/Device/D3D11/D3D11Formats.h"
#include "Display/Device/D3D11/D3D11States.h"
#include <system/debug/Blur2PCAutomation.h>
#include <limits>
#include <unordered_map>
#include <wrl/client.h>

namespace FX {

const void* GetDefaultVertexTextureFor(const void* texture); // System.cpp, PC FX mechanism

static char g_fx11LastError[256] = {};
static char g_fx11CurrentPass[256] = {};

namespace {
Display::D3D11::C_D3D11EffectLibrary g_effects;
bool g_opened = false;
unsigned g_failures = 0;
// [Blur2PC] Cache immutable descriptors, not IDs: SamplerStateManager::Clear
// may reuse an ID for another state. These objects share the FX library device.
std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11SamplerState>> g_samplers;

Display::D3D11::C_D3D11Effect* Find(const Effect* effect)
{
    if (!effect || !Display::Device() || !Display::Device()->GetD3DDevice()) return nullptr;
    if (!g_opened)
    {
        g_opened = true;
        wchar_t module[MAX_PATH] = {};
        if (GetModuleFileNameW(nullptr, module, MAX_PATH))
        {
            wchar_t* slash = wcsrchr(module, L'\\');
            if (slash) *(slash + 1) = 0;
            wcscat_s(module, L"fx11");
            const HRESULT hr = g_effects.Open(module);
            Blur2PCAutomation::LogF("DX11 FX library: %u effects, status %08x",
                static_cast<unsigned>(g_effects.EffectCount()), static_cast<unsigned>(hr));
        }
    }
    Display::D3D11::C_D3D11Effect* result = nullptr;
    const HRESULT hr = g_effects.Get(Display::Device()->GetD3DDevice(), effect->Name.Chars(), &result);
    if (FAILED(hr) && g_failures++ < 20)
        Blur2PCAutomation::LogF("DX11 FX missing %s: %08x", effect->Name.Chars(), static_cast<unsigned>(hr));
    return result;
}

int PassIndex(const Effect* effect, const Pass* pass, int technique)
{
    if (!effect || !pass || technique < 0 || technique >= effect->Techniques.Size()) return -1;
    const auto& passes = effect->Techniques[technique].Passes;
    for (int i = 0; i < passes.Size(); ++i)
        if (&passes[i] == pass) return i;
    return -1;
}

void BindTexture(const Effect* effect, Display::D3D11::C_D3D11Effect* native,
                 const char* sampler, ID3D11Resource* texture, const char* technique,
                 int samplerState, bool immediate = false)
{
    // [Blur1PCDX11] FX sampler values are GPUTexture pointers (Display2: ID3D11Resource), as D3D9's were IDirect3DBaseTexture9.
    const char* name = g_effects.TextureForSampler(effect->Name.Chars(), sampler);
    if (getenv("BLUR_DX11_TRACE_SCENE") && Blur2PCAutomation::Now() > 21 &&
        strstr(sampler, "DamageVolume"))
    {
        static unsigned tracedDamageVolume = 0;
        if (tracedDamageVolume++ < 12)
            Blur2PCAutomation::LogF("DX11 FX damage binding %s / %s lookup=%s resource=%p",
                effect->Name.Chars(), sampler, name ? name : "<missing>", texture);
    }
    if (!name) return;
    // [Blur1PCDX11] Every recovered vehicle damage-volume sampler is a
    // vertex sampler. Match FxContext's PC default substitution: its 2D
    // default becomes the initialized zero float volume, preserving type.
    if (strcmp(name, "g_sVehicleDamageVolumeTexture") == 0)
        texture = reinterpret_cast<ID3D11Resource*>(const_cast<void*>(GetDefaultVertexTextureFor(texture)));
    const Display::C_SamplerState& live = *g_SamplerStateMgr.GetSamplerState(samplerState);
    D3D11_SAMPLER_DESC desc = {};
    // [Blur2PC] PC Context_Base only applies Display2 sampler fields. Its
    // MaxAnisotropy override is Xenon-only, so retain the PC device default 1.
    HRESULT samplerResult = Display::D3D11::MakeSamplerDesc(
        static_cast<D3DTEXTUREFILTERTYPE>(live.m_TextureFilterMin),
        static_cast<D3DTEXTUREFILTERTYPE>(live.m_TextureFilterMag),
        static_cast<D3DTEXTUREFILTERTYPE>(live.m_TextureFilterMip),
        static_cast<D3DTEXTUREADDRESS>(live.m_TextureAddressU),
        static_cast<D3DTEXTUREADDRESS>(live.m_TextureAddressV),
        static_cast<D3DTEXTUREADDRESS>(live.m_TextureAddressW), 1, &desc,
        live.m_TextureBorderColour, live.m_TextureLodBias) ? S_OK : E_INVALIDARG;
    if (SUCCEEDED(samplerResult))
    {
        const std::string key(reinterpret_cast<const char*>(&desc), sizeof(desc));
        auto& state = g_samplers[key];
        if (!state) samplerResult = Display::Device()->GetD3DDevice()->CreateSamplerState(&desc, &state);
        if (SUCCEEDED(samplerResult))
            samplerResult = g_effects.SetTextureSampler(Display::Device()->GetD3DDevice(),
                effect->Name.Chars(), sampler, state.Get());
    }
    if (FAILED(samplerResult)) { static unsigned failures = 0; if (failures++ < 24)
        Blur2PCAutomation::LogF("DX11 FX sampler binding failed %s / %s result=%08x",
            effect->Name.Chars(), sampler, static_cast<unsigned>(samplerResult)); }
    ID3D11ShaderResourceView* view = nullptr;
    if (texture)
    {
        // [Blur2PC] The PC FX path binds the current parameter's sampler state,
        // including immediate overrides; compiled metadata only describes its default.
        bool srgb = live.m_TextureGamma == Display2::E_Bool_True;
        const Display::E_Bool forced = Display::Device()->GetForceTextureGamma();
        if (forced != Display::E_Bool_Default) srgb = forced == Display::E_Bool_True;
        const HRESULT result = Display::D3D11::CreateTextureView(
            Display::Device()->GetD3DDevice(), texture, srgb, &view);
        static const bool traceGamma = getenv("BLUR_DX11_TRACE_TEXTURE_GAMMA") != nullptr;
        if (traceGamma)
        {
            bool authored = false;
            const HRESULT metadata = g_effects.TextureUsesSRGB(effect->Name.Chars(), name, &authored);
            static unsigned counts[4] = {};
            const unsigned category = (immediate ? 2U : 0U) + (SUCCEEDED(metadata) && authored != srgb ? 1U : 0U);
            if (counts[category]++ < 12)
            {
                D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
                if (view) view->GetDesc(&desc);
                Blur2PCAutomation::LogF("DX11 FX gamma %s / %s immediate=%u state=%d authored=%u live=%u forced=%u view=%u result=%08x",
                    effect->Name.Chars(), name, immediate, samplerState, authored, srgb,
                    static_cast<unsigned>(forced), static_cast<unsigned>(desc.Format), static_cast<unsigned>(result));
            }
        }
        if (FAILED(result)) { static unsigned failures = 0; if (failures++ < 24)
            Blur2PCAutomation::LogF("DX11 FX texture view failed %s / %s srgb=%u result=%08x",
                effect->Name.Chars(), name, srgb, static_cast<unsigned>(result)); }
    }
    const HRESULT result = native->SetTexture(name, view);
    if (FAILED(result)) { static unsigned failures = 0; if (failures++ < 24)
        Blur2PCAutomation::LogF("DX11 FX texture binding failed %s / %s result=%08x", effect->Name.Chars(), name, static_cast<unsigned>(result)); }
    if (view) view->Release();
}

void BindSharedParameters(const Effect* effect, Display::D3D11::C_D3D11Effect* native,
                          void* const* sources, const char* technique)
{
    if (!sources[NGLFX_BIND_SHARED] || g_SharedEffect.ParameterData.Size() == 0) return;
    const uintptr_t base = reinterpret_cast<uintptr_t>(&g_SharedEffect.ParameterData[0]);
    const size_t totalBytes = static_cast<size_t>(g_SharedEffect.ParameterData.Size()) * sizeof(U32);
    for (int i = 0; i < g_SharedEffect.Parameters.Size(); ++i)
    {
        const Parameter& parameter = g_SharedEffect.Parameters[i];
        if (!parameter.Data || parameter.Size <= 0 ||
            !native->HasParameter(parameter.Name.Chars())) continue;
        const uintptr_t address = reinterpret_cast<uintptr_t>(parameter.Data);
        if (address < base || address - base > totalBytes ||
            static_cast<size_t>(parameter.Size) * sizeof(U32) > totalBytes - (address - base)) continue;
        const U8* data = static_cast<const U8*>(sources[NGLFX_BIND_SHARED]) + (address - base);
        if (strcmp(parameter.Name.Chars(), "g_mViewProj") == 0 ||
            strcmp(parameter.Name.Chars(), "g_mWorld") == 0)
        {
            static unsigned matricesTraced = 0;
            if (matricesTraced++ < 12)
            {
                const F32* floats = reinterpret_cast<const F32*>(data);
                Blur2PCAutomation::LogF("DX11 FX shared matrix %s / %s size=%d row0=%.3f %.3f %.3f %.3f row3=%.3f %.3f %.3f %.3f",
                    effect->Name.Chars(), parameter.Name.Chars(), parameter.Size,
                    floats[0], floats[1], floats[2], floats[3],
                    floats[12], floats[13], floats[14], floats[15]);
            }
        }
        if (parameter.Type == NGLFX_PARAM_SAMPLER)
        {
            const auto* sampler = reinterpret_cast<const nglFXParam_Sampler*>(data);
            BindTexture(effect, native, parameter.Name.Chars(),
                        reinterpret_cast<ID3D11Resource*>(sampler->Texture), technique, sampler->iSamplerState);
        }
        else
        {
            const HRESULT result = native->SetRawParameter(parameter.Name.Chars(), data,
                static_cast<UINT>(parameter.Size) * sizeof(U32));
            if (FAILED(result)) { static unsigned failures = 0; if (failures++ < 20)
                Blur2PCAutomation::LogF("DX11 FX shared binding failed %s / %s result=%08x", effect->Name.Chars(), parameter.Name.Chars(), static_cast<unsigned>(result)); }
        }
    }
}

void BindFunction(const Effect* effect, Display::D3D11::C_D3D11Effect* native,
                  const Function& function, void* const* sources, const char* technique)
{
    for (int i = 0; i < function.Bindings.Size(); ++i)
    {
        const FunctionBinding& binding = function.Bindings[i];
        if (binding.ParameterIdx >= effect->Parameters.Size() ||
            binding.Source > NGLFX_BIND_SHARED || !sources[binding.Source]) continue;
        const char* name = binding.Name && binding.Name[0]
            ? binding.Name : effect->Parameters[binding.ParameterIdx].Name.Chars();
        if (!name || !name[0]) continue;
        const U8* data = static_cast<const U8*>(sources[binding.Source]) + binding.Offset;
        if (binding.Type == NGLFX_BIND_SAMPLER)
        {
            const auto* sampler = reinterpret_cast<const nglFXParam_Sampler*>(data);
            BindTexture(effect, native, name,
                        reinterpret_cast<ID3D11Resource*>(sampler->Texture), technique, sampler->iSamplerState);
        }
        else if (binding.Count)
        {
            if (strcmp(name, "g_mViewProj") == 0 || strcmp(name, "g_mWorld") == 0)
            {
                static unsigned localMatricesTraced = 0;
                if (localMatricesTraced++ < 12)
                {
                    const F32* floats = reinterpret_cast<const F32*>(data);
                    Blur2PCAutomation::LogF("DX11 FX local matrix %s / %s count=%u row0=%.3f %.3f %.3f %.3f row3=%.3f %.3f %.3f %.3f",
                        effect->Name.Chars(), name, binding.Count,
                        floats[0], floats[1], floats[2], floats[3],
                        floats[12], floats[13], floats[14], floats[15]);
                }
            }
            static unsigned traced = 0;
            if (traced++ < 32)
            {
                const F32* floats = reinterpret_cast<const F32*>(data);
                Blur2PCAutomation::LogF("DX11 FX value %s / %s count=%u src=%u offset=%u first=%.4f %.4f %.4f %.4f",
                    effect->Name.Chars(), name, binding.Count, binding.Source, binding.Offset,
                    floats[0], floats[1], floats[2], floats[3]);
            }
            const HRESULT result = native->SetRawParameter(name, data, binding.Count * 16U);
            if (FAILED(result)) { static unsigned failures = 0; if (failures++ < 24)
                Blur2PCAutomation::LogF("DX11 FX parameter binding failed %s / %s result=%08x", effect->Name.Chars(), name, static_cast<unsigned>(result)); }
        }
    }
}
}

const void* NativeFX11Shader(const Effect* effect, int technique, int pass,
                            E_ShaderType type, unsigned* bytes)
{
    if (bytes) *bytes = 0;
    auto* native = Find(effect);
    if (!native || !bytes || technique < 0 || technique >= effect->Techniques.Size()) return nullptr;
    const char* name = effect->Techniques[technique].Name.Chars();
    const void* shader = nullptr;
    SIZE_T length = 0;
    const HRESULT hr = type == E_ShaderType_VertexShader ?
        native->GetPassVertexShaderBytecode(name, pass, &shader, &length) :
        native->GetPassPixelShaderBytecode(name, pass, &shader, &length);
    if (FAILED(hr) || length > std::numeric_limits<unsigned>::max()) return nullptr;
    *bytes = static_cast<unsigned>(length);
    return shader;
}

bool NativeFX11Apply(const Effect* effect, const Pass* pass, int technique,
                     void* const* sources)
{
    auto* native = Find(effect);
    const int index = PassIndex(effect, pass, technique);
    if (!native || index < 0 || !sources) return false;
    const char* techniqueName = effect->Techniques[technique].Name.Chars();
    BindSharedParameters(effect, native, sources, techniqueName);
    BindFunction(effect, native, pass->VertexShader, sources, techniqueName);
    BindFunction(effect, native, pass->PixelShader, sources, techniqueName);
    ID3D11DeviceContext* context = nullptr;
    Display::Device()->GetD3DDevice()->GetImmediateContext(&context);
    if (!context) return false;
    const HRESULT hr = native->Apply(effect->Techniques[technique].Name.Chars(), index, context);
    context->Release();
    if (FAILED(hr))
        sprintf_s(g_fx11LastError, "%.80s / %.80s pass=%d", effect->Name.Chars(),
            effect->Techniques[technique].Name.Chars(), index);
    static unsigned applied = 0;
    if (applied++ < 18 || (Blur2PCAutomation::Now() > 19.9 && Blur2PCAutomation::Now() < 20.1))
        Blur2PCAutomation::LogF("DX11 FX apply %s / %s pass=%d status=%08x t=%.2f",
            effect->Name.Chars(), effect->Techniques[technique].Name.Chars(), index,
            static_cast<unsigned>(hr), Blur2PCAutomation::Now());
    if (SUCCEEDED(hr))
    {
        sprintf_s(g_fx11CurrentPass, "%.80s / %.80s pass=%d", effect->Name.Chars(),
            effect->Techniques[technique].Name.Chars(), index);
        Display::Device()->SetNativeFXActive(true);
    }
    return SUCCEEDED(hr);
}

const char* NativeFX11LastError()
{
    return g_fx11LastError;
}

const char* NativeFX11CurrentPass()
{
    return g_fx11CurrentPass;
}

bool NativeFX11SetImmediate(const Effect* effect, const Pass* pass, int technique,
                            const char* parameter, const void* value, unsigned bytes,
                            bool texture, int samplerState)
{
    auto* native = Find(effect);
    const int index = PassIndex(effect, pass, technique);
    if (!native || index < 0 || !parameter || !value) return false;
    if (texture && samplerState < 0) return false;
    if (texture)
        BindTexture(effect, native, parameter,
                    *reinterpret_cast<ID3D11Resource* const*>(value), effect->Techniques[technique].Name.Chars(),
                    samplerState, true);
    else
        native->SetRawParameter(parameter, value, bytes);
    ID3D11DeviceContext* context = nullptr;
    Display::Device()->GetD3DDevice()->GetImmediateContext(&context);
    if (!context) return false;
    const HRESULT hr = native->Apply(effect->Techniques[technique].Name.Chars(), index, context);
    context->Release();
    if (SUCCEEDED(hr)) Display::Device()->SetNativeFXActive(true);
    return SUCCEEDED(hr);
}
}
