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
#include "WutScaleFX.h"
#include "WutOutputFilter.h"
#include "WutUpscaleFilters.h"
#include "shaders/Texture2DShader.h"
#include "../../fceugx.h"
#include "../../videosupport.h"

bool shutter_3d_mode, anaglyph_3d_mode, eye_3d;

void Check3D() { }

namespace
{
	// Darkness of the scanline gaps (0..1) when Scanline Overlay is on
	const float SCANLINE_STRENGTH = 0.5f;

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
	, frame{ {0, 0, 0, 0}, {0, 0, 0, 0} }
	, placement{ {0, 0, 0, 0}, {0, 0, 0, 0} }
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
 * Recomputes the on-screen placement of the game quad on every output target
 ***************************************************************************/
void WutEmulatorVideo::resetVideo()
{
	const float canvasWidth = (float)videoDriver->getScreenWidth();
	const float canvasHeight = (float)videoDriver->getScreenHeight();

	const bool correct = EmuSettings.videoAspectRatioCorrection == VIDEO_ASPECT_RATIO_CORRECTION_16_9;

	for (int i = 0; i < OUTPUT_TARGET_COUNT; i++)
	{
		const OutputTarget target = static_cast<OutputTarget>(i);
		const float tw = (float) videoDriver->getTargetWidth(target);
		const float th = (float) videoDriver->getTargetHeight(target);

		// Base size as a fraction of the target: full height, and either the
		// full width or the width that gives a 4:3 picture on this target
		const float baseW = correct ? ((4.0f / 3.0f) * th / tw) : 1.0f;
		const float baseH = 1.0f;

		FrameRect& r = frame[i];
		r.w = baseW * EmuSettings.videoZoomHor;
		r.h = baseH * EmuSettings.videoZoomVert;

		// Centered, then shifted. The shift setting is in UI-canvas pixels;
		// as a fraction of the canvas it means the same on every target.
		r.x = (1.0f - r.w) * 0.5f + (float)EmuSettings.videoXshift / canvasWidth;
		r.y = (1.0f - r.h) * 0.5f + (float)EmuSettings.videoYshift / canvasHeight;

		placement[i].x = r.x * tw;
		placement[i].y = r.y * th;
		placement[i].w = r.w * tw;
		placement[i].h = r.h * th;
	}

	// The menu's blurred background is drawn on the canvas, so mirror the TV
	// (primary display) rect into canvas pixels for it
	const FrameRect& tv = frame[static_cast<int>(OutputTarget::TV)];
	quadX = tv.x * canvasWidth;
	quadY = tv.y * canvasHeight;
	quadWidth = tv.w * canvasWidth;
	quadHeight = tv.h * canvasHeight;

	syncScreenshotMetrics(NES_WIDTH - (getBorderWidth() << 1), NES_HEIGHT - (getBorderHeight() << 1));
}

/****************************************************************************
 * mapPointerToFrame
 *
 * Maps a UI-canvas pointer position to a pixel of the NES framebuffer
 * (XBuf coordinates, which is what the Zapper reads), through the game's
 * actual placement on the output the pointer is on.
 ***************************************************************************/
bool WutEmulatorVideo::mapPointerToFrame(float canvasX, float canvasY, bool onGamePad, int* frameX, int* frameY)
{
	if (!frameX || !frameY)
		return false;

	const FrameRect& r = frame[static_cast<int>(onGamePad ? OutputTarget::DRC : OutputTarget::TV)];
	if (r.w <= 0.0f || r.h <= 0.0f) // resetVideo() hasn't run yet
		return false;

	float u = ((canvasX / (float)videoDriver->getScreenWidth()) - r.x) / r.w;
	float v = ((canvasY / (float)videoDriver->getScreenHeight()) - r.y) / r.h;
	u = u < 0.0f ? 0.0f : (u > 1.0f ? 1.0f : u);
	v = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);

	const int borderW = getBorderWidth();
	const int borderH = getBorderHeight();
	const int visibleW = NES_WIDTH - (borderW << 1);
	const int visibleH = NES_HEIGHT - (borderH << 1);

	int x = borderW + (int)(u * visibleW);
	int y = borderH + (int)(v * visibleH);
	if (x > borderW + visibleW - 1) x = borderW + visibleW - 1;
	if (y > borderH + visibleH - 1) y = borderH + visibleH - 1;

	*frameX = x;
	*frameY = y;
	return true;
}

/****************************************************************************
 * getVisibleFrameRect
 *
 * Cropped overscan is removed from the uploaded texture entirely and the
 * remainder is stretched to the game rect, so only this region is reachable
 ***************************************************************************/
bool WutEmulatorVideo::getVisibleFrameRect(int* x, int* y, int* w, int* h)
{
	if (!x || !y || !w || !h)
		return false;

	*x = getBorderWidth();
	*y = getBorderHeight();
	*w = NES_WIDTH - (getBorderWidth() << 1);
	*h = NES_HEIGHT - (getBorderHeight() << 1);
	return true;
}

