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
		void uploadFrame(const uint8_t* buffer);
		void drawQuad();
		void syncScreenshotMetrics(int width, int height);

		// Hidden-overscan border, in NES source pixels, cropped out of the
		// texture on each side (see EmuSettings.hideoverscan). 0 when off.
		uint8_t getBorderWidth() const;
		uint8_t getBorderHeight() const;

		WutVideoDriver* videoDriver;

		GX2Texture* texture;
		GX2Sampler sampler;

		// On-screen placement of the game quad, in design-canvas pixels
		// (top-left x/y, size w/h) - recomputed by resetVideo().
		float quadX, quadY, quadWidth, quadHeight;

		// Raw NES framebuffer (palette indices) passed to the most recent
		// presentFrame() - reused by readFrameRGB24() so screenshots don't
		// need a GX2 texture readback. Only valid to dereference synchronously
		// (TakeScreenshot() runs before the emulator core produces another frame).
		const uint8_t* lastBuffer;
};
