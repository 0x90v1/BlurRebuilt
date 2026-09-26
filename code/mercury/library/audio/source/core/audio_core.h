#ifndef _CORE_H_
#define _CORE_H_

//#define AUDIO_OPTIMIZE_OFF
//#define AUDIO_DSP_DEBUG

//#pragma warning( disable: 4996 )

// audio spu code is bigger in release builds than in speeds and we're sometimes running out of spu memory..
// so for now, disable dsp in release builds on spu
#if defined(PLATFORM_SPU) && defined(CFG_RELEASE)
#define AUDIO_DISABLE_DSP
#endif

#define MAX_NUM_SURROUND_SPEAKERS 7

#define MAX_STREAMER_FILE_PATH_LENGTH	64

#ifdef PLATFORM_SPU
#define audio_virtual
#else
#define audio_virtual	virtual
#endif

#ifdef PLATFORM_PS3
#include "vec_types.h"
#endif

#include "../common/types.h"
#include "../common/SampleTypes.h"

//#define AUDIO_CHECK_SAMPLES

// with this defined a stringhash will be created for each component.
// components can then be looked up by stringhash instead of by string.
#ifndef PLATFORM_SPU
#define AUDIO_STRING_HASH_COMPONENTS
#ifndef CFG_GOLD
#define AUDIO_STRING_HASH_OBJECT_NAMES
#define DUMP_PROCESSING_STATS
#endif
#endif

#if defined (PLATFORM_XENON)
#define COMPONENTS_USE_ALLOCATOR

#ifndef USE_XAUDIO2
#define USE_XAUDIO2
#endif
#endif

#define AUDIO_CORE_ALIGN_16(x) ((x + 0xf) & ~0xf)

#define AUDIO_MATHS_VECTOR_OPTIMISE_DECODER_XMA
#define AUDIO_MATHS_VECTOR_OPTIMISE_FRAME
#define AUDIO_MATHS_VECTOR_OPTIMISE_SATURATOR
#if ! defined (PLATFORM_SPU) && ! defined (PLATFORM_PS3)
#define AUDIO_MATHS_VECTOR_OPTIMISE_LOW_PASS_1
#define AUDIO_MATHS_VECTOR_OPTIMISE_HIGH_PASS_1
#endif
#define AUDIO_MATHS_VECTOR_OPTIMISE_FILL_F32_BUFFER_FROM_S16
#define AUDIO_MATHS_VECTOR_OPTIMISE_RESAMPLE

#if defined( PLATFORM_PC )
#define AUDIO_REMOVE_DENORMALS
#endif

#if !defined(PLATFORM_SPU) && !defined(PLATFORM_PS3)
#define VF_DECODERS
#endif

#if defined(PLATFORM_PC)
#pragma warning(disable:4996)
#endif

#define AUDIO_MAX_INSTANCE_BUFFERS (1000)

#if defined(PLATFORM_XENON)
#define AUDIO_USE_XMP_CAPTURE
#endif

#define NUM_AUDIO_OVERLAYS 2

#define NO_MERCURY_ASSERT
#include <System/Perf/Profiler.h>
#include <System/perf.h>

// jkc- currently saves 2.5k on spu code on release builds
#if defined(PLATFORM_SPU) && defined(CFG_RELEASE)
#define SPU_STRIP_PERF_FROM_AUDIO_RELEASE
#endif

#if defined(CFG_GOLD)
#define AUDIO_PERF_EVENT(Tag)
#define AUDIO_PERF_MICRO_EVENT(Tag)
#define AUDIO_PERF_BEGIN_EVENT(Tag)
#define AUDIO_PERF_END_EVENT()
#else // !CFG_GOLD
#if defined(CFG_SPEED) || defined(SPU_STRIP_PERF_FROM_AUDIO_RELEASE)
#define AUDIO_PERF_EVENT(Tag) PERF_EVENT(Tag)
#define AUDIO_PERF_MICRO_EVENT(Tag)
#define AUDIO_PERF_BEGIN_EVENT(Tag) Perf::BeginNamedEvent(Tag)
#define AUDIO_PERF_END_EVENT() Perf::EndNamedEvent()
#else // !CFG_SPEED
#define AUDIO_PERF_EVENT(Tag) PERF_EVENT(Tag)
#define AUDIO_PERF_MICRO_EVENT(Tag) PERF_EVENT(Tag)
#define AUDIO_PERF_BEGIN_EVENT(Tag) Perf::BeginNamedEvent(Tag)
#define AUDIO_PERF_END_EVENT() Perf::EndNamedEvent()
#endif // CFG_SPEED
#endif // CFG_GOLD

