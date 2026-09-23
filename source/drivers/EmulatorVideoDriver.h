/****************************************************************************
 * libgui
 *
 * Daryl Borth 2009-2026
 * EmulatorVideoDriver.h
 ***************************************************************************/
#pragma once

#include <stdint.h>
#include "VideoDriver.h"

class EmulatorVideoDriver
{
	public:
		virtual ~EmulatorVideoDriver() = default;

		virtual void init(VideoDriver* videoDriver) = 0;
		virtual void resetVideo() = 0;
		virtual void presentFrame(const uint8_t* buffer) = 0;
		virtual void presentStereoFrame(const uint8_t* bufferLeft, const uint8_t* bufferRight) = 0;
		virtual void readFrameRGB24(uint8_t* dst) = 0;

		// Maps a UI-canvas pointer position (IR pointer / touch, in the same canvas coordinates as 
		// InputPadData::cursor_x/y) to a pixel of the emulated console's framebuffer, following the 
		// game's actual on-screen placement (aspect correction, zoom, shift, cropping)
		virtual bool mapPointerToFrame(float canvasX, float canvasY, bool onGamePad, int* frameX, int* frameY)
		{
			(void)canvasX; (void)canvasY; (void)onGamePad; (void)frameX; (void)frameY;
			return false;
		}

		// The part of the framebuffer that is actually shown, in framebuffer pixels: hidden overscan
		// (Cropping) is excluded. Returns false if the whole framebuffer is shown / the driver doesn't say.
		virtual bool getVisibleFrameRect(int* x, int* y, int* w, int* h)
		{
			(void)x; (void)y; (void)w; (void)h;
			return false;
		}
};
