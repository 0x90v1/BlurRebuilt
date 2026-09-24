#include "AudioPch.h"
#include "coreheaders.h"
#ifdef PLATFORM_SPU
#include "SystemHeadersSPU.h"
#else
#include "../SystemHeaders.h"
#endif

#ifndef PLATFORM_PC
#include <core/Maths.h>
#endif 

//using namespace Audio;
using namespace Audio::Core;

#ifdef AUDIO_OPTIMIZE_OFF
#pragma optimize ("", off) // AUDIO_OPTIMIZE_OFF
#endif	//	AUDIO_OPTIMIZE_OFF

#ifndef CFG_SPEED
//#define CHECK_OUTPUT_QUALITY
#endif

#ifndef PLATFORM_SPU
Debug::Bool g_switchResampler("Audio.System.NewResampler", true);
//Debug::Bool g_switchFillBufferFromS16("Audio.PerfTweaks.NewFillBufferS16", true);
#endif


#ifdef PLATFORM_SPU
U32 u32CurrentOverlayId = 0;

void SwitchInGeneratorOverlay()
{
#ifndef AUDIO_GENERATORS_NOT_OVERLAID
	u32CurrentOverlayId = Audio::System::Get()->GetCurrentCodeOverlayId();

	if (u32CurrentOverlayId != AUDIO_GENERATOR_OVERLAY_ID)
		CoreLoadCodeOverlay(AUDIO_GENERATOR_OVERLAY_ID);
#endif
}


void SwitchBackToOriginalOverlay()
{
#ifndef AUDIO_GENERATORS_NOT_OVERLAID
	if (u32CurrentOverlayId != AUDIO_GENERATOR_OVERLAY_ID)
		CoreLoadCodeOverlay(u32CurrentOverlayId);
#endif
}
#endif

#ifdef AUDIO_NEW_DECODERS
F32* ISampleRateConverter::m_afResampleBuffer = NULL;
U32 ISampleRateConverter::m_uBufferLength = 0;
////////////////////////////////////////////////////////////////////////////////
//	Init
////////////////////////////////////////////////////////////////////////////////
void Audio::Core::ISampleRateConverter::Init( U32 uBufferLength )
{
	m_uBufferLength = uBufferLength;

	m_afResampleBuffer = new F32[ uBufferLength ];
}

