#ifndef _DECODERXMA_H_
#define _DECODERXMA_H_

#ifdef PLATFORM_XENON
#include "XMAHardwareAbstraction.h"
#include "..\common\lock.h"
#include <core/threads/mutex.h>
#include <btl/Containers/list.h>

#define XMA_DOUBLE_BUFFER

#define AUDIO_XMA_NUM_CACHE_LINES 8
#define AUDIO_XMA_MAX_NUM_HW_DECODERS 180 // While the Xbox 360 hardware supports up to 320 hardware decoder contexts, the Xbox 360 Guide reserves four of them, allowing titles to create a total of 316.

#ifndef CFG_GOLD
#define XMA_CONTEXT_DEBUG_LIST
#endif

// notes on memory usage for xma decoders:
// there are 2 types of buffer allocators needed for xma decoders.
//
// XMA_BufferAllocator()
// ------------------
// for memory which is passed to XMAPlaybackCreate(). (also used temporarily for buffers we give to the decoder with encoded data in).
// this uses 1 fixed size contigous buffer of an arbitrary size. memory is allocated from this buffer.
//
// XMADecoderBuffer()
// ----------------
// for decoded data which is passed back from the decoder. each decoder created has a pair
// of buffers which holds decoded data, which are used for double buffering.
// the size of this entire buffer is determined by how many hardware decoders we are using.


// array of pointers to free fixed size memory blocks.
// these pointers are pointers into the XMA_BufferAllocator's buffer and are block's which have previously been freed.
class XMA_FixedSizeBufferArray
{
public:
    
    XMA_FixedSizeBufferArray()
        : m_Entries( 0 ) { m_PtrArray.Reserve(AUDIO_XMA_MAX_NUM_HW_DECODERS); }
    
    ~XMA_FixedSizeBufferArray() { }
    
	// remove from free list
	void* Alloc();

	// put into the free list
	void Free( void* ptr );
   
private:
    
    static const U32    m_MaxEntries = AUDIO_XMA_MAX_NUM_HW_DECODERS;
    U32                 m_Entries;											// how many free memory block pointers have we got in this array?
	Btl::Vector<void*>	 m_PtrArray;
};

// buffer allocator used to hand out physical memory blocks.
// this memory is used to pass to XMAPlaybackCreate().
// it has one contigious memory buffer and hands out memory blocks of a specified size. when a block
// is freed it puts it in an array of fixed size blocks. when a block is requested/alloced it looks through the
// array of same sized free blocks and returns the first free one. if it cant find a free one it allocs one from
// the stack.
class XMA_BufferAllocator
{
public:
    XMA_BufferAllocator();
    ~XMA_BufferAllocator();
    
	static void Init();

    void* Alloc( const U32 input_size);
    void Free( void* ptr, const U32 input_size);
    
	static XMA_BufferAllocator* TheBuffer() { return m_pXMA_PlaybackBuffer; }

	static U32 PeakMemoryUsed() { return m_PeakMemoryUsed; } 
	static U32 MemorySize() { return m_MemorySize; } 

private:
	static XMA_BufferAllocator*	m_pXMA_PlaybackBuffer;
    
    static const U32			m_MaxEntries = 32;								// number of different block sizes the allocator can handle
    U32							m_Entries;										// number of current different block sizes
    S32							m_Size[ m_MaxEntries ];							// block size for each of the array of free memory blocks
    XMA_FixedSizeBufferArray	m_FixedSizeFreeBlocks[ m_MaxEntries ];			// array of free memory blocks.
    
    static const U32			m_MemorySize = 800 * 1024;
    static U32					m_PeakMemoryUsed;
    U32							m_MemoryFree;
    void*						m_pMemoryBase;									// the pointer to the main buffer
    char*						m_pMemoryFree;									// pointer to the next free block of memory

	Audio::Core::Lock			m_Lock;
};