/****************************************************************************
 * syncScreenshotMetrics
 *
 * Publishes the game quad's on-screen rect through gameScreenPng, which the
 * pause menu's blurred background (CreateBlurredGameTexture) draws from
 ***************************************************************************/
void WutEmulatorVideo::syncScreenshotMetrics(int width, int height)
{
	float screenWidth = (float)videoDriver->getScreenWidth();
	float screenHeight = (float)videoDriver->getScreenHeight();

	gameScreenPng.width = width;
	gameScreenPng.height = height;

	// The menu truncates width * scaleX to an int; the 0.5f keeps float
	// rounding error from dropping the last pixel and leaving a hairline gap
	gameScreenPng.scaleX = (quadWidth + 0.5f) / (float)width;
	gameScreenPng.scaleY = (quadHeight + 0.5f) / (float)height;
	gameScreenPng.xoffset = (int)((quadX + quadWidth * 0.5f) - (screenWidth * 0.5f));
	gameScreenPng.yoffset = (int)((quadY + quadHeight * 0.5f) - (screenHeight * 0.5f));
}

uint8_t WutEmulatorVideo::getBorderWidth() const
{
	if(EmuSettings.hideOverscan == HIDEOVERSCAN_HORIZONTAL || EmuSettings.hideOverscan == HIDEOVERSCAN_BOTH)
		return 8;
	return 0;
}

uint8_t WutEmulatorVideo::getBorderHeight() const
{
	if(EmuSettings.hideOverscan == HIDEOVERSCAN_VERTICAL || EmuSettings.hideOverscan == HIDEOVERSCAN_BOTH)
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

	float colorIntensity[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	Texture2DShader* shader = Texture2DShader::instance();

	// NDC placement of the game quad on a target, from its physical-pixel rect
	auto placementNdc = [&](OutputTarget target, float offset[3], float scale[3]) {
		const TargetPlacement& p = placement[static_cast<int>(target)];
		PixelRectToNdc(p.x, p.y, p.w, p.h, videoDriver->getTargetWidth(target), videoDriver->getTargetHeight(target), offset, scale);
	};

	auto drawPass = [&](OutputTarget target) {
		float offset[3];
		float scale[3];
		placementNdc(target, offset, scale);

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

	const bool sharp = EmuSettings.videoUpscalingFilter == UPSCALE_SHARP_BILINEAR;
	const float scanlines = EmuSettings.videoScanlines ? SCANLINE_STRENGTH : 0.0f;

	// Output filter: sharp bilinear and/or scanlines. Returns false if it is unavailable.
	auto outputFilterPass = [&](OutputTarget target, const GX2Texture* tex, bool linear, bool sharpSampling) {
		const TargetPlacement& p = placement[static_cast<int>(target)];

		WutOutputFilter::Params pp;
		pp.texture = tex;
		placementNdc(target, pp.offset, pp.scale);
		pp.outWidth = p.w;
		pp.outHeight = p.h;
		pp.linear = linear;
		pp.sharp = sharpSampling;
		pp.scanlineStrength = scanlines;
		pp.sourceLines = (float) texture->surface.height;
		return WutOutputFilter::instance()->draw(pp);
	};

	// The frame texture on a target: plain textured quad, or the output filter when it has work to do
	auto drawGame = [&](OutputTarget target) {
		if ((sharp || scanlines > 0.0f) && outputFilterPass(target, texture, EmuSettings.videoBilinearFilter, sharp))
			return;
		drawPass(target);
	};

	// ScaleFX (TV output only)
	WutScaleFX* scalefx = WutScaleFX::instance();
	bool useScaleFX = false;

	if (EmuSettings.videoUpscalingFilter == UPSCALE_SCALEFX)
	{
		if (scalefx->prepare(texture->surface.width, texture->surface.height))
		{
			scalefx->run(texture);
			useScaleFX = true;
		}
	}
	else
	{
		scalefx->release();
	}

	WHBGfxBeginRenderTV();
	if (useScaleFX)
	{
		// Scanlines go through the output filter, otherwise the ScaleFX final stage draws it
		if (scanlines <= 0.0f || !outputFilterPass(OutputTarget::TV, scalefx->outputTexture(), true, false))
		{
			float offset[3];
			float scale[3];
			placementNdc(OutputTarget::TV, offset, scale);
			scalefx->drawTV(offset, scale);
		}
	}
	else
	{
		drawGame(OutputTarget::TV);
	}
	WHBGfxBeginRenderDRC(); drawGame(OutputTarget::DRC);
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

	// Reflects the (possibly cropped) size back to TakeScreenshot()/menu.cpp,
	// with the scale re-derived for that size so it still fills the quad
	syncScreenshotMetrics((int)width, (int)height);
}