#define BUFFER_STACK_MAX_SIZE 16

#define FS_STREAM_HANDLE				Audio::PagedArrayHandle

namespace Sys
{
	class I_File;
}

namespace Audio
{
	enum SPUAudioTag
	{
		eScheduler,
		eAudioTask,
		ePhysicsFastJoint,
		ePhysicsGetContacts,
		ePhysicsRayTrace,
		ePhysicsStaticCache,
		ePhysicsSolve,
		eLoadOverlayTag,
		eObjectSetupDMAsTag,
		eObjectSetupTag,
		eObjectUpdateTag,
		eObjectGenerateTag,
		eObjectShutdownTag,
		ePCMDecoderTag,
		eXMADecoderTag,
		eADPCMDecoderTag,
		eWaveTag,
		eMixerTag,
		eMemCpyTag,
		eFirstOverlayTag,
		eNextTag = eFirstOverlayTag + NUM_AUDIO_OVERLAYS,
	};

	extern bool g_bUseAltConnectMode;

	// forward decl
	class File;

////////////////////////////////////////////////////////////////////////////////
//	Core
////////////////////////////////////////////////////////////////////////////////
	namespace Core
	{
#if defined(PLATFORM_PS3)
        const U32 uSampleRate = 48000;
#else
        const U32 uSampleRate = 48000;
#endif
#if defined(PLATFORM_PC)
        const U32 uFrameLength = 512;
#else
        const U32 uFrameLength = 256;
#endif //PLATFORM_PC

        extern const F32 fSampleRate;
        extern const F32 fNyquist;
        extern const F32 fFrameLength;

		extern const F32 fRcpFrameLength;

		extern const F32 fRcpSampleRate;

        extern U32 uFrameNum;

		typedef F32 Sample;

////////////////////////////////////////////////////////////////////////////////
//	wave server enumerated types
////////////////////////////////////////////////////////////////////////////////
		enum WaveStreamType
		{
			eUnknownType = 0,
			eFileStream,
			eMP3,

			NumWaveStreamTypes
		};


		enum StreamSourceType
		{
			eUnknownSource = 0,

			eFile
		};


		enum WaveServerMessageType
		{
			eMessageTypeMP3 = 0,
			eMessageTypeFileStream,
			eMessageTypeDeallocateResource
		};

////////////////////////////////////////////////////////////////////////////////
//	FileStreamFileWrapper
////////////////////////////////////////////////////////////////////////////////
        class FileStreamFileWrapper
        {

			friend class FileStreamManager;
			friend class WaveStreamLoadingThread;

        private:
			FileStreamFileWrapper();
			~FileStreamFileWrapper();

			bool				Load(const char* strFilename);
			bool				Open();
			bool				Close();
			bool				GetHeader(U32 u32HeaderSize, void* pDest);
			bool				RewindToStart();
			Sys::I_File*		GetFilePointer();
			inline void			FlagForRelease() { m_bFlaggedForRelease = true; }
			inline bool			IsReleasing() { return m_bFlaggedForRelease; }
			inline bool			IsNotInUse() { return (m_u32StreamRefCount == 0); }
			inline void			IncRefCount() { m_u32StreamRefCount++; }
			inline void			DecRefCount() { m_u32StreamRefCount--; }
			inline bool			IsOpen() { return (m_File != 0); }
#ifdef PLATFORM_XENON
			inline U32			GetCurrentLayer() { return m_u32CurrentLayer; }
			inline void			SetCurrentLayer(U32 u32Layer) { m_u32CurrentLayer = u32Layer; }
#endif

