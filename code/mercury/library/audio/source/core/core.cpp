#include "AudioPch.h"
#include "coreheaders.h"

#include <system/scheduler/overlays.h>

#ifndef PLATFORM_SPU
// shouldn't really be including system headers here, but we need it for the adpcm buffer memory allocs..
#include "../systemheaders.h"
#else
#include "SystemHeadersSPU.h"
#endif


#define AUDIO_CORE_DATA_TYPE        F32
#define AUDIO_CORE_SAMPLE_RATE      48000
#define AUDIO_CORE_FRAME_LENGTH     256
#define AUDIO_CORE_STRIDE           AUDIO_CORE_FRAME_LENGTH
#define AUDIO_CORE_MIXER_INPUTS     16
#define AUDIO_CORE_NUM_DSP          16

bool Audio::g_bUseAltConnectMode = true;
bool Audio::g_bQuitAllTasks = false;

#ifdef PLATFORM_SPU
U32 g_u32CurrentCodeOverlayId = (U32)-1;
//void* g_pOverlayTable = NULL;
#endif

#ifndef PLATFORM_SPU
Audio::Core::Renderer* g_pRenderer = NULL;
#endif



namespace Audio
{
	namespace Core
	{
        const F32 fSampleRate = (F32)uSampleRate;
        const F32 fNyquist = (F32)( uSampleRate >> 1 );
        const F32 fFrameLength = (F32)uFrameLength;
		const F32 fRcpFrameLength = 1.f / F32( uFrameLength );
		const F32 fRcpSampleRate = 1.f / F32( uSampleRate );

#if defined(PLATFORM_PC)
		// [Blur2PC] T144: retail 0x01028c3c and its setters 0x0053f660 / 0x0053f6d0 (see audio_core.h)
		F32 fOutputRateRatio = 1.f;

		void ResetOutputSampleRate()
		{
			fOutputRateRatio = 1.f;
		}

		void SetOutputSampleRate(U32 uOutputSampleRate)
		{
			fOutputRateRatio = fSampleRate / (F32)uOutputSampleRate;
		}
#endif
	} // namespace Core
} // namespace Audio



U32 Audio::Core::PCMS16Decoder::m_uTotalPCM = 0;
U32 Audio::Core::PCMS16Decoder::m_uActivePCM = 0;
U32 Audio::Core::ADPCM4bitDecoder::m_uTotalADPCM = 0;
U32 Audio::Core::ADPCM4bitDecoder::m_uActiveADPCM = 0;

#if defined (PLATFORM_XENON)
U32 Audio::Core::XMADecoder::m_uTotalXMA = 0;
U32 Audio::Core::XMADecoder::m_uActiveXMA = 0;
U32 Audio::Core::XMADecoder::m_uNumDoomedContexts = 0;

Audio::Core::XMADoomedPlayback* Audio::Core::XMADecoder::m_XMADoomedPlaybackContexts = NULL;

U32 Audio::Core::XMADecoder::m_DoomedIndex = 0;
#endif

#ifdef AUDIO_OPTIMIZE_OFF
#pragma optimize ("", off) // AUDIO_OPTIMIZE_OFF
#endif	//	AUDIO_OPTIMIZE_OFF


#if defined( __INTEL_COMPILER )

template < U32 uEffects, U32 uOutputs >
void* Audio::Core::Generator< uEffects, uOutputs >::operator new( size_t sz )
{
	//return new U8[sz];
	return malloc( sz );
};

// TODO: FIND WORKAROUND FOR INTERNAL COMPILER ERROR WITH INTEL COMPILER!
template < U32 uEffects, U32 uOutputs >
void Audio::Core::Generator< uEffects, uOutputs >::operator delete( void* ptr )
{
	//delete ptr;
	free( ptr );
};

template < U32 uEffects, U32 uOutputs >
void* Audio::Core::Generator< uEffects, uOutputs >::operator new( size_t sz, void* pHere )
{
	return pHere;
};

template < U32 uEffects, U32 uOutputs >
void Audio::Core::Generator< uEffects, uOutputs >::operator delete( void* ptr, void* pHere )
{
};

#endif

////////////////////////////////////////////////////////////////////////////////
//	CreateRenderer
////////////////////////////////////////////////////////////////////////////////
#ifndef PLATFORM_SPU
Audio::Core::Renderer* Audio::CreateRenderer()
{
    if (g_pRenderer == NULL)
        g_pRenderer = new Core::Renderer(8);

	return g_pRenderer;
}
#endif


#ifdef PLATFORM_SPU

bool bDebugOverlayInfo = false;
bool bOverlayBreak = false;