namespace Audio
{
    namespace Core
    {
		// buffer allocator used for double buffering decoded data given back from the xma decoder
		class XMADecoderBuffer
		{
		public:
#ifdef XMA_DOUBLE_BUFFER
			bool	m_bUsed[2 * AUDIO_XMA_MAX_NUM_HW_DECODERS];
#else
			bool	m_bUsed[AUDIO_XMA_MAX_NUM_HW_DECODERS];
#endif
			S16*	m_asBuffer;			// temp scratch pad buffer

			static Core::Lock m_Lock;
			MT::Lock m_ThreadLock;

		public:
			XMADecoderBuffer();
			~XMADecoderBuffer();

			static XMADecoderBuffer& Instance()
			{	
				static XMADecoderBuffer *instance = NULL;
				if (!instance) 
				{
					m_Lock.Acquire();
					if(!instance) 
						instance = new XMADecoderBuffer;
					m_Lock.Release();
				}
				return *instance;
			}

			U32 Allocate(S16*& ptr);
			void Free(U32 index);

		};
		
		struct XMADoomedPlayback
		{
		    PXMAPLAYBACK m_pXMAPlayback;
		    void*        m_pBuffer;
		    U32          m_BufferSize;
		};

////////////////////////////////////////////////////////////////////////////////
//	XMADecoder
////////////////////////////////////////////////////////////////////////////////
        class XMADecoder : public Decoder
        {
        public:
            //  this struct defines the wave data
            struct WaveData
            {
                U8 uChannels;
                U32 uLength;
                U32 uSampleRate;
				U32 uFrames;
                S16* psData;
				U32* puSeekData;
				U32 uLoopStartOffsetBit;
				U32 uLoopEndOffsetBit;
				bool bLoop;
				U8	uSubframeData;
				U8	uMaxPitchDown;
				U8	uMaxPitchUp;
				WaveServerLocalIndex hWaveData;
//				FS_STREAM_HANDLE hStream;
            };

            //  this struct holds the state (and data) for a decoding sound
            struct State
            {
                PXMAPLAYBACK pXmaPlayback;
                bool bLoop;
				bool bActive;
                U32 uPacket;
                U32 uStartSample;
                U32 uProgress;
                F32 fFraction;
                F32 fPrevPitch;
                F32 fPitch;
                S16 sPrevious;
                bool bPlaying;
                bool bStarving;
				bool bStreamPrimed;
				bool bResumed;
                U32 u32CallbackId;
                U32 u32StateOffset;
				bool bUseSrc;
				U32 uOctaveMaxPitch;

				F32 m_fA;
                F32 m_fB;

				F32 fPrevSample;


				S32 sNumSamples;		// how many samples we've got in any one of the double buffers

				F32 fPrevSampleR;
				F32	fPrevSampleL;

				F32 fPrevInL;
				F32 fPrevInR;
				F32 fPrevOutL;
				F32 fPrevOutR;

				S32 sIndex;				// index into how far we've read through the current read buffer
				
				bool bTriggerCallback;

				#if !defined( AUDIO_MATHS_VECTOR_OPTIMISE_DECODER_XMA ) // JASOND
				F32 fPrevIn;
				F32 fPrevOut;
				#else
				Maths::Vector4 m_Garbage0; // JASOND: This can go when this structure is 16 byte aligned!
				Maths::Scalar  m_vA;
				Maths::Scalar  m_vB;
				Maths::Vector4 m_vPrevIn;
				Maths::Vector4 m_vPrevOut;
				Maths::Vector4 m_Garbage1; // JASOND: This can go when this structure is 16 byte aligned!
				#endif
            };

