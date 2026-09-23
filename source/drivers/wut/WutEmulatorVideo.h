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
#include "WutVideoDriver.h"

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
		bool mapPointerToFrame(float canvasX, float canvasY, bool onGamePad, int* frameX, int* frameY) override;
		bool getVisibleFrameRect(int* x, int* y, int* w, int* h) override;

	private:
		void rebuildTexture(int width, int height);
		void destroyTexture();
		void uploadFrame(const uint8_t* buffer);
		void drawQuad();
		void syncScreenshotMetrics(int width, int height);

		// Hidden-overscan border, in NES source pixels, cropped out of the
		// texture on each side (see EmuSettings.hideOverscan). 0 when off.
		uint8_t getBorderWidth() const;
		uint8_t getBorderHeight() const;

		WutVideoDriver* videoDriver;

		GX2Texture* texture;
		GX2Sampler sampler;

		// The game quad's placement is decided per output target
		struct FrameRect { float x, y, w, h; }; // fractions of the target, top-left origin
		FrameRect frame[OUTPUT_TARGET_COUNT];

		// The same rect in physical pixels of each target (top-left x/y, size
		// w/h): what actually gets drawn, and what scaling/filtering needs.
		struct TargetPlacement { float x, y, w, h; };
		TargetPlacement placement[OUTPUT_TARGET_COUNT];

		// The TV rect expressed in UI-canvas pixels. Only used to mirror the placement into gameScreenPng
		float quadX, quadY, quadWidth, quadHeight;

		// Raw NES framebuffer (palette indices) passed to the most recent
		// presentFrame() - reused by readFrameRGB24() so screenshots don't
		// need a GX2 texture readback. Only valid to dereference synchronously
		// (TakeScreenshot() runs before the emulator core produces another frame).
		const uint8_t* lastBuffer;
};
