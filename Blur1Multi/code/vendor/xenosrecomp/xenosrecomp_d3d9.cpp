// [Blur1Multi] See xenosrecomp_d3d9.h.

#include "pch.h"
#include "xenosrecomp_d3d9.h"
#include "shader_recompiler.h"
#include "shader_common_d3d9.h"

namespace XenosRecompD3D9
{
	bool Recompile(const unsigned char* container, size_t size, std::string& hlsl, bool& isPixelShader,
		std::string& unsupported, std::string& error)
	{
		// ShaderContainer (shader.h): big-endian; the recompiler asserts these.
		if (size < sizeof(ShaderContainer))
		{
			error = "shorter than a shader container";
			return false;
		}
		const ShaderContainer* c = reinterpret_cast<const ShaderContainer*>(container);
		if ((c->flags & 0xFFFFFF00) != 0x102A1100 || c->constantTableOffset == 0 ||
			c->constantTableOffset >= size || c->shaderOffset >= size || c->virtualSize > size)
		{
			error = "not a Xenos shader container";
			return false;
		}

		ShaderRecompiler recompiler;
		recompiler.recompile(container, g_xenosRecompD3D9Header);
		hlsl.swap(recompiler.out);
		isPixelShader = recompiler.isPixelShader;
		unsupported = recompiler.unsupported;
		return true;
	}
}
