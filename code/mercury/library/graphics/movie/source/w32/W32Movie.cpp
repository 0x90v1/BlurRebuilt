#include "stdafx.h"
#include "Movie/W32/W32Movie.h"
#include <d3d9.h>
#include <d3dx9.h>

extern Debug::Bool g_debugMovieDisableRender;

#ifndef CFG_GOLD
Debug::Bool	g_debugRenderMovieShowBinkSummary	("Graphics.Render.Movie.ShowBinkSummary",	false);
Debug::Int	g_debugRenderMovieBinkSummaryPosX	("Graphics.Render.Movie.BinkSummaryPosX",	50, Debug::Range(0, 1024));
Debug::Int	g_debugRenderMovieBinkSummaryPosY	("Graphics.Render.Movie.BinkSummaryPosY",	150, Debug::Range(0, 768));
Debug::Int	g_debugRenderMovieBinkSummarySize	("Graphics.Render.Movie.BinkSummarySize",	15, Debug::Range(0, 50));
Debug::Int	g_debugRenderMovieBinkSummaryFade	("Graphics.Render.Movie.BinkSummaryFade",	128, Debug::Range(0, 255));
#endif

namespace Movie
{

C_W32Movie::C_W32Movie() :
	m_BinkIsDirty(false),
	m_BinkIsPlaying(false),
	m_pBink(NULL)
{
}

C_W32Movie::~C_W32Movie()
{
	BIZ_ASSERT(!m_pBink);
}

bool C_W32Movie::HasReachedEnd() const
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

void C_W32Movie::SetPaused(const bool paused)
{
	m_Paused = paused;

	if(m_pBink)
		BinkPause(m_pBink, m_Paused ? 1 : 0);
}

void C_W32Movie::SetVolume(const S32 volume)
{
	if(!m_pBink)
		return;

	for (U32 i = 0; i < m_Params.m_NumTracks; ++i)
	{
		BinkSetVolume(m_pBink, m_Params.m_TrackIDsToPlay[i], volume);
	}
	
	m_Volume = volume;
}

void C_W32Movie::SetBinkIOSize(U32 size)
{
	BinkSetIOSize(size);
}

bool C_W32Movie::CreateBink(const C_MovieParams& params)
{
	if(m_pBink)
		return false;

	m_BinkIsPlaying = false;

	if(!CreateShaders())
		return false;

	//const Util::C_String filePath = Display::FormatString(Sys::C_FileSystem::Instance().GetWorkingDirectory() + "/" + m_FileName);
	const Util::C_String filePath = m_FileName;

	m_Params = params;
	BinkSetSoundTrack(m_Params.m_NumTracks, m_Params.m_TrackIDsToPlay);

	U32 flags = BINKSNDTRACK | BINKNOFRAMEBUFFERS | BINKALPHA;

	if(m_Params.m_CreateFlags == E_MovieCreateFlags_LoadIntoMemory)
		flags |= BINKPRELOADALL;
	if(m_Params.m_CreateFlags == E_MovieCreateFlags_PlayFromMemory)
		flags |= BINKFROMMEMORY;
	if(m_Params.m_CreateFlags & E_MovieCreateFlags_SetBufferIOSize)
		flags |= BINKIOSIZE;

	BinkSoundUseWaveOut();

	if(m_Params.m_CreateFlags == E_MovieCreateFlags_PlayFromMemory)
	{
		m_pBink = BinkOpen((const char*)m_Params.m_pMovieData, flags);
	}
	else
	{
		m_pBink = BinkOpen(filePath.Chars(), flags);
	}

#ifdef MILES//PLATFORM_PC
	U32 volumes[ 6 ]; // 6 channels for 5.1

	// turn on the front left and right for the first Bink track
	memset( volumes, 0, sizeof( volumes ) );
	volumes[ 0 ] = 0;
	volumes[ 1 ] = 0;
	BinkSetMixBinVolumes( m_pBink, m_Params.m_TrackIDsToPlay[0], 0, (S32*)&volumes, 6 ); 

	// turn on the center for the second Bink track
	memset( volumes, 0, sizeof( volumes ) );
	volumes[ 2 ] = 0;
	BinkSetMixBinVolumes( m_pBink, m_Params.m_TrackIDsToPlay[1], 0, (S32*)&volumes, 6 ); 

	// turn on the sub woofer for the third Bink track
	memset( volumes, 0, sizeof( volumes ) );
	volumes[ 3 ] = 0;
	BinkSetMixBinVolumes( m_pBink, m_Params.m_TrackIDsToPlay[2], 0, (S32*)&volumes, 6 ); 

	// turn on the back left and right for the fourth Bink track
	memset( volumes, 0, sizeof( volumes ) );
	volumes[ 4 ] = 0;
	volumes[ 5 ] = 0;
	BinkSetMixBinVolumes( m_pBink, m_Params.m_TrackIDsToPlay[3], 0, (S32*)&volumes, 6 ); 

	// route fifth Bink track to centre speaker (mono VO channel)
	memset( volumes, 0, sizeof( volumes ) );
	volumes[ 2 ] = 0;
	BinkSetMixBinVolumes( m_pBink, m_Params.m_TrackIDsToPlay[4], 0, (S32*)&volumes, 6 ); 

	// route sixth and final Bink track to front speakers (stereo VO channel)
	memset( volumes, 0, sizeof( volumes ) );
	volumes[ 4 ] = 0;
	volumes[ 5 ] = 0;
	BinkSetMixBinVolumes( m_pBink, m_Params.m_TrackIDsToPlay[5], 0, (S32*)&volumes, 6 ); 
#endif

	if(!m_pBink)
	{
		LOGWARN(Render::g_Log, "Failed to open file '%s'\n", filePath.Chars());
		return false;
	}

	if(m_pBink->FrameRateDiv>0)
	{
		F32 frame_rate = (F32)m_pBink->FrameRate / (F32)m_pBink->FrameRateDiv;
		m_TotalTime = (F32)m_pBink->Frames / frame_rate;
	}

	m_BinkIsPlaying = true;

	return true;
}

void C_W32Movie::DestroyBink()
{
	//DestroyShaders();

	if(m_pBink)
	{
		BinkClose(m_pBink);
		m_pBink = NULL;
	}

	m_BinkIsPlaying = false;
}

bool C_W32Movie::CreateShaders()
{
	if(!Create_Bink_shaders(Display::Device()->GetD3DDevice()))
		return false;

	return true;
}

void C_W32Movie::DestroyShaders()
{
	Free_Bink_shaders();
}

S32 FindNextPowerOf2(const S32 n)
{
	S32 p2;
	for(p2=1; p2<n; p2 <<= 1);
	return p2;
}

void ResetBinkTexture(IDirect3DTexture9* pd3dTexture, const S32 value)
{
	BIZ_ASSERT(pd3dTexture);

	D3DSURFACE_DESC desc;
	HRESULT hr = pd3dTexture->GetLevelDesc(0, &desc);
	if(FAILED(hr))
	{
		BIZ_ASSERT(0);
		return;
	}

	D3DLOCKED_RECT lockedRect;
	hr = pd3dTexture->LockRect(0, &lockedRect, NULL, NULL);
	if(FAILED(hr))
	{
		BIZ_ASSERT(0);
		return;
	}

	BIZ_ASSERT(desc.Format == D3DFMT_L8);

	U32 width = 0;
	U32 height = 0;
	
	if(desc.Pool != D3DPOOL_SYSTEMMEM && Display::D3D9::C_DeviceCaps::NonPowerOfTwoTextures())
	{
		width = FindNextPowerOf2(desc.Width);
		height = FindNextPowerOf2(desc.Height);
	}
	else
	{
		width = desc.Width;
		height = desc.Height;
	}

	const U32 size = width * height * 1;

	memset(lockedRect.pBits, value, size);

	pd3dTexture->UnlockRect(0);
}

void RGBtoYUV(const S32 R, const S32 G, const S32 B, S32& Y, S32& U, S32& V)
{
	Y  = (S32)((0.257f * (F32)R) + (0.504f * (F32)G) + (0.098f * (F32)B) + 16.0f);
	V =  (S32)((0.439f * (F32)R) - (0.368f * (F32)G) - (0.071f * (F32)B) + 128.0f);
	U = -(S32)((0.148f * (F32)R) - (0.291f * (F32)G) + (0.439f * (F32)B) + 128.0f);
}

void ResetBinkFrameTexture(BINKFRAMETEXTURES& ft)
{
	S32 Y = 0, U = 0, V = 0;
	RGBtoYUV(0, 0, 0, Y, U, V);

	if(ft.Ytexture)
		ResetBinkTexture(ft.Ytexture, Y);
	if(ft.cRtexture)
		ResetBinkTexture(ft.cRtexture, V);
	if(ft.cBtexture)
		ResetBinkTexture(ft.cBtexture, U);
	if(ft.Atexture)
		ResetBinkTexture(ft.Atexture, 0);
}

bool C_W32Movie::CreateTextures()
{
	if(!m_pBink)
		return false;

	if(!Display::Device()->CreateRenderTexture2D(m_pBink->Width, m_pBink->Height, 1, Display::E_Usage_Colour, Display::E_Format_A8R8G8B8, Display::E_Multisample_None, NULL, &m_RenderTexture))
		return false;

	#if TRACK_DISPLAY2_RESOURCES
	Display2::Device()->SetRenderTexture2DName(&m_RenderTexture, "_MovieRT");
	#endif

	BinkGetFrameBuffersInfo(m_pBink, &m_BinkTextureSet.bink_buffers);

	if(!Create_Bink_textures(Display::Device()->GetD3DDevice(), &m_BinkTextureSet))
		return false;

	BinkRegisterFrameBuffers(m_pBink, &m_BinkTextureSet.bink_buffers);

	Rewind();

	for(U32 i=0; i<BINKMAXFRAMEBUFFERS; i++)
		ResetBinkFrameTexture(m_BinkTextureSet.textures[i]);
	//ResetBinkFrameTexture(m_BinkTextureSet.tex_draw);

	m_BinkIsDirty = true;

	return true;
}

void C_W32Movie::DestroyTextures()
{
	if(!m_pBink)
		return;

	Display::Device()->Destroy(&m_RenderTexture);

	Free_Bink_textures(Display::Device()->GetD3DDevice(), &m_BinkTextureSet);
}

void C_W32Movie::Rewind()
{
	if(!m_pBink)
		return;

	m_pBink->FrameNum = 0;

	m_BinkIsPlaying = true;
}

bool C_W32Movie::Update()
{
	if(!m_pBink)
		return true;

	if(m_BinkIsPlaying)
	{
		if(m_pBink->ReadError != 0)
		{
			m_BinkIsPlaying = false; // Bink read error
		}
		else
		if(m_pBink->FrameNum >= m_pBink->Frames)
		{
			m_BinkIsPlaying = false; // Bink has played all the frames of the movie
		}
	}

	if(m_pBink->FrameRateDiv>0)
	{
		F32 frame_rate = (F32)m_pBink->FrameRate / (F32)m_pBink->FrameRateDiv;
		m_CurrentTime = (F32)m_pBink->FrameNum / frame_rate;
	}

	return !m_BinkIsPlaying;
}

void C_W32Movie::Render()
{
	#ifndef CFG_GOLD
	if(g_debugMovieDisableRender)
		return;
	#endif

	if(m_pBink && !BinkWait(m_pBink))
	{
		Display::Device()->SetColourDepthStencilSurface(&m_RenderTexture.GetSurface(), NULL);

		Display::Device()->Clear(Display::E_Clear_Colour, Maths::RGBA(0, 0, 0, 0));
		Display::Device()->SetDepthEnable(Display::E_Bool_False);
		Display::Device()->SetColourWriteEnable(Display::E_ColourWriteEnable_All);

		Display::C_Viewport viewport;
		viewport.m_X = 0;
		viewport.m_Y = 0;
		viewport.m_W = m_pBink->Width;
		viewport.m_H = m_pBink->Height;
		Display::Device()->SetViewport(viewport);

		// Lock the textures
		Lock_Bink_textures(&m_BinkTextureSet);

		// Register our locked texture pointers with Bink
		BinkRegisterFrameBuffers(m_pBink, &m_BinkTextureSet.bink_buffers);

		// Decompress the next frame (but we don't show it yet)
		BinkDoFrame(m_pBink);

		// Do we need to skip a frame?
		while(BinkShouldSkip(m_pBink))
		{
			BinkNextFrame(m_pBink); // TRC VIOLATION : Need to check for m_pBink->ReadError != 0 and handle appropriatly after BinkNextFrame call
			BinkDoFrame(m_pBink);

			if(m_pBink->FrameNum >= m_pBink->Frames)
			{
				m_BinkIsPlaying = false; // Bink has played all the frames of the movie
			}
		}

		// Unlock the textures
		Unlock_Bink_textures(Display::Device()->GetD3DDevice(), &m_BinkTextureSet, m_pBink, m_BinkIsDirty);

		F32 uEnd0 = 1.f; F32 vEnd0 = 1.f;
		F32 uEnd1 = 1.f; F32 vEnd1 = 1.f;

		C_MovieBase::CalculateUVRange(m_BinkTextureSet.bink_buffers, uEnd0, vEnd0, uEnd1, vEnd1);
		
		// Draw the texture
		Draw_Bink_textures(
			Display::Device()->GetD3DDevice(),
			&m_BinkTextureSet,
			m_pBink->Width, m_pBink->Height,
			0.0f, 0.0f, 1.0f, 1.0f, 1.0f, uEnd1, vEnd1);

		// Keep playing the movie
		if(!m_Paused && (m_pBink->FrameNum < m_pBink->Frames || m_Looping))
		{
			BinkNextFrame(m_pBink); // TRC VIOLATION : Need to check for m_pBink->ReadError != 0 and handle appropriatly after BinkNextFrame call
		}
	
		Display::Device()->InvalidateState();

		m_BinkIsDirty = false;
	}
	else
	{
		if(m_BinkIsDirty)
		{
			Display::Device()->SetColourDepthStencilSurface(&m_RenderTexture.GetSurface(), NULL);

			Display::Device()->Clear(Display::E_Clear_Colour, Maths::RGBA(0, 0, 0, 255));
			
			Display::Device()->InvalidateState();

			m_BinkIsDirty = false;
		}
	}
}

void C_W32Movie::DebugRender(F32& x, F32& y, const F32 s)
{
	#ifndef CFG_GOLD
	if(!m_pBink)
		return;

	Display::BasicRender()->DrawTextured2DRect(x, y, s, s, Maths::RGBA(255, 255, 255, 255), &m_RenderTexture.GetTexture());
	x += s;
	
	if(g_debugRenderMovieShowBinkSummary)
	{
		BINKSUMMARY summary;
		BinkGetSummary(m_pBink, &summary);

		Display::BasicRender()->DrawColoured2DRect(20.f, 20.f, 984.f, 728.f, Maths::RGBA(0, 0, 0, g_debugRenderMovieBinkSummaryFade));

		F32 x = (F32)g_debugRenderMovieBinkSummaryPosX;
		F32 y = (F32)g_debugRenderMovieBinkSummaryPosY;
		F32 s = (F32)g_debugRenderMovieBinkSummarySize;

		Util::C_String temp;

		temp.Printf("Width %i Height %i TotalTime %i",
			summary.Width,
			summary.Height,
			summary.TotalTime);
		Display::FontRender()->Draw(temp.Chars(), x, y, s, s, Maths::RGBA(255, 255, 255, 255));
		y += s*1.5f;

		temp.Printf("FileFrameRate %i FileFrameRateDiv %i",
			summary.FileFrameRate,
			summary.FileFrameRateDiv);
		Display::FontRender()->Draw(temp.Chars(), x, y, s, s, Maths::RGBA(255, 255, 255, 255));
		y += s*1.5f;

		temp.Printf("FrameRate % FrameRateDiv %i",
			summary.FrameRate,
			summary.FrameRateDiv);
		Display::FontRender()->Draw(temp.Chars(), x, y, s, s, Maths::RGBA(255, 255, 255, 255));
		y += s*1.5f;

		temp.Printf("TotalOpenTime %i TotalFrames %i TotalPlayedFrames %i",
			summary.TotalOpenTime,
			summary.TotalFrames,
			summary.TotalPlayedFrames);
		Display::FontRender()->Draw(temp.Chars(), x, y, s, s, Maths::RGBA(255, 255, 255, 255));
		y += s*1.5f;

		temp.Printf("SkippedFrames %i SkippedBlits %i SoundSkips %i",
			summary.SkippedFrames,
			summary.SkippedBlits,
			summary.SoundSkips);
		Display::FontRender()->Draw(temp.Chars(), x, y, s, s, Maths::RGBA(255, 255, 255, 255));
		y += s*1.5f;

		temp.Printf("SoundSkips %i TotalBlitTime %i TotalReadTime %i",
			summary.SoundSkips,
			summary.TotalBlitTime,
			summary.TotalReadTime);
		Display::FontRender()->Draw(temp.Chars(), x, y, s, s, Maths::RGBA(255, 255, 255, 255));
		y += s*1.5f;

		temp.Printf("TotalVideoDecompTime %i TotalAudioDecompTime %i",
			summary.TotalVideoDecompTime,
			summary.TotalAudioDecompTime);
		Display::FontRender()->Draw(temp.Chars(), x, y, s, s, Maths::RGBA(255, 255, 255, 255));
		y += s*1.5f;

		temp.Printf("TotalIdleReadTime %i TotalBackReadTime %i TotalReadSpeed %i",
			summary.TotalIdleReadTime,
			summary.TotalBackReadTime,
			summary.TotalReadSpeed);
		Display::FontRender()->Draw(temp.Chars(), x, y, s, s, Maths::RGBA(255, 255, 255, 255));
		y += s*1.5f;

		temp.Printf("SlowestFrameTime %i Slowest2FrameTime %i",
			summary.SlowestFrameTime,
			summary.Slowest2FrameTime);
		Display::FontRender()->Draw(temp.Chars(), x, y, s, s, Maths::RGBA(255, 255, 255, 255));
		y += s*1.5f;

		temp.Printf("SlowestFrameNum %i Slowest2FrameNum %i",
			summary.SlowestFrameNum,
			summary.Slowest2FrameNum);
		Display::FontRender()->Draw(temp.Chars(), x, y, s, s, Maths::RGBA(255, 255, 255, 255));
		y += s*1.5f;

		temp.Printf("AverageDataRate %i AverageFrameSize %i",
			summary.AverageDataRate,
			summary.AverageFrameSize);
		Display::FontRender()->Draw(temp.Chars(), x, y, s, s, Maths::RGBA(255, 255, 255, 255));
		y += s*1.5f;

		temp.Printf("HighestMemAmount %i TotalIOMemory %i",
			summary.HighestMemAmount,
			summary.TotalIOMemory);
		Display::FontRender()->Draw(temp.Chars(), x, y, s, s, Maths::RGBA(255, 255, 255, 255));
		y += s*1.5f;

		temp.Printf("HighestIOUsed %i Highest1SecRate %i Highest1SecFrame %i",
			summary.HighestIOUsed,
			summary.Highest1SecRate,
			summary.Highest1SecFrame);
		Display::FontRender()->Draw(temp.Chars(), x, y, s, s, Maths::RGBA(255, 255, 255, 255));
		y += s*1.5f;

		static U32 TotalVideoDecompTime = 0;
		static U32 TotalAudioDecompTime = 0;

		temp.Printf("LastVideoDecompTime %i LastAudioDecompTime %i",
			summary.TotalVideoDecompTime-TotalVideoDecompTime,
			summary.TotalAudioDecompTime-TotalAudioDecompTime);
		Display::FontRender()->Draw(temp.Chars(), x, y, s, s, Maths::RGBA(255, 255, 255, 255));
		y += s*1.5f;

		TotalVideoDecompTime = summary.TotalVideoDecompTime;
		TotalAudioDecompTime = summary.TotalAudioDecompTime;
	}
	#endif
}

}; // namespace Movie
