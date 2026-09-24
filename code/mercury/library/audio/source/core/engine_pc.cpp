
#include "AudioPch.h"

#include "engine_pc.h"
#include "primary_sound.h"

#include "coreheaders.h"
#include "../SystemHeaders.h"


#ifdef AUDIO_OPTIMIZE_OFF
#pragma optimize ("", off) // AUDIO_OPTIMIZE_OFF
#endif	//	AUDIO_OPTIMIZE_OFF


HANDLE g_ne(NULL);

//reverb params
#ifdef USE_EAX_REVERB	
Debug::Bool g_AudioDebugSetReverbParams("Audio.EFX.Reverb.OverrideGameReverbForTestingOnly",	false);
Debug::Bool g_AudioDebugUseLegacyReverbParams("Audio.EFX.Reverb.UseLegacyPreset",	false);

Debug::Float g_AudioDebugflDensity("Audio.EFX.Reverb.flDensity",	0.0625f,	Debug::Range(0.f, 10.f));
Debug::Float g_AudioDebugflDiffusion("Audio.EFX.Reverb.flDiffusion",	0.5f,	Debug::Range(0.f, 10.f));
Debug::Float g_AudioDebugflGain("Audio.EFX.Reverb.flGain",	1.f,	Debug::Range(0.f, 10.f));
Debug::Float g_AudioDebugflGainHF("Audio.EFX.Reverb.flGainHF",	0.84042704f,	Debug::Range(0.f, 10.f));
Debug::Float g_AudioDebugflGainLF("Audio.EFX.Reverb.flGainLF",	1.0f,	Debug::Range(0.f, 10.f));
Debug::Float g_AudioDebugflDecayTime("Audio.EFX.Reverb.flDecayTime",	1.5f,	Debug::Range(0.f, 10.f));
Debug::Float g_AudioDebugflDecayHFRatio("Audio.EFX.Reverb.flDecayHFRatio",	0.91f,	Debug::Range(0.f, 10.f));
Debug::Float g_AudioDebugflDecayLFRatio("Audio.EFX.Reverb.flDecayLFRatio",	1.0f,	Debug::Range(0.f, 10.f));
Debug::Float g_AudioDebugflReflectionsGain("Audio.EFX.Reverb.flReflectionsGain",	0.4864f,	Debug::Range(0.f, 10.f));
Debug::Float g_AudioDebugflReflectionsDelay("Audio.EFX.Reverb.flReflectionsDelay",	0.02f,	Debug::Range(0.f, 10.f));
Debug::Float g_AudioDebugflLateReverbGain("Audio.EFX.Reverb.flLateReverbGain",	2.437f,	Debug::Range(0.f, 10.f));
Debug::Float g_AudioDebugflLateReverbDelay("Audio.EFX.Reverb.flLateReverbDelay",	0.029f,	Debug::Range(0.f, 10.f));
Debug::Float g_AudioDebugflEchoTime("Audio.EFX.Reverb.flEchoTime",	0.25f,	Debug::Range(0.f, 10.f));
Debug::Float g_AudioDebugflEchoDepth("Audio.EFX.Reverb.flEchoDepth",	0.0f,	Debug::Range(0.f, 10.f));
Debug::Float g_AudioDebugflModulationTime("Audio.EFX.Reverb.flModulationTime",	4.0f,	Debug::Range(0.f, 10.f));
Debug::Float g_AudioDebugflModulationDepth("Audio.EFX.Reverb.flModulationDepth",	1.0f,	Debug::Range(0.f, 10.f));
Debug::Float g_AudioDebugflAirAbsorptionGainHF("Audio.EFX.Reverb.flAirAbsorptionGainHF",	0.0994f,	Debug::Range(0.f, 10.f));
Debug::Float g_AudioDebugflHFReference("Audio.EFX.Reverb.flHFReference",	5000.0f,	Debug::Range(0.f, 10000.f));
Debug::Float g_AudioDebugflLFReference("Audio.EFX.Reverb.flLFReference",	250.0f,	Debug::Range(0.f, 1000.f));
Debug::Float g_AudioDebugflRoomRolloffFactor("Audio.EFX.Reverb.flRoomRolloffFactor",	0.0f,	Debug::Range(0.f, 10.f));
Debug::Int   g_AudioDebugiDecayHFLimit("Audio.EFX.Reverb.iDecayHFLimit",	0,	Debug::Range(0, 5));
#endif USE_EAX_REVERB	
//#endif

#ifdef USE_OPENAL
ALenum Audio::Core::Engine::m_eBufferFormat = 0;
ALuint Audio::Core::Engine::m_source = 0;
ALuint Audio::Core::Engine::m_aBuffers[NUM_BUFFERS];
ALCcontext* Audio::Core::Engine::m_pContext = NULL;
ALCdevice* Audio::Core::Engine::m_pDevice = NULL;
S16* Audio::Core::Engine::m_pData = NULL;
//S16 Audio::Core::Engine::fCurr = 0;

#ifdef USE_EAX_REVERB	
ALenum Audio::Core::Engine::m_eReverbBufferFormat = 0;
ALuint Audio::Core::Engine::m_aReverbBuffers[NUM_BUFFERS];
S16* Audio::Core::Engine::m_pReverbData = NULL;

EFXEAXREVERBPROPERTIES Audio::Core::Engine::efxReverb;
ALuint		Audio::Core::Engine::m_uiEffectSlot = NULL, Audio::Core::Engine::m_uiEffect = NULL;
//int Audio::Core::Engine::m_sawCount = 0;
ALboolean Audio::Core::Engine::m_bEffectCreated = AL_FALSE;

ALuint Audio::Core::Engine::m_sourceReverbVoice = 0;
ALuint Audio::Core::Engine::m_sourceFilter = 0;
// Imported EFX functions

// Effect objects
LPALGENEFFECTS alGenEffects = NULL;
LPALDELETEEFFECTS alDeleteEffects = NULL;
LPALISEFFECT alIsEffect = NULL;
LPALEFFECTI alEffecti = NULL;
LPALEFFECTIV alEffectiv = NULL;
LPALEFFECTF alEffectf = NULL;
LPALEFFECTFV alEffectfv = NULL;
LPALGETEFFECTI alGetEffecti = NULL;
LPALGETEFFECTIV alGetEffectiv = NULL;
LPALGETEFFECTF alGetEffectf = NULL;
LPALGETEFFECTFV alGetEffectfv = NULL;

