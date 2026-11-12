/****************************************************************************
 * FCE Ultra GX
 *
 * Daryl Borth 2026
 *
 * WutEmulatorAudio.cpp
 ***************************************************************************/
#include <coreinit/cache.h>
#include <sndcore2/core.h>
#include <string.h>

#include "WutEmulatorAudio.h"
#include "../Platform.h"
#include "../../fceultra/driver.h"

extern int turbomode;

namespace
{
	WutEmulatorAudio* instance = nullptr;
	inline bool isForeground() { return platform->getStatus() == Status::Running; }
}

void WutEmulatorAudioFrameCallback()
{
	if (instance)
		instance->onAppFrame();
}

WutEmulatorAudio::WutEmulatorAudio()
{
	memset(ring, 0, sizeof(ring));
	instance = this;
}

WutEmulatorAudio::~WutEmulatorAudio()
{
	shutdown();

	if (instance == this)
		instance = nullptr;
}

void WutEmulatorAudio::init()
{
	samplerate = 48000;

	voice = AXAcquireVoice(31, nullptr, nullptr);
	if (!voice)
		return;

	AXVoiceBegin(voice);
	AXSetVoiceType(voice, 0);

	// Mono source, mixed equally to both channels on both output devices
	// it's two call sites total.
	AXVoiceDeviceMixData tvMix[6];
	AXVoiceDeviceMixData drcMix[4];
	memset(tvMix, 0, sizeof(tvMix));
	memset(drcMix, 0, sizeof(drcMix));
	tvMix[0].bus[0].volume = 0x8000;
	tvMix[1].bus[0].volume = 0x8000;
	drcMix[0].bus[0].volume = 0x8000;
	drcMix[1].bus[0].volume = 0x8000;
	AXSetVoiceDeviceMix(voice, AX_DEVICE_TYPE_TV, 0, tvMix);
	AXSetVoiceDeviceMix(voice, AX_DEVICE_TYPE_DRC, 0, drcMix);

	AXVoiceOffsets offsets;
	memset(&offsets, 0, sizeof(offsets));
	offsets.data = ring;
	offsets.dataType = AX_VOICE_FORMAT_LPCM16;
	offsets.loopingEnabled = AX_VOICE_LOOP_ENABLED;
	offsets.loopOffset = 0;
	offsets.endOffset = RING_SAMPLES - 1;
	offsets.currentOffset = 0;
	AXSetVoiceOffsets(voice, &offsets);

	AXVoiceVeData ve;
	ve.volume = 0x8000;
	ve.delta = 0;
	AXSetVoiceVe(voice, &ve);

	AXSetVoiceState(voice, AX_VOICE_STATE_STOPPED);
	AXVoiceEnd(voice);

	AXVoiceSrc src;
	memset(&src, 0, sizeof(src));
	src.ratio = 0x00010000; // 1.0 in 16.16
	AXSetVoiceSrc(voice, &src);
	AXSetVoiceSrcType(voice, AX_VOICE_SRC_TYPE_NONE);

	uint32_t frame = AXGetInputSamplesPerFrame();
	minFrames = frame * 3; // ~9ms buffered - below this, stop rather than starve

	// This driver's own ring-refill/underrun tracking, registered as an
	// *app* frame callback
	AXRegisterAppFrameCallback(WutEmulatorAudioFrameCallback);
}

void WutEmulatorAudio::shutdown()
{
	AXDeregisterAppFrameCallback(WutEmulatorAudioFrameCallback);

	if (voice)
	{
		AXSetVoiceState(voice, AX_VOICE_STATE_STOPPED);
		AXFreeVoice(voice);
		voice = nullptr;
	}

	voiceRunning = false;
}

void WutEmulatorAudio::resetAudio()
{
	// Called when loading a new game - wipe any stale buffered audio
	// so playback doesn't open with leftovers from whatever was running before.
	memset(ring, 0, sizeof(ring));
	DCStoreRange(ring, sizeof(ring));

	writePos = 0;
	queuedFrames = 0;
	voiceRunning = false;
	rateState = RATE_STATE_NEUTRAL;
	resamplePos = 0;
	lastSample = 0;

	if (voice)
	{
		AXSetVoiceState(voice, AX_VOICE_STATE_STOPPED);
		AXSetVoiceCurrentOffset(voice, 0);
	}
}

void WutEmulatorAudio::stopAudio()
{
	if (voice)
		AXSetVoiceState(voice, AX_VOICE_STATE_STOPPED);

	voiceRunning = false;
}