bool CoreLoadCodeOverlay(U32 u32OverlayId)
{
return true;	// temp, overlays possibly not required anymore
	if (g_u32CurrentCodeOverlayId == u32OverlayId)
		return true;

	U32 u32RetValue = 0;

	g_u32CurrentCodeOverlayId = u32OverlayId;

	//if (! g_pOverlayTable)
	//	DebugBreak();

	switch (u32OverlayId)
	{
#ifdef AUDIO_GENERATORS_NOT_OVERLAID
		case 1:
		{
			u32RetValue = cellOvisStartMapping_CodeOverlay1((uint64_t)g_pOverlayTable, 20);
			cellOvisWaitMapping_CodeOverlay1(20);
		}
		break;

		case 2:
		{
			u32RetValue = cellOvisStartMapping_CodeOverlay2((uint64_t)g_pOverlayTable, 20);
			cellOvisWaitMapping_CodeOverlay2(20);
		}
		break;

		case 3:
		{
			u32RetValue = cellOvisStartMapping_CodeOverlay3((uint64_t)g_pOverlayTable, 20);
			cellOvisWaitMapping_CodeOverlay3(20);
		}
		break;

		case 4:
		{
			u32RetValue = cellOvisStartMapping_CodeOverlay4((uint64_t)g_pOverlayTable, 20);
			cellOvisWaitMapping_CodeOverlay4(20);
		}
		break;

		case 5:
		{
			u32RetValue = cellOvisStartMapping_CodeOverlay5((uint64_t)g_pOverlayTable, 20);
			cellOvisWaitMapping_CodeOverlay5(20);
		}
		break;

		case 6:
		{
			u32RetValue = cellOvisStartMapping_CodeOverlay6((uint64_t)g_pOverlayTable, 20);
			cellOvisWaitMapping_CodeOverlay6(20);
		}
		break;

#ifndef AUDIO_SPU_MINIMAL
		case 7:
		{
			u32RetValue = cellOvisStartMapping_CodeOverlay7((uint64_t)g_pOverlayTable, 20);
			cellOvisWaitMapping_CodeOverlay7(20);
		}
		break;

		case 8:
		{
			u32RetValue = cellOvisStartMapping_CodeOverlay8((uint64_t)g_pOverlayTable, 20);
			cellOvisWaitMapping_CodeOverlay8(20);
		}
		break;

		case 9:
		{
			u32RetValue = cellOvisStartMapping_CodeOverlay9((uint64_t)g_pOverlayTable, 20);
			cellOvisWaitMapping_CodeOverlay9(20);
		}
		break;

		case 10:
		{
			u32RetValue = cellOvisStartMapping_CodeOverlay10((uint64_t)g_pOverlayTable, 20);
			cellOvisWaitMapping_CodeOverlay10(20);
		}
		break;

		case 11:
		{
			u32RetValue = cellOvisStartMapping_CodeOverlay11((uint64_t)g_pOverlayTable, 20);
			cellOvisWaitMapping_CodeOverlay11(20);
		}
		break;

		case 12:
		{
			u32RetValue = cellOvisStartMapping_CodeOverlay12((uint64_t)g_pOverlayTable, 20);
			cellOvisWaitMapping_CodeOverlay12(20);
		}
		break;
#endif
#else
		case 1:
		{
			//if (bDebugOverlayInfo)
			//	spu_printf("loading overlay 1\n");
			if (bOverlayBreak)
				DebugBreak();
			Scheduler::SelectOverlay(AUDIO_COMPONENT_OVERLAY_ID);
		}
		break;

		case 2:
		{
			//if (bDebugOverlayInfo)
			//	spu_printf("loading overlay 2\n");
			if (bOverlayBreak)
				DebugBreak();
			Scheduler::SelectOverlay(AUDIO_GENERATOR_OVERLAY_ID);
		}
		break;

#endif

		default:
		{
			u32RetValue = 1;
			DebugBreak();
		}
		break;
	}

	if (u32RetValue != 0)
	{
//		spu_printf("overlap error- error code is 0x%x, overlay table location is 0x%x\n", u32RetValue, g_pOverlayTable);
		DebugBreak();
		//Audio::Assert(0);
		return false;
	}
	else
		return true;
}
#endif // PLATFORM_SPU

////////////////////////////////////////////////////////////////////////////////
//	FFT
// Sorensen in-place split-radix FFT for real values
// data: array of F32s:
// re(0),re(1),re(2),...,re(size-1)
// 
// output:
// re(0),re(1),re(2),...,re(size/2),im(size/2-1),...,im(1)
// normalized by array length
//
// Source: 
// Sorensen et al: Real-Valued Fast Fourier Transform Algorithms,
// IEEE Trans. ASSP, ASSP-35, No. 6, June 1987
////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

