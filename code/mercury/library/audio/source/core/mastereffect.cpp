#include "AudioPch.h"
#include "coreheaders.h"

#ifdef PLATFORM_XENON
#include "mastereffect.h"

#ifdef AUDIO_OPTIMIZE_OFF
#pragma optimize ("", off) // AUDIO_OPTIMIZE_OFF
#endif	//	AUDIO_OPTIMIZE_OFF

#ifdef USE_XAUDIO2

Debug::Float g_MasterEffectRMSMuteThreshold("Audio.System.MasterEffectRMSMuteThreshold", 5.0f);
Debug::Float g_MasterAttenuation("Audio.System.MasterEffectAttenuation", 1.0f, Debug::Range(0.f, 1.f));
//--------------------------------------------------------------------------------------
// Name: CRendererAPO::CRendererAPO
// Desc: Constructor
//--------------------------------------------------------------------------------------
CRendererAPO::CRendererAPO() : CAPOBase< CRendererAPO, RendererAPOParams >()
{
}

//--------------------------------------------------------------------------------------
// Name: CRendererAPO::~CRendererAPO
// Desc: Destructor
//--------------------------------------------------------------------------------------
CRendererAPO::~CRendererAPO()
{
}

//--------------------------------------------------------------------------------------
// Name: CRendererAPO::DoProcess
//--------------------------------------------------------------------------------------
void CRendererAPO::DoProcess( const RendererAPOParams& params, FLOAT32* __restrict pData, UINT32 cFrames, UINT32 cChannels )
{
	using namespace Audio;

	Assert(cChannels == 6);
	Assert(cFrames == 256);

    //  no renderer set
    if (params.pRenderer == NULL)
        return;

	F32* pfMono = NULL;
	F32* pfStereo = NULL;
	F32* pfSurround = NULL;
	F32* pfSurroundLFE = NULL;

	//	Interleave buffer (128 byte aligned)
	F32 afInterleaved[6 * 256 + 128] = {0};
	F32* pfInterleaved = (F32*)((U32)(afInterleaved + 0x7f) & ~0x7f);

	//	get render buffers
	params.pRenderer->GetNextFrame( &pfSurroundLFE, &pfSurround, &pfStereo, &pfMono );

	//	mix into interleave buffer
	if (pfMono)
		Core::Renderer::FoldDown( pfInterleaved, cChannels, pfMono, 1, Core::Renderer::eMix, 1.f );

	if (pfStereo)
		Core::Renderer::FoldDown( pfInterleaved, cChannels, pfStereo, 2, Core::Renderer::eMix, 1.f );

	if (pfSurround)
		Core::Renderer::FoldDown( pfInterleaved, cChannels, pfSurround, 5, Core::Renderer::eMix, 1.f );

	if (pfSurroundLFE)
		Core::Renderer::FoldDown( pfInterleaved, cChannels, pfSurroundLFE, 6, Core::Renderer::eMix, 1.f );

	//	interleave by sample not frame
	for (int ch = 0; ch < 6; ch++)
	{
		for (int sample = 0; sample < 256; sample++)
			pData[sample * 6 + ch] = pfInterleaved[ch * 256 + sample];
	}

#ifndef CFG_GOLD
	// emergency code to mute any loud noises.
	for (int ch = 0; ch < 6; ch++)
	{
		for (int sample = 0; sample < 256; sample++)
		{
			F32 fVal = pData[sample * 6 + ch];
			if (fVal > 100.f || !_finite(fVal))
			{
				#ifdef AUDIO_BUILD
				AUDIO_CORE_WARN("DETECTED EXTREME PEAK VALUE (OR NAN) (%.2f) IN MASTER EFFECT! MUTING FRAME", fVal);

				if (fVal ==	Audio::Maths::fMax)
					AUDIO_CORE_WARN("VALUE is fMax");
				else
				if (fVal ==	Audio::Maths::fInfinity)
					AUDIO_CORE_WARN("VALUE is fInfinity");
				else
				if (fVal == Audio::Maths::fMinusInfinity)
					AUDIO_CORE_WARN("VALUE is fMinusInfinity");
				else
				if (fVal == Audio::Maths::fInvalid)
					AUDIO_CORE_WARN("VALUE is fInvalid");
				#endif
				memset(pData, 0, 256 * 6); 
				return;
			}
		}
	}

	F32 fVal=0;
	for (int ch = 0; ch < 6; ch++)
	{
		for (int sample = 0; sample < 256; sample++)
		{
			fVal += pData[sample * 6 + ch];
		}
	}
			
	fVal /= 256 * 6;
	if (sqrtf(fVal) > g_MasterEffectRMSMuteThreshold)
	{
		#ifdef AUDIO_BUILD
		AUDIO_CORE_WARN("DETECTED EXTREME RMS VALUE (%.2f) IN MASTER EFFECT! MUTING FRAME", fVal);
		#endif
		memset(pData, 0, 256 * 6); 
	}
#endif
}

