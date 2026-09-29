#ifndef DISPLAY_DEVICE_D3D11_EFFECT_LIBRARY_H
#define DISPLAY_DEVICE_D3D11_EFFECT_LIBRARY_H

// [Blur1PCDX11] Resolves the game's Jam effect names to native FX5 binaries.
#include "Display/Device/D3D11/D3D11Effect.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace Display { namespace D3D11 {

class C_D3D11EffectLibrary
{
public:
    HRESULT Open(const wchar_t* root);
    HRESULT Get(ID3D11Device* device, const char* gameEffectName,
                C_D3D11Effect** effect);
    HRESULT TextureUsesSRGB(const char* gameEffectName, const char* textureParameter,
                            bool* srgb) const;
    const char* TextureForSampler(const char* gameEffectName, const char* samplerName) const;
    HRESULT PreloadAll(ID3D11Device* device, std::string* failedName = nullptr);
    size_t EffectCount() const { return m_Effects.size(); }

private:
    struct C_Entry
    {
        std::wstring path;
        std::unique_ptr<C_D3D11Effect> effect;
        std::unordered_map<std::string, bool> textureSRGB;
        std::unordered_map<std::string, std::string> samplerTextures;
        std::unordered_map<std::string, std::string> textureNames;
    };
    std::unordered_map<std::string, C_Entry> m_Effects;
};

} }

#endif