			Sys::I_File*		m_File;
			char				m_strFilename[MAX_STREAMER_FILE_PATH_LENGTH];
			U32					m_u32HeaderSize;
#ifdef PLATFORM_XENON
			U32					m_u32CurrentLayer;
#endif
			U32					m_u32StreamRefCount;
			bool				m_bLoaded;
			bool				m_bFlaggedForRelease;
		};

////////////////////////////////////////////////////////////////////////////////
//	WaveStreamReadRequest
////////////////////////////////////////////////////////////////////////////////
		struct WaveStreamReadRequest
		{
			FileStreamFileWrapper*	m_pFileWrapper;
			U32						m_u32BytesToRead;
			U32						m_u32SeekPosition;
			void*					m_pDestBuffer;
#ifdef PLATFORM_PS3
			void*					m_pDestBufferR;
#endif
			WaveServerHandle		m_hStream;
			U32						m_u32RequestId;
			bool					m_bLoop;
			bool					m_bTempAmaxNullFileReadHack;
			bool					m_bDeallocateResources;
			U8						m_u8ReadRequestType;
		};

////////////////////////////////////////////////////////////////////////////////
//	WaveStreamData
////////////////////////////////////////////////////////////////////////////////
		struct WaveStreamData
		{
			WaveStreamData() { Init(); }

			bool Destroy();
			void Init()
			{
				u32BytesConsumed = 0;
				bPlaying = false;
				bFinished = false;
				bResourcesAllocated = false;
				u32SourceDataPosition = 0;
				u32SourceDataLength = 0;
				u32LoopCount = 0;
				u32BytesAvailable = 0;
				hWaveData.SetInvalidHandle();
				bRequiresUpdate = false;
				u32LocalOffset = 0;
			}
			U32 GetBytesAvailable() { return u32BytesAvailable; }
			U8* GetData(U32 u32NumBytes, U32* pu32StreamsToUpdate)
			{
				SetToUpdate(pu32StreamsToUpdate);

				if (! pData)
					return 0;

				U32 u32BytesToConsume = (u32NumBytes < u32BytesAvailable ? u32NumBytes : u32BytesAvailable);
				u32BytesConsumed += u32BytesToConsume;
				u32BytesAvailable -= u32BytesToConsume;
				U8* pu8RetAddr = &(pData[u32LocalOffset]);
				u32LocalOffset += u32BytesToConsume;
				return pu8RetAddr;
			}
			bool IsFinished() { return bFinished; }
			bool IsPlaying() { return bPlaying; }
			void SetPlaying(bool bParamPlaying, U32* pu32StreamsToUpdate) { bPlaying = bParamPlaying; SetToUpdate(pu32StreamsToUpdate); }
			bool IsPrimed() { return bResourcesAllocated; }
			void Prime(U32* pu32StreamsToUpdate) { SetPlaying(true, pu32StreamsToUpdate); }
			U32 GetSourceDataPosition() { return u32SourceDataPosition; }
			void SetSourceDataPosition(U32 u32Pos, U32* pu32StreamsToUpdate) { u32SourceDataPosition = u32Pos; SetToUpdate(pu32StreamsToUpdate); }
			U32 GetSourceDataLength() { return u32SourceDataLength; }
//			U32 GetSourceTotalBytesRemainingInStream() { return u32SourceDataLength - u32SourceDataPosition; }	// TODO: need to actually set u32SourceDataLength somewhere before this will work
			U32 GetLoopCount() { return u32LoopCount; }
			void ResetLoopCount() { u32LoopCount = 0; }
			void SetToUpdate(U32* pu32StreamsToUpdate) { Assert (hWaveData.IsValid()); if (! bRequiresUpdate) (*(pu32StreamsToUpdate))++; bRequiresUpdate = true; }
			U32 GetBytesConsumed() { return u32BytesConsumed; }
			void OverrideBytesConsumed(U32 u32OverrideValue)
			{
				S32 s32Diff = (S32)u32BytesConsumed - (S32)u32OverrideValue;
				u32LocalOffset -= s32Diff;
				u32BytesAvailable += s32Diff;
				u32BytesConsumed = u32OverrideValue;
			}