#else // !USE_XAUDIO2

//#define AUDIO_OVERWRITE_MASTER_MIX
#define AUDIO_MASTER_ATTENUATION (1.f)

Debug::Float g_MasterEffectRMSMuteThreshold("Audio.System.MasterEffectRMSMuteThreshold", 5.0f);
Debug::Float g_MasterAttenuation("Audio.System.MasterEffectAttenuation", 1.0f, Debug::Range(0.f, 1.f));

//Audio::Core::Profiler g_Profiler("All", 256.0 / 48000.0);

extern const F32 Audio::Maths::fMax;          // = 3.402823466e+38f;
extern const F32 Audio::Maths::fInfinity;     // = 0x7f800000;
extern const F32 Audio::Maths::fMinusInfinity;// = 0xff800000;
extern const F32 Audio::Maths::fInvalid;      // = 0xffffffff;

//--------------------------------------------------------------------------------------
// Name: CMasterInputEffect
// Desc: Constructor
//--------------------------------------------------------------------------------------
CMasterInputEffect::CMasterInputEffect()
{
    m_pAllocator =      NULL;
    m_dwRefCount =      1;
    m_dwFrameIndex =    0;
    m_pfClipped =       NULL;
    m_bPrimed =         FALSE;
    ZeroMemory( &m_MasterInputEffectInit, sizeof( m_MasterInputEffectInit ) );

    m_uFrame = 0;
};


//--------------------------------------------------------------------------------------
// Name: Initialize
//--------------------------------------------------------------------------------------
STDMETHODIMP CMasterInputEffect::Initialize(const XAUDIOFXINIT* pInit, IXAudioBatchAllocator* pAllocator)
{
	using namespace Audio;

    Assert( !m_pfClipped );
    Assert( !m_pfPeaks );
    HRESULT hr = S_OK;

    // Copy the contents of the data passed in.
    m_MasterInputEffectInit = *( LPMASTERINPUTEFFECT_INIT )pInit;

    m_pAllocator = pAllocator;

    // Use our own function to figure out how many bytes we need.
    DWORD dwAllocBytes = 0;
    QuerySizeMasterInputEffectBuffers( pInit, &dwAllocBytes );

    m_pfClipped = XAUDIO_BATCHALLOC_ALLOC( FLOAT, 
                                           ( dwAllocBytes / sizeof( FLOAT ) ), 
                                           pAllocator );
    ZeroMemory( m_pfClipped, dwAllocBytes );

    m_pfPeaks = XAUDIO_BATCHALLOC_ALLOC( FLOAT, 
                                         ( dwAllocBytes / sizeof( FLOAT ) ), 
                                         pAllocator );
    ZeroMemory( m_pfPeaks, dwAllocBytes );

    m_pfRMS = XAUDIO_BATCHALLOC_ALLOC( FLOAT, 
                                         ( dwAllocBytes / sizeof( FLOAT ) ), 
                                         pAllocator );
    ZeroMemory( m_pfRMS, dwAllocBytes );


    return hr;
}