//Filter objects
LPALGENFILTERS alGenFilters = NULL;
LPALDELETEFILTERS alDeleteFilters = NULL;
LPALISFILTER alIsFilter = NULL;
LPALFILTERI alFilteri = NULL;
LPALFILTERIV alFilteriv = NULL;
LPALFILTERF alFilterf = NULL;
LPALFILTERFV alFilterfv = NULL;
LPALGETFILTERI alGetFilteri = NULL;
LPALGETFILTERIV alGetFilteriv = NULL;
LPALGETFILTERF alGetFilterf = NULL;
LPALGETFILTERFV alGetFilterfv = NULL;

// Auxiliary slot object
LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots = NULL;
LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots = NULL;
LPALISAUXILIARYEFFECTSLOT alIsAuxiliaryEffectSlot = NULL;
LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti = NULL;
LPALAUXILIARYEFFECTSLOTIV alAuxiliaryEffectSlotiv = NULL;
LPALAUXILIARYEFFECTSLOTF alAuxiliaryEffectSlotf = NULL;
LPALAUXILIARYEFFECTSLOTFV alAuxiliaryEffectSlotfv = NULL;
LPALGETAUXILIARYEFFECTSLOTI alGetAuxiliaryEffectSloti = NULL;
LPALGETAUXILIARYEFFECTSLOTIV alGetAuxiliaryEffectSlotiv = NULL;
LPALGETAUXILIARYEFFECTSLOTF alGetAuxiliaryEffectSlotf = NULL;
LPALGETAUXILIARYEFFECTSLOTFV alGetAuxiliaryEffectSlotfv = NULL;

// XRAM functions and enum values

LPEAXSETBUFFERMODE eaxSetBufferMode = NULL;
LPEAXGETBUFFERMODE eaxGetBufferMode = NULL;

ALenum eXRAMSize = 0;
ALenum eXRAMFree = 0;
ALenum eXRAMAuto = 0;
ALenum eXRAMHardware = 0;
ALenum eXRAMAccessible = 0;


EAXREVERBPROPERTIES eaxReverbPreset = REVERB_PRESET_GENERIC;

#endif //use eax reverb
#endif

Audio::Core::Engine Audio::Core::Engine::s_instance;

