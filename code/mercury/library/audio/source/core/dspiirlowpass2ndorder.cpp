#include "AudioPch.h"
#include "coreheaders.h"


#ifdef AUDIO_OPTIMIZE_OFF
#pragma optimize ("", off) // AUDIO_OPTIMIZE_OFF
#endif	//	AUDIO_OPTIMIZE_OFF

////////////////////////////////////////////////////////////////////////////////
//	static data
////////////////////////////////////////////////////////////////////////////////
bool Audio::Core::DSP::IIRLowPass2ndOrder::bDisableAll = false;

////////////////////////////////////////////////////////////////////////////////
//	IIRLowPass2ndOrder
////////////////////////////////////////////////////////////////////////////////
#ifndef PLATFORM_SPU
Audio::Core::DSP::IIRLowPass2ndOrder::IIRLowPass2ndOrder( const char* pchInfo, U16 uChannels ) : Effect( pchInfo, uChannels )
{
#ifndef AUDIO_DISABLE_DSP
	m_eType = Core::DSP::Effect::eIIRLowPass2ndOrder;

	m_fTargetCutoff = fSampleRate * 0.5f;
    m_fPrevCutoff = -1.f;
    m_fPrevQ = -1.f;
    m_fPrevGain = -1.f;
    m_fCutoff = fSampleRate * 0.5f;
    m_fQ = 0.f;
    m_fGain = 1.f;
	m_fClamp = Maths::fMax;

    for (U32 uCh = 0; uCh < uChannels; uCh++)
    {
#if defined(PLATFORM_XENON)
        m_vPrevIn[uCh] = D3DXVECTOR4( 0.f, 0.f, 0.f, 0.f );
        m_vPrevOut0[uCh] = D3DXVECTOR4( 0.f, 0.f, 0.f, 0.f );
        m_vPrevOut1[uCh] = D3DXVECTOR4( 0.f, 0.f, 0.f, 0.f );
#else
        m_fPrevIn0[uCh] = 0.f;
        m_fPrevOut0[uCh] = 0.f;
        m_fPrevIn1[uCh] = 0.f;
        m_fPrevOut1[uCh] = 0.f;
#endif
    }

#endif	//	#ifndef AUDIO_DISABLE_DSP
}
#endif

