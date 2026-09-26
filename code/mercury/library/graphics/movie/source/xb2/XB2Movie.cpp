#include "stdafx.h"
#include <system/time/scopedtimer.h>
#include "Movie/XB2/XB2Movie.h"
#include "Movie/MovieTimings.h"
#include <xaudio2.h>

#ifdef CFG_ALLOW_OPTIMIZE_OFF_RENDER_MOVIE
#pragma optimize( "", off ) // CFG_ALLOW_OPTIMIZE_OFF_RENDER_MOVIE
#endif

namespace Audio
{
	namespace Core
	{
		extern IXAudio2* g_pXAudio2;
	}
}

void *bink_mem_alloc( U32 num_bytes )
{
	return BizAlignedMalloc( num_bytes, 32 );
}

void bink_mem_free( void *ptr )
{
	BizAlignedFree( ptr );
}

namespace Movie
{

#if !CFG_GOLD
Debug::Bool g_debugMovieDrawFrameInfo		("Graphics.Movie.DrawFrameInfo",		false);
Debug::Int	g_debugMovieDrawFrameInfoPosX	("Graphics.Movie.DrawFrameInfoPosX",	50,		Debug::Range(0, 1024));
Debug::Int	g_debugMovieDrawFrameInfoPosY	("Graphics.Movie.DrawFrameInfoPosX",	250,	Debug::Range(0, 768));
Debug::Int	g_debugMovieDrawFrameInfoSize	("Graphics.Movie.DrawFrameInfoSize",	50,		Debug::Range(0, 250));
#endif

bool C_XB2Movie::m_XAudioInit 			= false;
bool C_XB2Movie::m_XAudioUsed 			= false;

U32 C_XB2Movie::m_TextureMemoryFlags	= 0;
U32 C_XB2Movie::m_TextureMemorySize		= 0;
U8* C_XB2Movie::m_pTextureMemory		= NULL;

C_XB2Movie::C_XB2Movie() : 
	m_IsPlaying(false),
	m_FirstFrame(true),
	m_pBink(NULL)
{
	BinkSetMemory( bink_mem_alloc, bink_mem_free );
}

C_XB2Movie::~C_XB2Movie()
{
	BIZ_ASSERT(!m_pBink);
}

bool C_XB2Movie::HasReachedEnd() const
{
	if(m_pBink)
	{
		return m_pBink->FrameNum >= m_pBink->Frames ? true : false;
	}
	else
	{
		return true;
	}
}

void C_XB2Movie::SetAudio(const bool audio)
{
	if(!m_pBink)
		return;

	m_Audio = audio;

	//BinkSetSoundOnOff(m_pBink, m_Audio ? 1 : 0);
}

void C_XB2Movie::SetVideo(const bool video)
{
	if(!m_pBink)
		return;

	m_Video = video;

	BinkSetVideoOnOff(m_pBink, m_Video ? 1 : 0);
}

void C_XB2Movie::SetPaused(const bool paused)
{
	if(!m_pBink)
		return;

	m_Paused = paused;

	BinkPause(m_pBink, m_Paused ? 1 : 0);
}

void C_XB2Movie::SetVolume(const S32 volume)
{
	if(!m_pBink)
		return;

	if(m_XAudioUsed)
	{
		for (U32 i = 0; i < m_Params.m_NumTracks; ++i)
		{
			BinkSetVolume(m_pBink, m_Params.m_TrackIDsToPlay[i], volume);
		}
	}
	
	m_Volume = volume;
}

void C_XB2Movie::SetBinkIOSize(const U32 size)
{
	BinkSetIOSize(size);
}

bool C_XB2Movie::CreateBink(const C_MovieParams& params)
{
	m_IsPlaying = false;

	if(m_pBink)
		return false;

	if(!CreateShaders())
		return false;

	LOGDEBUG(g_MovieLog, "Playing movie %s\n", m_FileName.Chars());

	if (m_Params.m_CreateFlags & E_MovieCreateFlags_PlayFromMemory)
	{
		Sys::C_FileInfo fileInfo;
		Sys::GetDefaultFileSystem()->FileInfo(m_FileName.Chars(), &fileInfo);
		U32 fileSize = (U32)fileInfo.m_FileSize;
		U32 u32fileSize = (fileSize + 3) & ~3;
		m_pMovieData = new U8[u32fileSize];

		Sys::I_File* pFile = Sys::GetDefaultFileSystem()->Open(m_FileName.Chars());
		if(pFile)
		{
			{
				Time::C_ScopedTimer at("Loading movie");
				pFile->Read(m_pMovieData, fileSize);
			}

			#if defined(PLATFORM_XENON) || defined(PLATFORM_PS3)
			{
				Time::C_ScopedTimer at("Endian swapping movie");
				U32 *pData = (U32*)m_pMovieData;
				for(U32 i=0; i<u32fileSize/4; i++)
				{
					Ut::DoEndianSwap::EndianSwapD(pData);
					pData++;
				}
			}
			#endif

			SAFE_DELETE_PTR(pFile);
		}	
		else
		{
			return false;
		}
	}

	m_Params = params;

	if(!m_XAudioInit)
	{
		#ifndef CFG_DEBUG
		m_XAudioUsed = BinkSoundUseXAudio2(Audio::Core::g_pXAudio2) != 0 ? true : false;
		#else
		m_XAudioUsed = false;
		#endif
		m_XAudioInit = true;
	}

	if(m_XAudioUsed)
	{
		BinkSetSoundTrack(m_Params.m_NumTracks, m_Params.m_TrackIDsToPlay);
	}
	else
	{
		BinkSetSoundTrack(0, 0);
	}

	U32 flags = BINKSNDTRACK | BINKNOFRAMEBUFFERS | BINKALPHA;

	if(m_Params.m_CreateFlags & E_MovieCreateFlags_LoadIntoMemory)
	{
		flags |= BINKPRELOADALL;
	}

	if(m_Params.m_CreateFlags & E_MovieCreateFlags_PlayFromMemory)
	{
		flags |= BINKFROMMEMORY;
	}

	if(m_Params.m_CreateFlags & E_MovieCreateFlags_SetBufferIOSize)
	{
		BIZ_ASSERT(m_Params.m_BinkIOSize);
		flags |= BINKIOSIZE;
		//SetBinkIOSize(m_Params.m_BinkIOSize);
		SetBinkIOSize(1024 * 1024 * 8);
	}

	if(m_Params.m_CreateFlags & E_MovieCreateFlags_PlayFromMemory)
	{
		m_pBink = BinkOpen((const char*)m_pMovieData, flags);
	}
	else
	{
		if(m_FileName == "")
			return false; // Called when the renderers are created but the movie hasn't been setup yet?
		
		m_pBink = BinkOpen(m_FileName.Chars(), flags);
	}

	if(!m_pBink)
	{
		LOGERROR(g_MovieLog, "Failed to open bink file '%s'\n", m_FileName.Chars());
		return false;
	}

	if(!SetupSpeakers())
	{
		LOGERROR(g_MovieLog, "Failed to create the bink shaders for file '%s'\n", m_FileName.Chars());
		return false;
	}

	SetVolume(m_Volume);

	m_IsPlaying = true;

	return true;
}

void C_XB2Movie::DestroyBink()
{
	m_IsPlaying = false;

	DestroyShaders();
	
	if(m_pBink)
	{
		BinkClose(m_pBink);
		m_pBink = NULL;
	}

	if (m_Params.m_CreateFlags & E_MovieCreateFlags_PlayFromMemory)
	{
		SAFE_DELETE_ARY( m_pMovieData );
	}
}

bool C_XB2Movie::SetupSpeakers()
{
	if(m_XAudioUsed)
	{
		U32 bins[2];
		
		bins[0] = 0; // XAUDIO2SPEAKER_FRONTLEFT;
        bins[1] = 1; // XAUDIOSPEAKER_FRONTRIGHT;
        BinkSetMixBins(m_pBink, m_Params.m_TrackIDsToPlay[0], bins, 2);
        
		bins[0] = 2; // XAUDIOSPEAKER_FRONTCENTER;
        BinkSetMixBins(m_pBink, m_Params.m_TrackIDsToPlay[1], bins, 1);
        
		bins[0] = 3; // XAUDIOSPEAKER_LOWFREQUENCY;
        BinkSetMixBins(m_pBink, m_Params.m_TrackIDsToPlay[2], bins, 1);

        bins[0] = 4; // XAUDIOSPEAKER_BACKLEFT;
        bins[1] = 5; // XAUDIOSPEAKER_BACKRIGHT;
        BinkSetMixBins(m_pBink, m_Params.m_TrackIDsToPlay[3], bins, 2);

		// Mono VO channel
		bins[0] = 2; // XAUDIOSPEAKER_FRONTCENTER
		BinkSetMixBins(m_pBink, m_Params.m_TrackIDsToPlay[4], bins, 1);

		// Stereo VO channel
		bins[0] = 0; // XAUDIOSPEAKER_FRONTLEFT
		bins[1] = 1; // XAUDIOSPEAKER_FRONTRIGHT
		BinkSetMixBins(m_pBink, m_Params.m_TrackIDsToPlay[5], bins, 2);
	}
	
	return true;
}

bool C_XB2Movie::CreateShaders()
{
	if(!Create_Bink_shaders(Display::Device()->GetD3DDevice()))
		return false;

	return true;
}

void C_XB2Movie::DestroyShaders()
{
	//Free_Bink_shaders();
}

bool C_XB2Movie::CreateTextures()
{
	if(!m_pBink)
		return false;

	U32 flags = 0;

	if(m_Params.m_MemoryFlags == E_MovieMemoryFlags_System)
		flags = Display::E_Usage_System;
	else
		flags = Display::E_Usage_Video;

	if(m_pTextureMemory)
		flags |= Display::E_Usage_Indirect;
	
	if(!Display::Device()->CreateRenderTexture2D(m_pBink->Width, m_pBink->Height, 1, flags, Display::E_Format_A8R8G8B8, Display::E_Multisample_None, NULL, &m_RenderTexture, m_pTextureMemory, m_TextureMemorySize))
		return false;

	#if TRACK_DISPLAY2_RESOURCES
	Display2::Device()->SetRenderTexture2DName(&m_RenderTexture, "_MovieRT");
	#endif

	BinkGetFrameBuffersInfo(m_pBink, &m_BinkTextureSet.bink_buffers);

	if(!Create_Bink_textures(Display::Device()->GetD3DDevice(), &m_BinkTextureSet))
		return false;

	BinkRegisterFrameBuffers(m_pBink, &m_BinkTextureSet.bink_buffers);

	return true;
}

void C_XB2Movie::DestroyTextures()
{
	if(!m_pBink)
		return;

	Free_Bink_textures(Display::Device()->GetD3DDevice(), &m_BinkTextureSet);

	Display::Device()->Destroy(&m_RenderTexture);
}

void C_XB2Movie::Rewind()
{
	if(!m_pBink)
		return;

	m_pBink->FrameNum = 0;

	m_IsPlaying = true;
}

bool C_XB2Movie::Update()
{
	if(!m_pBink)
		return true;

	if(m_IsPlaying)
	{
		if(m_pBink->ReadError != 0)
		{
			m_IsPlaying = false; // Bink read error
		}
		//else
		//if(m_pBink->FrameNum >= m_pBink->Frames)
		//{
			//m_IsPlaying = false; // Bink has played all the frames of the movie
		//}
	}
	
	return !m_IsPlaying;
}

#if !CFG_GOLD
void C_XB2Movie::DrawFrameInfo()
{
	if(!m_pBink)
		return;

	Display::C_View view;
	Display::C_ViewSetup viewSetup;
	Display::Device()->SetupView(&view, viewSetup, m_pBink->Width, m_pBink->Height);
	Display::Device()->SetView(&view);

	F32 posx = (F32)g_debugMovieDrawFrameInfoPosX;
	F32 posy = (F32)g_debugMovieDrawFrameInfoPosY;
	F32 size = (F32)g_debugMovieDrawFrameInfoSize;

	char str[128];
	sprintf_s(str, sizeof(str), "%i / %i (%i)", m_pBink->FrameNum, m_pBink->Frames, m_Looping);
	Display::FontRender()->Draw(str, posx, posy, size, size, Maths::RGBA(255, 255, 255, 255));

	Display::Device()->SetView(NULL);
}
#endif

void C_XB2Movie::Render()
{
	if(!m_IsPlaying)
		return;

	if(m_pBink && !BinkWait(m_pBink))
	{
		MOVIE_TIMING("Render", MOVIE_DEFAULT_THRESHOLD, m_pBink->FrameNum, m_pBink->Frames);

		{
			MOVIE_TIMING("Wait_for_Bink_textures", MOVIE_DEFAULT_THRESHOLD, m_pBink->FrameNum, m_pBink->Frames);
			Wait_for_Bink_textures(&m_BinkTextureSet);
		}

		{
			MOVIE_TIMING("BinkDoFrame", MOVIE_DEFAULT_THRESHOLD, m_pBink->FrameNum, m_pBink->Frames);
			BinkDoFrame(m_pBink);
		}

		{
			MOVIE_TIMING("BinkShouldSkip", MOVIE_DEFAULT_THRESHOLD, m_pBink->FrameNum, m_pBink->Frames);
			while(BinkShouldSkip(m_pBink))
			{
				MOVIE_TIMING("BinkSkip", MOVIE_DEFAULT_THRESHOLD, m_pBink->FrameNum, m_pBink->Frames);
				BinkNextFrame(m_pBink);
				BinkDoFrame(m_pBink);
			}
		}

		Display::Device()->SetColourDepthStencilSurface(&m_RenderTexture.GetSurface(), NULL);
		Display::Device()->Clear(Display::E_Clear_Colour, Maths::RGBA(0, 0, 0, 255));
		Display::Device()->SetAlphaTestEnable(Display::E_Bool_False);
		Display::Device()->SetDepthEnable(Display::E_Bool_False);
		
		U32 flags = Display::E_ColourWriteEnable_Red|Display::E_ColourWriteEnable_Green|Display::E_ColourWriteEnable_Blue;
		if(m_pBink->BinkType & BINKALPHA)
			flags |= Display::E_ColourWriteEnable_Alpha;
		Display::Device()->SetColourWriteEnable(flags);
		
		Display::C_Viewport viewport;
		viewport.m_W = m_pBink->Width;
		viewport.m_H = m_pBink->Height;
		Display::Device()->SetViewport(viewport);

		F32 uEnd0 = 1.f;
		F32 vEnd0 = 1.f;
		F32 uEnd1 = 1.f;
		F32 vEnd1 = 1.f;
		C_MovieBase::CalculateUVRange(m_BinkTextureSet.bink_buffers, uEnd0, vEnd0, uEnd1, vEnd1);

		{
			MOVIE_TIMING("Draw_Bink_textures", MOVIE_DEFAULT_THRESHOLD, m_pBink->FrameNum, m_pBink->Frames);
			Draw_Bink_textures(
				Display::Device()->GetD3DDevice(),
				&m_BinkTextureSet,
				m_pBink->Width, m_pBink->Height,
				0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
				uEnd1, vEnd1);
		}

		#if !CFG_GOLD
		if(g_debugMovieDrawFrameInfo)
			DrawFrameInfo();
		#endif

		Display::Device()->Resolve(Display::E_Resolve_Colour, &m_RenderTexture.GetTexture());
		Display::Device()->SetColourWriteEnable(Display::E_ColourWriteEnable_All);
		Display::Device()->SetColourDepthStencilSurface(NULL, NULL);
		Display::Device()->InvalidateState();

		if(m_pBink->FrameNum < m_pBink->Frames || m_Looping)
		{
			MOVIE_TIMING("BinkNextFrame", MOVIE_DEFAULT_THRESHOLD, m_pBink->FrameNum, m_pBink->Frames);
			
			bool suspendAndResume = m_pBink->FrameNum >= m_pBink->Frames ? true : false;

			if(suspendAndResume)
				Display::Device()->GetD3DDevice()->Suspend();

			BinkNextFrame(m_pBink);

			if(suspendAndResume)
				Display::Device()->GetD3DDevice()->Resume();
		}
		else
		{
			m_IsPlaying = false;
		}

		m_FirstFrame = false;
	}
	else
	{
		if(m_FirstFrame)
		{
			Display::Device()->SetColourDepthStencilSurface(&m_RenderTexture.GetSurface(), NULL);
			Display::Device()->Clear(Display::E_Clear_Colour, Maths::RGBA(0, 0, 0, 255));

			#if !CFG_GOLD
			if(g_debugMovieDrawFrameInfo)
				DrawFrameInfo();
			#endif

			Display::Device()->Resolve(Display::E_Resolve_Colour, &m_RenderTexture.GetTexture());
			Display::Device()->SetColourDepthStencilSurface(NULL, NULL);
			Display::Device()->InvalidateState();

			m_FirstFrame = false;
		}
	}
}

void C_XB2Movie::Unbind()
{
	Display::Device()->GetD3DDevice()->SetVertexShader(NULL);
	Display::Device()->GetD3DDevice()->SetPixelShader(NULL);

	Display::Device()->GetD3DDevice()->SetTexture(0, NULL);
	Display::Device()->GetD3DDevice()->SetTexture(1, NULL);
	Display::Device()->GetD3DDevice()->SetTexture(2, NULL);
	Display::Device()->GetD3DDevice()->SetTexture(3, NULL);

	Display::Device()->GetD3DDevice()->BlockUntilIdle();
}

bool C_XB2Movie::AllocTextureMemory()
{
	if(m_pTextureMemory)
		return true;

	m_TextureMemoryFlags = Display2::E_Flags_Video;
	m_TextureMemorySize = Display::GetTexture2DSize(1280, 720, m_TextureMemoryFlags, Display::E_Format_A8R8G8B8);

	if(!Display2::Device()->AllocMemory(m_TextureMemorySize, ALIGNMENT_TEXTURE2D, m_TextureMemoryFlags, &m_pTextureMemory))
	{
		LOGERROR(g_MovieLog, "Failed to alloc %i bytes of movie texture memory\n", m_TextureMemorySize);
		return false;
	}

	return true;
}

void C_XB2Movie::FreeTextureMemory()
{
	if(!m_pTextureMemory)
		return;

	Display2::Device()->FreeMemory(m_TextureMemorySize, m_pTextureMemory);

	m_TextureMemoryFlags = 0;
	m_TextureMemorySize = 0;
	m_pTextureMemory = NULL;
}

}; // namespace Movie
