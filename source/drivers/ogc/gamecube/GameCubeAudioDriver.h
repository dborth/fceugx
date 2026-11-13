/****************************************************************************
 * FCE Ultra GX
 *
 * Daryl Borth 2026
 *
 * GameCubeAudioDriver.h
 *
 * Audio driver
 ****************************************************************************/
#pragma once

#include <stdint.h>
#include <ogc/audio.h>
#include "../OgcEmulatorAudio.h"
#include "../../AudioDriver.h"

class GameCubeAudioDriver : public AudioDriver
{
	public:
		~GameCubeAudioDriver() override { delete emulatorAudio; }

		void init() override { AUDIO_Init(NULL); AUDIO_SetDSPSampleRate(AI_SAMPLERATE_48KHZ); emulatorAudio = new OgcEmulatorAudio(); emulatorAudio->init(); }
		void shutdown() override { emulatorAudio->stopAudio(); }
		void startMenuAudio() override {}
		void stopMenuAudio() override {}
		void startEmulatorAudio() override { AUDIO_RegisterDMACallback(AudioSwitchBuffers); }
		void stopEmulatorAudio() override { emulatorAudio->stopAudio(); }

		OgcEmulatorAudio* getEmulatorAudio() override { return emulatorAudio; }

		int32_t playVoice(const uint8_t*, int32_t, int) override { return -1; }
		void stopVoice(int32_t) override {}
		void pauseVoice(int32_t) override {}
		void resumeVoice(int32_t) override {}
		bool isVoicePlaying(int32_t) override { return false; }
		void setVoiceVolume(int32_t, int) override {}

		void playStream(const uint8_t*, int32_t, bool, int) override {}
		void stopStream() override {}
		void pauseStream() override {}
		void resumeStream() override {}
		bool isStreamPlaying() override { return false; }
		void setStreamVolume(int) override {}

	private:
		OgcEmulatorAudio* emulatorAudio = nullptr;
};
