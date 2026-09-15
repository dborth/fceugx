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
	AXVoiceDeviceMixData mix;
	memset(&mix, 0, sizeof(mix));
	mix.bus[0].volume = 0x8000;
	mix.bus[1].volume = 0x8000;
	AXSetVoiceDeviceMix(voice, AX_DEVICE_TYPE_TV, 0, &mix);
	AXSetVoiceDeviceMix(voice, AX_DEVICE_TYPE_DRC, 0, &mix);

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
	minFrames = frame * 3;    // ~9ms buffered - below this, stop rather than starve
	loadFrames = frame * 10;  // ~30ms buffered - required before (re)starting playback

	// This driver's own ring-refill/underrun tracking. Registered as an
	// *app* frame callback, not the single exclusive AXRegisterFrameCallback
	// slot WutAudioDriver::init() already owns for the menu stream - see
	// the design notes above.
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

void WutEmulatorAudio::playSound(const int32_t* buffer, int count)
{
	if (!voice || count <= 0 || !isForeground())
		return;

	// Room available ahead of the estimated AX playhead - never overwrite
	// samples AX hasn't played yet.
	uint32_t avail = (queuedFrames < RING_SAMPLES) ? (RING_SAMPLES - queuedFrames) : 0;
	if ((uint32_t)count > avail)
		count = (int)avail; // drop the overflow rather than block the emulation thread

	if (count <= 0)
		return;

	uint32_t pos = writePos;
	int firstRun = RING_SAMPLES - pos;
	if (firstRun > count)
		firstRun = count;
	int remaining = count - firstRun;

	for (int i = 0; i < firstRun; i++)
		ring[pos + i] = (int16_t)(buffer[i] & 0xffff);
	for (int i = 0; i < remaining; i++)
		ring[i] = (int16_t)(buffer[firstRun + i] & 0xffff);

	if (firstRun > 0)
		DCStoreRange(&ring[pos], firstRun * sizeof(int16_t));
	if (remaining > 0)
		DCStoreRange(&ring[0], remaining * sizeof(int16_t));

	writePos = (pos + count) % RING_SAMPLES;
	queuedFrames += count;

	if (!voiceRunning && queuedFrames >= loadFrames)
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

	if (queuedFrames < minFrames)
	{
		// Starving - stop outright rather than let AX loop over stale
		// ring content. playSound() won't call start() again until
		// loadFrames worth is buffered back up.
		AXSetVoiceState(voice, AX_VOICE_STATE_STOPPED);
		voiceRunning = false;
		return;
	}

	uint32_t frame = AXGetInputSamplesPerFrame();
	queuedFrames = (queuedFrames > frame) ? (queuedFrames - frame) : 0;
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
