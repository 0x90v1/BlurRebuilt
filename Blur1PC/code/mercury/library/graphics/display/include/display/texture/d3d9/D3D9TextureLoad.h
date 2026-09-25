#ifndef DISPLAY_TEXTURE_D3D9_D3D9_TEXTURE_LOAD_H
#define DISPLAY_TEXTURE_D3D9_D3D9_TEXTURE_LOAD_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Texture/D3D9/D3D9Texture.h"
#include "Display/Texture/D3D9/D3D9Texture2D.h"

namespace Sys
{
	class I_BinaryStream;
};

namespace Display
{
namespace D3D9
{

// Load a texture from a stream, including GPU data.
void LoadTexture2D(C_Texture2D *pTexture, Sys::I_BinaryStream *pStream);

void CloneTexture2D(C_Texture2D *pDst, const C_Texture2D *pSrc, int newWidth, int newHeight, int newLevels, int newSize);

// Load texture data from a stream, after creating the texture.
void LoadTextureData2D(C_Texture2D *pTexture, Sys::I_BinaryStream *pStream, int size);

// Allocate GPU resources for a texture.
void AllocateTextureMemory(C_Texture2D *pTexture);

#ifdef PLATFORM_PC

void PCLoadTexture(C_Texture2D *pTexture, Sys::I_BinaryStream *pStream);

// Copy as many mip levels as possible from pSrc to pDst.
void CopyTextureData2D(C_Texture2D *pDst, const C_Texture2D *pSrc);

// Create a texture from a stream, but don't load the data.
S32 CreateTexture2D(C_Texture2D *pTexture, Sys::I_BinaryStream *pStream);

#else

// Copy GPU data.
void CopyGPUData(void* pDst, const void* pSrc, S32 size);

// Create a texture from a stream, but don't load the data.
S32 CreateTexture2D(C_Texture2D *pTexture, Sys::I_BinaryStream *pStream, bool allocate = true);

#endif

}; // namespace D3D9
}; // namespace Display

#endif // DISPLAY_TEXTURE_D3D9_D3D9_TEXTURE_LOAD_H
