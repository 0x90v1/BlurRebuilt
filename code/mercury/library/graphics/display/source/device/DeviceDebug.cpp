#include "stdafx.h"
#include "Display/Device/DeviceDebug.h"
#include "Display/Surface/SurfaceUtils.h"

DEBUG_ENUM(Display::E_Bool)
	DE_OPTION(Display::E_Bool_True,						"True")
	DE_OPTION(Display::E_Bool_False,					"False")
END_DEBUG_ENUM

DEBUG_ENUM(Display::E_CubeTextureFace)
	DE_OPTION(Display::E_CubeTextureFace_PosX,			"PosX")
	DE_OPTION(Display::E_CubeTextureFace_PosY,			"PosY")
	DE_OPTION(Display::E_CubeTextureFace_PosZ,			"PosZ")
	DE_OPTION(Display::E_CubeTextureFace_NegX,			"NegX")
	DE_OPTION(Display::E_CubeTextureFace_NegY,			"NegY")
	DE_OPTION(Display::E_CubeTextureFace_NegZ,			"NegZ")
END_DEBUG_ENUM

DEBUG_ENUM(Display::E_CullMode)
	DE_OPTION(Display::E_CullMode_Default,				"Default")
	DE_OPTION(Display::E_CullMode_None,					"None")
	DE_OPTION(Display::E_CullMode_Cw,					"Cw")
	DE_OPTION(Display::E_CullMode_Ccw,					"Ccw")
END_DEBUG_ENUM

DEBUG_ENUM(Display::E_TextureAddress)
	DE_OPTION(Display::E_TextureAddress_Default,		"Default")
	DE_OPTION(Display::E_TextureAddress_Wrap,			"Wrap")
	DE_OPTION(Display::E_TextureAddress_Clamp,			"Clamp")
	DE_OPTION(Display::E_TextureAddress_Mirror,			"Mirror")
	DE_OPTION(Display::E_TextureAddress_Border,			"Border")
END_DEBUG_ENUM

DEBUG_ENUM(Display::E_TextureFilter)
	DE_OPTION(Display::E_TextureFilter_Default,			"Default")
	DE_OPTION(Display::E_TextureFilter_None,			"None")
	DE_OPTION(Display::E_TextureFilter_Point,			"Point")
	DE_OPTION(Display::E_TextureFilter_Linear,			"Linear")
	DE_OPTION(Display::E_TextureFilter_Anisotropic,		"Anisotropic")
END_DEBUG_ENUM

namespace Display
{

C_ResolutionManager::C_ResolutionManager() :
	m_MaxSize(0)
{
}

C_ResolutionManager::~C_ResolutionManager()
{
	ClearResolutions();
}

void C_ResolutionManager::ClearResolutions()
{
	m_Resolutions.Clear();
	m_Options.Clear();
}

void C_ResolutionManager::ResetResolutions()
{
	ClearResolutions();

	m_Resolutions.PushBack();
	m_Resolutions.Back().m_Value		= 0;
	m_Resolutions.Back().m_Width		= 0;
	m_Resolutions.Back().m_Height		= 0;
	m_Resolutions.Back().m_Multisample	= Display::E_Multisample_None;
	m_Resolutions.Back().m_Name			= "Disabled";
}

void C_ResolutionManager::AddResolution(const U32 width, const U32 height, const E_Format format, const E_Multisample multisample, const U32 flags)
{
	U32 size = GetSurfaceSize(width, height, flags, format, multisample);
	if(size <= m_MaxSize || m_MaxSize == 0)
	{
		Util::C_String aa;
		switch(multisample)
		{
		case E_Multisample_None:
			aa = "1";
			break;
		case E_Multisample_2:
			aa = "2";
			break;
		case E_Multisample_4:
			aa = "4";
			break;
		default:
			return;
		}
		m_Resolutions.PushBack();
		m_Resolutions.Back().m_Value		= m_Resolutions.Size()-1;
		m_Resolutions.Back().m_Width		= width;
		m_Resolutions.Back().m_Height		= height;
		m_Resolutions.Back().m_Multisample	= multisample;
		m_Resolutions.Back().m_Name.Printf("%ix%ix%s", width, height, aa.Chars());
	}
}

void C_ResolutionManager::SetResolutions(Debug::EnumBase* pEnum)
{
	if(pEnum)
	{
		const S32 resolutionCount = m_Resolutions.Size();
		if(resolutionCount > 0)
		{
			m_Options.Clear();

			for(S32 resolutionIndex=0; resolutionIndex<resolutionCount; resolutionIndex++)
			{
				C_Resolution& resolution = m_Resolutions[resolutionIndex];

				m_Options.PushBack();
				m_Options.Back().value = resolution.m_Value;
				m_Options.Back().name = resolution.m_Name.Chars();
			}

			m_Options.PushBack();
			m_Options.Back().value = -1;
			m_Options.Back().name = 0;
		}

		const S32 optionCount = m_Options.Size();
		if(optionCount > 0)
		{
			pEnum->SetOptions(&m_Options.Front());
		}
	}
}

void C_ResolutionManager::SetMaxResolution(const U32 width, const U32 height, const E_Format format, const E_Multisample multisample, const U32 flags)
{
	m_MaxSize = GetSurfaceSize(width, height, flags, format, multisample);
}

C_Resolution* C_ResolutionManager::GetResolution(const S32 resolutionIndex)
{
	if(resolutionIndex > 0 && resolutionIndex < m_Resolutions.Size())
	{
		return &m_Resolutions[resolutionIndex];
	}
	else
	{
		return NULL;
	}
}

C_ResolutionManager g_Resolutions;

}; // namespace Display