void Audio::Core::DSP::RealFFT_Split(F32 *data,long n){

#ifndef AUDIO_DISABLE_DSP

	const F32	PI		= 3.1415926535897932384626433832795;

  long i,j,k,i5,i6,i7,i8,i0,id,i1,i2,i3,i4,n2,n4,n8;
  F32 t1,t2,t3,t4,t5,t6,a3,ss1,ss3,cc1,cc3,a,e,sqrt2;

  const F32 fRoot2          = 1.41421356237f;
  
  sqrt2=fRoot2;
  n4=n-1;
  
  //data shuffling
	  for (i=0,j=0,n2=n/2; i<n4 ; i++){
	  if (i<j){
				t1=data[j];
				data[j]=data[i];
				data[i]=t1;
				}
	  k=n2;
	  while (k<=j){
				j-=k;
				k>>=1;	
				}
	  j+=k;
	  }
	
/*----------------------*/
	
	//length two butterflies	
	i0=0;
	id=4;
   do{
	   for (; i0<n4; i0+=id){ 
			i1=i0+1;
			t1=data[i0];
			data[i0]=t1+data[i1];
			data[i1]=t1-data[i1];
		}
	   id<<=1;
	   i0=id-2;
	   id<<=1;
	} while ( i0<n4 );

   /*----------------------*/
   //L shaped butterflies
n2=2;
for(k=n;k>2;k>>=1){  
	n2<<=1;
	n4=n2>>2;
	n8=n2>>3;
	e = 2*PI/(n2);
	i1=0;
	id=n2<<1;
	do{ 
		for (; i1<n; i1+=id){
			i2=i1+n4;
			i3=i2+n4;
			i4=i3+n4;
			t1=data[i4]+data[i3];
			data[i4]-=data[i3];
			data[i3]=data[i1]-t1;
			data[i1]+=t1;
			if (n4!=1){
				i0=i1+n8;
				i2+=n8;
				i3+=n8;
				i4+=n8;
				t1=(data[i3]+data[i4])/sqrt2;
				t2=(data[i3]-data[i4])/sqrt2;
				data[i4]=data[i2]-t1;
				data[i3]=-data[i2]-t1;
				data[i2]=data[i0]-t2;
				data[i0]+=t2;
			}
		 }
		 id<<=1;
		 i1=id-n2;
		 id<<=1;
	  } while ( i1<n );
	a=e;
	for (j=2; j<=n8; j++){  
		  a3=3*a;
		  cc1=cos(a);
		  ss1=sin(a);
		  cc3=cos(a3);
		  ss3=sin(a3);
		  a=j*e;
		  i=0;
		  id=n2<<1;
		  do{
		   for (; i<n; i+=id){  
			  i1=i+j-1;
			  i2=i1+n4;
			  i3=i2+n4;
			  i4=i3+n4;
			  i5=i+n4-j+1;
			  i6=i5+n4;
			  i7=i6+n4;
			  i8=i7+n4;
			  t1=data[i3]*cc1+data[i7]*ss1;
			  t2=data[i7]*cc1-data[i3]*ss1;
			  t3=data[i4]*cc3+data[i8]*ss3;
			  t4=data[i8]*cc3-data[i4]*ss3;
			  t5=t1+t3;
			  t6=t2+t4;
			  t3=t1-t3;
			  t4=t2-t4;
			  t2=data[i6]+t6;
			  data[i3]=t6-data[i6];
			  data[i8]=t2;
			  t2=data[i2]-t3;
			  data[i7]=-data[i2]-t3;
			  data[i4]=t2;
			  t1=data[i1]+t5;
			  data[i6]=data[i1]-t5;
			  data[i1]=t1;
			  t1=data[i5]+t4;
			  data[i5]-=t4;
			  data[i2]=t1;
			 }
		   id<<=1;
		   i=id-n2;
		   id<<=1;
		 } while(i<n);
	   }
	  }

	//division with array length
 //  for(i=0;i<n;i++) data[i]/=n;

#endif	//	#ifndef AUDIO_DISABLE_DSP
}

