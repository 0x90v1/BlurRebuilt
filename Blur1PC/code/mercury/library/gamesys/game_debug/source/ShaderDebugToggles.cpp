#include "GameDebugPch.h"

#include <game_debug/ShaderDebugToggles.h>

namespace GameDebug
{

#ifdef PLATFORM_XENON
Debug::Bool g_ShowLumels("Graphics.Debug.ShaderDebug.ShowLumels", false);
Debug::Bool g_ShowTangentNormal("Graphics.Debug.ShaderDebug.ShowTangentNormal", false);
Debug::Bool g_ShowSpecular("Graphics.Debug.ShaderDebug.ShowSpecular", false);
Debug::Bool g_NoLighting("Graphics.Debug.ShaderDebug.NoLighting", false);
Debug::Bool g_ShowWorldTangent("Graphics.Debug.ShaderDebug.ShowWorldTangent", false);
Debug::Bool g_ShowLightmapRange("Graphics.Debug.ShaderDebug.ShowLightmapRange", false);

#endif

void SetShaderDebugToggles()
{
#ifdef PLATFORM_XENON
	const S32 startRegister = 96;
	const S32 numDebugToggles = 6;
	BOOL debugToggles[numDebugToggles];
	debugToggles[0] = g_ShowLumels;
	debugToggles[1] = g_ShowTangentNormal;
	debugToggles[2] = g_ShowSpecular;
	debugToggles[3] = g_NoLighting;
	debugToggles[4] = g_ShowWorldTangent;
	debugToggles[5] = g_ShowLightmapRange;

	Display::Device()->GetD3DDevice()->SetPixelShaderConstantB( startRegister, debugToggles, numDebugToggles );
#endif
}

}; // namespace Game
