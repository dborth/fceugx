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
	rateState = RATE_STATE_NEUTRAL;
	resamplePos = 0;
	lastSample = 0;
}

/****************************************************************************
 * selectResampleStep
 *
 * Discrete states and hysteresis, expressed as a resample step because
 * FCEUX has no cheap way to change its output rate mid-stream:
 * FCEUI_Sound() rebuilds the filters and clears the wave buffers, which
 * would itself click.
 ***************************************************************************/
uint32_t OgcEmulatorAudio::selectResampleStep(int queued)
{
	// Steer only while DMA is actually draining the ring - while it is
	// stopped (pre-roll) the depth means nothing - and never in fast-forward,
	// where overflow is dropped instead.
	if (!isPlaying || turbomode)
	{
		rateState = RATE_STATE_NEUTRAL;
		return STEP_NEUTRAL;
	}

	if (rateState == RATE_STATE_DRAINING && queued <= HIGH_RELEASE)
		rateState = RATE_STATE_NEUTRAL;
	else if (rateState == RATE_STATE_FILLING && queued >= LOW_RELEASE)
		rateState = RATE_STATE_NEUTRAL;

	if (queued > HIGH_WATER)
		rateState = RATE_STATE_DRAINING;
	else if (queued < LOW_WATER)
		rateState = RATE_STATE_FILLING;

	if (rateState == RATE_STATE_DRAINING)
		return (queued >= HIGH_CRITICAL) ? STEP_EMERGENCY_DRAIN : STEP_DRAIN;

	if (rateState == RATE_STATE_FILLING)
		return (queued <= CRITICAL) ? STEP_EMERGENCY_FILL : STEP_FILL;

	return STEP_NEUTRAL;
}

/****************************************************************************
 * playSound
 *
 * Puts incoming mono samples into mixbuffer
 * Splits mono samples into two channels (stereo)
 ****************************************************************************/
void OgcEmulatorAudio::playSound(const int32_t* buffer, int count)
{
	if (count <= 0)
		return;

	int head = mixhead;
	const int tail = mixtail;

	int queued = head - tail;
	if (queued < 0)
		queued += MIX_SAMPLES;

	const uint32_t step = selectResampleStep(queued);

	// Room left before the ring is full - never overwrite samples DMA hasn't
	// collected yet, and drop the overflow rather than block the emulation
	// thread.
	const int room = (queued < MAX_QUEUED) ? (MAX_QUEUED - queued) : 0;

	// Resample straight into the ring. Output sample k sits at 16.16
	// position resamplePos + k*step in the input, where index -1 is the
	// previous buffer's last sample, and is linearly interpolated. At the
	// neutral step every position is a whole number, so this is an exact
	// copy of the input.
	int written = 0;
	if (room > 0)
	{
		const int64_t lastPos = (int64_t)(count - 1) << 16;
		int64_t p = resamplePos;
		bool full = false;

		while (p <= lastPos)
		{
			if (written == room)
			{
				full = true;
				break;
			}

			const int32_t i = (int32_t)(p >> 16);
			const uint32_t frac = (uint32_t)(p & 0xffff);
			const int32_t a = (i < 0) ? lastSample : (int16_t)buffer[i];
			int32_t out = a;

			if (frac)
			{
				// p <= lastPos with a fraction means i < count - 1
				const int32_t b = (int16_t)buffer[i + 1];
				out = a + (int32_t)(((int64_t)(b - a) * frac) >> 16);
			}

			// Duplicate the 16-bit mono sample into both stereo channels.
			const u32 sample = (u32)(out & 0xffff);
			mixbuffer[head++] = sample | (sample << 16);
			if (head == MIX_SAMPLES)
				head = 0;
			written++;
			p += step;
		}

		// Carry the position into the next buffer, whose index -1 is this
		// one's last sample. If the ring filled up the rest of this buffer
		// was dropped, so just start the next one clean.
		resamplePos = full ? 0 : p - ((int64_t)count << 16);
		lastSample = (int16_t)buffer[count - 1];
	}

	mixhead = head;

	// (Re)start sound processing once enough is buffered to ride out
	// variation in when frames arrive.
	if (isPlaying == 0 && queued + written >= START_LEVEL) {
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