/////////////////////////////////////////////////////////
// Sorensen in-place inverse split-radix FFT for real values
// data: array of F32s:
// re(0),re(1),re(2),...,re(size/2),im(size/2-1),...,im(1)
//
// output:
// re(0),re(1),re(2),...,re(size-1)
// NOT normalized by array length
//
// Source:
// Sorensen et al: Real-Valued Fast Fourier Transform Algorithms,
// IEEE Trans. ASSP, ASSP-35, No. 6, June 1987
void Audio::Core::DSP::IRealFFT_Split(F32 *data,long n)
{

	#ifndef AUDIO_DISABLE_DSP


	const F32	pi		= 3.1415926535897932384626433832795;

	long i,j,k,i5,i6,i7,i8,i0,id,i1,i2,i3,i4,n2,n4,n8,n1;
	F32 t1,t2,t3,t4,t5,a3,ss1,ss3,cc1,cc3,a,e,sqrt2;
	sqrt2=sqrt(2.f);
	n1=n-1;
	n2=n<<1;
	for(k=n;k>2;k>>=1){
	id=n2;
	n2>>=1;
	n4=n2>>2;
	n8=n2>>3;
	e = 2*pi/(n2);
	i1=0;
	
	do
	{
		for (; i1<n; i1+=id)
		{
			i2=i1+n4;
			i3=i2+n4;
			i4=i3+n4;
			t1=data[i1]-data[i3];
			data[i1]+=data[i3];
			data[i2]*=2;
			data[i3]=t1-2*data[i4];
			data[i4]=t1+2*data[i4];
			
				if (n4!=1)
				{
					i0=i1+n8;
					i2+=n8;
					i3+=n8;
					i4+=n8;
					t1=(data[i2]-data[i0])/sqrt2;
					t2=(data[i4]+data[i3])/sqrt2;
					data[i0]+=data[i2];
					data[i2]=data[i4]-data[i3];
					data[i3]=2*(-t2-t1);
					data[i4]=2*(-t2+t1);
				}
		}

			id<<=1;
			i1=id-n2;
			id<<=1;
	} while ( i1<n1 );


			a=e;

			for (j=2; j<=n8; j++)
			{

				a3=3*a;
				cc1=cos(a);
				ss1=sin(a);
				cc3=cos(a3);
				ss3=sin(a3);
				a=j*e;
				i=0;
				id=n2<<1;
			
				do
				{
					for (; i<n; i+=id)
					{
							i1=i+j-1;
							i2=i1+n4;
							i3=i2+n4;
							i4=i3+n4;
							i5=i+n4-j+1;
							i6=i5+n4;
							i7=i6+n4;
							i8=i7+n4;
							t1=data[i1]-data[i6];
							data[i1]+=data[i6];
							t2=data[i5]-data[i2];
							data[i5]+=data[i2];
							t3=data[i8]+data[i3];
							data[i6]=data[i8]-data[i3];
							t4=data[i4]+data[i7];
							data[i2]=data[i4]-data[i7];
							t5=t1-t4;
							t1+=t4;
							t4=t2-t3;
							t2+=t3;
							data[i3]=t5*cc1+t4*ss1;
							data[i7]=-t4*cc1+t5*ss1;
							data[i4]=t1*cc3-t2*ss3;
							data[i8]=t2*cc3+t1*ss3;
					}

					id<<=1;
					i=id-n2;
					id<<=1;

				} while(i<n1);
			}
		}
			/*----------------------*/
			i0=0;
			id=4;
			do
			{
			for (; i0<n1; i0+=id)
			{
			i1=i0+1;
			t1=data[i0];
			data[i0]=t1+data[i1];
			data[i1]=t1-data[i1];
		}
		id<<=1;
		i0=id-2;
		id<<=1;
	} while ( i0<n1 );
	/*----------------------*/
	//data shuffling
	for (i=0,j=0,n2=n/2; i<n1 ; i++)
	{
		if (i<j){
		t1=data[j];
		data[j]=data[i];
		data[i]=t1;
	}

	k=n2;	

	while (k<=j)
	{
		j-=k;
		k>>=1;
		}
		j+=k;
	}

		//division with array length
   for(i=0;i<n;i++) data[i]/=n;

#endif //disable DSP
}

//	Bring in ATG FFT

#ifdef _XBOX360

using namespace ATG;

#define _USE_MATH_DEFINES
#include <math.h> // definition of PI
#include <complex>

unsigned int reverse4( unsigned int n, unsigned int size )
{
    n = ( ( n & 0xcccccccc ) >> 2 ) | ( ( n & 0x33333333 ) << 2 );
    n = ( ( n & 0xf0f0f0f0 ) >> 4 ) | ( ( n & 0x0f0f0f0f ) << 4 );
    n = ( ( n & 0xff00ff00 ) >> 8 ) | ( ( n & 0x00ff00ff ) << 8 );
    n = ( ( n & 0xffff0000 ) >> 16 ) | ( ( n & 0x0000ffff ) << 16 );

    n = n >> ( 32 - size );
    return n;
}

unsigned int reverse2( unsigned int n, unsigned int size )
{
    n = ( ( n & 0xaaaaaaaa ) >> 1 ) | ( ( n & 0x55555555 ) << 1 );
    n = ( ( n & 0xcccccccc ) >> 2 ) | ( ( n & 0x33333333 ) << 2 );
    n = ( ( n & 0xf0f0f0f0 ) >> 4 ) | ( ( n & 0x0f0f0f0f ) << 4 );
    n = ( ( n & 0xff00ff00 ) >> 8 ) | ( ( n & 0x00ff00ff ) << 8 );
    n = ( ( n & 0xffff0000 ) >> 16 ) | ( ( n & 0x0000ffff ) << 16 );

    n = n >> ( 32 - size );
    return n;
}

