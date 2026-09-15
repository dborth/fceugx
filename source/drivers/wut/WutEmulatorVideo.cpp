/****************************************************************************
 * FCE Ultra GX
 *
 * Daryl Borth 2026
 *
 * WutEmulatorVideo.cpp
 ***************************************************************************/
#include <coreinit/memdefaultheap.h>
#include <gx2/mem.h>
#include <whb/gfx.h>

#include "WutEmulatorVideo.h"
#include "WutVideoDriver.h"
#include "shaders/Texture2DShader.h"
#include "../../fceugx.h"
#include "../../videosupport.h"

bool shutter_3d_mode, anaglyph_3d_mode, eye_3d;

void Check3D() { }

namespace
{
	void PixelRectToNdc(float x, float y, float w, float h, int designWidth, int designHeight, float offset[3], float scale[3])
	{
		float centerPxX = x + w * 0.5f;
		float centerPxY = y + h * 0.5f;

		offset[0] = (centerPxX / designWidth) * 2.0f - 1.0f;
		offset[1] = 1.0f - (centerPxY / designHeight) * 2.0f;
		offset[2] = 0.0f;

		scale[0] = w / designWidth;
		scale[1] = h / designHeight;
		scale[2] = 1.0f;
	}
}

WutEmulatorVideo::WutEmulatorVideo()
	: videoDriver(nullptr), texture(nullptr)
	, quadX(0), quadY(0), quadWidth(0), quadHeight(0)
	, lastBuffer(nullptr)
{
	GX2InitSampler(&sampler, GX2_TEX_CLAMP_MODE_CLAMP, GX2_TEX_XY_FILTER_MODE_LINEAR);
}

WutEmulatorVideo::~WutEmulatorVideo()
{
	destroyTexture();
}

void WutEmulatorVideo::init(VideoDriver* driver)
{
	videoDriver = static_cast<WutVideoDriver*>(driver);
}

/****************************************************************************
 * resetVideo
 *
 * Recomputes the on-screen placement of the game quad. Called once by
 * fceugx.cpp whenever emulation (re)starts, so game settings changed from
 * the menu (aspect ratio, zoom, shift) take effect on the next run.
 ***************************************************************************/
void WutEmulatorVideo::resetVideo()
{
	float screenWidth = (float)videoDriver->getScreenWidth();
	float screenHeight = (float)videoDriver->getScreenHeight();

	// Base size fills the design canvas at either the NES's native 4:3
	// shape, or stretched to 16:9 when the user asks for widescreen
	// correction - then user zoom/shift are layered on top.
	float baseHeight = screenHeight;
	float baseWidth = baseHeight * (EmuSettings.videoAspectRatioCorrection == VIDEO_ASPECT_RATIO_CORRECTION_16_9 ? (16.0f / 9.0f) : (4.0f / 3.0f));

	quadWidth = baseWidth * EmuSettings.videoZoomHor;
	quadHeight = baseHeight * EmuSettings.videoZoomVert;

	quadX = ((screenWidth - quadWidth) * 0.5f) + EmuSettings.videoXshift;
	quadY = ((screenHeight - quadHeight) * 0.5f) + EmuSettings.videoYshift;

	// Mirror the placement into gameScreenPng so the pause menu's blurred
	// background reproduces the same on-screen rect as readFrameRGB24()
	gameScreenPng.width = NES_WIDTH;
	gameScreenPng.height = NES_HEIGHT;
	gameScreenPng.scaleX = quadWidth / (float)NES_WIDTH;
	gameScreenPng.scaleY = quadHeight / (float)NES_HEIGHT;
	gameScreenPng.xoffset = (int)((quadX + quadWidth * 0.5f) - (screenWidth * 0.5f));
	gameScreenPng.yoffset = (int)((quadY + quadHeight * 0.5f) - (screenHeight * 0.5f));
}

uint8_t WutEmulatorVideo::getBorderWidth() const
{
	if(EmuSettings.hideoverscan == HIDEOVERSCAN_HORIZONTAL || EmuSettings.hideoverscan == HIDEOVERSCAN_BOTH)
		return 8;
	return 0;
}

uint8_t WutEmulatorVideo::getBorderHeight() const
{
	if(EmuSettings.hideoverscan == HIDEOVERSCAN_VERTICAL || EmuSettings.hideoverscan == HIDEOVERSCAN_BOTH)
		return 8;
	return 0;
}

/****************************************************************************
 * rebuildTexture / destroyTexture
 *
 * The game texture is recreated only when the emulator's rendered
 * width/height actually changes (see presentFrame), not every frame.
 ***************************************************************************/
void WutEmulatorVideo::destroyTexture()
{
	if (!texture)
		return;

	if (texture->surface.image)
		MEMFreeToDefaultHeap(texture->surface.image);

	delete texture;
	texture = nullptr;
}

void WutEmulatorVideo::rebuildTexture(int width, int height)
{
	destroyTexture();

	if (width <= 0 || height <= 0)
		return;

	texture = new GX2Texture();
	GX2InitTexture(texture, width, height, 1, 0, GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8, GX2_SURFACE_DIM_TEXTURE_2D, GX2_TILE_MODE_LINEAR_ALIGNED);

	GX2CalcSurfaceSizeAndAlignment(&texture->surface);
	GX2InitTextureRegs(texture);

	texture->surface.image = MEMAllocFromDefaultHeapEx(texture->surface.imageSize, texture->surface.alignment);
	if (!texture->surface.image)
	{
		delete texture;
		texture = nullptr;
	}
}