DWORD Audio::Core::Engine::ThreadProc(LPVOID lpThreadParameter)
{
#ifdef USE_OPENAL
	int iTotalBuffersProcessed = 0;
	int iTotalReverbBuffersProcessed = 0;
#endif

#ifdef PLATFORM_PC
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif

	while (1)
	{
#ifdef USE_OPENAL

		Sleep(5);

		// Request the number of OpenAL Buffers have been processed (played) on the Source
		int iBuffersProcessed = 0;
		alGetSourcei(m_source, AL_BUFFERS_PROCESSED, &iBuffersProcessed);
		
		// Keep a running count of number of buffers processed (for logging purposes only)
		iTotalBuffersProcessed += iBuffersProcessed;
		//AUDIO_CORE_DEBUG("Buffers Processed %d", iTotalBuffersProcessed);

		// For each processed buffer, remove it from the Source Queue, read next chunk of audio
		// data from disk, fill buffer with new data, and add it to the Source Queue
		while (iBuffersProcessed)
		{
			// Remove the Buffer from the Queue.  (uiBuffer contains the Buffer ID for the unqueued Buffer)
			ALuint uiBuffer = 0;
			alSourceUnqueueBuffers(m_source, 1, &uiBuffer);

			GetNextBuffer(m_pData);

			alBufferData(uiBuffer, m_eBufferFormat, m_pData, 6 * Core::uFrameLength * sizeof(S16), Core::uSampleRate);
			alSourceQueueBuffers(m_source, 1, &uiBuffer);

			iBuffersProcessed--;
		}

#ifdef USE_EAX_REVERB	
		//reverb **************************************
		// Request the number of OpenAL Buffers have been processed (played) on the Source
		int iReverbBuffersProcessed = 0;
		alGetSourcei(m_sourceReverbVoice, AL_BUFFERS_PROCESSED, &iReverbBuffersProcessed);
		
		// Keep a running count of number of buffers processed (for logging purposes only)
		iTotalReverbBuffersProcessed += iReverbBuffersProcessed;
		//AUDIO_CORE_DEBUG("Buffers Processed %d", iTotalBuffersProcessed);

		// For each processed buffer, remove it from the Source Queue, read next chunk of audio
		// data from disk, fill buffer with new data, and add it to the Source Queue
		while (iReverbBuffersProcessed)
		{
			// Remove the Buffer from the Queue.  (uiBuffer contains the Buffer ID for the unqueued Buffer)
			ALuint uiReverbBuffer = 0;
			alSourceUnqueueBuffers(m_sourceReverbVoice, 1, &uiReverbBuffer);

			////if(m_sawCount==425)
			//	m_sawCount = 0;

			////get reverb buffers
			GetNextReverbBuffer(m_pReverbData);

			alBufferData(uiReverbBuffer, m_eReverbBufferFormat, m_pReverbData, Core::uFrameLength * sizeof(S16), Core::uSampleRate);
			alSourceQueueBuffers(m_sourceReverbVoice, 1, &uiReverbBuffer);
			//
		//	m_sawCount++;

			iReverbBuffersProcessed--;
		}


		
		//[CIAN] NB these parameters are for testing only,and access EFX using new parameter format, the game uses
		//the parameters in GameReverb as before...
		if(g_AudioDebugSetReverbParams)
		{

			efxReverb.flDensity = 										   g_AudioDebugflDensity;
			efxReverb.flDiffusion = 								   g_AudioDebugflDiffusion;
			efxReverb.flGain = 										   g_AudioDebugflGain;
			efxReverb.flGainHF = 									   g_AudioDebugflGainHF;
			efxReverb.flGainLF = 									   g_AudioDebugflGainLF;
			efxReverb.flDecayTime = 								   g_AudioDebugflDecayTime;
			efxReverb.flDecayHFRatio =								   g_AudioDebugflDecayHFRatio;
			efxReverb.flDecayLFRatio =									g_AudioDebugflDecayLFRatio;
			efxReverb.flReflectionsGain =								g_AudioDebugflReflectionsGain;
			efxReverb.flReflectionsDelay =								g_AudioDebugflReflectionsDelay;
			efxReverb.flLateReverbGain =								 g_AudioDebugflLateReverbGain;
			efxReverb.flLateReverbDelay		=							 g_AudioDebugflLateReverbDelay;
			efxReverb.flEchoTime 		=								g_AudioDebugflEchoTime;
			efxReverb.flEchoDepth 		=								 g_AudioDebugflEchoDepth;
			efxReverb.flModulationTime 	=							 g_AudioDebugflModulationTime;
			efxReverb.flModulationDepth	=							g_AudioDebugflModulationDepth;
			efxReverb.flAirAbsorptionGainHF=						 g_AudioDebugflAirAbsorptionGainHF;
			efxReverb.flHFReference 	=							g_AudioDebugflHFReference;
			efxReverb.flLFReference		=							g_AudioDebugflLFReference;
			efxReverb.flRoomRolloffFactor= 							g_AudioDebugflRoomRolloffFactor;
			efxReverb.iDecayHFLimit	=								 g_AudioDebugiDecayHFLimit;

			//if(g_AudioDebugUseLegacyReverbParams)
			//{
			//	// reverb options************************
			//	// Reverb Preset is stored in legacy format, use helper function to convert to EFX EAX Reverb
			//	ConvertReverbParameters(&eaxReverbPreset, &efxReverb);
			//}

			// Set the Effect parameters
			if (!SetEFXEAXReverbProperties(&efxReverb, m_uiEffect))
				AUDIO_CORE_DEBUG("Failed to set Reverb Parameters");
			//END REVERB OPTIONS **************

			//Load Effect into Auxiliary Effect Slot
			alAuxiliaryEffectSloti(m_uiEffectSlot, AL_EFFECTSLOT_EFFECT, m_uiEffect);
		}
#endif //USE_EAX_REVERB	

		// Check the status of the Source.  If it is not playing, then playback was either completed,
		// or the Source was starved of audio data and needs to be restarted.
		ALint iState;
		alGetSourcei(m_source, AL_SOURCE_STATE, &iState);
		if (iState != AL_PLAYING)
		{
			// If there are Buffers in the Source Queue then the Source was starved of audio
			// data, so needs to be restarted (because there is more audio data to play)
			ALint iQueuedBuffers;
			alGetSourcei(m_source, AL_BUFFERS_QUEUED, &iQueuedBuffers);
			if (iQueuedBuffers)
			{
				AUDIO_CORE_DEBUG("Dropped", iTotalBuffersProcessed);
				alSourcePlay(m_source);
			}
			else
			{
				// Finished
				break;
			}
		}

#ifdef USE_EAX_REVERB			
		ALint iStateReverbVoice;
		alGetSourcei(m_sourceReverbVoice, AL_SOURCE_STATE, &iStateReverbVoice);
		if (iStateReverbVoice != AL_PLAYING)
		{
			// If there are Buffers in the Source Queue then the Source was starved of audio
			// data, so needs to be restarted (because there is more audio data to play)
			ALint iQueuedBuffersReverbVoice;
			alGetSourcei(m_sourceReverbVoice, AL_BUFFERS_QUEUED, &iQueuedBuffersReverbVoice);
			ALenum erro = alGetError();
			if (iQueuedBuffersReverbVoice)
			{
				AUDIO_CORE_DEBUG("Dropped Reverb Buffer", iTotalReverbBuffersProcessed);
				alSourcePlay(m_sourceReverbVoice);
			}
			else
			{
				// Finished
				break;
			}
		}
#endif
#else // USE_OPENAL

		WaitForSingleObject(g_ne, INFINITE);

		//OutputDebugString("Notify trigger...\n");

		static U32 frameCount(0);

		S32 lag(0);

go_again:
		const bool paused = Audio::System::Get()->IsPaused();

		//F32* inputBuffer = (!paused) ? Audio::Core::Engine::Instance().GetRenderer()->GetNextFrame() : NULL;

		//	dry
		F32* pfMono = NULL;
		F32* pfStereo = NULL;
		F32* pfSurround = NULL;
		F32* pfSurroundLFE = NULL;

		//	get render buffers
		Audio::Core::Engine::Instance().GetRenderer()->GetNextFrame(&pfSurroundLFE, &pfSurround, &pfStereo, &pfMono);

		//Audio::Core::Engine::Instance().PrimarySound()->HandleWaveStreamNotification(inputBuffer, uFrameLength, 6, lag);
/*
		if (!paused)
		{
			Audio::Core::Engine::Instance().GetRenderer()->Process(frameCount++, true);
		}

		//	render specified outputs
		if (Audio::Core::Engine::Instance().GetRenderer()->SurroundInput())
		{
			F32* inputBuffer = (!paused) ? Audio::Core::Engine::Instance().GetRenderer()->SurroundInput()->FrameData() : NULL;

			Audio::Core::Engine::Instance().PrimarySound()->HandleWaveStreamNotification(inputBuffer, uFrameLength, 6, lag);
			Audio::Core::Engine::Instance().GetRenderer()->SurroundInput()->Clear();
		}
		else if (Audio::Core::Engine::Instance().GetRenderer()->StereoInput())
		{
			F32* inputBuffer = (!paused) ? Audio::Core::Engine::Instance().GetRenderer()->StereoInput()->FrameData() : NULL;

			Audio::Core::Engine::Instance().PrimarySound()->HandleWaveStreamNotification(inputBuffer, uFrameLength, 2, lag);
			Audio::Core::Engine::Instance().GetRenderer()->StereoInput()->Clear();
		}
		else if (Audio::Core::Engine::Instance().GetRenderer()->MonoInput())
		{
			F32* inputBuffer = (!paused) ? Audio::Core::Engine::Instance().GetRenderer()->MonoInput()->FrameData() : NULL;

			Audio::Core::Engine::Instance().PrimarySound()->HandleWaveStreamNotification(inputBuffer, uFrameLength, 1, lag);
			Audio::Core::Engine::Instance().GetRenderer()->MonoInput()->Clear();
		}
*/

		if (lag > 1)
		{
			if (!paused)
				lag = -1;
			else
				lag--;

			goto go_again;
		}

#endif // USE_OPENAL
	}

	return 0;
}

Audio::Core::Engine::Engine() :
#ifndef USE_OPENAL
	m_pPrimarySound(NULL),
#endif
	m_pRenderer(NULL)
{
}

#ifndef USE_OPENAL
Audio::Core::Error Audio::Core::Engine::SetupDirectSound()
{
	LPDIRECTSOUNDBUFFER pDSBPrimary = NULL;

	//	create DSound
	HRESULT hr = DirectSoundCreate8(NULL, &m_pDS, NULL);
	if (hr != S_OK)
		return eFailedToCreateDirectSound;

	//	set co-op level
	hr = m_pDS->SetCooperativeLevel(Audio::System::Get()->GetHWND(), DSSCL_PRIORITY);
	if (hr != S_OK)
		return eFailedToSetCoopLevel;

	// get the primary buffer 
	DSBUFFERDESC dsbd;
	ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));
	dsbd.dwSize = sizeof(DSBUFFERDESC);
	dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
	dsbd.dwBufferBytes = 0;
	dsbd.lpwfxFormat = NULL;

	hr = m_pDS->CreateSoundBuffer(&dsbd, &pDSBPrimary, NULL);
	if (hr != S_OK)
		return eFailedToCreatePrimaryBuffer;

	WAVEFORMATEX wfx;
	ZeroMemory(&wfx, sizeof(WAVEFORMATEX)); 
	wfx.wFormatTag = (WORD)WAVE_FORMAT_PCM; 
	wfx.nChannels = (WORD)2; 
	wfx.nSamplesPerSec = (DWORD)48000; 
	wfx.wBitsPerSample = (WORD)16; 
	wfx.nBlockAlign = (WORD)(wfx.wBitsPerSample / 8 * wfx.nChannels);
	wfx.nAvgBytesPerSec = (DWORD)(wfx.nSamplesPerSec * wfx.nBlockAlign);

	hr = pDSBPrimary->SetFormat(&wfx);
	if (hr != S_OK)
		return eFailedToSetPrimaryBufferFormat;

	pDSBPrimary->Release();

	return eSuccess;
}
#endif // USE_OPENAL