void FFT_Unoptimized( float* pReal, float* pImaginary, float sign, DWORD countLog2, DWORD passes )
{
    if( passes == 0 ) passes = countLog2;
    DWORD count = 1 << countLog2;
    std::complex<float>* pData = new std::complex<float>[count];
    std::complex<float>* pTwiddles = new std::complex<float>[count];

    for( DWORD i = 0; i < count; ++i )
    {
        pTwiddles[i] = std::exp( sign * 2.0f * (float)M_PI * std::complex<float>(0.0f,1.0f) * (float)i/ (float)count );
    }

    for( DWORD i = 0; i < count; ++i )
    {
        pData[i] = std::complex<float>( pReal[i], pImaginary[i] );
    }

    for( DWORD i = 0; i < passes; ++i )
    {
        for( DWORD j = 0; j < count; j += count >> i )
        {
            DWORD len = (count/2)>>i;
            for( DWORD k = 0; k < len; ++k )
            {

                std::complex<float> a = pData[j + k];
                std::complex<float> b = pData[j + len + k];

                pData[j+k] = a + b;
                pData[j + len + k] = (a - b) * pTwiddles[k*(1<<i)];
            }
        }
    }
  
    for( DWORD i = 0; i < count; ++i )
    {
        pReal[i] = pData[reverse2(i,countLog2)].real();
        pImaginary[i] = pData[reverse2(i,countLog2)].imag();
    }
    delete[] pTwiddles;
    delete[] pData;
}


template< int length >
struct RootsOfUnity
{
    const static int width = length / 4;
    __vector4 real[width];
    __vector4 imag[width];

    RootsOfUnity()
    {
        float step = 2.0f * (float)M_PI / (float)length;
        for( int i = 0; i < 4; ++i )
        {
            for( int j = 0; j < width; ++j )
            {
                int index = ( i * width ) + j;
                real[index/4].v[index%4] = cos( (float)i * (float)j * step );
                imag[index/4].v[index%4] = -sin( (float)i * (float)j * step );
            }
        }
    }
};




//
// Parallel multiplication of four complex numbers, assuming that
// real and complex values are stored in separate vectors
//
void vmulComplex4( vector4_out r1, vector4_out i1, vector4_in r2, vector4_in i2 )
{
    // (r1,i1) * (r2,i2) = (r1r2 - i1i2, r1i2 + r2i1 )
    __vector4 rTemp = __vnmsubfp( i1, i2, __vmulfp( r1, r2 ) );
    i1 = __vmaddfp( r1, i2, __vmulfp( r2, i1 ) );
    r1 = rTemp;
}

void vmulComplex4( vector4_out rOut, vector4_out iOut, vector4_in r1, vector4_in i1, vector4_in r2, vector4_in i2 )
{
    // (r1,i1) * (r2,i2) = (r1r2 - i1i2, r1i2 + r2i1 )
    rOut = __vnmsubfp( i1, i2, __vmulfp( r1, r2 ) );
    iOut = __vmaddfp( r1, i2, __vmulfp( r2, i1 ) );
}


//
// Sign and permutation constants for radix-4 butterflies
//
const static __vector4 v4DFT4SignBits1 = { 0.0f, -0.0f, 0.0f, -0.0f };
const static __vector4 v4DFT4SignBits2 = { 0.0f, 0.0f, -0.0f, -0.0f };
const static __vector4 v4DFT4SignBits3 = { 0.0f, -0.0f, -0.0f, 0.0f };
const static ULONG __declspec(align(16)) ulPermute_Z1W2Z1W2[] = { 0x08090a0b, 0x1c1d1e1f, 0x08090a0b, 0x1c1d1e1f };
const static __vector4 v4Permute_Z1W2Z1W2 = *(__vector4*)ulPermute_Z1W2Z1W2;


void __forceinline ButterflyDIT2_4( vector4_out rData1, vector4_out rData2, vector4_out iData1, vector4_out iData2 )
{
    const __vector4 r = { 1, .707168f, 0, -.707168f };
    const __vector4 i = { 0, .707168f, 1, .707168f };
    vmulComplex4( rData2, iData2, r, i );

    rData1 = __vaddfp( rData1, rData2 );
    rData2 = __vsubfp( rData1, rData2 );
    iData1 = __vaddfp( iData1, iData2 );
    iData2 = __vsubfp( iData1, iData2 );
}

void __forceinline ButterflyDIF2_4( vector4_out rData1, vector4_out rData2, vector4_out iData1, vector4_out iData2 )
{
    const __vector4 r = { 1, .707168f, 0, -.707168f };
    const __vector4 i = { 0, .707168f, 1, .707168f };
    __vector4 r1 = __vaddfp( rData1, rData2 );
    __vector4 i1 = __vaddfp( iData1, iData2 );

    __vector4 r2 = __vsubfp( rData1, rData2 );
    __vector4 i2 = __vsubfp( iData1, iData2 );
    vmulComplex4( r2, i2, r, i );

    rData1 = r1;
    rData2 = r2;
    iData1 = i1;
    iData2 = i2;
}

