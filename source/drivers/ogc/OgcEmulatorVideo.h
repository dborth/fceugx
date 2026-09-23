/****************************************************************************
 * FCE Ultra GX
 *
 * Daryl Borth 2008-2026
 * Daryl Borth 2008-2026
 *
 * OgcEmulatorVideo.h
 ***************************************************************************/
#pragma once

#include <gccore.h>
#include <stdint.h>
#include "../EmulatorVideoDriver.h"

// Original (240p) NES video modes. OgcVideoDriver::findVideoMode() primes the
// viTVMode/viYOrigin fields of these to match the detected broadcast standard;
// OgcEmulatorVideo::resetVideo() selects one of them directly when
// VIDEOMODE_ORIGINAL_240P is active.
extern GXRModeObj PAL_240p;
extern GXRModeObj NTSC_240p;

class OgcVideoDriver;

class OgcEmulatorVideo : public EmulatorVideoDriver
{
	public:
		OgcEmulatorVideo() : videoDriver(nullptr), updateVideo(true), frameX(0), frameY(0), frameW(0), frameH(0) {}

		void init(VideoDriver* videoDriver) override;
		void resetVideo() override;
		void presentFrame(const uint8_t* buffer) override;
		void presentStereoFrame(const uint8_t* bufferLeft, const uint8_t* bufferRight) override;
		void readFrameRGB24(uint8_t* dst) override;
		bool mapPointerToFrame(float canvasX, float canvasY, bool onGamePad, int* outX, int* outY) override;
		bool getVisibleFrameRect(int* x, int* y, int* w, int* h) override;

	private:
		void updateFilterScale();
		void updateScaling();
		void applyOverscanScissor(uint8_t borderwidth, uint8_t borderheight);
		void initScanlineTexture();
		void setupScanlineFilterTEV();
		bool shouldApplyScanlines();
		void drawInit();
		void drawSquare(Mtx v);
		void resetFbWidth(int width, GXRModeObj *rmode);
		void untileRGB5A3ToRGB24(const void * tiledTexture, int tex_w, uint32_t crop_x, uint32_t crop_y, uint32_t crop_w, uint32_t crop_h, uint8_t* dst);
		uint8_t getBorderWidth();
		uint8_t getBorderHeight();

		OgcVideoDriver* videoDriver;
		bool updateVideo;

		// The game quad's on-screen rect in UI-canvas pixels (top-left x/y, size w/h)
		// Recomputed by updateScaling(); used to map the pointer to the frame.
		float frameX, frameY, frameW, frameH;
};
