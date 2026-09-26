#ifndef MOVIE_XB2_MOVIE_H
#define MOVIE_XB2_MOVIE_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Bink.h"
#include "Movie/XB2/Bink/BinkTextures.h"
#include "Movie/MovieBase.h"
#include <core/Threads/Mutex.h>
#include <Core/Threads/Event.h>
#include <Core/Threads/Thread.h>

namespace Movie
{

//! C_XB2Movie

class C_XB2Movie : public C_MovieBase
{
public:

	C_XB2Movie();
	virtual ~C_XB2Movie();

	bool IsCreated() const;
	bool HasReachedEnd() const;

	void SetAudio(const bool audio);
	void SetVideo(const bool video);
	void SetPaused(const bool paused);
	void SetVolume(const S32 volume);
	void SetBinkIOSize(const U32 size);

	void Rewind();
	bool Update();
	void Render();

	bool CreateTextures();
	void DestroyTextures();

	static bool AllocTextureMemory();
	static void FreeTextureMemory();

protected:

	bool CreateBink(const C_MovieParams& params);
	void DestroyBink();

	bool SetupSpeakers();
	
	bool CreateShaders();
	void DestroyShaders();

	void Unbind();

	#if !CFG_GOLD
	void DrawFrameInfo();
	#endif

protected:

	static bool			m_XAudioInit;
	static bool			m_XAudioUsed;

	bool				m_FirstFrame;
	bool				m_IsPlaying;
	HBINK				m_pBink;
	BINKTEXTURESET		m_BinkTextureSet;

	static U32			m_TextureMemoryFlags;
	static U32			m_TextureMemorySize;
	static U8*			m_pTextureMemory;
	U8*					m_pMovieData;

};

}; // namespace Movie

#include "Movie/XB2/XB2Movie.inl"

#endif // MOVIE_XB2_MOVIE_H
