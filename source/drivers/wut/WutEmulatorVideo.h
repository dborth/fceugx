/****************************************************************************
 * FCE Ultra GX
 *
 * Daryl Borth 2026
 *
 * WutEmulatorVideo.h
 *
 * EmulatorVideoDriver implementation for Wii U: uploads the raw NES
 * framebuffer into a linear GX2 texture and draws it with the shared
 * Texture2DShader.
 ***************************************************************************/
#pragma once

#include <stdint.h>
#include <gx2/sampler.h>
#include <gx2/texture.h>
#include "../EmulatorVideoDriver.h"

class WutVideoDriver;

class WutEmulatorVideo : public EmulatorVideoDriver
{
	public:
		WutEmulatorVideo();
		~WutEmulatorVideo() override;

		void init(VideoDriver* videoDriver) override;
		void resetVideo() override;
		void presentFrame(const uint8_t* buffer) override;
		void presentStereoFrame(const uint8_t* bufferLeft, const uint8_t* bufferRight) override;
		void readFrameRGB24(uint8_t* dst) override;

	private:
		void rebuildTexture(int width, int height);
		void destroyTexture();
		void uploadFrame();
		void drawQuad();

		WutVideoDriver* videoDriver;

		GX2Texture* texture;
		GX2Sampler sampler;

		// On-screen placement of the game quad, in design-canvas pixels
		// (top-left x/y, size w/h) - recomputed by resetVideo().
		float quadX, quadY, quadWidth, quadHeight;
};
