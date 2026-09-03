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
};