//--------------------------------------------------------------------------------------
// Name: Process
//--------------------------------------------------------------------------------------
STDMETHODIMP CMasterInputEffect::Process(IXAudioFrameBuffer* pInputBuffer, IXAudioFrameBuffer* pOutputBuffer)
{
	using namespace Audio;

	F32* pfFrameData = NULL;

    Assert(pInputBuffer != NULL);
    Assert(pOutputBuffer == NULL);

    XAUDIOFRAMEBUFDATA FrameBufferData;
    pInputBuffer->GetProcessingData(&FrameBufferData);
    Assert(XAUDIOSAMPLETYPE_NATIVE == FrameBufferData.Format.SampleType);

	MasterEffectInit* effectInit;
    Audio::Core::Renderer* pRenderer;
    GetContext((void**)&effectInit);
	pRenderer = effectInit->pRenderer;

    //  no renderer set
    if (pRenderer == NULL)
        return S_OK;

	//if (bTest)
	//{
	//	//[cian reverb] updated
	//	if (effectInit->bUseReverbBuffer)
	//		pfFrameData = pRenderer->GetNextReverbFrame();
	//	else
	//		pfFrameData = pRenderer->GetNextFrame();
	//}
	//else
		//USING U8/U32 seems to affect timings and causes artifacts
	//{
	//	if (effectInit->u32UseBuffer == REVERB_BUFFER)
	//		pfFrameData = pRenderer->GetNextReverbFrame();
	//	if (effectInit->u32UseBuffer == REVERB_BUFFER_2)
	//		 pfFrameData = pRenderer->GetNextReverbFrame();
	//	else 
	//		pfFrameData = pRenderer->GetNextFrame();
	//}

	//	-1=dry
	//	0=wet/no tunnel
	//	1=wet/tunnel
	if (effectInit->sIndex != -1)
	{
		//	wet
		//if (effectInit->bUseTunnelBuffer)
		//if (effectInit->sIndex == 1)
		//	pRenderer->GetNextReverbFrame2(&pfFrameData);
		//else
		//	pRenderer->GetNextReverbFrame(&pfFrameData);
		pRenderer->GetNextReverbFrame(&pfFrameData, effectInit->sIndex);

		if (pfFrameData)
			Core::Renderer::FoldDown(FrameBufferData.pSampleBuffer,
				FrameBufferData.Format.ChannelCount,
				pfFrameData,
				1,
				Core::Renderer::eMix,
				g_MasterAttenuation);
	}
	else
	{
		//	dry
		F32* pfMono = NULL;
		F32* pfStereo = NULL;
		F32* pfSurround = NULL;
		F32* pfSurroundLFE = NULL;

		//	get render buffers
		pRenderer->GetNextFrame(&pfSurroundLFE, &pfSurround, &pfStereo, &pfMono);

		//	mix into system buffers
		if (pfMono)
			Core::Renderer::FoldDown(FrameBufferData.pSampleBuffer,
				FrameBufferData.Format.ChannelCount,
				pfMono,
				1,
				Core::Renderer::eMix,
				g_MasterAttenuation);

		if (pfStereo)
			Core::Renderer::FoldDown(FrameBufferData.pSampleBuffer,
				FrameBufferData.Format.ChannelCount,
				pfStereo,
				2,
				Core::Renderer::eMix,
				g_MasterAttenuation);

		if (pfSurround)
			Core::Renderer::FoldDown(FrameBufferData.pSampleBuffer,
				FrameBufferData.Format.ChannelCount,
				pfSurround,
				5,
				Core::Renderer::eMix,
				g_MasterAttenuation);

		if (pfSurroundLFE)
			Core::Renderer::FoldDown(FrameBufferData.pSampleBuffer,
				FrameBufferData.Format.ChannelCount,
				pfSurroundLFE,
				6,
				Core::Renderer::eMix,
				g_MasterAttenuation);
	}

	//if (effectInit->bUseReverbBuffer)
		//for(S32 i = 0;i<256*FrameBufferData.Format.ChannelCount;i++)
		//	FrameBufferData.pSampleBuffer[i] = Maths::Clamp(FrameBufferData.pSampleBuffer[i],-1,1);


	// emergency code to deal with loud noise bug START
	for(S32 i = 0;i<256*FrameBufferData.Format.ChannelCount;i++)
	{
		F32 fVal = FrameBufferData.pSampleBuffer[i];
		if (fVal > 100.f || !_finite(fVal))
		{
			#ifdef AUDIO_BUILD
			AUDIO_CORE_WARN("DETECTED EXTREME PEAK VALUE (OR NAN) (%.2f) IN %s MASTER EFFECT! MUTING FRAME", fVal, effectInit->sIndex==-1?"DRY":"WET");

			if (fVal ==	Audio::Maths::fMax)
				AUDIO_CORE_WARN("VALUE is fMax");
			else
			if (fVal ==	Audio::Maths::fInfinity)
				AUDIO_CORE_WARN("VALUE is fInfinity");
			else
			if (fVal == Audio::Maths::fMinusInfinity)
				AUDIO_CORE_WARN("VALUE is fMinusInfinity");
			else
			if (fVal == Audio::Maths::fInvalid)
				AUDIO_CORE_WARN("VALUE is fInvalid");
			#endif
			memset(FrameBufferData.pSampleBuffer, 0, FrameBufferData.Format.ChannelCount*sizeof(F32)*256); 
		}
	}

	F32 fVal=0;
	for(S32 i = 0;i<256*FrameBufferData.Format.ChannelCount;i++)
		fVal += FrameBufferData.pSampleBuffer[i] * FrameBufferData.pSampleBuffer[i];
	fVal /= 256*FrameBufferData.Format.ChannelCount;
	if (sqrtf(fVal) > g_MasterEffectRMSMuteThreshold)
	{
		#ifdef AUDIO_BUILD
		AUDIO_CORE_WARN("DETECTED EXTREME RMS VALUE (%.2f) IN %s MASTER EFFECT! MUTING FRAME", fVal, effectInit->sIndex==-1?"DRY":"WET");
		#endif
		memset(FrameBufferData.pSampleBuffer, 0, FrameBufferData.Format.ChannelCount*sizeof(F32)*256); 
	}
	// emergency code to deal with loud noise bug END


    return S_OK;
};


