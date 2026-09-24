#include "stdafx.h"
#include "Display/Utils/Screenshot.h"
#include "Display/Device/Device.h"

enum E_DisplayScreenShot
{
	E_DisplayScreenShot_ColourRGB,
	E_DisplayScreenShot_ColourA,
	E_DisplayScreenShot_Depth,
};

DEBUG_ENUM(E_DisplayScreenShot)
	DE_OPTION(E_DisplayScreenShot_ColourRGB,	"ColourRGB")
	DE_OPTION(E_DisplayScreenShot_ColourA,		"ColourA")
	DE_OPTION(E_DisplayScreenShot_Depth,		"Depth")
END_DEBUG_ENUM

Debug::Enum<E_DisplayScreenShot> g_debugDisplayScreenshotMode("Graphics.Render.ScreenShotMode", E_DisplayScreenShot_ColourRGB);

namespace Display
{

void SaveAsTGA(const Sys::C_FilePath& fileName, const S32 width, const S32 height, const U8* pData)
{
	Sys::I_File* pFile = Sys::GetDefaultFileSystem()->Open(fileName.Chars(), Sys::E_FileAccess_Write);
	if(!pFile)
	{
		LOGWARN(g_Log, "Failed to open file '%s'\n", fileName.Chars());
		return;
	}

	U8 tgaHeader[12] = {0,0,2,0,0,0,0,0,0,0,0,0};
	pFile->Write(tgaHeader, sizeof(U8)*12);

	U8 imgHeader[6] = {width%256,width/256,height%256,height/256,32,0};
	pFile->Write(imgHeader, sizeof(U8)*6);

	#ifdef PLATFORM_PS3
	U32 alignedWidth = GCM_ALIGN_SIZE(width, 64);
	U32 alignedHeight = height;
	#else
	U32 alignedWidth = width;
	U32 alignedHeight = height;
	#endif
	
	for(S32 y=0; y<height; y++)
	{
		for(S32 x=0; x<width; x++)
		{
			U8 r=0, g=0, b=0, a=0;
#if 1
			r = pData[(((alignedHeight-y)*alignedWidth*4)+(x*4))+3];
			g = pData[(((alignedHeight-y)*alignedWidth*4)+(x*4))+2];
			b = pData[(((alignedHeight-y)*alignedWidth*4)+(x*4))+1];
			a = pData[(((alignedHeight-y)*alignedWidth*4)+(x*4))+0];
#else
			switch(g_debugDisplayScreenshotMode)
			{
			case E_DisplayScreenShot_ColourRGB:
				{
					r = pData[(((alignedHeight-y)*alignedWidth*4)+(x*4))+3];
					g = pData[(((alignedHeight-y)*alignedWidth*4)+(x*4))+2];
					b = pData[(((alignedHeight-y)*alignedWidth*4)+(x*4))+1];
				}
				break;
			case E_DisplayScreenShot_ColourA:
				{
					r = pData[(((alignedHeight-y)*alignedWidth*4)+(x*4))+0];
					g = pData[(((alignedHeight-y)*alignedWidth*4)+(x*4))+0];
					b = pData[(((alignedHeight-y)*alignedWidth*4)+(x*4))+0];
				}
				break;
			case E_DisplayScreenShot_Depth:
				{
					r = pData[(((alignedHeight-y)*alignedWidth*4)+(x*4))+0];
					g = pData[(((alignedHeight-y)*alignedWidth*4)+(x*4))+1];
					b = pData[(((alignedHeight-y)*alignedWidth*4)+(x*4))+2];
					a = pData[(((alignedHeight-y)*alignedWidth*4)+(x*4))+3];
				}
				break;
			default:
				assert(0 && "Unsupported screenshot mode");
				break;
			}
#endif
			pFile->Write(&r, sizeof(U8));
			pFile->Write(&g, sizeof(U8));
			pFile->Write(&b, sizeof(U8));
			pFile->Write(&a, sizeof(U8));
		}
	}

	SAFE_DELETE_PTR(pFile);
}

void TakeScreenshot(const Sys::C_FilePath& fileName, const C_Surface* pSurface)
{
	if(!pSurface || !pSurface->IsCreated())
	{
		LOGWARN(g_Log, "Failed to save screenshot '%s'\n", fileName.Chars());
		return;
	}

	const Sys::C_FilePath filePath = 
		Sys::GetDefaultFileSystem()->GetWorkingDirectory() + "/" + fileName;
	
#ifdef PLATFORM_PC
	const HRESULT hr = D3DXSaveSurfaceToFile(
		filePath.Chars(),
		D3DXIFF_TGA,
		pSurface->m_pd3dSurface,
		NULL,
		NULL);
	if(FAILED(hr))
	{
		LOGWARN(g_Log, "Failed to save screenshot '%s'\n", filePath.Chars());
	}
#else
	U32 width = pSurface->GetWidth();
	U32 height = pSurface->GetHeight();
	const U8* pAddress = (const U8*)pSurface->GetAddress();
	E_Multisample multisample = (E_Multisample)pSurface->GetMultisample();
	
	switch(multisample)
	{
	case E_Multisample_2:
		width *= 2;
		break;
	case E_Multisample_4:
		width *= 2;
		height *= 2;
		break;
	}
	
	SaveAsTGA(filePath, width, height, pAddress);
#endif
}

void TakeScreenshot(const Sys::C_FilePath fileName)
{
	if(!DeviceExists())
		return;
	
	#if defined(GRAPHICS_D3D9)
	{
		const Util::C_String filePath = 
			(Sys::GetDefaultFileSystem()->GetWorkingDirectory() + "/" + fileName).Chars();
		IDirect3DSurface9* pd3dSurface = NULL;
		Display::Device()->GetD3DDevice()->GetRenderTarget(0, &pd3dSurface);
		const HRESULT hr = D3DXSaveSurfaceToFile(filePath.Chars(), D3DXIFF_TGA, pd3dSurface, NULL, NULL);
		if(SUCCEEDED(hr))
		{
			LOGINFO(g_Log, "Successfully wrote file '%s'\n", filePath.Chars());
			pd3dSurface->Release();
		}
		else
		{
			LOGWARN(g_Log, "Failed to write file '%s'\n", filePath.Chars());
		}
	}
	#elif defined(GRAPHICS_GCM)
	{
		const C_Surface* pSurface = NULL;
#if 1
		switch(g_debugDisplayScreenshotMode)
		{
		case E_DisplayScreenShot_Depth:
			{
				const C_BackBuffer& backBuffer = Display::Device()->GetBackBuffer();
				pSurface = backBuffer.GetDepthStencilSurface();
			}
			break;
		default:
			{
				const C_BackBuffer& backBuffer = Display::Device()->GetBackBuffer();
				pSurface = backBuffer.GetColourSurface();
			}
			break;
		}
#else
		switch(g_debugDisplayScreenshotMode)
		{
		case E_DisplayScreenShot_Depth:
			{
				const C_BackBuffer& backBuffer = Display::Device()->GetBackBuffer();
				pSurface = backBuffer.GetDepthStencilSurface();
			}
			break;
		default:
			{
				const C_FrontBuffer& frontBuffer = Display::Device()->GetSwappedFrontBuffer();
				pSurface = frontBuffer.GetSurface();
			}
			break;
		}
#endif
		TakeScreenshot(fileName, pSurface);
	}
	#endif
}

}; // namespace Display
