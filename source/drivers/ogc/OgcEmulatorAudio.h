/****************************************************************************
 * FCE Ultra GX
 *
 * Daryl Borth 2008-2026
 * eke-eke October 2008
 *
 * OgcEmulatorAudio.h
 *
 * Audio driver
 ****************************************************************************/
#pragma once

#include <stdint.h>
#include "../EmulatorAudioDriver.h"

// Hardware DMA callback that feeds the sound buffer ring to AUDIO_InitDMA.
// Registered with AUDIO_RegisterDMACallback(), which requires a bare C
// function pointer, so this trampoline forwards to the current
// OgcEmulatorAudio instance's switchBuffers() rather than being a member itself.
void AudioSwitchBuffers();

class OgcEmulatorAudio : public EmulatorAudioDriver
{
	public:
		OgcEmulatorAudio();
		~OgcEmulatorAudio() override;

		void init() override;
		void resetAudio() override;
		void stopAudio() override;
		void playSound(const int32_t* buffer, int samples) override;
		void updateSampleRate(int rate) override;
		void setSampleRate() override;

		//! Hardware DMA callback that feeds the sound buffer ring to the audio
		//! backend. Invoked from interrupt context by the platform's DMA
		//! callback trampoline - never called directly by emulator core code.
		void switchBuffers();
	private:
		int mixerCollect(uint8_t* outbuffer, int len);

		// Each DMA buffer holds 20ms of 16-bit stereo samples.
		static constexpr int DMA_BUFFER_BYTES = 3840;

		// Ring buffer of stereo samples (one u32 == one L/R sample pair).
		static constexpr int MIX_SAMPLES = 8192;

		// The emulated console and the audio hardware never run at exactly
		// the same speed, and frames reach playSound() with some timing
		// variation, so the ring slowly fills or drains. With no margin it
		// ran dry every few frames and each empty DMA buffer played as 10ms
		// of silence. Instead the ring is pre-rolled before playback starts
		// and playSound() steers its depth: it resamples each frame's
		// samples by a tiny amount (inaudible, 0.5%) whenever the depth
		// leaves its comfortable band, with hysteresis so it doesn't hunt.
		// Depths are in stereo samples, measured as the ring stands just
		// before a frame's samples are added.
		static constexpr int LOW_WATER = 1440;       // below this we risk running dry: stretch
		static constexpr int LOW_RELEASE = 2400;     // keep stretching until back up to here
		static constexpr int HIGH_RELEASE = 2400;    // keep compressing until back down to here
		static constexpr int HIGH_WATER = 3840;      // above this latency is building: compress
		static constexpr int CRITICAL = 960;         // nearly dry: stretch harder
		static constexpr int HIGH_CRITICAL = 5760;   // nearly full: compress harder
		static constexpr int MAX_QUEUED = 6720;      // fast-forward: drop what doesn't fit
		static constexpr int START_LEVEL = 2400;     // buffered before (re)starting DMA

		// Resample step: input samples consumed per output sample, 16.16.
		// Above 1.0 emits fewer samples (drains the ring), below 1.0 more.
		static constexpr uint32_t STEP_NEUTRAL = 0x10000;
		static constexpr uint32_t STEP_DRAIN = 65864;           // 1.005
		static constexpr uint32_t STEP_FILL = 65208;            // 0.995
		static constexpr uint32_t STEP_EMERGENCY_DRAIN = 66519; // 1.015
		static constexpr uint32_t STEP_EMERGENCY_FILL = 64553;  // 0.985

		enum RateState
		{
			RATE_STATE_NEUTRAL,
			RATE_STATE_DRAINING, // compressing to shrink an over-full ring
			RATE_STATE_FILLING   // stretching to grow an under-full ring
		};

		//! Picks the resample step for this frame's samples from the ring depth.
		uint32_t selectResampleStep(int queued);

		// AUDIO_InitDMA requires 32-byte aligned lengths.
		static constexpr int DMA_ALIGN = 32;

		uint8_t soundbuffer[2][DMA_BUFFER_BYTES] __attribute__((aligned(32)));
		uint32_t mixbuffer[MIX_SAMPLES] __attribute__((aligned(32)));

		// Shared between the emulator thread (producer) and the DMA interrupt
		// callback (consumer); must not be cached in registers.
		volatile int mixhead = 0;
		volatile int mixtail = 0;
		volatile int isPlaying = 0;
		int whichab = 0;
		int samplerate = 0;

		// Only touched by playSound() (the emulation thread)
		RateState rateState = RATE_STATE_NEUTRAL;
		int64_t resamplePos = 0;  // 16.16 position of the next output sample in the next input buffer
		int16_t lastSample = 0;   // final input sample of the previous buffer (interpolation continuity)
};