//--------------------------------------------------------------------------------------
// Name: GetInfo
// Desc: Returns info used by XAudio.  This is called from XAudio during effect 
//       creation.
//--------------------------------------------------------------------------------------
STDMETHODIMP CMasterInputEffect::GetInfo( XAUDIOFXINFO* pInfo )
{
    Assert( pInfo != NULL );
    pInfo->DataFlow = XAUDIODATAFLOW_INPLACE;
    pInfo->TrailFrames = 1;
//    pInfo->PrimeFrames = 0;
    return S_OK;
};


//--------------------------------------------------------------------------------------
// Name: GetParam
// Desc: Gets the PeakMeter and peak levels.  The only valid parameter ID here is
//       MASTERINPUTEFFECT_GETDATA_PARAMID.
//--------------------------------------------------------------------------------------
STDMETHODIMP CMasterInputEffect::GetParam( XAUDIOFXPARAMID nParamId, XAUDIOFXPARAMTYPE nParamType, XAUDIOFXPARAM* pParam ) 
{
    Assert( pParam != NULL );
    HRESULT hr = S_OK;
    MASTERINPUTEFFECTDATA* pPeakMeterDataOut = NULL;
    
    XAudioLock(XAUDIOLOCKTYPE_LOCK | XAUDIOLOCKTYPE_SPINLOCK);
    switch ( nParamId )
    {
        case MASTERINPUTEFFECT_GETDATA_PARAMID:
        {
            // Make sure the parameter type is correct.
            Assert( nParamType == XAUDIOFXPARAMTYPE_DATA );

            // Make sure the buffer size is correct.
            Assert( pParam->Data.BufferSize == sizeof( MASTERINPUTEFFECTDATA ) );

            // Cast the user's pointer.
            pPeakMeterDataOut = ( MASTERINPUTEFFECTDATA * )pParam->Data.pBuffer;
            
            // Get the frame index.
            pPeakMeterDataOut->dwFrameIndex = m_dwFrameIndex;
            
           
            // Calculate the PeakMeters of those values as we put them into the requested data structure.
            for ( DWORD i=0; i<m_MasterInputEffectInit.dwNumChannels; i++ )
            {            
                // Get the pointer to data to copy.
                FLOAT* pfClippedSource = &m_pfClipped[ i ];
                // Get the pointer to data to copy.
                FLOAT* pfPeakSource = &m_pfPeaks[ i ];
                // Get the pointer to data to copy.
                FLOAT* pfRMSSource = &m_pfRMS[ i ];
                // Reset the total for this channel to 0.
                FLOAT  fTotalClipped = 0.0f;
                // Reset the total for this channel to 0.
                FLOAT  fPeak = 0.0f;                       
                // Reset the total for this channel to 0.
                FLOAT  fRMSPeak = 0.0f;


                for ( DWORD j=0; j<m_MasterInputEffectInit.dwNumFramesStored; ++j )
                {
                    fTotalClipped += *pfClippedSource;
                    // Jump to the next frame.
                    pfClippedSource += m_MasterInputEffectInit.dwNumChannels;     

					fPeak = Audio::Maths::Max( fPeak, *pfPeakSource );
                    // Jump to the next frame.
                    pfPeakSource += m_MasterInputEffectInit.dwNumChannels;

                    fRMSPeak = Audio::Maths::Max (fRMSPeak, *pfRMSSource );
                    // Jump to the next frame.
                    pfRMSSource += m_MasterInputEffectInit.dwNumChannels;
                }
                
                FLOAT fLocalwSampleCount        = FLOAT( g_wSampleCount );
                FLOAT fLocaldwNumFramesStored   = FLOAT( m_MasterInputEffectInit.dwNumFramesStored );

                // Verify we're not dividing by zero.
                Assert( g_wSampleCount != 0 );
                Assert( m_MasterInputEffectInit.dwNumFramesStored != 0 );

                // If the above lines didn't Audio::Assert, then these lines shouldn't Audio::Assert either.
                Assert( fLocalwSampleCount != 0.f );
                Assert( fLocaldwNumFramesStored != 0.f );
                
                pPeakMeterDataOut->fClippingRatio[ i ] = fTotalClipped / 
                                                         fLocalwSampleCount / 
                                                         fLocaldwNumFramesStored;
                pPeakMeterDataOut->fPeakLevels[ i ] = fPeak;
                pPeakMeterDataOut->fRMSPeakLevels [ i ] = fRMSPeak;
            }

            // All of the above data is worthless if this is FALSE, but it only happens the
            // first m_MasterInputEffectInit.dwNumChannels so it's not worth an "if" statement.
            pPeakMeterDataOut->bPrimed = m_bPrimed;
            break;
        }

        default:
            hr = E_FAIL;
//            ATG::FatalError(
//                "Error: CMasterInputEffect::GetParam receieved invalid Param ID %u\n", 
//                nParamId );
            break;            
    }
    XAudioLock(XAUDIOLOCKTYPE_UNLOCK | XAUDIOLOCKTYPE_SPINLOCK);
    return hr;
    
};