        public:
            XMADecoder(U32 uXmaFormat, bool bResample, WaveData& waveData);
            ~XMADecoder();
            void Init(const DecoderDataInfo& info);
            void Shutdown(const DecoderDataInfo& info);
			bool IsActive(const DecoderDataInfo& info)
			{
				return m_state.bActive;//(m_state.pXmaPlayback != 0);
			}
//            void SetCallbackId(U32 u32StateOffset, U32 u32CallbackId);
			void SetHasCallback() { m_state.bTriggerCallback = true; }
			void FreeSubmittedBuffers(const DecoderDataInfo& info);
            bool FillBuffer(F32* afData, const ProcessInfo& info);
            F32 Position(const DecoderDataInfo& info);
            void SkipBuffer(const ProcessInfo& info);
			void Resume(const DecoderDataInfo& info);
            void Reset(const DecoderDataInfo& info, U32 uOffset = 0);
            void SetPitch(F32 fPitch);
			U32 Length() { return m_waveData.uFrames * 512; }
			bool IsPlaying(const DecoderDataInfo& info) { if (GetWaveData(info)) return GetWaveData(info)->IsPlaying(); return m_state.bPlaying; }
			U32 GetBytesAvailable(const DecoderDataInfo& info);
			void SetPlaying(const DecoderDataInfo& info, const bool bPlaying) { if (GetWaveData(info)) GetWaveData(info)->SetPlaying(bPlaying, (U32*)(info.pWaveServerData)); /*else*/ m_state.bPlaying = bPlaying; }

			//bool Finished() { return !Decoder::IsActive(); }

			WaveStreamData* GetWaveData(const DecoderDataInfo& info);

            //static XMADecoder* CreateDecoder() { return new XMADecoder; }   //  TODO: release
            //static void ReleaseDecoder() {}

			void SetWaveData(WaveData& waveData) { m_waveData = waveData; }

			void	UpdateFilterCoeffs(F32 fPitch, const ProcessInfo& info);

			void	SetWaveName(const char* name,const char* bank){waveName = name;waveBank = bank;}

			static const U32 Total() { return m_uTotalXMA; }
			static const U32 Active() { return m_uActiveXMA; }

			static void DestroyAllDoomedContexts();
			static void DumpDoomedContexts();
			static void SetNumDoomedContexts(U32 uNum) { m_uNumDoomedContexts = uNum; }
			
            static void AddDoomedContext( PXMAPLAYBACK pPlaybackContext, void* pBuffer, U32 bufferSize );
            
			void AcquireContext(const DecoderDataInfo& info);
			void ReleaseContext(const DecoderDataInfo& info);

		    static XMADoomedPlayback* m_XMADoomedPlaybackContexts;

			static Core::Lock& GetLock() { return m_Lock; }
		protected:
            static U32 m_uTotalXMA;
            static U32 m_uActiveXMA;

			static U32 m_uNumDoomedContexts;

			static U32 m_DoomedIndex;
			
			

		private:
            U32 GetData(const DecoderDataInfo& info);
#ifdef XMA_DOUBLE_BUFFER
			bool GetNextBuffer();
			bool PeekNextBuffer();
#endif

		private:
			//static bool m_bUsed[AUDIO_XMA_MAX_NUM_HW_DECODERS];
            //static S16 m_asBuffer[2 * AUDIO_XMA_MAX_NUM_HW_DECODERS * m_state.uOctaveMaxPitch * 128];

            //static U32 m_uTotal;
            //static U32 m_uInUse;

#ifdef XMA_DOUBLE_BUFFER
            S16* m_asDecoded[2];		// decoded sample data

			U32 m_uIndex[2];

			U8 m_uRead;					// index of buffer we are reading from ("front buffer")
			U8 m_uWrite;				// index of buffer we are reading from ("back buffer")

			U32 m_uWritten;				// number of samples written. if this is non zero we havent filled the buffer
#else
            S16* m_asDecoded;		// decoded sample data

			U32 m_uIndex;
#endif
			U32 m_uWaveDataOffset;
			//Btl::Vector<void*> m_SubmittedDataChunksList;
			Btl::List<void*> m_SubmittedDataChunksList;

			WaveData m_waveData;
			State m_state;
			//File m_File;
			const char* waveName;
			const char* waveBank;
				
            void* m_pPlaybackBuffer;
            U32   m_PlaybackBufferSize;

			static Core::Lock m_Lock;
        };
    }
}

#endif // PLATFORM_XENON

#endif // _DECODERXMA_H_