/****************************************************************************
 * uploadFrame
 *
 * Converts the emulator's raw framebuffer (one palette-index byte per
 * pixel, NES_WIDTH stride) into the linear RGBA8 texture, dropping the
 * hidden-overscan border (if any) in the process rather than uploading it
 * and cropping later - the texture is already sized to match by presentFrame.
 ***************************************************************************/
void WutEmulatorVideo::uploadFrame(const uint8_t* buffer)
{
	if(!texture || !texture->surface.image || !buffer)
		return;

	uint8_t borderWidth = getBorderWidth();
	uint8_t borderHeight = getBorderHeight();

	const uint8_t* src = buffer + (borderHeight * NES_WIDTH) + borderWidth;
	uint8_t* dst = static_cast<uint8_t*>(texture->surface.image);
	const uint32_t dstStride = texture->surface.pitch * 4;

	for(uint32_t y = 0; y < texture->surface.height; y++)
	{
		const uint8_t* srcRow = src + y * NES_WIDTH;
		uint8_t* dstRow = dst + y * dstStride;

		for(uint32_t x = 0; x < texture->surface.width; x++)
		{
			const pcpal& color = pcpalette[srcRow[x]];
			uint8_t* px = dstRow + x * 4;
			px[0] = color.r;
			px[1] = color.g;
			px[2] = color.b;
			px[3] = 255;
		}
	}

	GX2Invalidate(GX2_INVALIDATE_MODE_CPU_TEXTURE, texture->surface.image, texture->surface.imageSize);
}

/****************************************************************************
 * drawQuad
 ***************************************************************************/
void WutEmulatorVideo::drawQuad()
{
	if (!texture || !videoDriver->isForeground())
		return;

	// Cheap enough to just re-set every frame rather than tracking whether
	// the setting changed since the last draw.
	GX2InitSampler(&sampler, GX2_TEX_CLAMP_MODE_CLAMP,
		EmuSettings.videoBilinearFilter ? GX2_TEX_XY_FILTER_MODE_LINEAR : GX2_TEX_XY_FILTER_MODE_POINT);

	float offset[3];
	float scale[3];
	PixelRectToNdc(quadX, quadY, quadWidth, quadHeight, videoDriver->getScreenWidth(), videoDriver->getScreenHeight(), offset, scale);

	float colorIntensity[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	Texture2DShader* shader = Texture2DShader::instance();

	auto drawPass = [&]() {
		shader->setShaders();
		shader->setAttributeBuffer();
		shader->setAngle(0.0f);
		shader->setOffset(offset);
		shader->setScale(scale);
		shader->setColorIntensity(colorIntensity);
		shader->clearBlur();
		shader->setTextureAndSampler(texture, &sampler);
		shader->draw(GX2_PRIMITIVE_MODE_QUADS, 4);
	};

	WHBGfxBeginRenderTV(); drawPass();
	WHBGfxBeginRenderDRC(); drawPass();
}

/****************************************************************************
 * presentFrame
 ***************************************************************************/
void WutEmulatorVideo::presentFrame(const uint8_t* buffer)
{
	if(!buffer)
		return;

	lastBuffer = buffer;

	int width = NES_WIDTH - (getBorderWidth() << 1);
	int height = NES_HEIGHT - (getBorderHeight() << 1);

	if(!texture || (int)texture->surface.width != width || (int)texture->surface.height != height)
		rebuildTexture(width, height);

	uploadFrame(buffer);
	drawQuad();

	videoDriver->presentBuffer();
}

/****************************************************************************
 * presentStereoFrame
 ***************************************************************************/
void WutEmulatorVideo::presentStereoFrame(const uint8_t* bufferLeft, const uint8_t*)
{
	presentFrame(bufferLeft);
}

/****************************************************************************
 * readFrameRGB24
 *
 * Converts straight from the emulator's raw framebuffer (same source as
 * uploadFrame) rather than reading back the GX2 texture - simpler, and
 * avoids depending on GX2 surface padding/pitch for a CPU readback.
 ***************************************************************************/
void WutEmulatorVideo::readFrameRGB24(uint8_t* dst)
{
	if(!dst || !lastBuffer)
		return;

	uint8_t borderWidth = getBorderWidth();
	uint8_t borderHeight = getBorderHeight();

	uint32_t width = NES_WIDTH - (borderWidth << 1);
	uint32_t height = NES_HEIGHT - (borderHeight << 1);

	const uint8_t* src = lastBuffer + (borderHeight * NES_WIDTH) + borderWidth;

	for(uint32_t y = 0; y < height; y++)
	{
		const uint8_t* srcRow = src + y * NES_WIDTH;
		uint8_t* dstRow = dst + y * width * 3;

		for(uint32_t x = 0; x < width; x++)
		{
			const pcpal& color = pcpalette[srcRow[x]];
			dstRow[x * 3 + 0] = color.r;
			dstRow[x * 3 + 1] = color.g;
			dstRow[x * 3 + 2] = color.b;
		}
	}

	// Reflects the (possibly cropped) size back to TakeScreenshot()/menu.cpp -
	// resetVideo() leaves gameScreenPng.scaleX/scaleY/xoffset/yoffset valid
	// for this smaller size.
	gameScreenPng.width = width;
	gameScreenPng.height = height;
}
