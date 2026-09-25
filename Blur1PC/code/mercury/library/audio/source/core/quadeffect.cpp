#include "AudioPch.h"
#include "coreheaders.h"
#include "quadeffect.h"


#ifdef PLATFORM_XENON
#ifndef USE_XAUDIO2

//--------------------------------------------------------------------------------------
//	CQuadEffect
//--------------------------------------------------------------------------------------
CQuadEffect::CQuadEffect(LPVOID pvContext)
{
    m_pvContext  = pvContext;
    m_dwRefCount = 1;

	m_uMode = 0;

   	m_fRearGain = QUADFX_REAR_GAIN_DEFAULT;
	m_fResponse = QUADFX_RESPONSE_DEFAULT;
	m_fBalance = QUADFX_BALANCE_DEFAULT;

    m_fFrontRMS = 0.f;
    m_fRearRMS = 0.f;

    m_uCurrSOS = 0;
    ZeroMemory(m_afFrontSOS, QUADFX_NUM_RMS_VALUES * sizeof(F32));
    ZeroMemory(m_afRearSOS, QUADFX_NUM_RMS_VALUES * sizeof(F32));

	m_fCurrRearGain = 0.f;

	m_fLevel = 0.1f;
}


//--------------------------------------------------------------------------------------
//	Initialize
//--------------------------------------------------------------------------------------
STDMETHODIMP CQuadEffect::Initialize(const XAUDIOFXINIT* pInit, IXAudioBatchAllocator* pAllocator)
{
    HRESULT hr = S_OK;

	return hr;
}

bool g_bDisableQuad = false;
//--------------------------------------------------------------------------------------
//	Process
//--------------------------------------------------------------------------------------
STDMETHODIMP CQuadEffect::Process(IXAudioFrameBuffer* pInputBuffer, IXAudioFrameBuffer* pOutputBuffer)
{
	//if (Audio::System::m_bNoAudioDSP || g_bDisableQuad)
	//	return S_OK;

    Assert( pInputBuffer != NULL );
    Assert( pOutputBuffer == NULL );
    XAUDIOFRAMESIZE wSampleCount = XAUDIOFRAMESIZE_NATIVE;        
    HRESULT hr = S_OK;    

    // Verify the format is "internal".  It should always be internal (float)
    XAUDIOFRAMEBUFDATA FrameBufferData;
    pInputBuffer->GetProcessingData( &FrameBufferData );  
    Assert( XAUDIOSAMPLETYPE_NATIVE == FrameBufferData.Format.SampleType );

	Assert(6 == FrameBufferData.Format.ChannelCount);

	FLOAT* afFrontLeft = &FrameBufferData.pSampleBuffer[0];
	FLOAT* afFrontRight = &FrameBufferData.pSampleBuffer[wSampleCount];
	FLOAT* afRearLeft = &FrameBufferData.pSampleBuffer[4 * wSampleCount];
	FLOAT* afRearRight = &FrameBufferData.pSampleBuffer[5 * wSampleCount];

	for (U32 j = 0; j < wSampleCount; j++)
		afFrontLeft[j] *= m_fLevel;

	for (U32 j = 0; j < wSampleCount; j++)
		afFrontRight[j] *= m_fLevel;

	//	pseudo surround mode
	if (m_uMode = 2)
	{
		//	create out-of-phase surround channels
		for (U32 j = 0; j < wSampleCount; j++)
			afRearLeft[j] = afFrontLeft[j] - afFrontRight[j];

		for (U32 j = 0; j < wSampleCount; j++)
			afRearRight[j] = afFrontRight[j] - afFrontLeft[j];

		//	clear sos
		m_afFrontSOS[m_uCurrSOS] = 0.f;
		m_afRearSOS[m_uCurrSOS] = 0.f;

		//  sum squares
		int iSamp;
		for (iSamp = 0; iSamp < wSampleCount; iSamp++)
		{
			F32 fSampIn = afFrontLeft[iSamp];
			m_afFrontSOS[m_uCurrSOS] += fSampIn * fSampIn;
		}
		for (iSamp = 0; iSamp < wSampleCount; iSamp++)
		{
			F32 fSampIn = afFrontRight[iSamp];
			m_afFrontSOS[m_uCurrSOS] += fSampIn * fSampIn;
		}
		for (iSamp = 0; iSamp < wSampleCount; iSamp++)
		{
			F32 fSampIn = afRearLeft[iSamp];
			m_afRearSOS[m_uCurrSOS] += fSampIn * fSampIn;
		}
		for (iSamp = 0; iSamp < wSampleCount; iSamp++)
		{
			F32 fSampIn = afRearRight[iSamp];
			m_afRearSOS[m_uCurrSOS] += fSampIn * fSampIn;
		}

		m_uCurrSOS++;

		if (m_uCurrSOS == QUADFX_NUM_RMS_VALUES)
			m_uCurrSOS = 0;

		int iCount;
		F32 fTotalFrontSOS = 0.f;
		F32 fTotalRearSOS = 0.f;
		for (iCount = 0; iCount < QUADFX_NUM_RMS_VALUES; iCount++)
			fTotalFrontSOS += m_afFrontSOS[iCount];
		for (iCount = 0; iCount < QUADFX_NUM_RMS_VALUES; iCount++)
			fTotalRearSOS += m_afRearSOS[iCount];

		//  current RMS
		m_fFrontRMS = sqrtf(fTotalFrontSOS / (wSampleCount * 2 * QUADFX_NUM_RMS_VALUES));
		m_fRearRMS = sqrtf(fTotalRearSOS / (wSampleCount * 2 * QUADFX_NUM_RMS_VALUES));

		//	do the thing
		if (m_fRearRMS != 0)
		{
			//	get quietness scale
			F32 fRatio = (m_fFrontRMS / m_fRearRMS) - 8.f;
			if (fRatio < 1.f)
				fRatio = 1.f;

			//	get target gain based on front level
			F32 fTarget = m_fFrontRMS * m_fRearGain;

			//	scale by quietness to avoid introducing artifacts for quiet surrounds
			F32 fTargetRearGainUnscaled = (fTarget / m_fRearRMS) / fRatio;

			//	scale to avoid large gains on mono source
			F32 fMaxGain = 2.f;
			F32 fTargetRearGain = fMaxGain * (1.f - (1.f / (1.f + (fTargetRearGainUnscaled / fMaxGain))));

			//	blend using response time
			F32 fNewRearGain = (1.f - m_fResponse) * m_fCurrRearGain + m_fResponse * fTargetRearGain;

			//	blend between no balancing and full gain balancing
			F32 fRearGainBalanced = m_fCurrRearGain * m_fBalance + (1.f - m_fBalance);

			//	apply rear gain, sub-frame interpolate
			F32 fStep = (fNewRearGain - m_fCurrRearGain) / wSampleCount;
			for ( INT j = 0; j < wSampleCount; ++j)
			{
				afRearLeft[j] *= fRearGainBalanced + (j * fStep);
				afRearRight[j] *= fRearGainBalanced + (j * fStep);
			}

			m_fCurrRearGain = fTargetRearGain;
		}
/*
		//	test in stereo
		for ( INT j = 0; j < wSampleCount; ++j)
		{
			afFrontLeft[j] = afFrontRight[j];
			afFrontRight[j] = afRearRight[j];
		}
*/
	}
	//	quad, fronts -> surrounds
	else if (m_uMode == 1)
	{
		//	just do 4 channel stereo
		for (U32 j = 0; j < wSampleCount; j++)
			afRearLeft[j] = m_fRearGain * afFrontLeft[j];

		for (U32 j = 0; j < wSampleCount; j++)
			afRearRight[j] = m_fRearGain * afFrontRight[j];
	}
	//	else straight stereo, no messing

	return hr;
}