			U32 u32BytesConsumed;
			U32 u32BytesAvailable;
			U32 u32SourceDataPosition;
			U32 u32SourceDataLength;
			U32 u32LoopCount;
			U32 u32LocalOffset;
			U8* pData;
			StreamHandle hWaveData;
			bool bPlaying;
			bool bFinished;
			bool bResourcesAllocated;
			bool bRequiresUpdate;
		};

////////////////////////////////////////////////////////////////////////////////
//	WaveServerRegisterSourceParams
////////////////////////////////////////////////////////////////////////////////
		struct WaveServerRegisterSourceParams
		{
			WaveServerRegisterSourceParams()
			{
				m_eType = eUnknownSource;
				m_pSourceData = 0;
			}

			StreamSourceType		m_eType;
			void*					m_pSourceData;
		};

////////////////////////////////////////////////////////////////////////////////
//	WaveServerStreamCreateParams
////////////////////////////////////////////////////////////////////////////////
		struct WaveServerStreamCreateParams
		{
			StreamSourceHandle	m_hSourceHandle;	// will contain stream handle for file streamer
			void*				m_pMemoryResidentData;
			U32					m_u32StreamEncodedDataFormat;

			U32					m_u32DataProcessMaxChunkSize;	// size of return buffers. maximum number of bytes GetData() can return
			U32					m_u32FileReadChunkSize;			// number of bytes we read from disk in one file read
			U32					m_u32NumInitialWriteChunks;
			U32					m_u32BufferSizeInReadChunks;	// number of chunks of size m_u32FileReadChunkSize we've got for holding data
			U32					m_u32FileStartPosition;
			U32					m_u32FileEndPosition;
			U32					m_u32ReturnBufferAlignment;
			U32					m_u32HeaderSkipSize;
			U32					m_u32MaxReturnBufferSize;
			bool				m_bReturnBufferPhysicalMemory;
			bool				m_bLoop;
			bool				m_bPrecache;
			bool				m_bTempAmaxNullFileReadHack;
			U8					m_u8NumChannels;

			#ifndef CFG_SPEED
			Btl::String			m_strBankFilename;
			Btl::String			m_strWaveName;
			#endif

			WaveServerStreamCreateParams()
			{
				m_pMemoryResidentData = 0;
				m_u32StreamEncodedDataFormat = 0;
				m_u32DataProcessMaxChunkSize = 0;
				m_u32FileReadChunkSize = 0;
				m_u32NumInitialWriteChunks = 0;
				m_u32BufferSizeInReadChunks = 0;
				m_u32FileStartPosition = 0;
				m_u32FileEndPosition = 0;
				m_u32ReturnBufferAlignment = 0;
				m_u32HeaderSkipSize = 0;
				m_u32MaxReturnBufferSize = 0;
				m_bReturnBufferPhysicalMemory = false;
				m_bLoop = false;
				m_bPrecache = false;
				m_u8NumChannels = 1;
				m_bTempAmaxNullFileReadHack = false;
			}
		};

////////////////////////////////////////////////////////////////////////////////
//	AdpcmWaveInfo
////////////////////////////////////////////////////////////////////////////////

		enum AdpcmWaveBufferRequest
		{
			eNoRequest = 0,
			eAllocateBuffer,
			eReleaseBuffer
		};

		enum AdpcmWaveBufferState
		{
			eBufferNotAllocated = 0,
			eBufferAllocated
		};

		struct AdpcmWaveInfo
		{
			void* pPrevDecodedFrameBuffer;
			U16 u16BufferIndex;
			U8 u8BufferRequest;
			U8 u8BufferState;

			AdpcmWaveInfo() { Init(); }