Audio::Core::Engine::~Engine()
{
}

bool Audio::Core::Engine::Init(Audio::Core::Renderer* pRenderer)
{
	m_pRenderer = pRenderer;

#ifdef USE_OPENAL

	ALenum error;
	m_pContext = NULL;
	m_pDevice = alcOpenDevice(NULL);

	ALchar* pString = (ALchar*)(alcGetString(NULL, ALC_DEVICE_SPECIFIER));

	if (pString)
	{
		while (*(pString) != 0)
		{
			AUDIO_CORE_DEBUG("Found audio device- %s", pString);

			while (*(pString) != 0)
				pString = &(pString[1]);

			pString = &(pString[1]);
		}
	}

	if (m_pDevice)
	{
		AUDIO_CORE_DEBUG("Opened %s audio device", alcGetString(m_pDevice, ALC_DEVICE_SPECIFIER));

		m_pContext = alcCreateContext(m_pDevice, NULL);
		if (m_pContext)
		{
			AUDIO_CORE_DEBUG("Created audio context");

			alcMakeContextCurrent(m_pContext);

			ALboolean bEAX = alIsExtensionPresent("EAX2.0");
			AUDIO_CORE_DEBUG("EAX2 %s supported", bEAX ? "is" : "is not");

			m_eBufferFormat = alGetEnumValue("AL_FORMAT_51CHN16");
			if (!m_eBufferFormat)
			{
				AUDIO_CORE_ERROR("No support for 5.1 playback!");
				goto failed;
			}

#ifdef USE_EAX_REVERB	
			m_eReverbBufferFormat = alGetEnumValue("AL_FORMAT_MONO16");
			if (!m_eReverbBufferFormat)
			{
				AUDIO_CORE_ERROR("No support for mono playback!");
				goto failed;
			}
#endif //USE_EAX_REVERB	

			// clear error code
			alGetError();

			// Generate buffers + sources
			alGenBuffers(NUM_BUFFERS, m_aBuffers);
			if ((error = alGetError()) != AL_NO_ERROR)
			{
				AUDIO_CORE_ERROR("alGenBuffers : %d", error);
				goto failed;
			}
			alGenSources(1, &m_source);
			if ((error = alGetError()) != AL_NO_ERROR)
			{
				AUDIO_CORE_ERROR("alGenSources : %d", error);
				goto failed;
			}

#ifdef USE_EAX_REVERB	
			//*******REVERB INIT			
			alGenBuffers(NUM_BUFFERS, m_aReverbBuffers);
			if ((error = alGetError()) != AL_NO_ERROR)
			{
				AUDIO_CORE_ERROR("alGenBuffers : %d", error);
				goto failed;
			}

			alGenSources(1, &m_sourceReverbVoice);
			if ((error = alGetError()) != AL_NO_ERROR)
			{
				AUDIO_CORE_ERROR("alGenSourcesReverbVoice : %d", error);
				goto failed;
			}
#endif// USE_EAX_REVERB	
			////********

			U32 uBufferSize = 6 * Core::uFrameLength * sizeof(S16);
			m_pData = (S16*)malloc(uBufferSize);
			memset(m_pData, 0, uBufferSize);

#ifdef USE_EAX_REVERB	
			//*****************************REVERB BUFFERS
			U32 uReverbBufferSize = Core::uFrameLength * sizeof(S16);
			m_pReverbData = (S16*)malloc(uReverbBufferSize);
			memset(m_pReverbData,0,uReverbBufferSize);
			//*****************************************
#endif

			// queue initial data
			int iLoop = 0;
			for (iLoop = 0; iLoop < NUM_BUFFERS; iLoop++)
			{
				alBufferData(m_aBuffers[iLoop], m_eBufferFormat, m_pData, 6 * Core::uFrameLength * sizeof(S16), Core::uSampleRate);
				alSourceQueueBuffers(m_source, 1, &m_aBuffers[iLoop]);
				if (alGetError() != AL_NO_ERROR)
					break;
			}

#ifdef USE_EAX_REVERB	
			//************** queue initial reverb data
			int iReverbLoop = 0;
			for (iReverbLoop = 0; iReverbLoop < NUM_BUFFERS; iReverbLoop++)
			{
				
				alBufferData(m_aReverbBuffers[iReverbLoop], m_eReverbBufferFormat, m_pReverbData, Core::uFrameLength * sizeof(S16), Core::uSampleRate);
				ALenum erro = alGetError();
				alSourceQueueBuffers(m_sourceReverbVoice, 1, &m_aReverbBuffers[iReverbLoop]);
				ALenum err = alGetError();
				if (alGetError() != AL_NO_ERROR)
					break;
			}
			
			// Check for EFX Extension and create the effect slot and effect itself
			if (ALFWIsEFXSupported())
			{
				AUDIO_CORE_DEBUG("EFX SUPPORTED\n");
				if (CreateAuxEffectSlot(&m_uiEffectSlot))
				{
				//	// Once we have an Auxiliary Effect Slot, we can generate an Effect Object, set its Type
				//	// and Parameter Values, and then load the Effect into the Auxiliary Effect Slot ...
				if (CreateEffect(&m_uiEffect, AL_EFFECT_EAXREVERB))
					{
						Audio::Core::Engine::m_bEffectCreated = AL_TRUE;
					}
					else
					{
						AUDIO_CORE_DEBUG("Failed to Create an EAX Reverb Effect\n");
					}
				}
				else
				{
					AUDIO_CORE_DEBUG("Failed to generate an Auxilary Effect Slot\n");
				}
			}
#endif //USE_EAX_REVERB	
			//**********************

			if (iLoop != NUM_BUFFERS)
				AUDIO_CORE_ERROR("Failed to attach buffers to source");
			else
				alSourcePlay(m_source);

#ifdef USE_EAX_REVERB	
			////play reverb source
			if (iReverbLoop != NUM_BUFFERS)
				AUDIO_CORE_ERROR("Failed to attach reverb buffers to source");
			else
			{
				alSourcePlay(m_sourceReverbVoice);
				static float vSourceVoicePos[3] = {-1000.f, 0.f, 0.f };
				alSourcefv(m_sourceReverbVoice, AL_POSITION, vSourceVoicePos);
				alAuxiliaryEffectSloti(m_uiEffect, AL_EFFECTSLOT_AUXILIARY_SEND_AUTO, AL_FALSE);

				alGenFilters(1, &m_sourceFilter);

				if (alIsFilter(m_sourceFilter))
				{
					/* Set Filter type to Low-Pass and set parameters */
					alFilteri(m_sourceFilter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
					if (alGetError() == AL_NO_ERROR)
					{
						alFilterf(m_sourceFilter, AL_LOWPASS_GAIN, 0.0f);
						alFilterf(m_sourceFilter, AL_LOWPASS_GAINHF, 0.0f);

						alSourcei(m_sourceReverbVoice, AL_DIRECT_FILTER, m_sourceFilter);
					}
				}


			}

			//*************get the reverb parameter and load the effect into an aux slot
			if (Audio::Core::Engine::m_bEffectCreated)
			{
				AUDIO_CORE_DEBUG("Playing Source with Send to Reverb Effect\n");
				
				// Reverb Preset is stored in legacy format, use helper function to convert to EFX EAX Reverb
				ConvertReverbParameters(&eaxReverbPreset, &efxReverb);
					
				//// Set the Effect parameters
				if (!SetEFXEAXReverbProperties(&efxReverb, m_uiEffect))
					AUDIO_CORE_DEBUG("Failed to set Reverb Parameters\n");

				//// Load Effect into Auxiliary Effect Slot
				alAuxiliaryEffectSloti(m_uiEffectSlot, AL_EFFECTSLOT_EFFECT, m_uiEffect);

				alSource3i(m_sourceReverbVoice, AL_AUXILIARY_SEND_FILTER, m_uiEffectSlot, 0, AL_FILTER_NULL);
			}
#endif //USE_EAX_REVERB	
			///************************
		}
		else
		{
			AUDIO_CORE_ERROR("Could not create audio context");
			alcCloseDevice(m_pDevice);
			return false;
		}
	}
	else
	{
		AUDIO_CORE_ERROR("Could not open audio device");
		return false;
	}

	m_notificationThread = CreateThread(NULL, 16*1024, LPTHREAD_START_ROUTINE(ThreadProc), NULL, 0, NULL);
	SetThreadPriority(m_notificationThread, THREAD_PRIORITY_HIGHEST); // THREAD_PRIORITY_TIME_CRITICAL

	return true;

failed:

	alcMakeContextCurrent(NULL);
	alcDestroyContext(m_pContext);
	alcCloseDevice(m_pDevice);
	return false;

#else // USE_OPENAL

	g_ne = m_notificationEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	SetupDirectSound();

	m_notificationThread = CreateThread(NULL, 16*1024, LPTHREAD_START_ROUTINE(ThreadProc), NULL, 0, NULL);
	SetThreadPriority(m_notificationThread, THREAD_PRIORITY_HIGHEST); // THREAD_PRIORITY_TIME_CRITICAL
	//SetThreadPriority(m_notificationThread, THREAD_PRIORITY_TIME_CRITICAL);

	DWORD dwNotifySize = Audio::Core::uFrameLength * 2 * 2; // Frame::Size() * 2 * sizeof(S16);

	HRESULT hr = CreatePrimarySoundBuffer(&m_pPrimarySound,
		DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2, 
		GUID_NULL, 16, 
		dwNotifySize, m_notificationEvent);

	m_pPrimarySound->Play(0, DSBPLAY_LOOPING);

	return true;
#endif
}

#ifdef USE_OPENAL
void Audio::Core::Engine::GetNextBuffer(S16* pData)
{
	F32* pSurroundLFE = NULL;
	F32* pSurround = NULL;
	F32* pStereo = NULL;
	F32* pMono = NULL;

	//	[TODO] non 6-channel renderers
	Audio::Core::Engine::Instance().GetRenderer()->GetNextFrame(&pSurroundLFE, &pSurround, &pStereo, &pMono);

	if (! Audio::Core::Engine::Instance().GetRenderer()->IsOutputMuted())
	{
		//	clear it
		for ( U32 uCount = 0; uCount < 6 * Core::uFrameLength; uCount++ )
			pData[ uCount ] = 0;

		if (pMono)
		{
			for (int j=0; j<Core::uFrameLength; j++)
				pData[j * 6 + 2] += (S16)(16384.f * Maths::Clamp(pMono[j], -1.f, 1.f)); // C
		}
		if (pStereo)
		{
			for (int j=0; j<Core::uFrameLength; j++)
			{
				pData[j * 6] += (S16)(16384.f * Maths::Clamp(pStereo[j], -1.f, 1.f)); // L
				pData[j * 6 + 1] += (S16)(16384.f * Maths::Clamp(pStereo[j + 1], -1.f, 1.f)); // R
			}
		}
		if (pSurround)
		{
			for (int j=0; j<Core::uFrameLength; j++)
			{
				pData[j * 6] += (S16)(16384.f * Maths::Clamp(pSurroundLFE[j], -1.f, 1.f)); // L
				pData[j * 6 + 1] += (S16)(16384.f * Maths::Clamp(pSurroundLFE[Core::uFrameLength + j], -1.f, 1.f)); // R
				pData[j * 6 + 2] += (S16)(16384.f * Maths::Clamp(pSurroundLFE[2 *Core::uFrameLength + j], -1.f, 1.f)); // C
				pData[j * 6 + 4] += (S16)(16384.f * Maths::Clamp(pSurroundLFE[3 *Core::uFrameLength + j], -1.f, 1.f)); // SL
				pData[j * 6 + 5] += (S16)(16384.f * Maths::Clamp(pSurroundLFE[4 *Core::uFrameLength + j], -1.f, 1.f)); // SR
			}
		}
		if (pSurroundLFE)
		{
			for (int j=0; j<Core::uFrameLength; j++)
			{
				pData[j * 6] += (S16)(16384.f * Maths::Clamp(pSurroundLFE[j], -1.f, 1.f)); // L
				pData[j * 6 + 1] += (S16)(16384.f * Maths::Clamp(pSurroundLFE[Core::uFrameLength + j], -1.f, 1.f)); // R
				pData[j * 6 + 2] += (S16)(16384.f * Maths::Clamp(pSurroundLFE[2 *Core::uFrameLength + j], -1.f, 1.f)); // C
				pData[j * 6 + 3] += (S16)(16384.f * Maths::Clamp(pSurroundLFE[3 *Core::uFrameLength + j], -1.f, 1.f)); // LFE
				pData[j * 6 + 4] += (S16)(16384.f * Maths::Clamp(pSurroundLFE[4 *Core::uFrameLength + j], -1.f, 1.f)); // SL
				pData[j * 6 + 5] += (S16)(16384.f * Maths::Clamp(pSurroundLFE[5 *Core::uFrameLength + j], -1.f, 1.f)); // SR
			}
		}
	}
	else
	{
		//if (pSurroundLFE)
		{
			for (int j=0; j<Core::uFrameLength; j++)
			{
				pData[j * 6] = 0;
				pData[j * 6 + 1] = 0;
				pData[j * 6 + 2] = 0;
				pData[j * 6 + 3] = 0;
				pData[j * 6 + 4] = 0;
				pData[j * 6 + 5] = 0;
			}
		}
	}
}
#ifdef USE_EAX_REVERB	
Debug::Float g_DebugReverbVoiceLevel("Audio.System.ReverbVoiceLevel", 1.f, Debug::Range( 0.0f, 10.0f ));
Debug::Float g_DebugReverbAuxEffectLevel("Audio.System.ReverbAuxEffectLevel", 1.f, Debug::Range( 0.0f, 10.0f ));
Debug::Float g_DebugSourceDistanceOffset("Audio.System.SourceDistanceOffset", 1.f, Debug::Range( 0.0f, 1000.f));
Debug::Bool g_DebugAuxSendAuto("Audio.System.ReverbAuxSendAuto", false);
Debug::Float g_DebugFilterLowPassGain("Audio.System.DrySourceLowPassGain", 0.5f, Debug::Range(0.f, 1.f));
Debug::Float g_DebugFilterLowPassGainHF("Audio.System.DrySourceLowPassGainHF", 0.5f, Debug::Range(0.f, 1.f));
void Audio::Core::Engine::GetNextReverbBuffer(S16* pData)
{	
	alSourcef(m_sourceReverbVoice, AL_GAIN, g_DebugReverbVoiceLevel);
	alAuxiliaryEffectSlotf(m_uiEffectSlot, AL_EFFECTSLOT_GAIN, g_DebugReverbAuxEffectLevel);

	static float vSourceVoicePos2[3] = {g_DebugSourceDistanceOffset, 0.f, 0.f };
	alSourcefv(m_sourceReverbVoice, AL_POSITION, vSourceVoicePos2);
	alAuxiliaryEffectSloti(m_uiEffect, AL_EFFECTSLOT_AUXILIARY_SEND_AUTO, (g_DebugAuxSendAuto) ? AL_TRUE : AL_FALSE);

	if (alIsFilter(m_sourceFilter))
	{
		/* Set Filter type to Low-Pass and set parameters */
		alFilterf(m_sourceFilter, AL_LOWPASS_GAIN, g_DebugFilterLowPassGain);
		alFilterf(m_sourceFilter, AL_LOWPASS_GAINHF, g_DebugFilterLowPassGainHF);
	}

	F32* pReverb = NULL;

	//	[TODO] non 6-channel renderers
	Audio::Core::Engine::Instance().GetRenderer()->GetNextReverbFrame(&pReverb,0);

	if (pReverb)
	{
		if (! Audio::Core::Engine::Instance().GetRenderer()->IsOutputMuted())
		{
			for (int j=0; j<Core::uFrameLength; j++)
			{
				pData[j] = (S16)(32767.f * Maths::Clamp(pReverb[j], -1.f, 1.f));
			}
		}
		else
		{
			for (int j=0; j<Core::uFrameLength; j++)
			{
				pData[j] = 0;
			}
		}
	}
}

ALboolean Audio::Core::Engine::CreateAuxEffectSlot(ALuint *puiAuxEffectSlot)
{
	ALboolean bReturn = AL_FALSE;

	// Clear AL Error state
	alGetError();

	// Generate an Auxiliary Effect Slot
	alGenAuxiliaryEffectSlots(1, puiAuxEffectSlot);
	if (alGetError() == AL_NO_ERROR)
		bReturn = AL_TRUE;

	return bReturn;
}

ALboolean Audio::Core::Engine::CreateEffect(ALuint *puiEffect, ALenum eEffectType)
{
	ALboolean bReturn = AL_FALSE;

	if (puiEffect)
	{
		// Clear AL Error State
		alGetError();

		// Generate an Effect
		alGenEffects(1, puiEffect);
		if (alGetError() == AL_NO_ERROR)
		{

			// Set the Effect Type
			alEffecti(*puiEffect, AL_EFFECT_TYPE, eEffectType);
			if (alGetError() == AL_NO_ERROR)
				bReturn = AL_TRUE;
			else
				alDeleteEffects(1, puiEffect);
		}
	}

	return bReturn;
}

ALboolean Audio::Core::Engine::SetEFXEAXReverbProperties(EFXEAXREVERBPROPERTIES *pEFXEAXReverb, ALuint uiEffect)
{
	ALboolean bReturn = AL_FALSE;

	if (pEFXEAXReverb)
	{
		// Clear AL Error code
		alGetError();

		alEffectf(uiEffect, AL_EAXREVERB_DENSITY, pEFXEAXReverb->flDensity);
		alEffectf(uiEffect, AL_EAXREVERB_DIFFUSION, pEFXEAXReverb->flDiffusion);
		alEffectf(uiEffect, AL_EAXREVERB_GAIN, pEFXEAXReverb->flGain);
		alEffectf(uiEffect, AL_EAXREVERB_GAINHF, pEFXEAXReverb->flGainHF);
		alEffectf(uiEffect, AL_EAXREVERB_GAINLF, pEFXEAXReverb->flGainLF);
		alEffectf(uiEffect, AL_EAXREVERB_DECAY_TIME, pEFXEAXReverb->flDecayTime);
		alEffectf(uiEffect, AL_EAXREVERB_DECAY_HFRATIO, pEFXEAXReverb->flDecayHFRatio);
		alEffectf(uiEffect, AL_EAXREVERB_DECAY_LFRATIO, pEFXEAXReverb->flDecayLFRatio);
		alEffectf(uiEffect, AL_EAXREVERB_REFLECTIONS_GAIN, pEFXEAXReverb->flReflectionsGain);
		alEffectf(uiEffect, AL_EAXREVERB_REFLECTIONS_DELAY, pEFXEAXReverb->flReflectionsDelay);
		alEffectfv(uiEffect, AL_EAXREVERB_REFLECTIONS_PAN, pEFXEAXReverb->flReflectionsPan);
		alEffectf(uiEffect, AL_EAXREVERB_LATE_REVERB_GAIN, pEFXEAXReverb->flLateReverbGain);
		alEffectf(uiEffect, AL_EAXREVERB_LATE_REVERB_DELAY, pEFXEAXReverb->flLateReverbDelay);
		alEffectfv(uiEffect, AL_EAXREVERB_LATE_REVERB_PAN, pEFXEAXReverb->flLateReverbPan);
		alEffectf(uiEffect, AL_EAXREVERB_ECHO_TIME, pEFXEAXReverb->flEchoTime);
		alEffectf(uiEffect, AL_EAXREVERB_ECHO_DEPTH, pEFXEAXReverb->flEchoDepth);
		alEffectf(uiEffect, AL_EAXREVERB_MODULATION_TIME, pEFXEAXReverb->flModulationTime);
		alEffectf(uiEffect, AL_EAXREVERB_MODULATION_DEPTH, pEFXEAXReverb->flModulationDepth);
		alEffectf(uiEffect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, pEFXEAXReverb->flAirAbsorptionGainHF);
		alEffectf(uiEffect, AL_EAXREVERB_HFREFERENCE, pEFXEAXReverb->flHFReference);
		alEffectf(uiEffect, AL_EAXREVERB_LFREFERENCE, pEFXEAXReverb->flLFReference);
		alEffectf(uiEffect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, pEFXEAXReverb->flRoomRolloffFactor);
		alEffecti(uiEffect, AL_EAXREVERB_DECAY_HFLIMIT, pEFXEAXReverb->iDecayHFLimit);

		ALenum erro = alGetError();

		if (alGetError() == AL_NO_ERROR)
			bReturn = AL_TRUE;
	}

	return bReturn;
}

// Extension Queries

ALboolean Audio::Core::Engine::ALFWIsXRAMSupported()
{
	ALboolean bXRAM = AL_FALSE;
	
	if (alIsExtensionPresent("EAX-RAM") == AL_TRUE)
	{
		// Get X-RAM Function pointers
		eaxSetBufferMode = (EAXSetBufferMode)alGetProcAddress("EAXSetBufferMode");
		eaxGetBufferMode = (EAXGetBufferMode)alGetProcAddress("EAXGetBufferMode");

		if (eaxSetBufferMode && eaxGetBufferMode)
		{
			eXRAMSize = alGetEnumValue("AL_EAX_RAM_SIZE");
			eXRAMFree = alGetEnumValue("AL_EAX_RAM_FREE");
			eXRAMAuto = alGetEnumValue("AL_STORAGE_AUTOMATIC");
			eXRAMHardware = alGetEnumValue("AL_STORAGE_HARDWARE");
			eXRAMAccessible = alGetEnumValue("AL_STORAGE_ACCESSIBLE");

			if (eXRAMSize && eXRAMFree && eXRAMAuto && eXRAMHardware && eXRAMAccessible)
				bXRAM = AL_TRUE;
		}
	}

	return bXRAM;
}

ALboolean Audio::Core::Engine::ALFWIsEFXSupported()
{
	ALCdevice *pDevice = NULL;
	ALCcontext *pContext = NULL;
	ALboolean bEFXSupport = AL_FALSE;

	pContext = alcGetCurrentContext();
	pDevice = alcGetContextsDevice(pContext);

	if (alcIsExtensionPresent(pDevice, (ALCchar*)ALC_EXT_EFX_NAME))
	{
		// Get function pointers
		alGenEffects = (LPALGENEFFECTS)alGetProcAddress("alGenEffects");
		alDeleteEffects = (LPALDELETEEFFECTS )alGetProcAddress("alDeleteEffects");
		alIsEffect = (LPALISEFFECT )alGetProcAddress("alIsEffect");
		alEffecti = (LPALEFFECTI)alGetProcAddress("alEffecti");
		alEffectiv = (LPALEFFECTIV)alGetProcAddress("alEffectiv");
		alEffectf = (LPALEFFECTF)alGetProcAddress("alEffectf");
		alEffectfv = (LPALEFFECTFV)alGetProcAddress("alEffectfv");
		alGetEffecti = (LPALGETEFFECTI)alGetProcAddress("alGetEffecti");
		alGetEffectiv = (LPALGETEFFECTIV)alGetProcAddress("alGetEffectiv");
		alGetEffectf = (LPALGETEFFECTF)alGetProcAddress("alGetEffectf");
		alGetEffectfv = (LPALGETEFFECTFV)alGetProcAddress("alGetEffectfv");
		alGenFilters = (LPALGENFILTERS)alGetProcAddress("alGenFilters");
		alDeleteFilters = (LPALDELETEFILTERS)alGetProcAddress("alDeleteFilters");
		alIsFilter = (LPALISFILTER)alGetProcAddress("alIsFilter");
		alFilteri = (LPALFILTERI)alGetProcAddress("alFilteri");
		alFilteriv = (LPALFILTERIV)alGetProcAddress("alFilteriv");
		alFilterf = (LPALFILTERF)alGetProcAddress("alFilterf");
		alFilterfv = (LPALFILTERFV)alGetProcAddress("alFilterfv");
		alGetFilteri = (LPALGETFILTERI )alGetProcAddress("alGetFilteri");
		alGetFilteriv= (LPALGETFILTERIV )alGetProcAddress("alGetFilteriv");
		alGetFilterf = (LPALGETFILTERF )alGetProcAddress("alGetFilterf");
		alGetFilterfv= (LPALGETFILTERFV )alGetProcAddress("alGetFilterfv");
		alGenAuxiliaryEffectSlots = (LPALGENAUXILIARYEFFECTSLOTS)alGetProcAddress("alGenAuxiliaryEffectSlots");
		alDeleteAuxiliaryEffectSlots = (LPALDELETEAUXILIARYEFFECTSLOTS)alGetProcAddress("alDeleteAuxiliaryEffectSlots");
		alIsAuxiliaryEffectSlot = (LPALISAUXILIARYEFFECTSLOT)alGetProcAddress("alIsAuxiliaryEffectSlot");
		alAuxiliaryEffectSloti = (LPALAUXILIARYEFFECTSLOTI)alGetProcAddress("alAuxiliaryEffectSloti");
		alAuxiliaryEffectSlotiv = (LPALAUXILIARYEFFECTSLOTIV)alGetProcAddress("alAuxiliaryEffectSlotiv");
		alAuxiliaryEffectSlotf = (LPALAUXILIARYEFFECTSLOTF)alGetProcAddress("alAuxiliaryEffectSlotf");
		alAuxiliaryEffectSlotfv = (LPALAUXILIARYEFFECTSLOTFV)alGetProcAddress("alAuxiliaryEffectSlotfv");
		alGetAuxiliaryEffectSloti = (LPALGETAUXILIARYEFFECTSLOTI)alGetProcAddress("alGetAuxiliaryEffectSloti");
		alGetAuxiliaryEffectSlotiv = (LPALGETAUXILIARYEFFECTSLOTIV)alGetProcAddress("alGetAuxiliaryEffectSlotiv");
		alGetAuxiliaryEffectSlotf = (LPALGETAUXILIARYEFFECTSLOTF)alGetProcAddress("alGetAuxiliaryEffectSlotf");
		alGetAuxiliaryEffectSlotfv = (LPALGETAUXILIARYEFFECTSLOTFV)alGetProcAddress("alGetAuxiliaryEffectSlotfv");

		if (alGenEffects &&	alDeleteEffects && alIsEffect && alEffecti && alEffectiv &&	alEffectf &&
			alEffectfv && alGetEffecti && alGetEffectiv && alGetEffectf && alGetEffectfv &&	alGenFilters &&
			alDeleteFilters && alIsFilter && alFilteri && alFilteriv &&	alFilterf && alFilterfv &&
			alGetFilteri &&	alGetFilteriv && alGetFilterf && alGetFilterfv && alGenAuxiliaryEffectSlots &&
			alDeleteAuxiliaryEffectSlots &&	alIsAuxiliaryEffectSlot && alAuxiliaryEffectSloti &&
			alAuxiliaryEffectSlotiv && alAuxiliaryEffectSlotf && alAuxiliaryEffectSlotfv &&
			alGetAuxiliaryEffectSloti && alGetAuxiliaryEffectSlotiv && alGetAuxiliaryEffectSlotf &&
			alGetAuxiliaryEffectSlotfv)
			bEFXSupport = AL_TRUE;
	}

	return bEFXSupport;
}
#endif #ifdef USE_EAX_REVERB	
#endif

#ifndef USE_OPENAL
HRESULT Audio::Core::Engine::CreatePrimarySoundBuffer(
	CPrimarySound** ppPrimarySound, 
	DWORD dwCreationFlags, 
	GUID guid3DAlgorithm,
	DWORD dwNotifyCount, 
	DWORD dwNotifySize, 
	HANDLE hNotifyEvent)
{
    HRESULT hr;

    if( m_pDS == NULL )
        return CO_E_NOTINITIALIZED;
    if( ppPrimarySound == NULL || hNotifyEvent == NULL )
        return E_INVALIDARG;

    LPDIRECTSOUNDBUFFER pDSBuffer      = NULL;
    DWORD               dwDSBufferSize = NULL;
    DSBPOSITIONNOTIFY*  aPosNotify     = NULL; 
    LPDIRECTSOUNDNOTIFY pDSNotify      = NULL;

	// Figure out how big the DSound buffer should be 
    dwDSBufferSize = dwNotifySize * dwNotifyCount;

	WAVEFORMATEX wfx;
	memset(&wfx, 0, sizeof(WAVEFORMATEX)); 
	wfx.wFormatTag = WAVE_FORMAT_PCM; 
	wfx.nChannels = 2; 
	wfx.nSamplesPerSec = 48000; 
	wfx.nBlockAlign = 4; 
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign; 
	wfx.wBitsPerSample = 16; 

	// Set up the direct sound buffer.  Request the NOTIFY flag, so
    // that we are notified as the sound buffer plays.  Note, that using this flag
    // may limit the amount of hardware acceleration that can occur. 
    DSBUFFERDESC dsbd;
    ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
    dsbd.dwSize          = sizeof(DSBUFFERDESC);
    dsbd.dwFlags         = dwCreationFlags | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2;
    dsbd.dwBufferBytes   = dwDSBufferSize;
    dsbd.guid3DAlgorithm = guid3DAlgorithm;
	dsbd.lpwfxFormat     = &wfx; // m_pRenderer->GetWfxPtr();

    if( FAILED( hr = m_pDS->CreateSoundBuffer( &dsbd, &pDSBuffer, NULL ) ) )
    {
        // If wave format isn't then it will return 
        // either DSERR_BADFORMAT or E_INVALIDARG
        if( hr == DSERR_BADFORMAT || hr == E_INVALIDARG )
            return hr;//DXTRACE_ERR( TEXT("CreateSoundBuffer"), hr );

        return hr;//DXTRACE_ERR( TEXT("CreateSoundBuffer"), hr );
    }

    // Create the notification events, so that we know when to fill
    // the buffer as the sound plays. 
    if( FAILED( hr = pDSBuffer->QueryInterface( IID_IDirectSoundNotify, 
                                                (VOID**)&pDSNotify ) ) )
    {
        if( aPosNotify )
			delete [] aPosNotify;

        return hr;//DXTRACE_ERR( TEXT("QueryInterface"), hr );
    }

    aPosNotify = new DSBPOSITIONNOTIFY[ dwNotifyCount ];
    if( aPosNotify == NULL )
        return E_OUTOFMEMORY;

    for( DWORD i = 0; i < dwNotifyCount; i++ )
    {
        aPosNotify[i].dwOffset     = (dwNotifySize * i) + dwNotifySize - 1;
        aPosNotify[i].hEventNotify = hNotifyEvent;             
    }
    
    // Tell DirectSound when to notify us. The notification will come in the from 
    // of signaled events that are handled in WinMain()
    if (FAILED(hr = pDSNotify->SetNotificationPositions(dwNotifyCount, aPosNotify )))
    {
        if( pDSNotify )
		{
			pDSNotify->Release();
			pDSNotify = NULL;
		}
        if( aPosNotify )
		{
			delete [] aPosNotify;
			aPosNotify = NULL;
		}
        return hr;//DXTRACE_ERR( TEXT("SetNotificationPositions"), hr );
    }

    if( pDSNotify )
	{
		pDSNotify->Release();
		pDSNotify = NULL;
	}
    if( aPosNotify )
	{
		delete [] aPosNotify;
		aPosNotify = NULL;
	}

	// Ensure the primary buffer is empty before we start
	{
		void *audioPtr1, *audioPtr2;
		DWORD lockSize1, lockSize2;
		
		HRESULT hrLock = pDSBuffer->Lock(0, dwDSBufferSize, &audioPtr1, &lockSize1, &audioPtr2, &lockSize2, 0L);
		
		ZeroMemory(audioPtr1, lockSize1);
		ZeroMemory(audioPtr2, lockSize2);

		pDSBuffer->Unlock(audioPtr1, lockSize1, audioPtr2, lockSize2);
	}

    // Create the sound
    *ppPrimarySound = new CPrimarySound( pDSBuffer, dwDSBufferSize, this, dwNotifySize );

    return S_OK;
}
#endif // USE_OPENAL

#ifdef AUDIO_OPTIMIZE_OFF
#pragma optimize ("", on) // AUDIO_OPTIMIZE_OFF
#endif	//	AUDIO_OPTIMIZE_OFF