//
// Radix-4 decimation-in-time FFT butterfly
//
// This version assumes that all four elements of the butterfly are adjacent
// in a single __vector4.
//
void __forceinline ButterflyDIT4_1( vector4_out rData, vector4_out iData)
{
    /*  Compute the product of the complex input vector and the
    4-element DFT matrix:

    |  1  1  1  1  |    | (Xr,Xi) |      
    |  1 -j -1  j  |    | (Yr,Yi) |
    |  1 -1  1 -1  |    | (Zr,Zi) |
    |  1  j -1 -j  |    | (Wr,Wi) |

    This matrix can be decomposed into two simpler ones to reduce the number
    of additions needed. The decomposed matrices look like this:

    |  1  0  1  0  |    |  1  0  1  0  | 
    |  0  1  0 -j  |    |  1  0 -1  0  |
    |  1  0 -1  0  |    |  0  1  0  1  |
    |  0  1  0  j  |    |  0  1  0 -1  |

    Combine as follows:

            |  1  0  1  0  |    | (rInX,iInX) |          | (rInX + rInZ, iInX + iInZ) |
    Out =   |  1  0 -1  0  | *  | (rInY,iInY) |       =  | (rInX - rInZ, iInX - iInZ) |
            |  0  1  0  1  |    | (rInZ,iInZ) |          | (rInY + rInW, iInY + iInW) |
            |  0  1  0 -1  |    | (rInW,iInW) |          | (rInY - rInW, iInY - iInW) |

            |  1  0  1  0  |    | (rOutX,iOutX) |        | (rOutX + rOutZ, iOutX + iOutZ) |
    Out =   |  0  1  0 -j  | *  | (rOutY,iOutY) |     =  | (rOutY + iOutW, iOutY - rOutW) |
            |  1  0 -1  0  |    | (rOutZ,iOutZ) |        | (rOutX - rOutZ, iOutX - iOutZ) |
            |  0  1  0  j  |    | (rOutW,iOutW) |        | (rOutY - iOutW, iOutY + rOutW) |
    */

    // First matrix
    //
    __vector4 rTemp = __vaddfp( 
        __vpermwi( rData, VPERMWI_CONST( 0, 0, 1, 1 ) ),                              // X,X,Y,Y
        __vxor( __vpermwi( rData, VPERMWI_CONST( 2, 2, 3, 3 ) ), v4DFT4SignBits1 ) );   // Z,-Z,W,-W 

    __vector4 iTemp = __vaddfp(
        __vpermwi( iData, VPERMWI_CONST( 0, 0, 1, 1 ) ),      // X,X,Y,Y
        __vxor( __vpermwi( iData, VPERMWI_CONST( 2, 2, 3, 3 ) ), v4DFT4SignBits1 ) );    // Z,Z,W,W 

    // Second
    //
    __vector4 rZiWrZiW = __vperm( rTemp, iTemp, v4Permute_Z1W2Z1W2 );
    __vector4 iZrWiZrW = __vperm( iTemp, rTemp, v4Permute_Z1W2Z1W2 );
    rData = __vaddfp(
        __vpermwi( rTemp, VPERMWI_CONST( 0, 1, 0, 1 ) ),                 // X,Y,X,Y
        __vxor( rZiWrZiW, v4DFT4SignBits2 ) );                                

    iData = __vaddfp( 
        __vpermwi( iTemp, VPERMWI_CONST( 0, 1, 0, 1 ) ),                 // X,Y,X,Y
        __vxor( iZrWiZrW, v4DFT4SignBits3 ) );                               
}