void WutEmulatorAudio::startVoice()
{
	// Resume from (writePos - queuedFrames): the oldest sample in the
	// backlog, so playback picks up exactly where the buffered content
	// begins rather than wherever the write head currently is.
	uint32_t start = (writePos + RING_SAMPLES - (queuedFrames % RING_SAMPLES)) % RING_SAMPLES;
	AXSetVoiceCurrentOffset(voice, start);
	AXSetVoiceState(voice, AX_VOICE_STATE_PLAYING);
	voiceRunning = true;
}

/****************************************************************************
 * selectResampleStep
 *
 * Dynamic-rate control with discrete states and hysteresis, expressed as a
 * resample step because FCEUX has no cheap way to change its output rate
 * mid-stream.
 ***************************************************************************/
uint32_t WutEmulatorAudio::selectResampleStep(uint32_t queued)
{
	// Steer only while the voice is actually draining the queue, and never
	// in fast-forward, where overflow is dropped instead.
	if (!voiceRunning || turbomode)
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

void WutEmulatorAudio::playSound(const int32_t* buffer, int count)
{
	if (!voice || count <= 0 || !isForeground())
		return;

	const uint32_t queued = __atomic_load_n(&queuedFrames, __ATOMIC_RELAXED);
	const uint32_t step = selectResampleStep(queued);

	// Room left before the queue is full - never overwrite samples AX hasn't
	// played yet, and drop the overflow rather than block the emulation
	// thread.
	const uint32_t avail = (queued < MAX_QUEUED) ? (MAX_QUEUED - queued) : 0;
	if (avail == 0)
		return;

	// Resample straight into the ring. Output sample k sits at 16.16
	// position resamplePos + k*step in the input, where index -1 is the
	// previous buffer's last sample, and is linearly interpolated. At the
	// neutral step every position is a whole number, so this is an exact
	// copy of the input.
	const uint32_t startPos = writePos;
	uint32_t pos = startPos;
	uint32_t written = 0;
	const int64_t lastPos = (int64_t)(count - 1) << 16;
	int64_t p = resamplePos;
	bool full = false;

	while (p <= lastPos)
	{
		if (written == avail)
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

		ring[pos] = (int16_t)out;
		if (++pos == RING_SAMPLES)
			pos = 0;
		written++;
		p += step;
	}

	// Carry the position into the next buffer, whose index -1 is this one's
	// last sample. If the ring filled up the rest of this buffer was
	// dropped, so just start the next one clean.
	resamplePos = full ? 0 : p - ((int64_t)count << 16);
	lastSample = (int16_t)buffer[count - 1];

	if (written == 0)
		return;

	const uint32_t firstRun = (written < RING_SAMPLES - startPos) ? written : (RING_SAMPLES - startPos);
	DCStoreRange(&ring[startPos], firstRun * sizeof(int16_t));
	if (written > firstRun)
		DCStoreRange(&ring[0], (written - firstRun) * sizeof(int16_t));

	writePos = pos;

	const uint32_t total = __atomic_add_fetch(&queuedFrames, written, __ATOMIC_RELAXED);

	if (!voiceRunning && total >= START_LEVEL)
		startVoice();
}

void WutEmulatorAudio::onAppFrame()
{
	if (!voice || !voiceRunning)
		return;

	if (!isForeground())
	{
		// Lost the foreground - stop driving the voice rather than
		// continuing to play/consume audio in the background.
		AXSetVoiceState(voice, AX_VOICE_STATE_STOPPED);
		voiceRunning = false;
		return;
	}

	const uint32_t frame = AXGetInputSamplesPerFrame();

	uint32_t queued = __atomic_load_n(&queuedFrames, __ATOMIC_RELAXED);
	do
	{
		if (queued < minFrames)
		{
			// Starving - stop outright rather than let AX loop over stale
			// ring content. playSound() won't start it again until
			// START_LEVEL worth is buffered back up.
			AXSetVoiceState(voice, AX_VOICE_STATE_STOPPED);
			voiceRunning = false;
			return;
		}
	} while (!__atomic_compare_exchange_n(&queuedFrames, &queued, (queued > frame) ? (queued - frame) : 0, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED));
}

void WutEmulatorAudio::updateSampleRate(int rate)
{
	if (samplerate != rate)
	{
		samplerate = rate;
		FCEUI_Sound(samplerate);
	}
}

void WutEmulatorAudio::setSampleRate()
{
	FCEUI_Sound(samplerate);
}