////////////////////////////////////////////////////////////////////////////////
//	GetDecodedData
////////////////////////////////////////////////////////////////////////////////
bool Audio::Core::DecoderBase::GetDecodedFrame( F32* pfBuffer )
{
	if ( m_pSRC && m_pSRC->ResampleBuffer() )
	{
		if ( pfBuffer )
		{
			//	decode to resample buffer
			GetDecodedData( m_pSRC->ResampleBuffer(), m_pSRC->InputLength() );

			//	resample to output buffer, if buffer provided
			m_pSRC->Resample( pfBuffer, Audio::Core::uFrameLength );
		}
		else
		{
			//	reset history
			m_pSRC->ClearHistory();

			//	increment wave position but skip decoding
			GetDecodedData( NULL, m_pSRC->InputLength() );
		}
	}
	else
	{
		//	decode to output buffer
		GetDecodedData( pfBuffer, uFrameLength );
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////
//	ConvertS16ToF32
////////////////////////////////////////////////////////////////////////////////
void Audio::Core::DecoderBase::ConvertS16ToF32( F32* pfBufferOut, const S16* pfBufferIn, const U32 uSamplesIn )
{
	for ( U32 uCount=0; uCount < uSamplesIn; uCount++ )
		*pfBufferOut++ = F32(*pfBufferIn++) * Audio::Maths::fInv2pow15;
}

#endif // AUDIO_NEW_DECODERS

void ResampleOld(const float fPitch, const unsigned int uSamplesRequired, const F32* __restrict in, F32* __restrict out)
{
	if (fPitch != 1.f) // JASOND: uSamplesRequired != uFrameLength ?
	{
#ifdef PLATFORM_SPUA
		F32 fSubSample = 0.0f; //
		F32 fSubSampleDelta = (F32)uSamplesRequired * fRcpFrameLength; 
		
		vector float vSubSample = { 0.0f, 1.0f, 2.0f, 3.0f };
		vector float vSubSampleDelta = spu_splats( fSubSampleDelta );
		vector unsigned int vione = { 1, 1, 1, 1 };
		vector float vfour = spu_splats( 4.0f );
		for (U32 uCount = 0; uCount < uFrameLength; uCount+=4)
		{
			vector float vfIndex = spu_mul( vSubSample, vSubSampleDelta );
			vector unsigned int viIndex = spu_convtu(vfIndex, 0);
			vector float vfraction = spu_convtf(viIndex, 0);
			vfraction = spu_sub( vfIndex, vfraction );

			int indexA = spu_extract( viIndex, 0 );
			int indexB = spu_extract( viIndex, 1 );
			int indexC = spu_extract( viIndex, 2 );
			int indexD = spu_extract( viIndex, 3 );
			vector float vSampleA = { in[ indexA ], in[ indexB ], in[ indexC ], in[ indexD ] };

			viIndex = spu_add( viIndex, vione );
			indexA = spu_extract( viIndex, 0 );
			indexB = spu_extract( viIndex, 1 );
			indexC = spu_extract( viIndex, 2 );
			indexD = spu_extract( viIndex, 3 );
			vector float vSampleB = { in[ indexA ], in[ indexB ], in[ indexC ], in[ indexD ] };
						
			vector float vdiff = spu_sub( vSampleB, vSampleA );
			vector float vdelta = spu_mul( vdiff, vfraction );
			vector float vout = spu_add( vSampleA, vdelta );
			// look at using qword si store instruction
			out[ uCount + 0] = spu_extract( vout, 0 );			
			out[ uCount + 1] = spu_extract( vout, 1 );			
			out[ uCount + 2] = spu_extract( vout, 2 );			
			out[ uCount + 3] = spu_extract( vout, 3 );			
			
			vSubSample = spu_add( vSubSample, vfour );
		}
#else
		for (U32 uCount = 0; uCount < uFrameLength; uCount++)
		{
			F32 fSubSample = (F32)uCount * (F32)uSamplesRequired * fRcpFrameLength;

			F32 fIndex = floor(fSubSample);

			F32 fFraction = fSubSample - (F32)fIndex;
			S32 gIndex = (S32)fIndex;

			F32 fSampleA = in[gIndex];
			F32 fSampleB = in[gIndex + 1];

			F32 fDiff = fSampleB - fSampleA;
			F32 fDelta = fDiff * fFraction;
			out[uCount] = fSampleA + fDelta;	//	could do /32768 here on shift down?
		}
#endif
	}
	else
	{
		//	copy
		for (U32 uCount= 0; uCount < uFrameLength; uCount++)
			out[uCount] = in[uCount];
	}
}

#ifdef PLATFORM_XENON

void ResampleNew( const float fPitch, const unsigned int uSamplesRequired, const F32* __restrict in, F32* __restrict out )
{
	// Check if the pitch is such that a straight copy won't be the same
	if (fPitch != 1.f) // JASOND: uSamplesRequired != uFrameLength ?
	{
		_STATIC_ASSERT( Audio::Core::uFrameLength % 4 == 0 );
        
        #if defined( AUDIO_MATHS_VECTOR_OPTIMISE_RESAMPLE )
        
        using namespace Audio::Maths;
        
        Vector4 vIndexIncrement1 = MakeVector4( uSamplesRequired * 1 ) * MakeVector4( fRcpFrameLength );
        Vector4 vIndexIncrement8 = MakeVector4( uSamplesRequired * 8 ) * MakeVector4( fRcpFrameLength );
        
        //Vector4 vIndex_0123 = MakeVector4( g_VectorFloat_00_01_02_03 ) * vIndexIncrement8;
        //Vector4 vIndex_4567 = MakeVector4( g_VectorFloat_04_05_06_07 ) * vIndexIncrement8;
        Vector4 vIndex_0123 = MakeVector4( g_VectorFloat_00_01_02_03 ) * vIndexIncrement1;
        Vector4 vIndex_4567 = MakeVector4( g_VectorFloat_04_05_06_07 ) * vIndexIncrement1;
        
        U32 iIndex_x_uFrameLength = 0;
		for ( U32 uCount = 0; uCount < uFrameLength; uCount+=8 )
        {
            const U32 iIndex0 = iIndex_x_uFrameLength / uFrameLength; iIndex_x_uFrameLength += uSamplesRequired;
            const U32 iIndex1 = iIndex_x_uFrameLength / uFrameLength; iIndex_x_uFrameLength += uSamplesRequired;
            const U32 iIndex2 = iIndex_x_uFrameLength / uFrameLength; iIndex_x_uFrameLength += uSamplesRequired;
            const U32 iIndex3 = iIndex_x_uFrameLength / uFrameLength; iIndex_x_uFrameLength += uSamplesRequired;
            const U32 iIndex4 = iIndex_x_uFrameLength / uFrameLength; iIndex_x_uFrameLength += uSamplesRequired;
            const U32 iIndex5 = iIndex_x_uFrameLength / uFrameLength; iIndex_x_uFrameLength += uSamplesRequired;
            const U32 iIndex6 = iIndex_x_uFrameLength / uFrameLength; iIndex_x_uFrameLength += uSamplesRequired;
            const U32 iIndex7 = iIndex_x_uFrameLength / uFrameLength; iIndex_x_uFrameLength += uSamplesRequired;
            
            const Vector4 vSampleA_0123 = MakeVector4( in[iIndex0+0], in[iIndex1+0], in[iIndex2+0], in[iIndex3+0] );
            const Vector4 vSampleA_4567 = MakeVector4( in[iIndex4+0], in[iIndex5+0], in[iIndex6+0], in[iIndex7+0] );
            const Vector4 vSampleB_0123 = MakeVector4( in[iIndex0+1], in[iIndex1+1], in[iIndex2+1], in[iIndex3+1] );
            const Vector4 vSampleB_4567 = MakeVector4( in[iIndex4+1], in[iIndex5+1], in[iIndex6+1], in[iIndex7+1] );
            
            const Vector4 vFraction_0123 = vIndex_0123 - RoundToIntZero( vIndex_0123 );
            const Vector4 vFraction_4567 = vIndex_4567 - RoundToIntZero( vIndex_4567 );
            
            const Vector4 vDiff_0123 = vSampleB_0123 - vSampleA_0123;
            const Vector4 vDiff_4567 = vSampleB_4567 - vSampleA_4567;
            
            const Vector4 result_0123 = MulAdd( vFraction_0123, vDiff_0123, vSampleA_0123 );
            const Vector4 result_4567 = MulAdd( vFraction_4567, vDiff_4567, vSampleA_4567 );
            
            vIndex_0123 += vIndexIncrement8;
            vIndex_4567 += vIndexIncrement8;
            
            ReadWriteBarrier();
            
            StoreAligned( result_0123, out + uCount + 0 );
            StoreAligned( result_4567, out + uCount + 4 );
        }
        
        #else
        
		const static __declspec(align(16)) F32 sOffsetVec[4] = { 0.f, 1.f, 2.f, 3.f };

		__vector4 offsetVec		= __lvx(sOffsetVec, 0);

		__vector4 ssam = __vcfux(__lvlx(&uSamplesRequired, 0), 0);
		__vector4 sflr = __lvlx(&fRcpFrameLength, 0);

		__vector4 offsetVecInc	= __vcfux(__vspltisw(4), 0);
		__vector4 subSampleD	= __vspltw(__vmulfp(ssam, sflr), 0);

		int iIndex(0), iSSVal(0);

		//	resample
		for (U32 uCount = 0; uCount < uFrameLength; uCount+=4)
		{
			int index[4];

			for (int c=0; c < 4; c++)
			{
				index[c] = iIndex;

				iSSVal += uSamplesRequired;
				iIndex += (iSSVal & ~(uFrameLength-1)) / (uFrameLength / 4);
				iSSVal &= uFrameLength-1;
			}

			__vector4 vSubSample = __vmulfp(offsetVec, subSampleD);

			//F32 fIndex1		= Frame::Floor(fSubSample);
			//F32 fFraction1	= fSubSample - (F32)fIndex1;
			//S32 sIndex1		= (S32)fIndex1;
			__vector4 indexVec		= __vrfiz(vSubSample);
			__vector4 fractionVec	= __vsubfp(vSubSample, indexVec);

			//F32 fSampleAL1 = afShiftBuffer[sIndex1];
			//F32 fSampleBL1 = afShiftBuffer[sIndex1 + 1];
			__vector4 leftVecA;
			__vector4 leftVecB;

			{
				__vector4 tmp;

				leftVecA = __lvlx(in, index[0]);
				leftVecB = __lvlx(in, 4 + index[0]);

				__vector4 leftVecTmp = __lvlx(in, index[1]);
				tmp = __lvlx(in, 4 + index[1]);
				leftVecA = __vrlimi(leftVecA, leftVecTmp, ::Maths::VMX_MASK_INSERT_Y, ::Maths::VMX_ROTATE_X_TO_Y );
				leftVecB = __vrlimi(leftVecB, tmp, ::Maths::VMX_MASK_INSERT_Y, ::Maths::VMX_ROTATE_X_TO_Y );

				leftVecTmp	= __lvlx(in, index[2]);
				tmp = __lvlx(in, 4 + index[2]);
				leftVecA = __vrlimi(leftVecA, leftVecTmp, ::Maths::VMX_MASK_INSERT_Z, ::Maths::VMX_ROTATE_X_TO_Z );
				leftVecB = __vrlimi(leftVecB, tmp, ::Maths::VMX_MASK_INSERT_Z, ::Maths::VMX_ROTATE_X_TO_Z );

				leftVecTmp	= __lvlx(in, index[3]);
				tmp = __lvlx(in, 4 + index[3]);
				leftVecA = __vrlimi(leftVecA, leftVecTmp, ::Maths::VMX_MASK_INSERT_W, ::Maths::VMX_ROTATE_X_TO_W );
				leftVecB = __vrlimi(leftVecB, tmp, ::Maths::VMX_MASK_INSERT_W, ::Maths::VMX_ROTATE_X_TO_W );
			}

			//F32 fDiffL1 = fSampleBL1 - fSampleAL1;
			//F32 fDeltaL1 = fDiffL1 * fFraction1;
			//afDataOut[uCount]		= fSampleAL1 + fDeltaL1;					//	could do /32768 here on shift down?
			__vector4 ldiff			= __vsubfp(leftVecB, leftVecA);
			__vector4 lOut			= __vmaddfp(ldiff, fractionVec, leftVecA);
			__stvx(lOut, out, uCount * 4);

			offsetVec = __vaddfp(offsetVec, offsetVecInc);
		}
		
		#endif
	}
	else
	{
		// copy
		for (U32 uCount= 0; uCount < uFrameLength; uCount++)
			out[uCount] = in[uCount];
	}
}

#endif

void ResampleOld(const float fPitch, unsigned int uSamplesRequired, const F32* __restrict inLeft, const F32* __restrict inRight, F32* __restrict outLeft, F32* __restrict outRight)
{
	ResampleOld(fPitch, uSamplesRequired, inLeft, outLeft);
	ResampleOld(fPitch, uSamplesRequired, inRight, outRight);
	//if (fPitch != 1.f) // JASOND: uSamplesRequired != uFrameLength ?
	//{
	//	for (U32 uCount = 0; uCount < uFrameLength; uCount++)
	//	{
	//		F32 fSubSample = (F32)uCount * (F32)uSamplesRequired * fRcpFrameLength;

	//		F32 fIndex = Audio::Maths::Floor(fSubSample);

	//		F32 fFraction = fSubSample - (F32)fIndex;
	//		S32 gIndex = (S32)fIndex;

	//		F32 fSampleAL = inLeft[gIndex];
	//		F32 fSampleBL = inLeft[gIndex + 1];
	//		F32 fSampleAR = inRight[gIndex];
	//		F32 fSampleBR = inRight[gIndex + 1];

	//		F32 fDiffL = fSampleBL - fSampleAL;
	//		F32 fDiffR = fSampleBR - fSampleAR;
	//		F32 fDeltaL = fDiffL * fFraction;
	//		F32 fDeltaR = fDiffR * fFraction;

	//		outLeft[uCount]		= fSampleAL + fDeltaL;	//	could do /32768 here on shift down?
	//		outRight[uCount]	= fSampleAR + fDeltaR;	//	and here
	//	}
	//}
	//else
	//{
	//	//	copy
	//	for (U32 uCount=0; uCount < uFrameLength; uCount++)
	//	{
	//		outLeft[uCount] = inLeft[uCount];
	//		outRight[uCount] = inRight[uCount];
	//	}
	//	}
	//}
}

#ifdef PLATFORM_XENON

void ResampleNew(const float fPitch, const unsigned int uSamplesRequired, const F32* __restrict inLeft, const F32* __restrict inRight, F32* __restrict outLeft, F32* __restrict outRight)
{
	// Check if the pitch is such that a straight copy won't be the same
	if (fPitch != 1.f) // JASOND: uSamplesRequired != uFrameLength ?
	{
		_STATIC_ASSERT(Audio::Core::uFrameLength % 4 == 0);

		const static __declspec(align(16)) F32 sOffsetVec[4] = { 0.f, 1.f, 2.f, 3.f };

		__vector4 offsetVec		= __lvx(sOffsetVec, 0);

		__vector4 ssam = __vcfux(__lvlx(&uSamplesRequired, 0), 0);
		__vector4 sflr = __lvlx(&fRcpFrameLength, 0);

		__vector4 offsetVecInc	= __vcfux(__vspltisw(4), 0);
		__vector4 subSampleD	= __vspltw(__vmulfp(ssam, sflr), 0);

		int iIndex = 0;
		unsigned int iSSVal = 0;

		//	resample
		for (U32 uCount = 0; uCount < uFrameLength; uCount+=4)
		{
			int index[4];

			for (int c=0; c < 4; c++)
			{
				index[c] = iIndex;

				iSSVal += uSamplesRequired;
				iIndex += (iSSVal & ~(uFrameLength-1)) / (uFrameLength / 4);
				iSSVal &= uFrameLength-1;
			}

			__vector4 vSubSample = __vmulfp(offsetVec, subSampleD);

			//F32 fIndex1		= Frame::Floor(fSubSample);
			//F32 fFraction1	= fSubSample - (F32)fIndex1;
			//S32 sIndex1		= (S32)fIndex1;
			__vector4 indexVec		= __vrfiz(vSubSample);
			__vector4 fractionVec	= __vsubfp(vSubSample, indexVec);

			//F32 fSampleAL1 = afShiftBuffer[sIndex1];
			//F32 fSampleBL1 = afShiftBuffer[sIndex1 + 1];
			//F32 fSampleAR1 = afShiftBufferRight[sIndex1];
			//F32 fSampleBR1 = afShiftBufferRight[sIndex1 + 1];
			__vector4 leftVecA, leftVecB, rightVecA, rightVecB;

			// Left
			{
				__vector4 tmp;

				leftVecA = __lvlx(inLeft, index[0]);
				leftVecB = __lvlx(inLeft, 4 + index[0]);

				__vector4 leftVecTmp = __lvlx(inLeft, index[1]);
				tmp = __lvlx(inLeft, 4 + index[1]);
				leftVecA = __vrlimi(leftVecA, leftVecTmp, ::Maths::VMX_MASK_INSERT_Y, ::Maths::VMX_ROTATE_X_TO_Y );
				leftVecB = __vrlimi(leftVecB, tmp, ::Maths::VMX_MASK_INSERT_Y, ::Maths::VMX_ROTATE_X_TO_Y );

				leftVecTmp	= __lvlx(inLeft, index[2]);
				tmp = __lvlx(inLeft, 4 + index[2]);
				leftVecA = __vrlimi(leftVecA, leftVecTmp, ::Maths::VMX_MASK_INSERT_Z, ::Maths::VMX_ROTATE_X_TO_Z );
				leftVecB = __vrlimi(leftVecB, tmp, ::Maths::VMX_MASK_INSERT_Z, ::Maths::VMX_ROTATE_X_TO_Z );

				leftVecTmp	= __lvlx(inLeft, index[3]);
				tmp = __lvlx(inLeft, 4 + index[3]);
				leftVecA = __vrlimi(leftVecA, leftVecTmp, ::Maths::VMX_MASK_INSERT_W, ::Maths::VMX_ROTATE_X_TO_W );
				leftVecB = __vrlimi(leftVecB, tmp, ::Maths::VMX_MASK_INSERT_W, ::Maths::VMX_ROTATE_X_TO_W );
			}

			// Right
			{
				__vector4 tmp;

				rightVecA = __lvlx(inRight, index[0]);
				rightVecB = __lvlx(inRight, 4 + index[0]);

				__vector4 rightVecTmp	= __lvlx(inRight, index[1]);
				tmp = __lvlx(inRight, 4 + index[1]);
				rightVecA = __vrlimi(rightVecA, rightVecTmp, ::Maths::VMX_MASK_INSERT_Y, ::Maths::VMX_ROTATE_X_TO_Y );
				rightVecB = __vrlimi(rightVecB, tmp, ::Maths::VMX_MASK_INSERT_Y, ::Maths::VMX_ROTATE_X_TO_Y );

				rightVecTmp	= __lvlx(inRight, index[2]);
				tmp = __lvlx(inRight, 4 + index[2]);
				rightVecA = __vrlimi(rightVecA, rightVecTmp, ::Maths::VMX_MASK_INSERT_Z, ::Maths::VMX_ROTATE_X_TO_Z );
				rightVecB = __vrlimi(rightVecB, tmp, ::Maths::VMX_MASK_INSERT_Z, ::Maths::VMX_ROTATE_X_TO_Z );

				rightVecTmp	= __lvlx(inRight, index[3]);
				tmp = __lvlx(inRight, 4 + index[3]);
				rightVecA = __vrlimi(rightVecA, rightVecTmp, ::Maths::VMX_MASK_INSERT_W, ::Maths::VMX_ROTATE_X_TO_W );
				rightVecB = __vrlimi(rightVecB, tmp, ::Maths::VMX_MASK_INSERT_W, ::Maths::VMX_ROTATE_X_TO_W );
			}

			//F32 fDiffL1 = fSampleBL1 - fSampleAL1;
			//F32 fDeltaL1 = fDiffL1 * fFraction1;
			//afDataOut[uCount]		= fSampleAL1 + fDeltaL1;					//	could do /32768 here on shift down?
			__vector4 ldiff			= __vsubfp(leftVecB, leftVecA);
			__vector4 lOut			= __vmaddfp(ldiff, fractionVec, leftVecA);
			__stvx(lOut, outLeft, uCount * 4);

			//F32 fDiffR1 = fSampleBR1 - fSampleAR1;
			//F32 fDeltaR1 = fDiffR1 * fFraction1;
			//afDataOut[uFrameLength + uCount]	= fSampleAR1 + fDeltaR1;	//	and here
			__vector4 rdiff			= __vsubfp(rightVecB, rightVecA);
			__vector4 rOut			= __vmaddfp(rdiff, fractionVec, rightVecA);
			__stvx(rOut, outRight, uCount * 4);

			offsetVec = __vaddfp(offsetVec, offsetVecInc);
		}
	}
	else
	{
		//	copy
		for (U32 uCount=0; uCount < uFrameLength; uCount++)
		{
			outLeft[uCount] = inLeft[uCount];
			outRight[uCount] = inRight[uCount];
		}
	}
}

#endif

void Decoder::Resample(const float fPitch, const unsigned int uSamplesRequired, const F32* __restrict in, F32* __restrict out)
{
#if ! defined (PLATFORM_XENON)
	ResampleOld(fPitch, uSamplesRequired, in, out);
#else
	if (!g_switchResampler)
		ResampleOld(fPitch, uSamplesRequired, in, out);
	else
	{
		ResampleNew(fPitch, uSamplesRequired, in, out);

#ifdef CHECK_OUTPUT_QUALITY
		// Check we did it right
		{
			F32 tempOut[Audio::Core::uFrameLength];

			ResampleOld(fPitch, uSamplesRequired, in, tempOut);

			for (int i=0; i < Audio::Core::uFrameLength; i++)
			{
				if (tempOut[i] != out[i])
					DebugBreak();
			}
		}
#endif
	}
#endif
}

void Decoder::Resample(const float fPitch, const unsigned int uSamplesRequired, const F32* __restrict inLeft, const F32* __restrict inRight, F32* __restrict outLeft, F32* __restrict outRight)
{
#if defined (PLATFORM_XENON)
	if (!g_switchResampler)
		ResampleOld(fPitch, uSamplesRequired, inLeft, inRight, outLeft, outRight);
	else
#else
	ResampleOld(fPitch, uSamplesRequired, inLeft, inRight, outLeft, outRight);
#endif
	{
#ifdef PLATFORM_XENON
		ResampleNew(fPitch, uSamplesRequired, inLeft, inRight, outLeft, outRight);

#ifdef CHECK_OUTPUT_QUALITY
		// Check we did it right
		{
			F32 tempOutL[Audio::Core::uFrameLength];
			F32 tempOutR[Audio::Core::uFrameLength];

			ResampleOld(fPitch, uSamplesRequired, inLeft, inRight, tempOutL, tempOutR);

			for (int i=0; i < Audio::Core::uFrameLength; i++)
			{
				if ((tempOutL[i] != outLeft[i]) ||(tempOutR[i] != outRight[i]))
					DebugBreak();
			}
		}
#endif
#endif
	}
}

// Fill buffer - uses fixed point maths 8:24, max pitch +8 octaves interpolate:
void FillF32BufferFromS16Old(const unsigned int samplesRequired, const S16* AUDIO_RESTRICT src, F32* AUDIO_RESTRICT dst)
{
	for (U32 uCount=0; uCount < samplesRequired; uCount++)
		*dst++ = F32(*src++) * Audio::Maths::fInv2pow15;
}

#ifdef PLATFORM_XENON

void FillF32BufferFromS16New(const unsigned int samplesRequired, const S16* AUDIO_RESTRICT src, F32* AUDIO_RESTRICT dst)
{
    using namespace Audio::Maths;
    
    if (samplesRequired > 1024)
        DebugBreak();
        
	U32 uCount(0);
	U32 uOffset = 0;
	U32 linesToPrefetch = ((U32(src) & 127) + (2*samplesRequired) + 127) / 128;

	for (U32 pf=0; pf < linesToPrefetch; pf++)
		__dcbt((pf*128) + (2*uOffset), src);

	// Need to get output aligned
	U32 align = U32(dst) & 0xf;

	if (align != 0)
	{
		for (; (align < 16) && (uCount < samplesRequired); uCount++, align+=4, uOffset++)
			dst[uCount] = (F32)src[uOffset] * Audio::Maths::fInv2pow15;
	}
    
    #if defined( AUDIO_MATHS_VECTOR_OPTIMISE_FILL_F32_BUFFER_FROM_S16 )
    
	//	Copy samples aligned while we can
	const U32 sampleCount32 = (uCount + ((samplesRequired - uCount) & ~0x1F)) & ~0x1F;

	for (; uCount < sampleCount32; uCount+=32, uOffset+=32)
	{
		const Vector8_S16 v_src0 = LoadUnaligned( src + uOffset + 0x00 );
		const Vector8_S16 v_src1 = LoadUnaligned( src + uOffset + 0x08 );
		const Vector8_S16 v_src2 = LoadUnaligned( src + uOffset + 0x10 );
		const Vector8_S16 v_src3 = LoadUnaligned( src + uOffset + 0x18 );
		
		const Vector4 v_dst0 = IntToFloatHi<15>( v_src0 );
		const Vector4 v_dst1 = IntToFloatLo<15>( v_src0 );
		const Vector4 v_dst2 = IntToFloatHi<15>( v_src1 );
		const Vector4 v_dst3 = IntToFloatLo<15>( v_src1 );
		const Vector4 v_dst4 = IntToFloatHi<15>( v_src2 );
		const Vector4 v_dst5 = IntToFloatLo<15>( v_src2 );
		const Vector4 v_dst6 = IntToFloatHi<15>( v_src3 );
		const Vector4 v_dst7 = IntToFloatLo<15>( v_src3 );
		
		StoreAligned( v_dst0, dst + uCount + 0x00 );
		StoreAligned( v_dst1, dst + uCount + 0x04 );
		StoreAligned( v_dst2, dst + uCount + 0x08 );
		StoreAligned( v_dst3, dst + uCount + 0x0C );
		StoreAligned( v_dst4, dst + uCount + 0x10 );
		StoreAligned( v_dst5, dst + uCount + 0x14 );
		StoreAligned( v_dst6, dst + uCount + 0x18 );
		StoreAligned( v_dst7, dst + uCount + 0x1C );
	}
    
    #endif
    
	//	Copy samples aligned while we can
	const U32 sampleCount8 = (uCount + ((samplesRequired - uCount) & ~0x7)) & ~0x7;

	for (; uCount < sampleCount8; uCount+=8, uOffset+=8)
	{
	    #if !defined( AUDIO_MATHS_VECTOR_OPTIMISE_FILL_F32_BUFFER_FROM_S16 )
	    
		//afShifted[uCount] = (F32)asDataIn[uOffset] * Maths::fInv2pow15;
		// Read in 8 values at once
		__vector4 tmp1 = __lvlx(src, 2 * uOffset);
		__vector4 tmp1a = __lvrx(src, 16 + 2*uOffset);
		tmp1 = __vor(tmp1, tmp1a);

		__vector4 tmp2 = __vupkhsh(tmp1);
		__vector4 tmp2a = __vupklsh(tmp1);
		__vector4 tmp3 = __vcfsx(tmp2, 15);
		__vector4 tmp3a = __vcfsx(tmp2a, 15);

		__stvx(tmp3, dst, uCount * 4);
		__stvx(tmp3a, dst, 16 + uCount * 4);
		
		#else
		
		const Vector8_S16 v_src = LoadUnaligned( src + uOffset );
		const Vector4 v_dst0 = IntToFloatHi<15>( v_src );
		const Vector4 v_dst1 = IntToFloatLo<15>( v_src );
		StoreAligned( v_dst0, dst + uCount + 0 );
		StoreAligned( v_dst1, dst + uCount + 4 );
		
		#endif
	}

	for (; uCount < samplesRequired; uCount++, uOffset++)
	{
		dst[uCount] = (F32)src[uOffset] * Audio::Maths::fInv2pow15;
	}
}

#endif

void Decoder::FillF32BufferFromS16(const unsigned int samplesRequired, const S16* AUDIO_RESTRICT src, F32* AUDIO_RESTRICT dst)
{
#ifndef PLATFORM_XENON
	FillF32BufferFromS16Old(samplesRequired, src, dst);
#else
	FillF32BufferFromS16New(samplesRequired, src, dst);

#ifdef _DEBUG
	// Check we did it right
	F32* tempOut = new F32[samplesRequired];

	FillF32BufferFromS16Old(samplesRequired, src, tempOut);

	for (unsigned int i=0; i < samplesRequired; i++)
	{
		if (tempOut[i] != dst[i])
			DebugBreak();
	}

	delete [] tempOut;
#endif
#endif
}

void FillF32BufferFromS16Old(const unsigned int samplesRequired, const S16* AUDIO_RESTRICT src, F32* AUDIO_RESTRICT dstL, F32* AUDIO_RESTRICT dstR)
{
	for (U32 uCount=0; uCount < samplesRequired; uCount++)
	{
		*dstL++ = F32(*src++) * Audio::Maths::fInv2pow15;
		*dstR++ = F32(*src++) * Audio::Maths::fInv2pow15;
	}
}

#ifdef PLATFORM_XENON

void FillF32BufferFromS16New(const unsigned int samplesRequired, const S16* AUDIO_RESTRICT src, F32* AUDIO_RESTRICT dstL, F32* AUDIO_RESTRICT dstR)
{
	U32 uCount(0);
	U32 uOffset = 0;
	U32 linesToPrefetch = ((U32(src) & 127) + (2*2*samplesRequired) + 127) / 128;

	for (U32 pf=0; pf < linesToPrefetch; pf++)
		__dcbt((pf*128) + (2*uOffset), src);

	// Need to write out 4 at a time

	// Need to get output aligned
	U32 align = U32(dstL) & 0xf;
	U32 align2 = U32(dstR) & 0xf;

	Assert(align == align2);

	if (align != 0)
	{
		for (; (align < 16) && (uCount < samplesRequired); uCount++, align+=4, uOffset+=2)
		{
			dstL[uCount] = (F32)src[uOffset] * Audio::Maths::fInv2pow15;
			dstR[uCount] = (F32)src[uOffset + 1] * Audio::Maths::fInv2pow15;
		}
	}

	//	Copy samples aligned while we can
	U32 sampleCount = uCount + ((samplesRequired - uCount) & ~0x3);

	const static __declspec(align(16)) char fPermEven[16] = {0,1,2,3, 8,9,10,11,   16,17,18,19, 24,25,26,27};
	const static __declspec(align(16)) char fPermOdd[16] =  {4,5,6,7, 12,13,14,15, 20,21,22,23, 28,29,30,31};

	register const __vector4 vPermEven = __lvx(fPermEven, 0);
	register const __vector4 vPermOdd =  __lvx(fPermOdd, 0);

	for (; uCount < sampleCount; uCount+=4, uOffset+=2*4)
	{
		//afShiftedL[uCount] = (F32)asDataIn[uOffset] * Maths::fInv2pow15;
		//afShiftedR[uCount] = (F32)asDataIn[uOffset + 1] * Maths::fInv2pow15;
		__vector4 tmp1 = __lvlx(src, 2 * uOffset);
		__vector4 tmp1a = __lvrx(src, 16 + 2 * uOffset);
		tmp1 = __vor(tmp1, tmp1a);
		__vector4 tmp2 = __vupkhsh(tmp1);
		__vector4 tmp2a = __vupklsh(tmp1);
		__vector4 tmp3 = __vcfsx(tmp2, 15);
		__vector4 tmp3a = __vcfsx(tmp2a, 15);

		__vector4 l1 = __vperm(tmp3, tmp3a, vPermEven);
		__stvx(l1, dstL, uCount * 4);

		__vector4 r1 = __vperm(tmp3, tmp3a, vPermOdd);
		__stvx(r1, dstR, uCount * 4);
	}

	for (; uCount < samplesRequired; uCount++, uOffset+=2)
	{
		dstL[uCount] = (F32)src[uOffset] * Audio::Maths::fInv2pow15;
		dstR[uCount] = (F32)src[uOffset + 1] * Audio::Maths::fInv2pow15;
	}
}

#endif

void Decoder::FillF32BufferFromS16(const unsigned int samplesRequired, const S16* AUDIO_RESTRICT src, F32* AUDIO_RESTRICT dstL, F32* AUDIO_RESTRICT dstR)
{
#ifndef PLATFORM_XENON
	FillF32BufferFromS16Old(samplesRequired, src, dstL, dstR);
#else
	FillF32BufferFromS16New(samplesRequired, src, dstL, dstR);

#ifdef _DEBUG
		// Check we did it right
	F32* tempOutL = new F32[samplesRequired];
	F32* tempOutR = new F32[samplesRequired];

	FillF32BufferFromS16Old(samplesRequired, src, tempOutL, tempOutR);

	for (unsigned int i=0; i < samplesRequired; i++)
	{
		if ((tempOutL[i] != dstL[i]) || (tempOutR[i] != dstR[i]))
			DebugBreak();
	}

	delete [] tempOutL;
	delete [] tempOutR;
#endif
#endif
}

void Decoder::LowPassFilter(unsigned int uSamplesRequired, MonoState& state, F32* AUDIO_RESTRICT buffer, float fA, float fB)
{
	//	1st order low pass filter
	F32 fPrevIn = state.fPrevIn;
	F32 fPrevOut = state.fPrevOut;
	
	for (U32 uCount=0; uCount < uSamplesRequired; uCount++)
	{
		F32 fIn  = buffer[uCount];
		F32 fOut = fA * (fIn + fPrevIn) + fB * fPrevOut;

		buffer[uCount] = fOut; 

		fPrevIn = fIn;
		fPrevOut = fOut;
	}
	state.fPrevIn = fPrevIn;
	state.fPrevOut = fPrevOut;

	{
		//	do extra sample
		F32 fIn  = buffer[uSamplesRequired];
		F32 fOut = fA * (fIn + fPrevIn) + fB * fPrevOut;

		buffer[uSamplesRequired] = fOut;

		//	store last sample for next iteration
		state.fPrevSample = buffer[uSamplesRequired];
	}
}

void Decoder::LowPassFilter(unsigned int uSamplesRequired, StereoState& state, F32* AUDIO_RESTRICT bufferL, F32* AUDIO_RESTRICT bufferR, float fA, float fB)
{
	//	1st order low pass filter
	F32 fPrevInL = state.fPrevInL;
	F32 fPrevInR = state.fPrevInR;
	F32 fPrevOutL = state.fPrevOutL;
	F32 fPrevOutR = state.fPrevOutR;

	for (U32 uCount = 0; uCount < uSamplesRequired; uCount++)
	{
		F32 fInL  = bufferL[uCount];
		F32 fOutL = fA * (fInL + fPrevInL) + fB * fPrevOutL;
		F32 fInR  = bufferR[uCount];
		F32 fOutR = fA * (fInR + fPrevInR) + fB * fPrevOutR;

		bufferL[uCount] = fOutL; 
		bufferR[uCount] = fOutR; 

		fPrevInL = fInL;
		fPrevInR = fInR;
		fPrevOutL = fOutL;
		fPrevOutR = fOutR;
	}
	state.fPrevInL = fPrevInL;
	state.fPrevInR = fPrevInR;
	state.fPrevOutL = fPrevOutL;
	state.fPrevOutR = fPrevOutR;

	//	do extra sample
	{
		F32 fInL  = bufferL[uSamplesRequired];
		F32 fOutL = fA * (fInL + fPrevInL) + fB * fPrevOutL;
		F32 fInR  = bufferR[uSamplesRequired];
		F32 fOutR = fA * (fInR + fPrevInR) + fB * fPrevOutR;

		bufferL[uSamplesRequired] = fOutL; 
		bufferR[uSamplesRequired] = fOutR; 

		//	store last sample for next iteration
		state.fPrevSampleL = fOutL;
		state.fPrevSampleR = fOutR;
	}
}

#ifndef VF_DECODERS

void Decoder::Init(const DecoderDataInfo& info)
{
#ifdef PLATFORM_SPU
	SwitchInGeneratorOverlay();
#endif

	switch (m_eType)
	{
		case uPCM16BitSignedInt:
		{
			if (m_u32NumChannels == 1)
				((MonoPCMS16Decoder*)this)->MonoPCMS16Decoder::Init(info);
			else if (m_u32NumChannels == 2)
				((StereoPCMS16Decoder*)this)->StereoPCMS16Decoder::Init(info);
		}
		break;

		case uADPCM4Bit:
		{
			if (m_u32NumChannels == 1)
				((MonoADPCM4bitDecoder*)this)->MonoADPCM4bitDecoder::Init(info);
			else if (m_u32NumChannels == 2)
				((StereoADPCM4bitDecoder*)this)->StereoADPCM4bitDecoder::Init(info);
		}
		break;

#ifdef PLATFORM_XENON
		case uXMA16Bit:
		case uXMA216Bit:
		{
			((XMADecoder*)this)->XMADecoder::Init(pWaveServerData);
		}
		break;
#endif

		default:
			Assert(0 && "Unknown audio decoder type.");
		break;
	}

#ifdef PLATFORM_SPU
	SwitchBackToOriginalOverlay();
#endif
}


void Decoder::Shutdown(const DecoderDataInfo& info)
{
#ifdef PLATFORM_SPU
	SwitchInGeneratorOverlay();
#endif

	switch (m_eType)
	{
		case uPCM16BitSignedInt:
		{
			((PCMS16Decoder*)this)->PCMS16Decoder::Shutdown(info);
		}
		break;

		case uADPCM4Bit:
		{
			((ADPCM4bitDecoder*)this)->ADPCM4bitDecoder::Shutdown(info);
		}
		break;

#ifdef PLATFORM_XENON
		case uXMA16Bit:
		case uXMA216Bit:
		{
			((XMADecoder*)this)->XMADecoder::Shutdown(info);
		}
		break;
#endif

		default:
			Assert(0 && "Unknown audio decoder type.");
		break;
	}

#ifdef PLATFORM_SPU
	SwitchBackToOriginalOverlay();
#endif
}


bool Decoder::IsActive(const DecoderDataInfo& info)
{
#ifdef PLATFORM_SPU
	SwitchInGeneratorOverlay();
#endif
	bool bRetvalue = false;

	switch (m_eType)
	{
		case uPCM16BitSignedInt:
		{
			bRetvalue = ((PCMS16Decoder*)this)->PCMS16Decoder::IsActive(info);
		}
		break;

		case uADPCM4Bit:
		{
			bRetvalue = ((ADPCM4bitDecoder*)this)->ADPCM4bitDecoder::IsActive(info);
		}
		break;

#ifdef PLATFORM_XENON
		case uXMA16Bit:
		case uXMA216Bit:
		{
			bRetvalue = ((XMADecoder*)this)->XMADecoder::IsActive(info);
		}
		break;
#endif

		default:
			Assert(0 && "Unknown audio decoder type.");
		break;
	}

#ifdef PLATFORM_SPU
	SwitchBackToOriginalOverlay();
#endif

	return bRetvalue;
}


//void Decoder::SetCallbackId(U32 u32StateOffset, U32 u32CallbackId)
//{
//#ifdef PLATFORM_SPU
//	SwitchInGeneratorOverlay();
//#endif
//
//	switch (m_eType)
//	{
//		case uPCM16BitSignedInt:
//		{
//			((PCMS16Decoder*)this)->PCMS16Decoder::SetCallbackId(u32StateOffset, u32CallbackId);
//		}
//		break;
//
//		case uADPCM4Bit:
//		{
//			((ADPCM4bitDecoder*)this)->ADPCM4bitDecoder::SetCallbackId(u32StateOffset, u32CallbackId);
//		}
//		break;
//
//#ifdef PLATFORM_XENON
//		case uXMA16Bit:
//		case uXMA216Bit:
//		{
//			((XMADecoder*)this)->XMADecoder::SetCallbackId(u32StateOffset, u32CallbackId);
//		}
//		break;
//#endif
//
//		default:
//			Assert(0 && "Unknown audio decoder type.");
//		break;
//	}
//
//#ifdef PLATFORM_SPU
//	SwitchBackToOriginalOverlay();
//#endif
//}

void Decoder::SetHasCallback()
{
#ifdef PLATFORM_SPU
	SwitchInGeneratorOverlay();
#endif

	switch (m_eType)
	{
		case uPCM16BitSignedInt:
		{
			((PCMS16Decoder*)this)->PCMS16Decoder::SetHasCallback();
		}
		break;

		case uADPCM4Bit:
		{
			((ADPCM4bitDecoder*)this)->ADPCM4bitDecoder::SetHasCallback();
		}
		break;

#ifdef PLATFORM_XENON
		case uXMA16Bit:
		case uXMA216Bit:
		{
			((XMADecoder*)this)->XMADecoder::SetHasCallback();
		}
		break;
#endif

		default:
			Assert(0 && "Unknown audio decoder type.");
		break;
	}

#ifdef PLATFORM_SPU
	SwitchBackToOriginalOverlay();
#endif
}


bool Decoder::FillBuffer(F32* afData, const ProcessInfo& info)
{
#ifdef PLATFORM_SPU
	SwitchInGeneratorOverlay();
#endif

	bool bRetValue = false;

	switch (m_eType)
	{
		case uPCM16BitSignedInt:
		{
			if (m_u32NumChannels == 1)
				bRetValue = ((MonoPCMS16Decoder*)this)->MonoPCMS16Decoder::FillBuffer(afData, info);
			else if (m_u32NumChannels == 2)
				bRetValue = ((StereoPCMS16Decoder*)this)->StereoPCMS16Decoder::FillBuffer(afData, info);
		}
		break;

		case uADPCM4Bit:
		{
			if (m_u32NumChannels == 1)
				bRetValue = ((MonoADPCM4bitDecoder*)this)->MonoADPCM4bitDecoder::FillBuffer(afData, info);
			else if (m_u32NumChannels == 2)
				bRetValue = ((StereoADPCM4bitDecoder*)this)->StereoADPCM4bitDecoder::FillBuffer(afData, info);
		}
		break;

#ifdef PLATFORM_XENON
		case uXMA16Bit:
		case uXMA216Bit:
		{
			bRetValue = ((XMADecoder*)this)->XMADecoder::FillBuffer(afData, info);
		}
		break;
#endif

		default:
			Assert(0 && "Unknown audio decoder type.");
		break;
	}

#ifdef PLATFORM_SPU
	SwitchBackToOriginalOverlay();
#endif

	return bRetValue;
}


void Decoder::SkipBuffer(const ProcessInfo& info)
{
#ifdef PLATFORM_SPU
	SwitchInGeneratorOverlay();
#endif

	switch (m_eType)
	{
		case uPCM16BitSignedInt:
		{
			if (m_u32NumChannels == 1)
				return ((MonoPCMS16Decoder*)this)->MonoPCMS16Decoder::SkipBuffer(info);
			else if (m_u32NumChannels == 2)
				return ((StereoPCMS16Decoder*)this)->StereoPCMS16Decoder::SkipBuffer(info);
		}
		break;

		case uADPCM4Bit:
		{
			if (m_u32NumChannels == 1)
				return ((MonoADPCM4bitDecoder*)this)->MonoADPCM4bitDecoder::SkipBuffer(info);
			else if (m_u32NumChannels == 2)
				return ((StereoADPCM4bitDecoder*)this)->StereoADPCM4bitDecoder::SkipBuffer(info);
		}
		break;

#ifdef PLATFORM_XENON
		case uXMA16Bit:
		case uXMA216Bit:
		{
			((XMADecoder*)this)->XMADecoder::SkipBuffer(info);
		}
		break;
#endif

		default:
			Assert(0 && "Unknown audio decoder type.");
		break;
	}

#ifdef PLATFORM_SPU
	SwitchBackToOriginalOverlay();
#endif
}


F32 Decoder::Position(const DecoderDataInfo& info)
{
#ifdef PLATFORM_SPU
	SwitchInGeneratorOverlay();
#endif

	F32 f32RetValue = 0.f;

	switch (m_eType)
	{
		case uPCM16BitSignedInt:
		{
			f32RetValue = ((PCMS16Decoder*)this)->PCMS16Decoder::Position(info);
		}
		break;

		case uADPCM4Bit:
		{
			f32RetValue = ((ADPCM4bitDecoder*)this)->ADPCM4bitDecoder::Position(info);
		}
		break;

#ifdef PLATFORM_XENON
		case uXMA16Bit:
		case uXMA216Bit:
		{
			f32RetValue = ((XMADecoder*)this)->XMADecoder::Position(pWaveServerData);
		}
		break;
#endif

		default:
			Assert(0 && "Unknown audio decoder type.");
		break;
	}

#ifdef PLATFORM_SPU
	SwitchBackToOriginalOverlay();
#endif

	return f32RetValue;
}


void Decoder::Reset(const DecoderDataInfo& info, U32 uOffset)
{
#ifdef PLATFORM_SPU
	SwitchInGeneratorOverlay();
#endif

	switch (m_eType)
	{
		case uPCM16BitSignedInt:
		{
			((PCMS16Decoder*)this)->PCMS16Decoder::Reset(info, uOffset);
		}
		break;

		case uADPCM4Bit:
		{
			((ADPCM4bitDecoder*)this)->ADPCM4bitDecoder::Reset(info, uOffset);
		}
		break;

#ifdef PLATFORM_XENON
		case uXMA16Bit:
		case uXMA216Bit:
		{
			((XMADecoder*)this)->XMADecoder::Reset(info, uOffset);
		}
		break;
#endif

		default:
			Assert(0 && "Unknown audio decoder type.");
		break;
	}

#ifdef PLATFORM_SPU
	SwitchBackToOriginalOverlay();
#endif
}


void Decoder::SetPitch(F32 fPitch)
{
#ifdef PLATFORM_SPU
	SwitchInGeneratorOverlay();
#endif

	switch (m_eType)
	{
		case uPCM16BitSignedInt:
		{
			((PCMS16Decoder*)this)->PCMS16Decoder::SetPitch(fPitch);
		}
		break;

		case uADPCM4Bit:
		{
			((ADPCM4bitDecoder*)this)->ADPCM4bitDecoder::SetPitch(fPitch);
		}
		break;

#ifdef PLATFORM_XENON
		case uXMA16Bit:
		case uXMA216Bit:
		{
			((XMADecoder*)this)->XMADecoder::SetPitch(fPitch);
		}
		break;
#endif

		default:
			Assert(0 && "Unknown audio decoder type.");
		break;
	}

#ifdef PLATFORM_SPU
	SwitchBackToOriginalOverlay();
#endif
}


void Decoder::SetPosition(U32 uPosition)
{
#ifdef PLATFORM_SPU
	SwitchInGeneratorOverlay();
#endif

	switch (m_eType)
	{
		case uPCM16BitSignedInt:
		{
			((PCMS16Decoder*)this)->PCMS16Decoder::SetPosition(uPosition);
		}
		break;

		case uADPCM4Bit:
		{
			((ADPCM4bitDecoder*)this)->ADPCM4bitDecoder::SetPosition(uPosition);
		}
		break;

#ifdef PLATFORM_XENON
		case uXMA16Bit:
		case uXMA216Bit:
		{
			((XMADecoder*)this)->XMADecoder::SetPosition(uPosition);
		}
		break;
#endif

		default:
			Assert(0 && "Unknown audio decoder type.");
		break;
	}

#ifdef PLATFORM_SPU
	SwitchBackToOriginalOverlay();
#endif
}


U32 Decoder::Length()
{
#ifdef PLATFORM_SPU
	SwitchInGeneratorOverlay();
#endif

	U32 u32RetValue = 0;

	switch (m_eType)
	{
		case uPCM16BitSignedInt:
		{
			if (m_u32NumChannels == 1)
				u32RetValue = ((MonoPCMS16Decoder*)this)->MonoPCMS16Decoder::Length();
			else if (m_u32NumChannels == 2)
				u32RetValue = ((StereoPCMS16Decoder*)this)->StereoPCMS16Decoder::Length();
		}
		break;

		case uADPCM4Bit:
		{
			if (m_u32NumChannels == 1)
				u32RetValue = ((MonoADPCM4bitDecoder*)this)->MonoADPCM4bitDecoder::Length();
			else if (m_u32NumChannels == 2)
				u32RetValue = ((StereoADPCM4bitDecoder*)this)->StereoADPCM4bitDecoder::Length();
		}
		break;

#ifdef PLATFORM_XENON
		case uXMA16Bit:
		case uXMA216Bit:
		{
			u32RetValue = ((XMADecoder*)this)->XMADecoder::Length();
		}
		break;
#endif

		default:
			Assert(0 && "Unknown audio decoder type.");
		break;
	}

#ifdef PLATFORM_SPU
	SwitchBackToOriginalOverlay();
#endif

	return u32RetValue;
}


/*
#ifdef PLATFORM_SPU
void Decoder::PrepareDmaTransfer()
{
	switch (m_eType)
	{
		case uPCM16BitSignedInt:
		{
			((PCMS16Decoder*)this)->PCMS16Decoder::PrepareDmaTransfer();
		}
		break;

		case uADPCM4Bit:
		{
			((ADPCM4bitDecoder*)this)->ADPCM4bitDecoder::PrepareDmaTransfer();
		}
		break;

		default:
			Assert(0 && "Unknown audio decoder type.");
		break;
	}
}


void Decoder::ReleaseDmaTransfer()
{
	switch (m_eType)
	{
		case uPCM16BitSignedInt:
		{
			((PCMS16Decoder*)this)->PCMS16Decoder::ReleaseDmaTransfer();
		}
		break;

		case uADPCM4Bit:
		{
			((ADPCM4bitDecoder*)this)->ADPCM4bitDecoder::ReleaseDmaTransfer();
		}
		break;

		default:
			Assert(0 && "Unknown audio decoder type.");
		break;
	}
}
#endif
*/

#endif	//	VF_DECODERS

void Decoder::Prime(const DecoderDataInfo& info)
{
	switch (m_eType)
	{
		case uPCM16BitSignedInt:
		{
			((PCMS16Decoder*)this)->PCMS16Decoder::SetPlaying(info, true);
		}
		break;

		case uADPCM4Bit:
		{
			((ADPCM4bitDecoder*)this)->ADPCM4bitDecoder::SetPlaying(info, true);
		}
		break;

#ifdef PLATFORM_XENON
		case uXMA16Bit:
		case uXMA216Bit:
		{
			((XMADecoder*)this)->XMADecoder::SetPlaying(info, true);
		}
		break;
#endif

		default:
			Assert(0 && "Unknown audio decoder type.");
		break;
	}
}


void Decoder::Flush(const DecoderDataInfo& info)
{
	switch (m_eType)
	{
		case uPCM16BitSignedInt:
		{
			((PCMS16Decoder*)this)->PCMS16Decoder::SetPlaying(info, false);
		}
		break;

		case uADPCM4Bit:
		{
			((ADPCM4bitDecoder*)this)->ADPCM4bitDecoder::SetPlaying(info, false);
		}
		break;

#ifdef PLATFORM_XENON
		case uXMA16Bit:
		case uXMA216Bit:
		{
			((XMADecoder*)this)->XMADecoder::SetPlaying(info, false);
		}
		break;
#endif

		default:
			Assert(0 && "Unknown audio decoder type.");
		break;
	}
}
