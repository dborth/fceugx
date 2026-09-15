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
#include "../EmulatorAudioDriver.h"

class WutEmulatorAudio : public EmulatorAudioDriver
{
	public:
		WutEmulatorAudio();
		~WutEmulatorAudio() override;

		void init() override;
		void resetAudio() override;
		void stopAudio() override;
		void playSound(const int32_t* buffer, int samples) override;
		void updateSampleRate(int rate) override;
		void setSampleRate() override;

	private:

};
