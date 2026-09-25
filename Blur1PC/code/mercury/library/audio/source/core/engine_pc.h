
#pragma once

#define USE_OPENAL
#define USE_EAX_REVERB

#ifdef USE_OPENAL
#include "al.h"
#include "alc.h"
#include "efx.h"
#include "xram.h"
#include "efx-creative.h"
#include "EFX-Util.h"
#else // USE_OPENAL
#include <dsound.h>
#endif // USE_OPENAL

class CPrimarySound;

#ifdef USE_OPENAL
#define NUM_BUFFERS 8


// EFX Extension function pointer variables

// Effect objects
extern LPALGENEFFECTS alGenEffects;
extern LPALDELETEEFFECTS alDeleteEffects;
extern LPALISEFFECT alIsEffect;
extern LPALEFFECTI alEffecti;
extern LPALEFFECTIV alEffectiv;
extern LPALEFFECTF alEffectf;
extern LPALEFFECTFV alEffectfv;
extern LPALGETEFFECTI alGetEffecti;
extern LPALGETEFFECTIV alGetEffectiv;
extern LPALGETEFFECTF alGetEffectf;
extern LPALGETEFFECTFV alGetEffectfv;

// Filter objects
extern LPALGENFILTERS alGenFilters;
extern LPALDELETEFILTERS alDeleteFilters;
extern LPALISFILTER alIsFilter;
extern LPALFILTERI alFilteri;
extern LPALFILTERIV alFilteriv;
extern LPALFILTERF alFilterf;
extern LPALFILTERFV alFilterfv;
extern LPALGETFILTERI alGetFilteri;
extern LPALGETFILTERIV alGetFilteriv;
extern LPALGETFILTERF alGetFilterf;
extern LPALGETFILTERFV alGetFilterfv;

// Auxiliary slot object
extern LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots;
extern LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots;
extern LPALISAUXILIARYEFFECTSLOT alIsAuxiliaryEffectSlot;
extern LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti;
extern LPALAUXILIARYEFFECTSLOTIV alAuxiliaryEffectSlotiv;
extern LPALAUXILIARYEFFECTSLOTF alAuxiliaryEffectSlotf;
extern LPALAUXILIARYEFFECTSLOTFV alAuxiliaryEffectSlotfv;
extern LPALGETAUXILIARYEFFECTSLOTI alGetAuxiliaryEffectSloti;
extern LPALGETAUXILIARYEFFECTSLOTIV alGetAuxiliaryEffectSlotiv;
extern LPALGETAUXILIARYEFFECTSLOTF alGetAuxiliaryEffectSlotf;
extern LPALGETAUXILIARYEFFECTSLOTFV alGetAuxiliaryEffectSlotfv;

// XRAM Extension function pointer variables and enum values

typedef ALboolean (__cdecl *LPEAXSETBUFFERMODE)(ALsizei n, ALuint *buffers, ALint value);
typedef ALenum    (__cdecl *LPEAXGETBUFFERMODE)(ALuint buffer, ALint *value);

extern LPEAXSETBUFFERMODE eaxSetBufferMode;
extern LPEAXGETBUFFERMODE eaxGetBufferMode;

// X-RAM Enum values
extern ALenum eXRAMSize, eXRAMFree;
extern ALenum eXRAMAuto, eXRAMHardware, eXRAMAccessible;
#endif

namespace Audio
{
	namespace Core
	{
		class Renderer;

		enum Error
		{
			eSuccess = 0,
			eUninitialized,
			eFailedToCreateDirectSound,
			eFailedToSetCoopLevel,
			eFailedToCreatePrimaryBuffer,
			eFailedToCreateDirectSoundBuffer,
			eFailedToSetPrimaryBufferFormat,
			eFailedToLockPrimaryBuffer,
			eQueryInterfaceFailed,
			eSetNotificationPositionsFailed,
			eRendererExists,
			eVoiceInputMixExists,
			eUnknownEffect,
			eOutOfMemory,

			eNumErrors
		};

		class Engine
		{
			//static void SetEffectCreated(bool value){Audio::Core::Engine::m_bEffectCreated = value;}
		private:
						
		protected:
			HANDLE			m_notificationEvent;
			HANDLE			m_notificationThread;

			Renderer*		m_pRenderer;

#ifndef USE_OPENAL
			LPDIRECTSOUND8	m_pDS;
			CPrimarySound*	m_pPrimarySound;
#else // USE_OPENAL
			static ALenum m_eBufferFormat;			
			static ALuint m_source;			
			static ALuint m_aBuffers[NUM_BUFFERS];			
			static ALCcontext* m_pContext;
			static ALCdevice* m_pDevice;
			static S16* m_pData;			
			/*static int m_sawCount;
			static S16 fCurr;*/

#ifdef USE_EAX_REVERB			
			static ALenum m_eReverbBufferFormat;
			static ALuint m_sourceReverbVoice;
			static ALuint m_aReverbBuffers[NUM_BUFFERS];
			static ALuint m_sourceFilter;
			static S16* m_pReverbData;
			//reverb effects

			static		EFXEAXREVERBPROPERTIES efxReverb;
			static		ALuint		m_uiEffectSlot, m_uiEffect;
			static		ALboolean	m_bEffectCreated;
#endif
						
#endif

		protected:
			static DWORD ThreadProc(LPVOID lpThreadParameter);

			static Engine s_instance;

#ifndef USE_OPENAL
			Error SetupDirectSound();

			HRESULT CreatePrimarySoundBuffer(
				CPrimarySound** ppPrimarySound, 
				DWORD dwCreationFlags, 
				GUID guid3DAlgorithm,
				DWORD dwNotifyCount, 
				DWORD dwNotifySize, 
				HANDLE hNotifyEvent);

			inline CPrimarySound* PrimarySound(void)
			{
				return m_pPrimarySound;
			}
#endif
			inline Renderer* GetRenderer(void)
			{
				return m_pRenderer;
			}

#ifdef USE_OPENAL
			static void GetNextBuffer(S16* pData);
#ifdef USE_EAX_REVERB	
			static void GetNextReverbBuffer(S16* pData);
#endif
			
#endif

		public:
			Engine();
			~Engine();

			static Engine& Instance(void)
			{
				return s_instance;
			}

#ifdef USE_EAX_REVERB	
			ALboolean CreateAuxEffectSlot(ALuint *puiAuxEffectSlot);
			ALboolean CreateEffect(ALuint *puiEffect, ALenum eEffectType);
			ALuint GetEffect(){return m_uiEffect;}
			ALuint GetEffectSlot(){return m_uiEffectSlot;}
			
			static ALboolean SetEFXEAXReverbProperties(EFXEAXREVERBPROPERTIES *pEFXEAXReverb, ALuint uiEffect);
			// Extension Queries 
			ALboolean ALFWIsXRAMSupported();
			ALboolean ALFWIsEFXSupported();
#endif
			bool Init(Renderer* pRenderer);
			
		};
	}
}