			void Init()
			{
				pPrevDecodedFrameBuffer = 0;
				u8BufferRequest = eNoRequest;
				u8BufferState = eBufferNotAllocated;
				u16BufferIndex = 0;
			}
		};

////////////////////////////////////////////////////////////////////////////////
//	debug struct
////////////////////////////////////////////////////////////////////////////////
        struct Internals
        {
            Internals()
            {
                uTotalNumGenerators = 0;
                uTotalNumMixers = 0;
                uTotalNumWaves = 0;
                uNumGeneratorsActive = 0;
                uNumMixersActive = 0;
                uNumWavesActive = 0;
            };

            U32 uTotalNumGenerators;
            U32 uTotalNumMixers;
            U32 uTotalNumWaves;
            U32 uNumGeneratorsActive;
            U32 uNumMixersActive;
            U32 uNumWavesActive;
        };

////////////////////////////////////////////////////////////////////////////////
//	lightweight core update struct
////////////////////////////////////////////////////////////////////////////////
		struct DecoderDataInfo
		{
			void* pWaveServerData;
			void* pAdpcmBufferData;
		};

////////////////////////////////////////////////////////////////////////////////
//	core update struct
////////////////////////////////////////////////////////////////////////////////
        struct ProcessInfo
        {
			U32 uFrameNum;
            U16 uFrameInt;
            U16 uFrameMod;
			F32 fFrameInt;
			F32 fFrameMod;
			F32 fFraction;
			F32 fWidth;

			U8* pCoreData;

			F32* pDynamicBuffers;
			F32* pStaticBuffers;

			DecoderDataInfo decoderDataInfo;

			bool bMutePrev;
			bool bMuteNext;
			bool bSuspendPrev;
			bool bSuspendNext;	//32
        };

		// lightweight core struct

////////////////////////////////////////////////////////////////////////////////
//	DSP
////////////////////////////////////////////////////////////////////////////////
	    namespace DSP
	    {
            template <U32 uSampleRate, U32 uFrameLength, U16 uChannels>
            class FFT;

		    class Effect;

			void RealFFT_Split(F32 *data,long n);
			void IRealFFT_Split(F32 *data,long n);
        }

		enum Type
		{
			eUndefined = -1,
			eInput,
			eOscillator,
			eWave,
			eMixer,
			eOutput,
			eReturn,

			//	dsp
			//eFeedbackDelay,

			eNumTypes
		};

        class Decoder;

		struct Item
		{
			Type eType;
			U32 uOffset;
		};

		class Input;

		typedef void (*RenderCallback)(void* pContext);

		class Renderer;

		class BufferStack
		{
		public:
			BufferStack() : m_uCurrDepth( 0 )
			{
				for( U32 uCount = 0; uCount < BUFFER_STACK_MAX_SIZE; uCount++ )
					m_aSize[ uCount ] = 0;
			}

			inline void SetDepth( U32 uCurrDepth )
			{
				Assert( uCurrDepth < BUFFER_STACK_MAX_SIZE );
				m_uCurrDepth = uCurrDepth;
			}
			inline void SetSize( U32 uSize ) { m_aSize[ m_uCurrDepth ] = m_aSize[ m_uCurrDepth ] > uSize ? m_aSize[ m_uCurrDepth ] : uSize; }

			inline U32 GetDepth() { return m_uCurrDepth; }
			inline U32 GetSize() { return m_aSize[ m_uCurrDepth ]; }
			inline U32 GetSize( U32 uDepth )
			{
				Assert( uDepth < BUFFER_STACK_MAX_SIZE );
				return m_aSize[ uDepth ];
			}
			inline U32 GetOffset( U32 uDepth )
			{
				Assert( uDepth < BUFFER_STACK_MAX_SIZE );
				U32 uOffset = 0;
				for( U32 uCount = 0; uCount < uDepth; uCount++ )
					uOffset += m_aSize[ uCount ];

				return uOffset;
			}

			inline U32 GetFootprint()
			{
				U32 uOffset = 0;
				for( U32 uCount = 0; uCount < BUFFER_STACK_MAX_SIZE; uCount++ )
					uOffset += m_aSize[ uCount ];

				return uOffset;
			}

		private:
			U32 m_aSize[ BUFFER_STACK_MAX_SIZE ];
			U32 m_uCurrDepth;
		};