//--------------------------------------------------------------------------------------
// Name: SetParam
// Desc: We don't have any parameters to set for this effect so this method should never
//       be called.
//--------------------------------------------------------------------------------------
STDMETHODIMP CMasterInputEffect::SetParam( XAUDIOFXPARAMID nParamId,
                                         XAUDIOFXPARAMTYPE nParamType,
                                         const XAUDIOFXPARAM* pParam ) 
{
    Assert( pParam != NULL );
    XAudioLock(XAUDIOLOCKTYPE_LOCK | XAUDIOLOCKTYPE_SPINLOCK);
    switch ( nParamId )
    {
        case MASTERINPUTEFFECT_GETDATA_PARAMID:
            Assert( FALSE && "Can't set MASTERINPUTEFFECT_GETDATA_PARAMID" );
            break;            
            
        default:
//            ATG::FatalError(
//                "Error: CMasterInputEffect::SetParam receieved invalid Param ID %u\n",
//                nParamId );
            break;            
    }
    XAudioLock(XAUDIOLOCKTYPE_UNLOCK | XAUDIOLOCKTYPE_SPINLOCK);
    return S_OK;
};


//--------------------------------------------------------------------------------------
// Name: GetContext
// Desc: Returns the context this effect was created with.
//--------------------------------------------------------------------------------------
STDMETHODIMP CMasterInputEffect::GetContext( LPVOID *ppvContext ) 
{
    Assert( ppvContext != NULL );
    *ppvContext = m_MasterInputEffectInit.EffectHeader.pContext;
    return S_OK;
};


