#ifndef DISPLAY_DEVICE_D3D11_EFFECT_H
#define DISPLAY_DEVICE_D3D11_EFFECT_H

// [Blur1PCDX11] Native FX5 effect resource for the game's authored materials.
#include <d3dx11effect.h>
#include <wrl/client.h>
#include <map>
#include <string>

namespace Display { namespace D3D11 {

class C_D3D11Effect
{
public:
    HRESULT LoadMemory(ID3D11Device* device, const void* bytes, SIZE_T size);
    HRESULT LoadFile(ID3D11Device* device, const wchar_t* path);
    bool HasParameter(const char* name) const;
    HRESULT SetRawParameter(const char* name, const void* data, UINT bytes);
    HRESULT SetTexture(const char* name, ID3D11ShaderResourceView* view);
    // Returns S_FALSE when the effect's currently retained view can be reused.
    HRESULT CreateTextureView(const char* name, ID3D11Device* device,
                              ID3D11Resource* texture, bool srgb,
                              ID3D11ShaderResourceView** view) const;
    HRESULT SetSampler(const char* name, ID3D11SamplerState* sampler);
    HRESULT GetPassInputSignature(const char* technique, UINT pass,
                                  const void** signature, SIZE_T* bytes) const;
    HRESULT GetPassPixelShaderBytecode(const char* technique, UINT pass,
                                      const void** bytecode, SIZE_T* bytes) const;
    HRESULT GetPassVertexShaderBytecode(const char* technique, UINT pass,
                                       const void** bytecode, SIZE_T* bytes) const;
    HRESULT Apply(const char* technique, UINT pass, ID3D11DeviceContext* context) const;

private:
    ID3DX11EffectVariable* FindVariable(const char* name) const;
    ID3DX11EffectPass* FindPass(const char* technique, UINT pass) const;
    Microsoft::WRL::ComPtr<ID3DX11Effect> m_Effect;
    // [Blur1PCDX11] Reflection pointers belong to m_Effect. Build the index at
    // load time so render-time lookups, including absent names, are read-only.
    std::map<std::string, ID3DX11EffectVariable*, std::less<>> m_Variables;
    bool m_VariablesComplete = false;
};

} }

#endif