//--------------------------------------------------------------------------------------
//	GetInfo
//--------------------------------------------------------------------------------------
STDMETHODIMP CQuadEffect::GetInfo(XAUDIOFXINFO* pInfo)
{
    Assert(pInfo != NULL);

    pInfo->DataFlow = XAUDIODATAFLOW_INPLACE;
    pInfo->TrailFrames = 1;
    //pInfo->PrimeFrames = 0;

    return S_OK;
}


//--------------------------------------------------------------------------------------
//	GetParam
//--------------------------------------------------------------------------------------
STDMETHODIMP CQuadEffect::GetParam(XAUDIOFXPARAMID nParamId, XAUDIOFXPARAMTYPE nParamType, XAUDIOFXPARAM* pParam) 
{
    Assert(pParam != NULL);

    XAUDIOFXPARAM Param = {0};
    HRESULT hr = S_OK;

    XAudioLock(XAUDIOLOCKTYPE_LOCK | XAUDIOLOCKTYPE_SPINLOCK);
    switch ( nParamId )
    {
    case QUADFX_REAR_GAIN_ID:
        Assert( XAUDIOFXPARAMTYPE_NUMERIC == nParamType );
        Param.Value = m_fRearGain;
        break;

    case QUADFX_RESPONSE_ID:
        Assert( XAUDIOFXPARAMTYPE_NUMERIC == nParamType );
        Param.Value = m_fResponse;
        break;

    case QUADFX_BALANCE_ID:
        Assert( XAUDIOFXPARAMTYPE_NUMERIC == nParamType );
        Param.Value = m_fBalance;
        break;

	case QUADFX_LEVEL_ID:
        Assert( XAUDIOFXPARAMTYPE_NUMERIC == nParamType );
        Param.Value = m_fLevel;
        break;

	case QUADFX_MODE_ID:
        Assert( XAUDIOFXPARAMTYPE_FLAG == nParamType );
		Param.Flags = m_uMode;
        break;

    default:
        hr = E_FAIL;
        break;            
    }
    
    *pParam = Param;
    XAudioLock(XAUDIOLOCKTYPE_UNLOCK | XAUDIOLOCKTYPE_SPINLOCK);
    return hr;
    
}