		WaveStreamData* GetWaveData(WaveServerLocalIndex hWaveData, const void* pWaveServerData);
		AdpcmWaveInfo* GetAdpcmBufferData(U32 uAdpcmWaveInfoIndex, const void* pAdpcmBufferData);
	};


////////////////////////////////////////////////////////////////////////////////
//	Renderer
////////////////////////////////////////////////////////////////////////////////
    Core::Renderer* CreateRenderer();

	extern bool g_bQuitAllTasks;
};

bool ProcessOutput(const Audio::Core::ProcessInfo& info, bool bProcess, U32 uInput, U32 uOffset);

#include "wavrenderer.h"

#define AUDIOMEMCPY( dest, source, size )			\
	{												\
		U32 nrQWords = size >> 4;					\
		qword* pSrc = (qword*)source;				\
		qword* pDest = (qword*)dest;				\
		for (U32 x = 0 ; x < nrQWords ; x++)		\
		{											\
			*pDest = *pSrc;							\
			pDest++;								\
			pSrc++;									\
		}											\
	}												\

//	Bring in ATG FFT
#ifdef _XBOX360

namespace ATG{

    //
    // CompileTimeInt
    //
    // Although it's possible to use integers as template parameters,
    // there are some important differences between integral parameters
    // and type parameters. This class helps iron out some of those
    // differences by effectively creating a new type for each unique
    // integer value used by the application. 
    //
    // This class conforms to a simple pattern for "meta-values." A
    // meta-value is a compile-time value that can be manipulated by
    // "meta-ops" or compile-time operations. A meta-value in this 
    // system is any type that has a public const static integral 
    // member called "value."
    //
    template< int V >
    struct CompileTimeInt
    {
        typedef CompileTimeInt<V> type;
        enum{ value = V};
            
        operator int() { return value; }
        
    };
    //
    // Recommended types for passing __vector4
    //
    typedef const __vector4 __declspec(passinreg) vector4_in;
    typedef __vector4 __declspec(passinreg)& vector4_out;
    typedef __vector4 __declspec(passinreg)& vector4_inout;
    typedef __vector4* __restrict vector4_ptr;

    //
    // Wrappers for VMX compiler intrinsics
    // 
    // VMX intrinsics can't be used as functors--they look like functions
    // but they have no storage, which makes things all confused. Plus, they
    // are mostly defined with return types, which makes them harder to use
    // with CompileTimeLoop. Fortunately they can be wrapped in functions
    // with no loss of efficiency as long as the functions are inlined.
    //
    void __forceinline vor( vector4_out result, vector4_in a, vector4_in b ){ result = __vor( a, b ); }
    void __forceinline vand( vector4_out result, vector4_in a, vector4_in b ){ result = __vand( a, b ); }

    void __forceinline vcmpgefp( vector4_out result, vector4_in a, vector4_in b ){ result = __vcmpgefp( a, b ); }
    void __forceinline vcmpgtfp( vector4_out result, vector4_in a, vector4_in b ){ result = __vcmpgtfp( a, b ); }

    void __forceinline vcmpgefp_vor( vector4_out result, vector4_in a, vector4_in b ){ result = __vor( result, __vcmpgefp( a, b ) ); }
    void __forceinline vcmpgtfp_vor( vector4_out result, vector4_in a, vector4_in b ){ result = __vor( result, __vcmpgtfp( a, b ) ); }
    void __forceinline vcmpgefp_vand( vector4_out result, vector4_in a, vector4_in b ){ result = __vand( result, __vcmpgefp( a, b ) ); }
    void __forceinline vcmpgtfp_vand( vector4_out result, vector4_in a, vector4_in b ){ result = __vand( result, __vcmpgtfp( a, b ) ); }

