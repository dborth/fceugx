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

		// On-screen placement of the game quad, in design-canvas pixels
		// (top-left x/y, size w/h) - recomputed by resetVideo(). This is the
		// source of truth for the zoom/shift/aspect settings, and the metrics
		// the menu's game screenshot background (gameScreenPng) is drawn with.
		float quadX, quadY, quadWidth, quadHeight;

		// The same quad in physical pixels of each render target (top-left
		// x/y, size w/h), derived from the canvas placement above by the
		// canvas-to-target stretch. This is what actually gets drawn, and what
		// scaling/filtering needs (source-to-output scale = size / vwidth,vheight).
		struct TargetPlacement { float x, y, w, h; };
		TargetPlacement placement[OUTPUT_TARGET_COUNT];

		// Raw NES framebuffer (palette indices) passed to the most recent
		// presentFrame() - reused by readFrameRGB24() so screenshots don't
		// need a GX2 texture readback. Only valid to dereference synchronously
		// (TakeScreenshot() runs before the emulator core produces another frame).
		const uint8_t* lastBuffer;
};