//--------------------------------------------------------------------------------------
//	SetParam
//--------------------------------------------------------------------------------------
STDMETHODIMP CQuadEffect::SetParam(XAUDIOFXPARAMID nParamId, XAUDIOFXPARAMTYPE nParamType, const XAUDIOFXPARAM* pParam) 
{
    Assert(pParam != NULL);

    XAudioLock(XAUDIOLOCKTYPE_LOCK | XAUDIOLOCKTYPE_SPINLOCK);

    switch (nParamId)
    {
    case QUADFX_REAR_GAIN_ID:
        Assert( XAUDIOFXPARAMTYPE_NUMERIC == nParamType );
        if( pParam->Value > QUADFX_REAR_GAIN_MAX
            || pParam->Value < QUADFX_REAR_GAIN_MIN )
            return E_FAIL;
        m_fRearGain = pParam->Value;
        break;
    case QUADFX_RESPONSE_ID:
        Assert( XAUDIOFXPARAMTYPE_NUMERIC == nParamType );
        if( pParam->Value > QUADFX_RESPONSE_MAX
            || pParam->Value < QUADFX_RESPONSE_MIN )
            return E_FAIL;
        m_fResponse = pParam->Value;
        break;
    case QUADFX_BALANCE_ID:
        Assert( XAUDIOFXPARAMTYPE_NUMERIC == nParamType );
        if( pParam->Value > QUADFX_BALANCE_MAX
            || pParam->Value < QUADFX_BALANCE_MIN )
            return E_FAIL;
        m_fBalance = pParam->Value;
        break;
    case QUADFX_LEVEL_ID:
        Assert( XAUDIOFXPARAMTYPE_NUMERIC == nParamType );
        if( pParam->Value > QUADFX_LEVEL_MAX
            || pParam->Value < QUADFX_LEVEL_MIN )
            return E_FAIL;
        m_fLevel = pParam->Value;
        break;
    case QUADFX_MODE_ID:
        Assert( XAUDIOFXPARAMTYPE_FLAG == nParamType );
		if( pParam->Flags > 2 )
            return E_FAIL;
        m_uMode = pParam->Flags;
        break;
    default:
        return E_FAIL;
        break;            
    }
   
    XAudioLock(XAUDIOLOCKTYPE_UNLOCK | XAUDIOLOCKTYPE_SPINLOCK);

    return S_OK;
}


//--------------------------------------------------------------------------------------
//	GetContext
//--------------------------------------------------------------------------------------
STDMETHODIMP CQuadEffect::GetContext(LPVOID *ppvContext) 
{
    Assert(ppvContext != NULL);

    *ppvContext = m_pvContext;
    return S_OK;
}


//--------------------------------------------------------------------------------------
//	QuerySizeQuadEffect
//--------------------------------------------------------------------------------------
HRESULT QuerySizeQuadEffect(LPCXAUDIOFXINIT pInit, LPDWORD pEffectSize)
{
    Assert(pInit != NULL);
    Assert(pEffectSize != NULL);
    
    *pEffectSize = sizeof(CQuadEffect);
    return S_OK;
}


//--------------------------------------------------------------------------------------
//	CreateQuadEffect
//--------------------------------------------------------------------------------------
HRESULT CreateQuadEffect(const XAUDIOFXINIT* pInit, IXAudioBatchAllocator* pAllocator, IXAudioEffect** ppEffect)
{
    Assert( pInit != NULL );
    Assert( ppEffect != NULL );
    Assert( pAllocator != NULL );

    HRESULT hr = S_OK;
    CQuadEffect *pUserEffect = NULL;

    pUserEffect = XAUDIO_BATCHALLOC_NEW( CQuadEffect( pInit->pContext ), pAllocator );
    pUserEffect->Initialize( pInit, pAllocator );
    
    *ppEffect = pUserEffect;

    return hr;
}


//--------------------------------------------------------------------------------------
//	AddRef
//--------------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CQuadEffect::AddRef(void)
{
    return ++m_dwRefCount;
}


//--------------------------------------------------------------------------------------
//	Release
//--------------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CQuadEffect::Release(void)
{

    if (0 == --m_dwRefCount)
    {
        delete this;
        return 0;
    }
    else
    {
        return m_dwRefCount;
    }
}

#endif // USE_XAUDIO2
#endif // PLATFORM_XENON