//--------------------------------------------------------------------------------------
// Name: QuerySizeMasterInputEffectBuffers
// Desc: Returns the size of the effect buffers.
//--------------------------------------------------------------------------------------
HRESULT QuerySizeMasterInputEffectBuffers( LPCXAUDIOFXINIT pInit, LPDWORD pEffectSize )
{
    Assert( pInit != NULL );
    Assert( pEffectSize != NULL );
    LPMASTERINPUTEFFECT_INIT pMasterInputEffectInit = ( LPMASTERINPUTEFFECT_INIT )pInit;    
    *pEffectSize = sizeof( FLOAT ) * (pMasterInputEffectInit->dwNumChannels) *
                   pMasterInputEffectInit->dwNumFramesStored;    
    return S_OK;
};


//--------------------------------------------------------------------------------------
// Name: QuerySizeMasterInputEffect
// Desc: Returns the size of the effect.
//--------------------------------------------------------------------------------------
HRESULT __stdcall QuerySizeMasterInputEffect( LPCXAUDIOFXINIT pInit, LPDWORD pEffectSize )
{
    Assert( pInit != NULL );
    Assert( pEffectSize != NULL );
    DWORD dwBufferSize = 0;
    DWORD dwObjectSize = 0;
    QuerySizeMasterInputEffectBuffers( pInit, &dwBufferSize );
    dwObjectSize = sizeof( CMasterInputEffect );
    *pEffectSize = dwBufferSize * 3 + dwObjectSize;    
    return S_OK;
};


//--------------------------------------------------------------------------------------
// Name: CreateMasterInputEffect
// Desc: Creates the effect.
//--------------------------------------------------------------------------------------
HRESULT __stdcall CreateMasterInputEffect( const XAUDIOFXINIT * pInit, IXAudioBatchAllocator *pAllocator,
                         IXAudioEffect **ppEffect )
{
    Assert( pInit != NULL );
    Assert( ppEffect != NULL );
    Assert( pAllocator != NULL );

    HRESULT hr = S_OK;
    CMasterInputEffect *pMasterInputEffect = NULL;

    pMasterInputEffect = XAUDIO_BATCHALLOC_NEW( CMasterInputEffect(), pAllocator );
    pMasterInputEffect->Initialize( pInit, pAllocator );
    *ppEffect = pMasterInputEffect;    
    return hr;
};


//--------------------------------------------------------------------------------------
// Name: AddRef
//--------------------------------------------------------------------------------------
STDMETHODIMP_( ULONG ) CMasterInputEffect::AddRef( void )
{
    return ++m_dwRefCount;
}


//--------------------------------------------------------------------------------------
// Name: Release
//--------------------------------------------------------------------------------------
STDMETHODIMP_( ULONG ) CMasterInputEffect::Release( void )
{
    if ( 0 == --m_dwRefCount )
    {
        //Free the data we're holding.
        XAUDIO_BATCHALLOC_FREE( m_pfClipped );    
        XAUDIO_BATCHALLOC_FREE( m_pfPeaks );    
        
        delete this;
        return 0;
    }
    else
    {
        return m_dwRefCount;
    }
}

#endif // USE_XAUDIO2

#ifdef AUDIO_OPTIMIZE_OFF
#pragma optimize ("", on) // AUDIO_OPTIMIZE_OFF
#endif	//	AUDIO_OPTIMIZE_OFF

#endif // _XBOX360