//
// Radix-4 decimation-in-time FFT butterfly
//
// This version assumes that the elements of the butterfly are
// in different __vector4s, so that each __vector4 in the input
// contains elements from four different butterflies. The four
// separate butterflies are processed in parallel.
//
template<int stride, bool last>
void __forceinline ButterflyDIT4_4( 
                             vector4_out rData0, 
                             vector4_out rData1, 
                             vector4_out rData2, 
                             vector4_out rData3, 
                             vector4_out iData0, 
                             vector4_out iData1, 
                             vector4_out iData2, 
                             vector4_out iData3, 
                            const __vector4* __restrict rW,
                            const __vector4* __restrict iW) 
{
    /*
    The calculations here are the same as the ones in the single-vector radix-4 DFT, 
    but instead of being done on a single vector (X,Y,Z,W) they're done in parallel on
    sixteen independent complex values. There is no interdependence between the vector
    elements.

    |  1  0  1  0  |    | (rIn0,iIn0) |                         | (rIn0 + rIn2, iIn0 + iIn2) |
    |  1  0 -1  0  | *  | (rIn1,iIn1) |       = (rOut,iOut) =   | (rIn0 - rIn2, iIn0 - iIn2) |
    |  0  1  0  1  |    | (rIn2,iIn2) |                         | (rIn1 + rIn3, iIn1 + iIn3) |
    |  0  1  0 -1  |    | (rIn3,iIn3) |                         | (rIn1 - rIn3, iIn1 - iIn3) |

            |  1  0  1  0  |    | (rOut0,iOut0) |        | (rOut0 + rOut2, iOut0 + iOut2) |
    Out =   |  0  1  0 -j  | *  | (rOut1,iOut1) |     =  | (rOut1 + iOut3, iOut1 - rOut3) |
            |  1  0 -1  0  |    | (rOut2,iOut2) |        | (rOut0 - rOut2, iOut0 - iOut2) |
            |  0  1  0  j  |    | (rOut3,iOut3) |        | (rOut1 - iOut3, iOut1 + rOut3) |
    */
    __vector4 rTemp0, rTemp1, rTemp2, rTemp3;
    __vector4 rTemp4, rTemp5, rTemp6, rTemp7;
    __vector4 iTemp0, iTemp1, iTemp2, iTemp3;
    __vector4 iTemp4, iTemp5, iTemp6, iTemp7;
    // first matrix
    rTemp0 = __vaddfp( rData0, rData2 );   iTemp0 = __vaddfp( iData0, iData2 );

    rTemp1 = __vsubfp( rData0, rData2 );   iTemp1 = __vsubfp( iData0, iData2 );

    rTemp2 = __vaddfp( rData1, rData3 );   iTemp2 = __vaddfp( iData1, iData3 );

    rTemp3 = __vsubfp( rData1, rData3 );   iTemp3 = __vsubfp( iData1, iData3 );

    // second matrix
    rTemp4 = __vaddfp( rTemp0, rTemp2 );     iTemp4 = __vaddfp( iTemp0, iTemp2 );
                                             
    rTemp5 = __vaddfp( rTemp1, iTemp3 );     iTemp5 = __vsubfp( iTemp1, rTemp3 );
                                             
    rTemp6 = __vsubfp( rTemp0, rTemp2 );     iTemp6 = __vsubfp( iTemp0, iTemp2 );
                                             
    rTemp7 = __vsubfp( rTemp1, iTemp3 );     iTemp7 = __vaddfp( iTemp1, rTemp3 );

    // Do the twiddle-factor multiplication
    // vmulComplex4( rOut0, iOut0, rOut0, iOut0, rW0, iW0 ); // first one is always trivial
    vmulComplex4( rTemp5, iTemp5, rW[1*stride], iW[1*stride] );
    vmulComplex4( rTemp6, iTemp6, rW[2*stride], iW[2*stride] );
    vmulComplex4( rTemp7, iTemp7, rW[3*stride], iW[3*stride] );

    if( last )
    {
        ButterflyDIT4_1( rTemp4, iTemp4 );
        ButterflyDIT4_1( rTemp5, iTemp5 );
        ButterflyDIT4_1( rTemp6, iTemp6 );
        ButterflyDIT4_1( rTemp7, iTemp7 );
    }

    rData0 = rTemp4;
    rData1 = rTemp5;
    rData2 = rTemp6;
    rData3 = rTemp7;

    iData0 = iTemp4;
    iData1 = iTemp5;
    iData2 = iTemp6;
    iData3 = iTemp7;

}

template< int stage_len >
struct FFT_DIT4
{
    template<int count>
    static void __forceinline eval( __vector4* __restrict rData, __vector4* __restrict iData )
    {
        const int total = count * stage_len;
        const int total_vectors = total / 4;
        const int stage_vectors = stage_len/4;
        const int stride = stage_vectors / 4; // stride between butterfly elements
        const int skip = stage_vectors - stride;

        // butterfly
        int n = 0;
        for( int i = 0; i < total_vectors / 4; ++i )
        {
            n = ( i/stride ) * ( stride + skip ) + ( i % stride );
            ButterflyDIT4_4<stride,false>( 
                rData[ n ],
                rData[ n + stride ],
                rData[ n + stride * 2 ],
                rData[ n + stride * 3 ],
                iData[ n ],
                iData[ n + stride * 1 ],
                iData[ n + stride * 2 ],
                iData[ n + stride * 3 ],
                roots.real + n % stage_vectors,
                roots.imag + n % stage_vectors);


        }
        FFT_DIT4< stage_len / 4>::eval<count * 4 >( rData, iData );
    }
    static RootsOfUnity<stage_len> roots;

};
template< int stage_len >
RootsOfUnity<stage_len> FFT_DIT4<stage_len>::roots;

template< >
struct FFT_DIT4<4>
{
    template<int nsubs>
    static void __forceinline eval( __vector4* __restrict rData, __vector4* __restrict iData )
    {
        for( int i = 0; i < nsubs; ++i )
        {
            ButterflyDIT4_1( rData[i], iData[i] );
        }
    }
};

template<>
struct FFT_DIT4<8>
{
    template<int nsubs>
    static void __forceinline eval( __vector4* __restrict rData, __vector4* __restrict iData )
    {
        for( int j = 0; j < nsubs; ++j )
        {
            __vector4* pReal = rData + j *2;
            __vector4* pImag = iData + j *2;

            __vector4 oddsR = { pReal[0].v[1], pReal[0].v[3], pReal[1].v[1], pReal[1].v[3] };
            __vector4 evensR = { pReal[0].v[0], pReal[0].v[2], pReal[1].v[0], pReal[1].v[2] };
            __vector4 oddsI = { pImag[0].v[1], pImag[0].v[3], pImag[1].v[1], pImag[1].v[3] };
            __vector4 evensI = { pImag[0].v[0], pImag[0].v[2], pImag[1].v[0], pImag[1].v[2] };

            ButterflyDIT4_1( oddsR, oddsI );
            ButterflyDIT4_1( evensR, evensI );

            const __vector4 wr1 = { 1, .707168f, 0, -.707168f };
            const __vector4 wi1 = { 0, -.707168f, -1, -.707168f };
            const __vector4 wr2 = { -1, -.707168f, 0, .707168f };
            const __vector4 wi2 = { 0, .707168f, 1, .707168f };

            __vector4 r, i;
            vmulComplex4( r, i, oddsR, oddsI, wr1, wi1 );
            pReal[0] = __vaddfp( evensR, r );
            pImag[0] = __vaddfp( evensI, i );

            vmulComplex4( r, i, oddsR, oddsI, wr2, wi2 );
            pReal[1] = __vaddfp( evensR, r );
            pImag[1] = __vaddfp( evensI, i );
        }
    }
};