////////////////////////////////////////////////////////////////////////////////
//	Process
////////////////////////////////////////////////////////////////////////////////
bool Audio::Core::DSP::IIRLowPass2ndOrder::Process(void* pFrameData, const ProcessInfo& info, bool bProcess)
{
	AUDIO_PERF_MICRO_EVENT("Audio.Generate.DSP.IIRLowPass2ndOrder.Process");

#ifndef AUDIO_DISABLE_DSP

	if( !m_bActive || bDisableAll )
		return true;

	//	limit freq sweep rate using Q
	F32 fCutoffDelta = (m_fTargetCutoff - m_fCutoff);
	F32 fThresh = Maths::RcpEst(Maths::Power(2.f, 3.f * m_fQ));	//	Q=0 -> 1oct, Q=1 -> 1/8oct per 256 samp frame
	F32 fStep = fThresh * m_fCutoff * fFrameLength * 0.001953125f; // 1/512
	fCutoffDelta = Maths::Clamp(fCutoffDelta, -fStep, fStep);
	m_fCutoff += fCutoffDelta;

    if ((m_fCutoff != m_fPrevCutoff) || (m_fQ != m_fPrevQ) || (m_fGain != m_fPrevGain))
    {
		F64 fRealQ = (F64)Maths::Power(2.f, 8.f * m_fQ);
		F32 fOmega = Maths::fPi * m_fCutoff * Core::fRcpSampleRate;
		F64 fSin = (F64)Maths::Sin(fOmega);
		F64 fCos = (F64)Maths::Cos(fOmega);
		F64 fAlpha = fSin / (2.0 * fRealQ);
		F64 fA0 = 1.0 / (1.0 + fAlpha);
        m_fA1 = (F32)((-2.0 * fCos) * fA0);
        m_fA2 = (F32)((1.0 - fAlpha) * fA0);
        m_fB1 = (F32)((1.0 - fCos) * fA0) * m_fGain;
        m_fB0 = m_fB1 * 0.5f;

        m_fPrevCutoff = m_fCutoff;
        m_fPrevQ = m_fQ;
        m_fPrevGain = m_fGain;
    }

    F32* AUDIO_RESTRICT aSample = (F32*)pFrameData;

#if defined(PLATFORM_XENON)

	//	NOTE: the optimised version does not clamp the result

    F32* AUDIO_RESTRICT FrameBufferData_pSampleBuffer = aSample;
    U32 uNumChannels = m_uChannels;

    XMVECTOR vCoefficients = XMVectorSet( 0.f, m_fB0, m_fB1, m_fB0 );
    XMVECTOR vA1 = XMVectorReplicate( -m_fA1 );
    XMVECTOR vA2 = XMVectorReplicate( -m_fA2 );
    XMVECTOR vLevel = XMVectorReplicate( m_fGain );

    F32* pSampleBuffer = FrameBufferData_pSampleBuffer;

    //  each channel

    for (DWORD ch = 0; ch < (DWORD)uNumChannels; ++ch)
    {
        XMVECTOR vPrevIn   = XMLoadFloat4( (XMFLOAT4*)&m_vPrevIn[ch] );
        XMVECTOR vPrevOut0 = XMLoadFloat4( (XMFLOAT4*)&m_vPrevOut0[ch] );
        XMVECTOR vPrevOut1 = XMLoadFloat4( (XMFLOAT4*)&m_vPrevOut1[ch] );

        F32* pSampleBufferEnd = pSampleBuffer + uFrameLength;

        //  each sample (in blocks of 16)
        while( pSampleBuffer < pSampleBufferEnd )
        {
            //  read in 16 input samples

            XMVECTOR vIn3_0 = XMLoadFloat4A( (XMFLOAT4A*)(pSampleBuffer + 0) );
            XMVECTOR vIn3_1 = XMLoadFloat4A( (XMFLOAT4A*)(pSampleBuffer + 4) );
            XMVECTOR vIn3_2 = XMLoadFloat4A( (XMFLOAT4A*)(pSampleBuffer + 8) );
            XMVECTOR vIn3_3 = XMLoadFloat4A( (XMFLOAT4A*)(pSampleBuffer + 12) );

            //  prepare the 16 shifted input vectors

            XMVECTOR vIn0_0 = XMVectorShiftLeft( vPrevIn, vIn3_0, 1 );
            XMVECTOR vIn1_0 = XMVectorShiftLeft( vPrevIn, vIn3_0, 2 );
            XMVECTOR vIn2_0 = XMVectorShiftLeft( vPrevIn, vIn3_0, 3 );

            XMVECTOR vIn0_1 = XMVectorShiftLeft( vIn3_0, vIn3_1, 1 );
            XMVECTOR vIn1_1 = XMVectorShiftLeft( vIn3_0, vIn3_1, 2 );
            XMVECTOR vIn2_1 = XMVectorShiftLeft( vIn3_0, vIn3_1, 3 );

            XMVECTOR vIn0_2 = XMVectorShiftLeft( vIn3_1, vIn3_2, 1 );
            XMVECTOR vIn1_2 = XMVectorShiftLeft( vIn3_1, vIn3_2, 2 );
            XMVECTOR vIn2_2 = XMVectorShiftLeft( vIn3_1, vIn3_2, 3 );

            XMVECTOR vIn0_3 = XMVectorShiftLeft( vIn3_2, vIn3_3, 1 );
            XMVECTOR vIn1_3 = XMVectorShiftLeft( vIn3_2, vIn3_3, 2 );
            XMVECTOR vIn2_3 = XMVectorShiftLeft( vIn3_2, vIn3_3, 3 );

            //  calculate the portion of the 16 outputs that only rely on the inputs

            XMVECTOR vInResult0_0 = XMVector4Dot( vIn0_0, vCoefficients );
            XMVECTOR vInResult1_0 = XMVector4Dot( vIn1_0, vCoefficients );
            XMVECTOR vInResult2_0 = XMVector4Dot( vIn2_0, vCoefficients );
            XMVECTOR vInResult3_0 = XMVector4Dot( vIn3_0, vCoefficients );

            XMVECTOR vInResult0_1 = XMVector4Dot( vIn0_1, vCoefficients );
            XMVECTOR vInResult1_1 = XMVector4Dot( vIn1_1, vCoefficients );
            XMVECTOR vInResult2_1 = XMVector4Dot( vIn2_1, vCoefficients );
            XMVECTOR vInResult3_1 = XMVector4Dot( vIn3_1, vCoefficients );

            //  add the portion of the 16 outputs that rely on the previous results

            XMVECTOR vOut0_0 = XMVectorMultiplyAdd( vPrevOut0, vA1, vInResult0_0 );
                        vOut0_0 = XMVectorMultiplyAdd( vPrevOut1, vA2, vOut0_0 );

            XMVECTOR vOut1_0 = XMVectorMultiplyAdd( vOut0_0,   vA1, vInResult1_0 );
                        vOut1_0 = XMVectorMultiplyAdd( vPrevOut0, vA2, vOut1_0 );

            XMVECTOR vOut2_0 = XMVectorMultiplyAdd( vOut1_0, vA1, vInResult2_0 );
                        vOut2_0 = XMVectorMultiplyAdd( vOut0_0, vA2, vOut2_0 );

            XMVECTOR vOut3_0 = XMVectorMultiplyAdd( vOut2_0, vA1, vInResult3_0 );
                        vOut3_0 = XMVectorMultiplyAdd( vOut1_0, vA2, vOut3_0 );

            XMVECTOR vOut0_1 = XMVectorMultiplyAdd( vOut3_0, vA1, vInResult0_1 );
                        vOut0_1 = XMVectorMultiplyAdd( vOut2_0, vA2, vOut0_1 );

            XMVECTOR vOut1_1 = XMVectorMultiplyAdd( vOut0_1, vA1, vInResult1_1 );
                        vOut1_1 = XMVectorMultiplyAdd( vOut3_0, vA2, vOut1_1 );

            XMVECTOR vInResult0_2 = XMVector4Dot( vIn0_2, vCoefficients );                  //  moved here for better code scheduling
            XMVECTOR vInResult1_2 = XMVector4Dot( vIn1_2, vCoefficients );                  //
            XMVECTOR vInResult2_2 = XMVector4Dot( vIn2_2, vCoefficients );                  //
            XMVECTOR vInResult3_2 = XMVector4Dot( vIn3_2, vCoefficients );                  //

            XMVECTOR vInResult0_3 = XMVector4Dot( vIn0_3, vCoefficients );                  //  moved here for better code scheduling
            XMVECTOR vInResult1_3 = XMVector4Dot( vIn1_3, vCoefficients );                  //
            XMVECTOR vInResult2_3 = XMVector4Dot( vIn2_3, vCoefficients );                  //
            XMVECTOR vInResult3_3 = XMVector4Dot( vIn3_3, vCoefficients );                  //

            XMVECTOR vOut2_1 = XMVectorMultiplyAdd( vOut1_1, vA1, vInResult2_1 );
                        vOut2_1 = XMVectorMultiplyAdd( vOut0_1, vA2, vOut2_1 );

            XMVECTOR vOut3_1 = XMVectorMultiplyAdd( vOut2_1, vA1, vInResult3_1 );
                        vOut3_1 = XMVectorMultiplyAdd( vOut1_1, vA2, vOut3_1 );

            XMVECTOR vOut0_2 = XMVectorMultiplyAdd( vOut3_1, vA1, vInResult0_2 );
                        vOut0_2 = XMVectorMultiplyAdd( vOut2_1, vA2, vOut0_2 );

            XMVECTOR vOut1_2 = XMVectorMultiplyAdd( vOut0_2, vA1, vInResult1_2 );
                        vOut1_2 = XMVectorMultiplyAdd( vOut3_1, vA2, vOut1_2 );

            XMVECTOR vResult_0 = vOut0_0;                                                   //  moved here for better code scheduling
                                                                                            //
            vResult_0 = XMVectorShiftLeft( vResult_0, vOut1_0, 1 );                         //
            vResult_0 = XMVectorShiftLeft( vResult_0, vOut2_0, 1 );                         //
            vResult_0 = XMVectorShiftLeft( vResult_0, vOut3_0, 1 );                         //
                                                                                            //
            vResult_0 = XMVectorMultiply( vResult_0, vLevel );                              //

            XMVECTOR vOut2_2 = XMVectorMultiplyAdd( vOut1_2, vA1, vInResult2_2 );
                        vOut2_2 = XMVectorMultiplyAdd( vOut0_2, vA2, vOut2_2 );

            XMVECTOR vOut3_2 = XMVectorMultiplyAdd( vOut2_2, vA1, vInResult3_2 );
                        vOut3_2 = XMVectorMultiplyAdd( vOut1_2, vA2, vOut3_2 );

            XMVECTOR vOut0_3 = XMVectorMultiplyAdd( vOut3_2, vA1, vInResult0_3 );
                        vOut0_3 = XMVectorMultiplyAdd( vOut2_2, vA2, vOut0_3 );

            XMVECTOR vResult_1 = vOut0_1;                                                   //  moved here for better code scheduling 
                                                                                            //
            vResult_1 = XMVectorShiftLeft( vResult_1, vOut1_1, 1 );                         //
            vResult_1 = XMVectorShiftLeft( vResult_1, vOut2_1, 1 );                         //
            vResult_1 = XMVectorShiftLeft( vResult_1, vOut3_1, 1 );                         //
                                                                                            //
            vResult_1 = XMVectorMultiply( vResult_1, vLevel );                              //

            XMVECTOR vOut1_3 = XMVectorMultiplyAdd( vOut0_3, vA1, vInResult1_3 );
                        vOut1_3 = XMVectorMultiplyAdd( vOut3_2, vA2, vOut1_3 );

            XMVECTOR vOut2_3 = XMVectorMultiplyAdd( vOut1_3, vA1, vInResult2_3 );
                        vOut2_3 = XMVectorMultiplyAdd( vOut0_3, vA2, vOut2_3 );

            XMVECTOR vOut3_3 = XMVectorMultiplyAdd( vOut2_3, vA1, vInResult3_3 );
                        vOut3_3 = XMVectorMultiplyAdd( vOut1_3, vA2, vOut3_3 );

            vPrevIn   = vIn3_3;
            vPrevOut0 = vOut3_3;
            vPrevOut1 = vOut2_3;

            XMStoreFloat4A( (XMFLOAT4A*)(pSampleBuffer + 0),  vResult_0 );                  //  moved here for better code scheduling 
            XMStoreFloat4A( (XMFLOAT4A*)(pSampleBuffer + 4),  vResult_1 );                  //

            //  combine the outputs into vectors of 4-outputs each

            XMVECTOR vResult_2 = vOut0_2;
            
            vResult_2 = XMVectorShiftLeft( vResult_2, vOut1_2, 1 );
            vResult_2 = XMVectorShiftLeft( vResult_2, vOut2_2, 1 );
            vResult_2 = XMVectorShiftLeft( vResult_2, vOut3_2, 1 );
            
            vResult_2 = XMVectorMultiply( vResult_2, vLevel );

            XMVECTOR vResult_3 = vOut0_3;

            vResult_3 = XMVectorShiftLeft( vResult_3, vOut1_3, 1 );
            vResult_3 = XMVectorShiftLeft( vResult_3, vOut2_3, 1 );
            vResult_3 = XMVectorShiftLeft( vResult_3, vOut3_3, 1 );

            vResult_3 = XMVectorMultiply( vResult_3, vLevel );

            //  store the 16 results
                
            XMStoreFloat4A( (XMFLOAT4A*)(pSampleBuffer + 8),  vResult_2 );
            XMStoreFloat4A( (XMFLOAT4A*)(pSampleBuffer + 12), vResult_3 );

            pSampleBuffer += 16;
        }

        XMStoreFloat4( (XMFLOAT4*)&m_vPrevIn[ch], vPrevIn );
        XMStoreFloat4( (XMFLOAT4*)&m_vPrevOut0[ch], vPrevOut0 );
        XMStoreFloat4( (XMFLOAT4*)&m_vPrevOut1[ch], vPrevOut1 );
    }

#else // PLATFORM_XENON

    F32 fPrevIn0;
    F32 fPrevOut0;
    F32 fPrevIn1;
    F32 fPrevOut1;

    //  each channel
    for (U32 uCh = 0; uCh < m_uChannels; uCh++)
    {
        fPrevIn0 = m_fPrevIn0[uCh];
        fPrevOut0 = m_fPrevOut0[uCh];
        fPrevIn1 = m_fPrevIn1[uCh];
        fPrevOut1 = m_fPrevOut1[uCh];

        //  each sample
        for (U32 i = 0; i < uFrameLength; i++)
        {
            U32 dwSample = uCh * uFrameLength + i;

            F32 fIn = aSample[dwSample];
            F32 fOut = m_fB0 * fIn + m_fB1 * fPrevIn0 + m_fB0 * fPrevIn1 - m_fA1 * fPrevOut0 - m_fA2 * fPrevOut1;

			fOut = Maths::Clamp(fOut, -m_fClamp, m_fClamp);

            fPrevIn1 = fPrevIn0;
            fPrevOut1 = fPrevOut0;
            fPrevIn0 = fIn;
            fPrevOut0 = fOut;

            aSample[dwSample] = fOut;
        }

        m_fPrevIn0[uCh] = fPrevIn0;
        m_fPrevOut0[uCh] = fPrevOut0;
        m_fPrevIn1[uCh] = fPrevIn1;
        m_fPrevOut1[uCh] = fPrevOut1;
    }

#endif // PLATFORM_XENON

    return true;

#else // AUDIO_DISABLE_DSP
	return false;
#endif // AUDIO_DISABLE_DSP
}
