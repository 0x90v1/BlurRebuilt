#ifndef FX_NATIVE_FX11_H
#define FX_NATIVE_FX11_H

// [Blur1PCDX11] Bind authored FX5 effects directly to the native D3D11 device.
namespace FX {
class Effect;
struct Pass;
enum E_ShaderType;

const void* NativeFX11Shader(const Effect* effect, int technique, int pass,
                            E_ShaderType type, unsigned* bytes);
bool NativeFX11Apply(const Effect* effect, const Pass* pass, int technique,
                     void* const* sources);
bool NativeFX11SetImmediate(const Effect* effect, const Pass* pass, int technique,
                            const char* parameter, const void* value, unsigned bytes,
                            bool texture, int samplerState = -1);

// [Blur1PCDX11] Name of the effect/technique whose apply last returned an error, for draw diagnostics.
const char* NativeFX11LastError();
}

#endif