template< >
struct FFT_DIT4<16>
{
    template<int nsubs>
    static void __forceinline eval( __vector4* __restrict rData, __vector4* __restrict iData )
    {
        // butterfly
        for( int i = 0; i < nsubs; ++i )
        {
            ButterflyDIT4_4<1,true>( 
                rData[i * 4],
                rData[ i * 4 + 1 ],
                rData[ i * 4 + 2 ],
                rData[ i * 4 + 3 ],
                iData[ i * 4 ],
                iData[ i * 4 + 1 ],
                iData[ i * 4 + 2 ],
                iData[ i * 4 + 3 ],
                roots.real,
                roots.imag );
        }
    }
    static RootsOfUnity<16> roots;
};
RootsOfUnity<16> FFT_DIT4<16>::roots;


void FFT_8( __vector4* __restrict pReal, __vector4* __restrict pImag )
{
    FFT_DIT4<8>::eval<1>(pReal, pImag);

}
void FFT_16( __vector4* __restrict pReal, __vector4* __restrict pImag )
{
    FFT_DIT4<16>::eval<1>( pReal, pImag );
}

void FFT_32( __vector4* __restrict pReal, __vector4* __restrict pImag )
{
    FFT_DIT4<32>::eval<1>( pReal, pImag );
}

void FFT_64( __vector4* __restrict pReal, __vector4* __restrict pImag )
{
    FFT_DIT4<64>::eval<1>( pReal, pImag );
}

void FFT_128( __vector4* __restrict pReal, __vector4* __restrict pImag )
{
    FFT_DIT4<128>::eval<1>( pReal, pImag );
}

void FFT_256( __vector4* __restrict pReal, __vector4* __restrict pImag )
{
    FFT_DIT4<256>::eval<1>( pReal, pImag );
}

void FFT_512( __vector4* __restrict pReal, __vector4* __restrict pImag )
{
    FFT_DIT4<512>::eval<1>( pReal, pImag );
}

void FFT_1024( __vector4* __restrict pReal, __vector4* __restrict pImag )
{
    FFT_DIT4<1024>::eval<1>( pReal, pImag );
}

void FFT_2048( __vector4* __restrict pReal, __vector4* __restrict pImag )
{
    FFT_DIT4<2048>::eval<1>( pReal, pImag );
}

void FFT_4096( __vector4* __restrict pReal, __vector4* __restrict pImag )
{
    FFT_DIT4<4096>::eval<1>( pReal, pImag );
}

void FFT_Unswizzle( float* __restrict pOut, float* __restrict pIn, int nFloatsLog2 )
{
    int nFloats = 1 << nFloatsLog2;

    if( 0 == ( 1 & nFloatsLog2 ) )
    {
        // Even powers of two.
        for( int index = 0; index < nFloats; ++index )
        {
            int other = reverse4( index, nFloatsLog2 );
            pOut[other] = pIn[index];
        }
    }
    else
    {
        // Odd powers of two.
        for( int index = 0; index < nFloats; ++index )
        {
            int other = reverse4( index >> 3, nFloatsLog2 - 3 );
            other |= ( index & 7 ) << ( nFloatsLog2 - 3 );
            pOut[other] = pIn[index];
        }
    }
}

#endif // _XBOX360


Audio::Core::WaveStreamData* Audio::Core::GetWaveData(WaveServerLocalIndex hWaveData, const void* pWaveServerData)
{
	if (hWaveData.IsValid())
		return &(((WaveStreamData*)((U8*)pWaveServerData + 4))[hWaveData]);
	else
		return 0;
}


Audio::Core::AdpcmWaveInfo* Audio::Core::GetAdpcmBufferData(U32 uAdpcmWaveInfoIndex, const void* pAdpcmBufferData)
{
	AdpcmWaveInfo* pInfo = (AdpcmWaveInfo*)(((U8*)pAdpcmBufferData) + 4);
	return &(pInfo[uAdpcmWaveInfoIndex]);
}


#ifdef AUDIO_OPTIMIZE_OFF
#pragma optimize ("", on) // AUDIO_OPTIMIZE_OFF
#endif	//	AUDIO_OPTIMIZE_OFF


