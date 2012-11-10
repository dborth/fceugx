/****************************************************************************
 * FCE Ultra
 * Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2009
 * eke-eke October 2008
 *
 * gcaudio.cpp
 *
 * Audio driver
 ****************************************************************************/

#include <gccore.h>
#include <string.h>
#include <asndlib.h>
#include "fceusupport.h"

static u8 ConfigRequested = 0;
static u8 soundbuffer[2][3840] ATTRIBUTE_ALIGN(32);
static u8 mixbuffer[16000];
static int mixhead = 0;
static int mixtail = 0;
static int whichab = 0;
static int IsPlaying = 0;
static int samplerate;

/****************************************************************************
 * MixerCollect
 *
 * Collects sound samples from mixbuffer and puts them into outbuffer
 * Makes sure to align them to 32 bytes for AUDIO_InitDMA
 ***************************************************************************/
static int MixerCollect( u8 *outbuffer, int len )
{
	u32 *dst = (u32 *)outbuffer;
	u32 *src = (u32 *)mixbuffer;
	int done = 0;

	// Always clear output buffer
	memset(outbuffer, 0, len);

	while ( ( mixtail != mixhead ) && ( done < len ) )
	{
		*dst++ = src[mixtail++];
		if (mixtail == 4000) mixtail = 0;
		done += 4;
	}

	// Realign to 32 bytes for DMA
	mixtail -= ((done&0x1f) >> 2);
	if (mixtail < 0)
		mixtail += 4000;
	done &= ~0x1f;
	if (!done)
		return len >> 1;

	return done;
}

/****************************************************************************
 * AudioSwitchBuffers
 *
 * Manages which buffer is played next
 ***************************************************************************/
static void AudioSwitchBuffers()
{
	if ( !ConfigRequested )
	{
		whichab ^= 1;
		int len = MixerCollect( soundbuffer[whichab], 3840 );
		DCFlushRange(soundbuffer[whichab], len);
		AUDIO_InitDMA((u32)soundbuffer[whichab], len);
		IsPlaying = 1;
	}
	else IsPlaying = 0;
}

/****************************************************************************
 * InitialiseAudio
 *
 * Initializes sound system on first load of emulator
 ***************************************************************************/
void InitialiseAudio()
{
	#ifdef NO_SOUND
	AUDIO_Init (NULL);
	AUDIO_SetDSPSampleRate(AI_SAMPLERATE_48KHZ);
	AUDIO_RegisterDMACallback(AudioSwitchBuffers);
	#else
	ASND_Init();
	#endif
	memset(soundbuffer, 0, 3840*2);
	memset(mixbuffer, 0, 16000);
}

/****************************************************************************
 * ResetAudio
 *
 * Reset audio output when loading a new game
 ***************************************************************************/
void ResetAudio()
{
	memset(soundbuffer, 0, 3840*2);
	memset(mixbuffer, 0, 16000);
	mixhead = mixtail = 0;
}

/****************************************************************************
 * SwitchAudioMode
 *
 * Switches between menu sound and emulator sound
 ***************************************************************************/
void
SwitchAudioMode(int mode)
{
	if(mode == 0) // emulator
	{
		#ifndef NO_SOUND
		ASND_Pause(1);
		AUDIO_StopDMA();
		AUDIO_RegisterDMACallback(AudioSwitchBuffers);
		#endif
		memset(soundbuffer[0],0,3840);
		memset(soundbuffer[1],0,3840);
		DCFlushRange(soundbuffer[0],3840);
		DCFlushRange(soundbuffer[1],3840);
		AUDIO_InitDMA((u32)soundbuffer[whichab],3200);
		AUDIO_StartDMA();
	}
	else // menu
	{
		IsPlaying = 0;
		#ifndef NO_SOUND
		ASND_Init();
		ASND_Pause(0);
		#else
		AUDIO_StopDMA();
		#endif
	}
}

/****************************************************************************
 * ShutdownAudio
 *
 * Shuts down audio subsystem. Useful to avoid unpleasant sounds if a
 * crash occurs during shutdown.
 ***************************************************************************/
void ShutdownAudio()
{
	AUDIO_StopDMA();
}

/****************************************************************************
 * PlaySound
 *
 * Puts incoming mono samples into mixbuffer
 * Splits mono samples into two channels (stereo)
 ****************************************************************************/
void PlaySound( int *Buffer, int count )
{
	int i;
	u16 sample;
	u32 *dst = (u32 *)mixbuffer;

	for( i = 0; i < count; i++ )
	{
		sample = Buffer[i] & 0xffff;
		dst[mixhead++] = sample | ( sample << 16);
		if (mixhead == 4000)
			mixhead = 0;
	}

	// Restart Sound Processing if stopped
	if (IsPlaying == 0)
	{
		AudioSwitchBuffers ();
	}
}

void UpdateSampleRate(int rate)
{
	samplerate = rate;
}

void SetSampleRate()
{
	FCEUI_Sound(samplerate);
}
