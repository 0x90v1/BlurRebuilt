#pragma once

#ifdef PLATFORM_XENON
#ifndef USE_XAUDIO2

const INT QUADFX_NUM_RMS_VALUES = 64;

//--------------------------------------------------------------------------------------
// Constants and defines
//--------------------------------------------------------------------------------------
// Effect IDs for all of our static effects.  These are passed as parameters to the 
// effect creation functions
#define QUADEFFECT_ID  ( XAUDIOFXID_FIRSTCLIENT+1 )

const XAUDIOFXPARAMID QUADFX_REAR_GAIN_ID = 0;
const XAUDIOFXPARAMID QUADFX_RESPONSE_ID = 1;
const XAUDIOFXPARAMID QUADFX_BALANCE_ID = 2;
const XAUDIOFXPARAMID QUADFX_LEVEL_ID = 3;
const XAUDIOFXPARAMID QUADFX_MODE_ID = 4;

#ifndef ASSERT
#define ASSERT assert       // For XAUDIO_USE_BATCHALLOC_NEWDELETE macro
#endif

// Parameter ranges and default values
const FLOAT QUADFX_LEVEL_MIN = 0.0f;
const FLOAT QUADFX_LEVEL_MAX = 1.0f;

const FLOAT QUADFX_REAR_GAIN_MIN = 0.0f;
const FLOAT QUADFX_REAR_GAIN_MAX = 1.0f;
const FLOAT QUADFX_REAR_GAIN_DEFAULT = 0.5f;

const FLOAT QUADFX_RESPONSE_MIN = 0.001f;
const FLOAT QUADFX_RESPONSE_MAX = 0.999f;
const FLOAT QUADFX_RESPONSE_DEFAULT = 0.025f;

const FLOAT QUADFX_BALANCE_MIN = 0.0f;
const FLOAT QUADFX_BALANCE_MAX = 1.0f;
const FLOAT QUADFX_BALANCE_DEFAULT = 1.0f;

//--------------------------------------------------------------------------------------
// Declarations
//--------------------------------------------------------------------------------------

// Size function.
HRESULT QuerySizeQuadEffect( LPCXAUDIOFXINIT pInit, LPDWORD pEffectSize );

// Effect creation function.  Part of the data needed by XAudioInitialize().
HRESULT CreateQuadEffect( const XAUDIOFXINIT *pInit, 
                           IXAudioBatchAllocator *pAllocator, 
                           IXAudioEffect **ppEffect );


//--------------------------------------------------------------------------------------
// Class definition
//--------------------------------------------------------------------------------------
class CQuadEffect: public IXAudioEffect
{
    XAUDIO_USE_BATCHALLOC_NEWDELETE();

    public:
        // IXAudioRefCount
        STDMETHOD_( ULONG, AddRef )( void );
        STDMETHOD_( ULONG, Release )( void );

        // IXAudioEffect    
        STDMETHOD( GetInfo )    ( XAUDIOFXINFO* pInfo );
        STDMETHOD( GetParam )   ( XAUDIOFXPARAMID nParamId, XAUDIOFXPARAMTYPE nParamType, XAUDIOFXPARAM* pParam );
        STDMETHOD( SetParam )   ( XAUDIOFXPARAMID nParamId, XAUDIOFXPARAMTYPE nParamType, const XAUDIOFXPARAM* pParam );
        STDMETHOD( GetContext ) ( LPVOID* ppvContext );
        STDMETHOD( Process )    ( IXAudioFrameBuffer* pInputBuffer, IXAudioFrameBuffer* pOutputBuffer );

        //CUserEffect
        CQuadEffect( LPVOID pvContext );
        STDMETHOD( Initialize )( const XAUDIOFXINIT* pInit, IXAudioBatchAllocator* pAllocator );

    private:
        LPVOID  m_pvContext;
        DWORD   m_dwRefCount;

		U32 m_uMode;

		F32	m_fLevel;

		F32 m_fRearGain;
		F32 m_fResponse;
		F32 m_fBalance;

		U32 m_uCurrSOS;
		F32 m_afFrontSOS[QUADFX_NUM_RMS_VALUES];
		F32 m_afRearSOS[QUADFX_NUM_RMS_VALUES];

	    F32 m_fFrontRMS;
	    F32 m_fRearRMS;

		F32 m_fCurrRearGain;
};

#endif // USE_XAUDIO2
#endif // PLATFORM_XENON
