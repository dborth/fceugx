/****************************************************************************
 * FCE Ultra GX
 *
 * Daryl Borth 2026
 *
 * WutEmulatorAudio.h
 *
 * Audio driver
 ****************************************************************************/
#pragma once

#include <stdint.h>
#include <sndcore2/voice.h>
#include "../EmulatorAudioDriver.h"

// AX app-frame-callback trampoline. AXRegisterAppFrameCallback requires a
// bare C function pointer, so this forwards to the live instance's
// onAppFrame() rather than being a member itself.
void WutEmulatorAudioFrameCallback();

//! Wii U in-game emulator audio path - the fast path FCEUX's core feeds every emulated frame
class WutEmulatorAudio : public EmulatorAudioDriver
{
	public:
		WutEmulatorAudio();
		~WutEmulatorAudio() override;

		void init() override;
		void resetAudio();
		void stopAudio();
		void startVoice();
		void playSound(const int32_t* buffer, int samples) override;
		void updateSampleRate(int rate) override;
		void setSampleRate() override;

		//! Frees the AXVoice and deregisters the frame callback.
		void shutdown();

		//! AX app-frame callback (fires every ~3ms). Retires played samples
		//! from the ring and stops the voice if it is about to starve.
		void onAppFrame();

	private:
		void applySrcRatio();

		AXVoice* voice = nullptr;

		// Unit for the thresholds below: the same 512-sample chunk (~10.7ms at
		// 48kHz) that snes9xgx's WutEmulatorAudio measures its queue in.
		static constexpr uint32_t CHUNK_SAMPLES = 512;

		// Mono ring buffer feeding the single AX voice (~170ms at 48kHz).
		// NES audio is a tiny ~800 samples/emulated frame.
		static constexpr uint32_t RING_SAMPLES = 16 * CHUNK_SAMPLES;
		alignas(32) int16_t ring[RING_SAMPLES];

		// The emulated console and the AX clock never run at exactly the same
		// speed (NES: 60.0988Hz of frames, each 48000/60.0988 samples, paced by
		// a ~60Hz display, consumed at 48000/s), so the queue slowly drains or
		// fills. PlaySound() steers the queue depth: it resamples each frame's
		// samples by a tiny amount whenever the queue leaves its comfortable
		// band, with hysteresis so it doesn't hunt. Depths are measured as
		// the queue stands just before a frame's samples are added - the lowest
		// point of its per-frame sawtooth.
		static constexpr uint32_t LOW_WATER = 4 * CHUNK_SAMPLES;      // below this we risk starving: stretch
		static constexpr uint32_t LOW_RELEASE = 6 * CHUNK_SAMPLES;    // keep stretching until back up to here
		static constexpr uint32_t HIGH_RELEASE = 6 * CHUNK_SAMPLES;   // keep compressing until back down to here
		static constexpr uint32_t HIGH_WATER = 8 * CHUNK_SAMPLES;     // above this latency is building: compress
		static constexpr uint32_t CRITICAL = 2 * CHUNK_SAMPLES;       // nearly dry: stretch harder
		static constexpr uint32_t HIGH_CRITICAL = 11 * CHUNK_SAMPLES; // nearly full: compress harder
		static constexpr uint32_t MAX_QUEUED = 12 * CHUNK_SAMPLES;    // fast-forward: drop what doesn't fit
		static constexpr uint32_t START_LEVEL = 6 * CHUNK_SAMPLES;    // buffered before (re)starting the voice

		// Resample step: input samples consumed per output sample, 16.16.
		// Above 1.0 emits fewer samples (drains the queue), below 1.0 more.
		static constexpr uint32_t STEP_NEUTRAL = 0x10000;
		static constexpr uint32_t STEP_DRAIN = 65864;           // 1.005
		static constexpr uint32_t STEP_FILL = 65208;            // 0.995
		static constexpr uint32_t STEP_EMERGENCY_DRAIN = 66519; // 1.015
		static constexpr uint32_t STEP_EMERGENCY_FILL = 64553;  // 0.985

		enum RateState
		{
			RATE_STATE_NEUTRAL,
			RATE_STATE_DRAINING, // compressing to shrink an over-full queue
			RATE_STATE_FILLING   // stretching to grow an under-full queue
		};

		//! Picks the resample step for this frame's samples from the queue depth.
		uint32_t selectResampleStep(uint32_t queued);

		// Software-owned queue depth: playSound() (producer, called from the
		// emulation loop) increments this, and the AX frame callback
		// (consumer) decrements it by one frame period per tick.
		volatile uint32_t writePos = 0;
		volatile uint32_t queuedFrames = 0;
		volatile bool voiceRunning = false;

		int samplerate = 0;

		// Only touched by playSound() (the emulation thread)
		RateState rateState = RATE_STATE_NEUTRAL;
		int64_t resamplePos = 0;  // 16.16 position of the next output sample in the next input buffer
		int16_t lastSample = 0;   // final input sample of the previous buffer (interpolation continuity)

		// Derived from the real AX frame size so it stays correct if the
		// renderer mode ever changes. The last resort: stop the voice once
		// buffered content drops below this rather than loop stale ring data.
		uint32_t minFrames = 0;
};
