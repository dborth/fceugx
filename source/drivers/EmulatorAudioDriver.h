/****************************************************************************
 * libgui
 *
 * Daryl Borth 2009-2026
 * EmulatorAudioDriver.h
 ***************************************************************************/
#pragma once

#include <stdint.h>

class EmulatorAudioDriver
{
	public:
		virtual ~EmulatorAudioDriver() = default;

		virtual void init() = 0;

		//! Clears buffered/queued audio state. Called when loading a new game.
		virtual void resetAudio() = 0;

		//! Halts hardware playback so it cleanly restarts the next time samples arrive
		virtual void stopAudio() = 0;

		//! Hardware DMA callback that feeds the sound buffer ring to the audio
		//! backend. Invoked from interrupt context by the platform's DMA
		//! callback trampoline - never called directly by emulator core code.
		virtual void switchBuffers() = 0;

		//! Accepts mono 16-bit samples from the emulator core and queues them for playback
		virtual void playSound(const int32_t* buffer, int samples) = 0;

		virtual void updateSampleRate(int rate) = 0;
		virtual void setSampleRate() = 0;
};