    void __forceinline vmulfp( vector4_out result, vector4_in a, vector4_in b ){ result = __vmulfp( a, b ); }
    void __forceinline vaddfp( vector4_out result, vector4_in a, vector4_in b ){ result = __vaddfp( a, b ); }
    void __forceinline vsubfp( vector4_out result, vector4_in a, vector4_in b ){ result = __vsubfp( a, b ); }
    void __forceinline vmaddfp( vector4_out result, vector4_in mul1, vector4_in mul2, vector4_in add ){ result = __vmaddfp( mul1, mul2, add ); }
    void __forceinline vmsum4fp( vector4_out result, vector4_in mul1, vector4_in mul2 ){ result = __vmsum4fp( mul1, mul2 ); }
    void __forceinline vmsum3fp( vector4_out result, vector4_in mul1, vector4_in mul2 ){ result = __vmsum3fp( mul1, mul2 ); }


    //
    // Things do get a little tricky when the intrinsic takes a constant
    // parameter, because the value for the parameter has to be known at
    // compile time in order for the compiler to emit the right machine code.
    // This could be solved in many cases by wrapping the intrinsic in 
    // a function template, but you can't pass a template function to another
    // template function if the second function isn't expecting a template.
    // We get around this problem by using functor structs--structures that
    // overload operator() and templatize the overload. The struct object
    // is what gets passed to CompileTimeLoop, which sees it as just another
    // function. When the operator actually gets called, the compiler 
    // deduces the correct integer to call it with.
    //
    struct vspltw_t
    {
        template<int element>
        void __forceinline operator()(vector4_out result, vector4_in src, CompileTimeInt<element> e ){ result = __vspltw( src, element ); }
    };
    __declspec(selectany) vspltw_t vspltw;

    struct vinsert_t
    {
        template<int srcE, int destE>
        void __forceinline operator()(vector4_out result, vector4_in src, CompileTimeInt<srcE> srcElement, CompileTimeInt<destE> destElement )
        { 
            result = __vrlimi( result, src, 8>>destE, ((destE-srcE)+3)%3);
        }
    };
    __declspec(selectany) vinsert_t vinsert;

    union fdw{ DWORD dw; float f; };
    const static fdw vpermX0 = { 0x00010203 };
    const static fdw vpermY0 = { 0x04050607 };
    const static fdw vpermZ0 = { 0x08090a0b };
    const static fdw vpermW0 = { 0x0c0d0e0f };
    const static fdw vpermX1 = { 0x10111213 };
    const static fdw vpermY1 = { 0x14151617 };
    const static fdw vpermZ1 = { 0x18191a1b };
    const static fdw vpermW1 = { 0x1c1d1e1f };

    union __declspec(align(16)) vdw{ DWORD dw[4]; __vector4 v; };
    const static __vector4 Ones = { 1.0f, 1.0f, 1.0f, 1.0f };
    const static vdw vectorSignBits = { 0x80000000, 0x80000000, 0x80000000, 0x80000000 };
    const static vdw vectorNonSignBits = { 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff };
} // namespace ATG

void FFT_Unoptimized( float* pReal, float* pImaginary, float sign, unsigned long countLog2, unsigned long passes = 0 );

void FFT_8( __vector4* __restrict pReal, __vector4* __restrict pImag );
void FFT_16( __vector4* __restrict pReal, __vector4* __restrict pImag );
void FFT_32( __vector4* __restrict pReal, __vector4* __restrict pImag );
void FFT_64( __vector4* __restrict pReal, __vector4* __restrict pImag );
void FFT_128( __vector4* __restrict pReal, __vector4* __restrict pImag );
void FFT_256( __vector4* __restrict pReal, __vector4* __restrict pImag );
void FFT_512( __vector4* __restrict pReal, __vector4* __restrict pImag );
void FFT_1024( __vector4* __restrict pReal, __vector4* __restrict pImag );
void FFT_2048( __vector4* __restrict pReal, __vector4* __restrict pImag );
void FFT_4096( __vector4* __restrict pReal, __vector4* __restrict pImag );

//
// Warning: Unswizzle function is not optimized (although it is intended
// for use with the optimized FFTs above--the FFT_Unoptimized function does
// it's own unswizzling)
//
void FFT_Unswizzle( float* __restrict pOut, float* __restrict pIn, int nFloatsLog2 );

#endif // _XBOX360

#endif // _CORE_H_

