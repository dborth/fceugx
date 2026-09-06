/****************************************************************************
 * FCE Ultra GX
 *
 * Daryl Borth 2008-2026
 * eke-eke October 2008
 *
 * OgcEmulatorAudio.cpp
 *
 * Audio driver
 ****************************************************************************/

#include <gccore.h>
#include <string.h>

#include "OgcEmulatorAudio.h"
#include "../../fceugx.h"
#include "../../fceusupport.h"

// The single OgcEmulatorAudio instance currently registered with the DMA
// callback trampoline below. There is only ever one emulator audio backend
// alive at a time.
static OgcEmulatorAudio* instance = nullptr;

/****************************************************************************
 * AudioSwitchBuffers
 *
 * Hardware DMA callback trampoline - forwards into the live instance
 ***************************************************************************/
void AudioSwitchBuffers()
{
	if (instance)
		instance->switchBuffers();
}

OgcEmulatorAudio::OgcEmulatorAudio()
{
	memset(soundbuffer, 0, sizeof(soundbuffer));
	memset(mixbuffer, 0, sizeof(mixbuffer));
	instance = this;
}

OgcEmulatorAudio::~OgcEmulatorAudio()
{
	if (instance == this)
		instance = nullptr;
}

void OgcEmulatorAudio::init()
{
}

/****************************************************************************
 * mixerCollect
 *
 * Collects sound samples from mixbuffer and puts them into outbuffer
 * Makes sure to align them to 32 bytes for AUDIO_InitDMA
 ***************************************************************************/
int OgcEmulatorAudio::mixerCollect(uint8_t* outbuffer, int len)
{
	u32* dst = (u32*)outbuffer;
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
 * switchBuffers
 *
 * Manages which buffer is played next
 ***************************************************************************/
void OgcEmulatorAudio::switchBuffers()
{
	if (appRequest == AppRequest::NONE) {
		isPlaying = 1;
		int len = mixerCollect(soundbuffer[whichab], DMA_BUFFER_BYTES);
		DCFlushRange(soundbuffer[whichab], len);
		AUDIO_InitDMA((u32)soundbuffer[whichab], len);
		whichab ^= 1;
	}
	else {
		isPlaying = 0;
	}
}

/****************************************************************************
 * stopAudio
 *
 * Halts DMA playback so it cleanly restarts on the next playSound call
 ***************************************************************************/
void OgcEmulatorAudio::stopAudio()
{
	isPlaying = 0;
	AUDIO_StopDMA();
	AUDIO_RegisterDMACallback(NULL);
}

/****************************************************************************
 * resetAudio
 *
 * Reset audio output when loading a new game
 ***************************************************************************/
void OgcEmulatorAudio::resetAudio()
{
	memset(soundbuffer, 0, sizeof(soundbuffer));
	memset(mixbuffer, 0, sizeof(mixbuffer));
	mixhead = 0;
	mixtail = 0;
}

/****************************************************************************
 * playSound
 *
 * Puts incoming mono samples into mixbuffer
 * Splits mono samples into two channels (stereo)
 ****************************************************************************/
void OgcEmulatorAudio::playSound(const int32_t* buffer, int count)
{
	int head = mixhead;

	for (int i = 0; i < count; i++) {
		// Duplicate the 16-bit mono sample into both stereo channels.
		u32 sample = (u32)(buffer[i] & 0xffff);
		mixbuffer[head++] = sample | (sample << 16);
		if (head == MIX_SAMPLES)
			head = 0;
	}

	mixhead = head;

	// Restart Sound Processing if stopped
	if (isPlaying == 0) {
		AUDIO_StartDMA();
	}
}

void OgcEmulatorAudio::updateSampleRate(int rate)
{
	if (samplerate != rate) {
		samplerate = rate;
		FCEUI_Sound(samplerate);
	}
}

void OgcEmulatorAudio::setSampleRate()
{
	FCEUI_Sound(samplerate);
}
