#include "Display/Device/D3D11/D3D11EffectLibrary.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>

namespace Display { namespace D3D11 {

namespace {
std::string EffectKey(const char* name)
{
    if (!name) return {};
    std::string key(name);
    const size_t slash = key.find_last_of("/\\");
    if (slash != std::string::npos) key.erase(0, slash + 1);
    const size_t dot = key.find_last_of('.');
    if (dot != std::string::npos) key.erase(dot);
    std::transform(key.begin(), key.end(), key.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return key;
}
}

HRESULT C_D3D11EffectLibrary::Open(const wchar_t* rootPath)
{
    if (!rootPath) return E_INVALIDARG;
    const std::filesystem::path root(rootPath);
    m_Effects.clear();
    std::ifstream index(root / "effects.index");
    if (!index) return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    std::string line;
    while (std::getline(index, line))
    {
        const size_t separator = line.find('\t');
        if (separator == std::string::npos) return E_FAIL;
        const std::string key = line.substr(0, separator);
        const std::filesystem::path relative(line.substr(separator + 1));
        if (key.empty() || relative.empty() || relative.is_absolute()) return E_FAIL;
        C_Entry entry;
        entry.path = (root / relative).wstring();
        if (!m_Effects.emplace(key, std::move(entry)).second) return E_FAIL;
    }
    if (index.bad() || m_Effects.empty()) return E_FAIL;
    std::ifstream samplers(root / "effects.samplers");
    if (!samplers) return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    while (std::getline(samplers, line))
    {
        const size_t a = line.find('\t');
        const size_t b = a == std::string::npos ? a : line.find('\t', a + 1);
        const size_t c = b == std::string::npos ? b : line.find('\t', b + 1);
        const size_t d = c == std::string::npos ? c : line.find('\t', c + 1);
        if (d == std::string::npos) return E_FAIL;
        const auto effect = m_Effects.find(line.substr(0, a));
        if (effect == m_Effects.end()) return E_FAIL;
        const std::string texture = EffectKey(line.substr(b + 1, c - b - 1).c_str());
        const std::string sampler = EffectKey(line.substr(a + 1, b - a - 1).c_str());
        const std::string value = line.substr(c + 1, d - c - 1);
        if (texture.empty() || (value != "0" && value != "1")) return E_FAIL;
        const bool srgb = value == "1";
        const auto found = effect->second.textureSRGB.find(texture);
        if (found != effect->second.textureSRGB.end() && found->second != srgb)
            return E_FAIL;
        effect->second.textureSRGB[texture] = srgb;
        effect->second.samplerTextures[sampler] = line.substr(b + 1, c - b - 1);
        effect->second.textureNames[texture] = line.substr(b + 1, c - b - 1);
    }
    return samplers.bad() ? E_FAIL : S_OK;
}

const char* C_D3D11EffectLibrary::TextureForSampler(const char* gameEffectName,
                                                      const char* samplerName) const
{
    const auto effect = m_Effects.find(EffectKey(gameEffectName));
    if (effect == m_Effects.end()) return nullptr;
    const std::string key = EffectKey(samplerName);
    const auto sampler = effect->second.samplerTextures.find(key);
    if (sampler != effect->second.samplerTextures.end()) return sampler->second.c_str();
    // [Blur1PCDX11] Function bindings and shared parameters name the texture (g_tAlbedo), not its sampler.
    const auto texture = effect->second.textureNames.find(key);
    return texture == effect->second.textureNames.end() ? nullptr : texture->second.c_str();
}

HRESULT C_D3D11EffectLibrary::TextureUsesSRGB(const char* gameEffectName,
                                               const char* textureParameter,
                                               bool* srgb) const
{
    if (!gameEffectName || !textureParameter || !srgb) return E_INVALIDARG;
    *srgb = false;
    const auto effect = m_Effects.find(EffectKey(gameEffectName));
    if (effect == m_Effects.end()) return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    const auto texture = effect->second.textureSRGB.find(EffectKey(textureParameter));
    if (texture == effect->second.textureSRGB.end())
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    *srgb = texture->second;
    return S_OK;
}

HRESULT C_D3D11EffectLibrary::Get(ID3D11Device* device, const char* gameEffectName,
                                   C_D3D11Effect** effect)
{
    if (!device || !gameEffectName || !effect) return E_INVALIDARG;
    *effect = nullptr;
    const auto found = m_Effects.find(EffectKey(gameEffectName));
    if (found == m_Effects.end()) return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    C_Entry& entry = found->second;
    if (!entry.effect)
    {
        std::unique_ptr<C_D3D11Effect> loaded(new C_D3D11Effect);
        const HRESULT result = loaded->LoadFile(device, entry.path.c_str());
        if (FAILED(result)) return result;
        entry.effect = std::move(loaded);
    }
    *effect = entry.effect.get();
    return S_OK;
}

HRESULT C_D3D11EffectLibrary::PreloadAll(ID3D11Device* device,
                                          std::string* failedName)
{
    if (!device) return E_INVALIDARG;
    if (failedName) failedName->clear();
    for (auto& item : m_Effects)
    {
        C_D3D11Effect* effect = nullptr;
        const HRESULT result = Get(device, item.first.c_str(), &effect);
        if (FAILED(result))
        {
            if (failedName) *failedName = item.first;
            return result;
        }
    }
    return S_OK;
}

} }
