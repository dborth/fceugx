/****************************************************************************
 * FCE Ultra
 * Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2023
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

extern int ScreenshotRequested;
extern int ConfigRequested;

// Each DMA buffer holds one frame's worth of 16-bit stereo samples.
#define DMA_BUFFER_BYTES 3840

// Ring buffer of stereo samples (one u32 == one L/R sample pair).
#define MIX_SAMPLES 4000

// AUDIO_InitDMA requires 32-byte aligned lengths.
#define DMA_ALIGN 32

static u8 soundbuffer[2][DMA_BUFFER_BYTES] ATTRIBUTE_ALIGN(32);
static u32 mixbuffer[MIX_SAMPLES] ATTRIBUTE_ALIGN(32);

// Shared between the emulator thread (producer) and the DMA interrupt
// callback (consumer); must not be cached in registers.
static volatile int mixhead = 0;
static volatile int mixtail = 0;
static volatile int IsPlaying = 0;
static int whichab = 0;
static int samplerate = 0;

/****************************************************************************
 * MixerCollect
 *
 * Collects sound samples from mixbuffer and puts them into outbuffer
 * Makes sure to align them to 32 bytes for AUDIO_InitDMA
 ***************************************************************************/
static int MixerCollect( u8 *outbuffer, int len )
{
	u32 *dst = (u32 *)outbuffer;
	const int maxsamples = len >> 2; // u32 samples that fit in outbuffer
	const int head = mixhead;        // snapshot the producer index once
	int tail = mixtail;

	// Number of buffered samples available to copy this pass
	int avail = head - tail;
	if (avail < 0)
		avail += MIX_SAMPLES;
	if (avail > maxsamples)
		avail = maxsamples;

	int done = avail;

	// Copy the (up to) two contiguous ring segments in bulk
	if (done > 0)
	{
		int first = MIX_SAMPLES - tail;
		if (first > done)
			first = done;
		memcpy(dst, &mixbuffer[tail], first * sizeof(u32));
		if (done > first)
			memcpy(dst + first, &mixbuffer[0], (done - first) * sizeof(u32));

		tail += done;
		if (tail >= MIX_SAMPLES)
			tail -= MIX_SAMPLES;
	}

	int bytes = done << 2;

	// Realign down to a 32-byte boundary for DMA, returning the truncated
	// samples to the ring so they are played next pass.
	int extra = (bytes & (DMA_ALIGN - 1)) >> 2;
	if (extra)
	{
		tail -= extra;
		if (tail < 0)
			tail += MIX_SAMPLES;
		bytes &= ~(DMA_ALIGN - 1);
	}

	mixtail = tail;

	// Zero only the unfilled tail of the output buffer (underrun gap).
	if (bytes < len)
		memset(outbuffer + bytes, 0, len - bytes);

	if (!bytes)
		return len >> 1;

	return bytes;
}

/****************************************************************************
 * AudioSwitchBuffers
 *
 * Manages which buffer is played next
 ***************************************************************************/
static void AudioSwitchBuffers()
{
	if ( !ScreenshotRequested && !ConfigRequested ) {
		IsPlaying = 1;
		int len = MixerCollect( soundbuffer[whichab], DMA_BUFFER_BYTES );
		DCFlushRange(soundbuffer[whichab], len);
		AUDIO_InitDMA((u32)soundbuffer[whichab], len);
		whichab ^= 1;
	}
	else {
		IsPlaying = 0;
	}
}

/****************************************************************************
 * InitialiseAudio
 *
 * Initializes sound system on first load of emulator
 ***************************************************************************/
void InitialiseAudio()
{
	#ifdef NO_SOUND
	AUDIO_Init(NULL);
	AUDIO_SetDSPSampleRate(AI_SAMPLERATE_48KHZ);
	AUDIO_RegisterDMACallback(AudioSwitchBuffers);
	#else
	ASND_Init();
	#endif
	memset(soundbuffer, 0, sizeof(soundbuffer));
	memset(mixbuffer, 0, sizeof(mixbuffer));
}

/****************************************************************************
 * ResetAudio
 *
 * Reset audio output when loading a new game
 ***************************************************************************/
void ResetAudio()
{
	memset(soundbuffer, 0, sizeof(soundbuffer));
	memset(mixbuffer, 0, sizeof(mixbuffer));
	mixhead = 0;
	mixtail = 0;
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
		ASND_End();
		AUDIO_StopDMA();
		AUDIO_RegisterDMACallback(NULL);
		DSP_Halt();
		AUDIO_RegisterDMACallback(AudioSwitchBuffers);
		#endif
	}
	else // menu
	{
		IsPlaying = 0;
		#ifndef NO_SOUND
		DSP_Unhalt();
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
void PlaySound( int32 *Buffer, int count )
{
	int head = mixhead;

	for( int i = 0; i < count; i++ ) {
		// Duplicate the 16-bit mono sample into both stereo channels.
		u32 sample = (u32)(Buffer[i] & 0xffff);
		mixbuffer[head++] = sample | (sample << 16);
		if (head == MIX_SAMPLES)
			head = 0;
	}

	mixhead = head;

	// Restart Sound Processing if stopped
	if (IsPlaying == 0) {
		AUDIO_StartDMA();
	}
}

void UpdateSampleRate(int rate)
{
	if(samplerate != rate) {
		samplerate = rate;
		FCEUI_Sound(samplerate);
	}
}

void SetSampleRate()
{
	FCEUI_Sound(samplerate);
}
