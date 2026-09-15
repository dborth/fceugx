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
		//! from the ring and applies underrun/restart hysteresis.
		void onAppFrame();

	private:
		void applySrcRatio();

		AXVoice* voice = nullptr;

		// Mono ring buffer feeding the single AX voice. Sized generously
		// (~128ms at 48kHz) so a slow frame or a menu transition doesn't
		// starve it - NES audio is a tiny ~800 samples/emulated frame.
		static constexpr uint32_t RING_SAMPLES = 6144;
		alignas(32) int16_t ring[RING_SAMPLES];

		// Software-owned queue depth: playSound() (producer, called from the
		// emulation loop) increments this, and the AX frame callback
		// (consumer) decrements it by one frame period per tick.
		volatile uint32_t writePos = 0;
		volatile uint32_t queuedFrames = 0;
		volatile bool voiceRunning = false;

		int samplerate = 0;

		// Hysteresis thresholds, derived from the real AX frame size so
		// they stay correct if the renderer mode ever changes.
		uint32_t minFrames = 0;   // stop the voice once buffered content drops below this (starving)
		uint32_t loadFrames = 0;  // don't restart until buffered back up to this
};
