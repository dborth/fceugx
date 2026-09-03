/****************************************************************************
 * FCE Ultra
 * Nintendo Wii/Gamecube Port
 *
 * Daryl Borth 2008-2026
 *
 * OgcEmulatorVideo.cpp
 ***************************************************************************/
#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ogc/texconv.h>
#include <ogc/machine/processor.h>

#include "OgcEmulatorVideo.h"
#include "OgcVideoDriver.h"
#include "OgcEmulatorAudio.h"
#include "../../fceugx.h"
#include "../../fceusupport.h"
#include "../../gcvideo.h"
#include "../../menu.h"
#include "../../pad.h"
#include "videofilters.h"

/*** 3D GX ***/
#define TEX_WIDTH 512
#define TEX_HEIGHT 512
#define TEXTUREMEM_SIZE (TEX_WIDTH * TEX_HEIGHT * 2)

/*** Texture memory ***/
static unsigned char texturemem[TEXTUREMEM_SIZE] ATTRIBUTE_ALIGN (32);
static unsigned char scanline_tex_data[32] ATTRIBUTE_ALIGN (32);
static GXTexObj texobj;
static GXTexObj scanlineTexObj;
static Mtx view;

bool shutter_3d_mode, anaglyph_3d_mode, eye_3d;

static unsigned short anaglyph555[64][64]; // Texture map left right combination anaglyph palette
static void GenerateAnaglyphPalette(); // function prototype for generating the anaglyph palette

/* New texture based scaler */
typedef struct tagcamera
{
  guVector pos;
  guVector up;
  guVector view;
}
camera;

/*** Square Matrix
     This structure controls the size of the image on the screen.
	 Think of the output as a -80 x 80 by -60 x 60 graph.
***/
static s16 square[] ATTRIBUTE_ALIGN (32) =
{
  /*
   * X,   Y,  Z
   * Values set are for roughly 4:3 aspect
   */
   -NES_WIDTH,  NES_HEIGHT, 0,	// 0
    NES_WIDTH,  NES_HEIGHT, 0,	// 1
    NES_WIDTH, -NES_HEIGHT, 0,	// 2
   -NES_WIDTH, -NES_HEIGHT, 0	// 3
};


static camera cam = { {0.0F, 0.0F, 0.0F},
{0.0F, 0.5F, 0.0F},
{0.0F, 0.0F, -0.5F}
};

/***
*** Custom Video modes (used to emulate original console video modes)
***/

/** Original NES PAL Resolutions: **/

/* 240 lines progressive (PAL 50Hz) */
GXRModeObj PAL_240p =
{
	VI_TVMODE_PAL_DS,       // viDisplayMode
	512,             // fbWidth
	240,             // efbHeight
	240,             // xfbHeight
	(VI_MAX_WIDTH_PAL - 644)/2,         // viXOrigin
	(VI_MAX_HEIGHT_PAL/2 - 480/2)/2,        // viYOrigin
	644,             // viWidth
	480,             // viHeight
	VI_XFBMODE_SF,   // xFBmode
	GX_FALSE,        // field_rendering
	GX_FALSE,        // aa

  // sample points arranged in increasing Y order
        {
                {6,6},{6,6},{6,6},  // pix 0, 3 sample points, 1/12 units, 4 bits each
                {6,6},{6,6},{6,6},  // pix 1
                {6,6},{6,6},{6,6},  // pix 2
                {6,6},{6,6},{6,6}   // pix 3
        },

  // vertical filter[7], 1/64 units, 6 bits each
        {
                 0,         // line n-1
                 0,         // line n-1
                21,         // line n
                22,         // line n
                21,         // line n
                 0,         // line n+1
                 0          // line n+1
        }
};

/** Original NES NTSC Resolutions: **/

/* 240 lines progressive (NTSC or PAL 60Hz) */
GXRModeObj NTSC_240p =
{
	VI_TVMODE_EURGB60_DS,      // viDisplayMode
	512,             // fbWidth
	240,             // efbHeight
	240,             // xfbHeight
	(VI_MAX_WIDTH_NTSC - 644)/2,	// viXOrigin
	(VI_MAX_HEIGHT_NTSC/2 - 480/2)/2,	// viYOrigin
	644,             // viWidth
	480,             // viHeight
	VI_XFBMODE_SF,   // xFBmode
	GX_FALSE,        // field_rendering
	GX_FALSE,        // aa

  // sample points arranged in increasing Y order
        {
                {6,6},{6,6},{6,6},  // pix 0, 3 sample points, 1/12 units, 4 bits each
                {6,6},{6,6},{6,6},  // pix 1
                {6,6},{6,6},{6,6},  // pix 2
                {6,6},{6,6},{6,6}   // pix 3
        },

  // vertical filter[7], 1/64 units, 6 bits each
        {
                  0,         // line n-1
                  0,         // line n-1
                 21,         // line n
                 22,         // line n
                 21,         // line n
                  0,         // line n+1
                  0          // line n+1
        }
};

/* TV Modes table */
static GXRModeObj *tvmodes[2] = {
	&NTSC_240p, &PAL_240p
};

static int fscale;

void OgcEmulatorVideo::updateFilterScale() {
	if (GCSettings.videoUpscalingFilter != FILTER_NONE && !shutter_3d_mode && !anaglyph_3d_mode)
	{
		fscale = GetFilterScale();
	}
	else {
		fscale = 1;
	}
}

/****************************************************************************
 * ApplyOverscanScissor
 *
 * Dynamically calculates a GX Scissor box to crop out the overscan borders
 * at the hardware level. This perfectly hides overscan without modifying
 * the CPU-bound filters, preventing memory stalls or stretching.
 ***************************************************************************/
void OgcEmulatorVideo::applyOverscanScissor(uint8_t borderwidth, uint8_t borderheight)
{
	if (borderwidth == 0 && borderheight == 0)
	{
		// Reset to full EFB screen
		GX_SetScissor(0, 0, vmode->fbWidth, vmode->efbHeight);
		return;
	}

	// The quad vertex coordinates in 'square' represent the physical dimensions of the
	// 256x240 NES space, scaled and shifted by user settings.
	// square[0] = Left, square[3] = Right, square[1] = Top, square[7] = Bottom.
	// Note: World Y is up (positive), EFB Y is down (positive).
	f32 q_left = (vmode->fbWidth / 2.0f) + square[0];
	f32 q_right = (vmode->fbWidth / 2.0f) + square[3];
	f32 q_top = (vmode->efbHeight / 2.0f) - square[1];
	f32 q_bottom = (vmode->efbHeight / 2.0f) - square[7];

	// Calculate how many EFB screen pixels correspond to the NES border count
	f32 crop_w = (q_right - q_left) * ((f32)borderwidth / (f32)NES_WIDTH);
	f32 crop_h = (q_bottom - q_top) * ((f32)borderheight / (f32)NES_HEIGHT);

	// Apply crop to the quad bounds to form the Scissor box
	s32 sc_x = (s32)(q_left + crop_w);
	s32 sc_y = (s32)(q_top + crop_h);
	s32 sc_w = (s32)((q_right - crop_w) - sc_x);
	s32 sc_h = (s32)((q_bottom - crop_h) - sc_y);

	// Safety clamp to EFB boundaries to prevent hardware crashes
	if (sc_x < 0) { sc_w += sc_x; sc_x = 0; }
	if (sc_y < 0) { sc_h += sc_y; sc_y = 0; }
	if (sc_x + sc_w > (s32)vmode->fbWidth) sc_w = vmode->fbWidth - sc_x;
	if (sc_y + sc_h > (s32)vmode->efbHeight) sc_h = vmode->efbHeight - sc_y;

	if (sc_w > 0 && sc_h > 0)
	{
		GX_SetScissor((u32)sc_x, (u32)sc_y, (u32)sc_w, (u32)sc_h);
	}
	else
	{
		GX_SetScissor(0, 0, 0, 0); // Hide completely if cropped out of bounds
	}
}

/****************************************************************************
 * Scanline Support Functions
 ***************************************************************************/
void OgcEmulatorVideo::initScanlineTexture()
{
	// GX_TF_I8 represents one byte per pixel.
	// We create an 8x4 tile: Rows 0 and 2 are white (0xFF), Rows 1 and 3 are dark (0xA0).
	for (int y = 0; y < 4; y++) {
		u8 intensity = (y % 2 == 0) ? 0xFF : 0xA0; // 0xA0 controls the scanline darkness
		for (int x = 0; x < 8; x++) {
			scanline_tex_data[y * 8 + x] = intensity;
		}
	}

	// Flush the CPU data cache. GX reads directly from main memory.
	DCStoreRange(scanline_tex_data, 32);

	// Initialize the texture object. Wrap modes MUST be GX_REPEAT to tile across the screen.
	GX_InitTexObj(&scanlineTexObj, scanline_tex_data, 8, 4, GX_TF_I8, GX_REPEAT, GX_REPEAT, GX_FALSE);

	// Filter mode MUST be GX_NEAR. GX_LINEAR will blur the lines into a muddy gray.
	GX_InitTexObjFilterMode(&scanlineTexObj, GX_NEAR, GX_NEAR);

	// Load the scanline texture into MAP1
	GX_LoadTexObj(&scanlineTexObj, GX_TEXMAP1);
}

void OgcEmulatorVideo::setupScanlineFilterTEV()
{
	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0);

	// Allow a second texture coordinate to be passed to the vertex stream
	GX_SetVtxDesc(GX_VA_TEX1, GX_DIRECT);

	// Enable two textures and two TEV stages
	GX_SetNumTexGens(2);
	GX_SetNumTevStages(2);
	GX_SetNumChans(0);

	// Configure Texture Coordinate Generation for both textures
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
	GX_SetTexCoordGen(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX1, GX_IDENTITY);

	// --- STAGE 0: Sample the Game Screen ---
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
	GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC);
	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);

	// Configure Stage 0 Alpha path
	GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);
	GX_SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);

	// --- STAGE 1: Multiply by Scanlines ---
	GX_SetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD1, GX_TEXMAP1, GX_COLORNULL);
	// Formula: d + ((1.0 - c) * a + c * b)
	// By setting: a=ZERO, b=CPREV, c=TEXC, d=ZERO -> (TEXC * CPREV)
	GX_SetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_CPREV, GX_CC_TEXC, GX_CC_ZERO);
	GX_SetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);

	// Configure Stage 1 Alpha path (Pass-through blend)
	GX_SetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_APREV, GX_CA_TEXA, GX_CA_ZERO);
	GX_SetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
}

bool OgcEmulatorVideo::shouldApplyScanlines()
{
	return GCSettings.videoScanlines && !shutter_3d_mode && !anaglyph_3d_mode && vmode->efbHeight > 300;
}

/****************************************************************************
 * Scaler Support Functions
 ***************************************************************************/
void OgcEmulatorVideo::drawInit()
{
	GX_ClearVtxDesc ();
	GX_SetVtxDesc (GX_VA_POS, GX_INDEX8);
	GX_SetVtxDesc (GX_VA_TEX0, GX_DIRECT);

	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	if(shouldApplyScanlines()) {
		setupScanlineFilterTEV();
	}
	else {
		GX_SetNumTexGens (1);
		GX_SetNumTevStages (1);
		GX_SetNumChans (0);

		GX_SetTexCoordGen (GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

		GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);
		GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
	}

	GX_SetArray (GX_VA_POS, square, 3 * sizeof (s16));

	memset (&view, 0, sizeof (Mtx));
	guLookAt(view, &cam.pos, &cam.up, &cam.view);
	GX_LoadPosMtxImm (view, GX_PNMTX0);

	GX_InvVtxCache ();	// update vertex cache
}

static inline void
draw_vert (u8 pos, f32 s, f32 t)
{
	GX_Position1x8 (pos);
	GX_TexCoord2f32 (s, t);
}

void OgcEmulatorVideo::drawSquare(Mtx v)
{
	Mtx m;			// model matrix.
	Mtx mv;			// modelview matrix.

	guMtxIdentity (m);
	guMtxTransApply (m, m, 0, 0, -100);
	guMtxConcat (v, m, mv);

	GX_LoadPosMtxImm (mv, GX_PNMTX0);
	GX_Begin (GX_QUADS, GX_VTXFMT0, 4);

	bool scanlines = shouldApplyScanlines();

	if(scanlines) {
		f32 quad_width = (f32)(square[3] - square[0]);
		f32 quad_height = (f32)(square[1] - square[7]);

		f32 u_repeat = quad_width / 8.0f;
		f32 v_repeat = quad_height / 4.0f;

		f32 u_off = 0.0625f;
		f32 v_off = 0.125f;

		draw_vert (0, 0.0f, 0.0f); // TEX0
		GX_TexCoord2f32 (u_off, v_off); // TEX1

		draw_vert (1, 1.0f, 0.0f); // TEX0
		GX_TexCoord2f32 (u_repeat + u_off, v_off); // TEX1

		draw_vert (2, 1.0f, 1.0f); // TEX0
		GX_TexCoord2f32 (u_repeat + u_off, v_repeat + v_off); // TEX1

		draw_vert (3, 0.0f, 1.0f); // TEX0
		GX_TexCoord2f32 (u_off, v_repeat + v_off); // TEX1
	}
	else {
		draw_vert (0, 0.0, 0.0);
		draw_vert (1, 1.0, 0.0);
		draw_vert (2, 1.0, 1.0);
		draw_vert (3, 0.0, 1.0);
	}
	GX_End ();

	if(scanlines) {
		// force identity matrix to ensure texture mapping is pristine and devoid of stray scaling
		Mtx texMtx;
		guMtxIdentity(texMtx);
		GX_LoadTexMtxImm(texMtx, GX_TEXMTX1, GX_MTX2x4);
	}
}

/****************************************************************************
 * UpdateScaling
 *
 * This function updates the quad aspect ratio.
 ***************************************************************************/
void OgcEmulatorVideo::updateScaling()
{
	int xscale, yscale;

	// update scaling
	if (GCSettings.videoMode == VIDEOMODE_ORIGINAL_240P)
	{
		xscale = 512 / 2; // use GX scaler instead VI
		yscale = NES_HEIGHT / 2;
	}
	else // unfiltered and filtered mode
	{
		xscale = NES_WIDTH;
		yscale = vmode->efbHeight / 2;
	}

	if (GCSettings.videoAspectRatioCorrection == VIDEO_ASPECT_RATIO_CORRECTION_16_9)
	{
		if(GCSettings.videoMode == VIDEOMODE_ORIGINAL_240P)
			xscale = (3*xscale)/4;
		else
			xscale = NES_WIDTH; // match the original console's width for "widescreen" to prevent flickering
	}

	xscale *= GCSettings.videoZoomHor;
	yscale *= GCSettings.videoZoomVert;

	// update vertex position matrix
	square[0] = square[9] = (-xscale) + GCSettings.videoXshift;
	square[3] = square[6] = (xscale) + GCSettings.videoXshift;
	square[1] = square[4] = (yscale) - GCSettings.videoYshift;
	square[7] = square[10] = (-yscale) - GCSettings.videoYshift;
	DCFlushRange (square, 32); // update memory BEFORE the GPU accesses it!

	updateFilterScale();

	GXRModeObj *menu_vmode = videoDriver->findVideoMode();

	// 1. Compensate for progressive/interlaced physical line density
	float viHeightAdjusted = (vmode->viHeight < 300) ? (vmode->viHeight * 2.0f) : (float)vmode->viHeight;
	float menuViHeightAdjusted = (menu_vmode->viHeight < 300) ? (menu_vmode->viHeight * 2.0f) : (float)menu_vmode->viHeight;

	// 2. Calculate physical fraction of the TV screen the hardware is utilizing
	float physical_width_ratio = (float)vmode->viWidth / (float)menu_vmode->viWidth;
	float physical_height_ratio = viHeightAdjusted / menuViHeightAdjusted;

	// 3. Calculate fraction of the EFB utilized by the game quad
	float width_frac  = (2.0f * xscale) / (float)vmode->fbWidth;
	float height_frac = (2.0f * yscale) / (float)vmode->efbHeight;

	// 4. Map completely into the Menu's 640x480 logical canvas
	float targetWidth  = videoDriver->getScreenWidth() * width_frac * physical_width_ratio;
	float targetHeight = videoDriver->getScreenHeight() * height_frac * physical_height_ratio;

	gameScreenPng.width  = NES_WIDTH * fscale;
	gameScreenPng.height = NES_HEIGHT * fscale;

	gameScreenPng.scaleX = targetWidth / (float)gameScreenPng.width;
	gameScreenPng.scaleY = targetHeight / (float)gameScreenPng.height;

	// 5. Shift calculations must map EFB distances physically through to the Menu canvas
	gameScreenPng.xoffset = GCSettings.videoXshift * (videoDriver->getScreenWidth() / (float)menu_vmode->viWidth) * ((float)vmode->viWidth / (float)vmode->fbWidth);
	gameScreenPng.yoffset = GCSettings.videoYshift * (videoDriver->getScreenHeight() / menuViHeightAdjusted) * (viHeightAdjusted / (float)vmode->efbHeight);

	drawInit ();
}

uint8_t OgcEmulatorVideo::getBorderWidth() {
	if(GCSettings.hideoverscan == HIDEOVERSCAN_HORIZONTAL || GCSettings.hideoverscan == HIDEOVERSCAN_BOTH)
		return 8;
	return 0;
}

uint8_t OgcEmulatorVideo::getBorderHeight() {
	if(GCSettings.hideoverscan == HIDEOVERSCAN_VERTICAL || GCSettings.hideoverscan == HIDEOVERSCAN_BOTH)
		return 8;
	return 0;
}

// Un-swizzles and crops a 4x4-tiled GX_TF_RGB5A3 texture directly to tightly packed RGB24
void OgcEmulatorVideo::untileRGB5A3ToRGB24(const void * tiledTexture, int tex_w, uint32_t crop_x, uint32_t crop_y, uint32_t crop_w, uint32_t crop_h, uint8_t* dst)
{
	int padded_width = (tex_w + 3) & ~3;

	const u16 * tex16 = (const u16 *) tiledTexture;

	for(u32 y = 0; y < crop_h; y++) {
		u32 tex_y = y + crop_y;
		int tile_y = tex_y / 4;
		int in_tile_y = tex_y % 4;

		for(u32 x = 0; x < crop_w; x++) {
			u32 tex_x = x + crop_x;
			int tile_x = tex_x / 4;
			int in_tile_x = tex_x % 4;

			int tex_pixel_idx = (tile_y * (padded_width / 4) + tile_x) * 16 + (in_tile_y * 4 + in_tile_x);
			u16 color = tex16[tex_pixel_idx];

			// Extract RGB555 from the RGB5A3 encoded integer
			u8 r = (color >> 10) & 0x1F;
			u8 g = (color >> 5) & 0x1F;
			u8 b = color & 0x1F;

			int out_idx = (y * crop_w + x) * 3;
			dst[out_idx]     = (r << 3) | (r >> 2);
			dst[out_idx + 1] = (g << 3) | (g >> 2);
			dst[out_idx + 2] = (b << 3) | (b >> 2);
		}
	}
}

/****************************************************************************
 * readFrameRGB24
 *
 * Un-swizzles the current texturemem screen into dst (RGB24), cropping out
 * hidden overscan borders when enabled. Used by TakeScreenshot(). Updates
 * gameScreenPng.width/height to match the (possibly cropped) output.
 ***************************************************************************/
void OgcEmulatorVideo::readFrameRGB24(uint8_t* dst)
{
	u32 crop_x = 0;
	u32 crop_y = 0;
	u32 crop_w = gameScreenPng.width;
	u32 crop_h = gameScreenPng.height;

	// Calculate physical crop bounds if overscan is hidden
	if(GCSettings.hideoverscan != HIDEOVERSCAN_OFF) {
		crop_x = getBorderWidth() * fscale;
		crop_y = getBorderHeight() * fscale;
		crop_w = gameScreenPng.width - (crop_x * 2);
		crop_h = gameScreenPng.height - (crop_y * 2);
	}

	// Read directly from texturemem and extract the bounding box
	untileRGB5A3ToRGB24(texturemem, gameScreenPng.width, crop_x, crop_y, crop_w, crop_h, dst);

	// Update GUI image dimensions so it knows it was physically truncated
	if(GCSettings.hideoverscan != HIDEOVERSCAN_OFF) {
		gameScreenPng.width = crop_w;
		gameScreenPng.height = crop_h;
	}
}

/****************************************************************************
 * Texture Generation Helper (Gekko/Broadway ASM Optimized)
 ****************************************************************************/

static void MakeTexture(const void *src, void *dst, s32 width, s32 height)
{
	// Calculate base offsets
	const s32 borderwidth = (256 - width) / 2;
	const s32 borderheight = (240 - height) / 2;

	// Initial pointers point directly to the top-left of the rendering area
	const u8 *srcBuf = (const u8 *)src + (borderheight << 8) + borderwidth;
	u8 *dstBuf = (u8 *)dst + (borderheight * 512) + (borderwidth * 8);

	u32 r_src_row=0, r_dst_row=0;
	u32 t0=0, t1=0, t2=0, t3=0, w0=0, w1=0;

	__asm__ __volatile__ (
		"srwi   %[width], %[width], 2\n"       // width_tiles = width / 4
		"srwi   %[height], %[height], 2\n"     // height_tiles = height / 4

	"2: mtctr   %[width]\n"                    // Inner loop X
		"mr     %[r_src_row], %[src]\n"        // Save row start anchors
		"mr     %[r_dst_row], %[dst]\n"

	"1: dcbz    0, %[dst]\n"                   // ZERO L1 CACHE (32 bytes)

		// ----------------------------------------------------
		// BLOCK 1: Row 0 & 1, Left Half
		// ----------------------------------------------------
		"lbz    %[t0], 0(%[src])\n"
		"lbz    %[t1], 1(%[src])\n"
		"lbz    %[t2], 256(%[src])\n"
		"lbz    %[t3], 257(%[src])\n"

		// Calculate offsets (index * 2 bytes)
		"slwi   %[t0], %[t0], 1\n"
		"slwi   %[t1], %[t1], 1\n"
		"slwi   %[t2], %[t2], 1\n"
		"slwi   %[t3], %[t3], 1\n"

		// Asynchronous Palette Lookups (lhzx)
		"lhzx   %[t0], %[pal], %[t0]\n"
		"lhzx   %[t1], %[pal], %[t1]\n"
		"lhzx   %[t2], %[pal], %[t2]\n"
		"lhzx   %[t3], %[pal], %[t3]\n"

		// Pack into 32-bit registers (w0, w1)
		"slwi   %[w0], %[t0], 16\n"
		"slwi   %[w1], %[t2], 16\n"
		"or     %[w0], %[w0], %[t1]\n"
		"or     %[w1], %[w1], %[t3]\n"

		// Force MSB (0x8000) for both 16-bit pixels in w0 and w1
		"oris   %[w0], %[w0], 0x8000\n"
		"ori    %[w0], %[w0], 0x8000\n"
		"oris   %[w1], %[w1], 0x8000\n"
		"ori    %[w1], %[w1], 0x8000\n"

		"stw    %[w0], 0(%[dst])\n"
		"stw    %[w1], 8(%[dst])\n"

		// ----------------------------------------------------
		// BLOCK 2: Row 0 & 1, Right Half
		// ----------------------------------------------------
		"lbz    %[t0], 2(%[src])\n"
		"lbz    %[t1], 3(%[src])\n"
		"lbz    %[t2], 258(%[src])\n"
		"lbz    %[t3], 259(%[src])\n"

		"slwi   %[t0], %[t0], 1\n"
		"slwi   %[t1], %[t1], 1\n"
		"slwi   %[t2], %[t2], 1\n"
		"slwi   %[t3], %[t3], 1\n"

		"lhzx   %[t0], %[pal], %[t0]\n"
		"lhzx   %[t1], %[pal], %[t1]\n"
		"lhzx   %[t2], %[pal], %[t2]\n"
		"lhzx   %[t3], %[pal], %[t3]\n"

		"slwi   %[w0], %[t0], 16\n"
		"slwi   %[w1], %[t2], 16\n"
		"or     %[w0], %[w0], %[t1]\n"
		"or     %[w1], %[w1], %[t3]\n"

		"oris   %[w0], %[w0], 0x8000\n"
		"ori    %[w0], %[w0], 0x8000\n"
		"oris   %[w1], %[w1], 0x8000\n"
		"ori    %[w1], %[w1], 0x8000\n"

		"stw    %[w0], 4(%[dst])\n"
		"stw    %[w1], 12(%[dst])\n"

		// ----------------------------------------------------
		// BLOCK 3: Row 2 & 3, Left Half
		// ----------------------------------------------------
		"lbz    %[t0], 512(%[src])\n"
		"lbz    %[t1], 513(%[src])\n"
		"lbz    %[t2], 768(%[src])\n"
		"lbz    %[t3], 769(%[src])\n"

		"slwi   %[t0], %[t0], 1\n"
		"slwi   %[t1], %[t1], 1\n"
		"slwi   %[t2], %[t2], 1\n"
		"slwi   %[t3], %[t3], 1\n"

		"lhzx   %[t0], %[pal], %[t0]\n"
		"lhzx   %[t1], %[pal], %[t1]\n"
		"lhzx   %[t2], %[pal], %[t2]\n"
		"lhzx   %[t3], %[pal], %[t3]\n"

		"slwi   %[w0], %[t0], 16\n"
		"slwi   %[w1], %[t2], 16\n"
		"or     %[w0], %[w0], %[t1]\n"
		"or     %[w1], %[w1], %[t3]\n"

		"oris   %[w0], %[w0], 0x8000\n"
		"ori    %[w0], %[w0], 0x8000\n"
		"oris   %[w1], %[w1], 0x8000\n"
		"ori    %[w1], %[w1], 0x8000\n"

		"stw    %[w0], 16(%[dst])\n"
		"stw    %[w1], 24(%[dst])\n"

		// ----------------------------------------------------
		// BLOCK 4: Row 2 & 3, Right Half
		// ----------------------------------------------------
		"lbz    %[t0], 514(%[src])\n"
		"lbz    %[t1], 515(%[src])\n"
		"lbz    %[t2], 770(%[src])\n"
		"lbz    %[t3], 771(%[src])\n"

		"slwi   %[t0], %[t0], 1\n"
		"slwi   %[t1], %[t1], 1\n"
		"slwi   %[t2], %[t2], 1\n"
		"slwi   %[t3], %[t3], 1\n"

		"lhzx   %[t0], %[pal], %[t0]\n"
		"lhzx   %[t1], %[pal], %[t1]\n"
		"lhzx   %[t2], %[pal], %[t2]\n"
		"lhzx   %[t3], %[pal], %[t3]\n"

		"slwi   %[w0], %[t0], 16\n"
		"slwi   %[w1], %[t2], 16\n"
		"or     %[w0], %[w0], %[t1]\n"
		"or     %[w1], %[w1], %[t3]\n"

		"oris   %[w0], %[w0], 0x8000\n"
		"ori    %[w0], %[w0], 0x8000\n"
		"oris   %[w1], %[w1], 0x8000\n"
		"ori    %[w1], %[w1], 0x8000\n"

		"stw    %[w0], 20(%[dst])\n"
		"stw    %[w1], 28(%[dst])\n"

		// -- Advance Pointers --
		"addi   %[src], %[src], 4\n"           // Advance X by 1 tile (4 pixels)
		"addi   %[dst], %[dst], 32\n"          // Advance Dst by 1 full tile
		"bdnz   1b\n"                          // Decrement CTR, loop X

		// -- Next Tile Row --
		"addi   %[src], %[r_src_row], 1024\n"  // Jump SRC down 4 pixel rows (4 * 256)
		"addi   %[dst], %[r_dst_row], 2048\n"  // Jump DST down 1 tile row (64 tiles * 32 bytes)
		"subic. %[height], %[height], 1\n"     // Decrement height counter
		"bne    2b"                            // Loop Y

		: [r_src_row] "=&b" (r_src_row), [r_dst_row] "=&b" (r_dst_row),
		  [t0] "=&r" (t0), [t1] "=&r" (t1), [t2] "=&r" (t2), [t3] "=&r" (t3),
		  [w0] "=&r" (w0), [w1] "=&r" (w1),
		  [dst] "+b" (dstBuf), [src] "+b" (srcBuf),
		  [width] "+r" (width), [height] "+r" (height)
		: [pal] "b" (rgb555)
		: "memory", "cc"
	);
}

static void MakeStereoTexture(const void *srcLeft, const void *srcRight, void *dst, s32 width, s32 height)
{
	const s32 borderwidth = (256 - width) / 2;
	const s32 borderheight = (240 - height) / 2;

	const u8 *srcBufL = (const u8 *)srcLeft + (borderheight << 8) + borderwidth;
	const u8 *srcBufR = (const u8 *)srcRight + (borderheight << 8) + borderwidth;
	u8 *dstBuf = (u8 *)dst + (borderheight * 512) + (borderwidth * 8);

	u32 r_src_row_L=0, r_src_row_R=0, r_dst_row=0;
	u32 tL0=0, tR0=0, tL1=0, tR1=0, w0=0;

	__asm__ __volatile__ (
		"srwi   %[width], %[width], 2\n"
		"srwi   %[height], %[height], 2\n"

	"2: mtctr   %[width]\n"
		"mr     %[r_src_row_L], %[srcL]\n"
		"mr     %[r_src_row_R], %[srcR]\n"
		"mr     %[r_dst_row], %[dst]\n"

	"1: dcbz    0, %[dst]\n"

		// ----------------------------------------------------
		// 8 sub-blocks to process 16 pixels. Example: Row 0 Left Half
		// Math trick: offset = ((L & 63) << 7) | ((R & 63) << 1)
		// ----------------------------------------------------

		// Row 0, Left Half
		"lbz    %[tL0], 0(%[srcL])\n"
		"lbz    %[tR0], 0(%[srcR])\n"
		"lbz    %[tL1], 1(%[srcL])\n"
		"lbz    %[tR1], 1(%[srcR])\n"
		"rlwinm %[tL0], %[tL0], 7, 19, 24\n"
		"rlwinm %[tR0], %[tR0], 1, 25, 30\n"
		"rlwinm %[tL1], %[tL1], 7, 19, 24\n"
		"rlwinm %[tR1], %[tR1], 1, 25, 30\n"
		"or     %[tL0], %[tL0], %[tR0]\n"
		"or     %[tL1], %[tL1], %[tR1]\n"
		"lhzx   %[tL0], %[pal], %[tL0]\n"
		"lhzx   %[tL1], %[pal], %[tL1]\n"
		"slwi   %[w0], %[tL0], 16\n"
		"or     %[w0], %[w0], %[tL1]\n"
		"oris   %[w0], %[w0], 0x8000\n"
		"ori    %[w0], %[w0], 0x8000\n"
		"stw    %[w0], 0(%[dst])\n"

		// Row 0, Right Half
		"lbz    %[tL0], 2(%[srcL])\n"
		"lbz    %[tR0], 2(%[srcR])\n"
		"lbz    %[tL1], 3(%[srcL])\n"
		"lbz    %[tR1], 3(%[srcR])\n"
		"rlwinm %[tL0], %[tL0], 7, 19, 24\n"
		"rlwinm %[tR0], %[tR0], 1, 25, 30\n"
		"rlwinm %[tL1], %[tL1], 7, 19, 24\n"
		"rlwinm %[tR1], %[tR1], 1, 25, 30\n"
		"or     %[tL0], %[tL0], %[tR0]\n"
		"or     %[tL1], %[tL1], %[tR1]\n"
		"lhzx   %[tL0], %[pal], %[tL0]\n"
		"lhzx   %[tL1], %[pal], %[tL1]\n"
		"slwi   %[w0], %[tL0], 16\n"
		"or     %[w0], %[w0], %[tL1]\n"
		"oris   %[w0], %[w0], 0x8000\n"
		"ori    %[w0], %[w0], 0x8000\n"
		"stw    %[w0], 4(%[dst])\n"

		// Row 1, Left Half
		"lbz    %[tL0], 256(%[srcL])\n"
		"lbz    %[tR0], 256(%[srcR])\n"
		"lbz    %[tL1], 257(%[srcL])\n"
		"lbz    %[tR1], 257(%[srcR])\n"
		"rlwinm %[tL0], %[tL0], 7, 19, 24\n"
		"rlwinm %[tR0], %[tR0], 1, 25, 30\n"
		"rlwinm %[tL1], %[tL1], 7, 19, 24\n"
		"rlwinm %[tR1], %[tR1], 1, 25, 30\n"
		"or     %[tL0], %[tL0], %[tR0]\n"
		"or     %[tL1], %[tL1], %[tR1]\n"
		"lhzx   %[tL0], %[pal], %[tL0]\n"
		"lhzx   %[tL1], %[pal], %[tL1]\n"
		"slwi   %[w0], %[tL0], 16\n"
		"or     %[w0], %[w0], %[tL1]\n"
		"oris   %[w0], %[w0], 0x8000\n"
		"ori    %[w0], %[w0], 0x8000\n"
		"stw    %[w0], 8(%[dst])\n"

		// Row 1, Right Half
		"lbz    %[tL0], 258(%[srcL])\n"
		"lbz    %[tR0], 258(%[srcR])\n"
		"lbz    %[tL1], 259(%[srcL])\n"
		"lbz    %[tR1], 259(%[srcR])\n"
		"rlwinm %[tL0], %[tL0], 7, 19, 24\n"
		"rlwinm %[tR0], %[tR0], 1, 25, 30\n"
		"rlwinm %[tL1], %[tL1], 7, 19, 24\n"
		"rlwinm %[tR1], %[tR1], 1, 25, 30\n"
		"or     %[tL0], %[tL0], %[tR0]\n"
		"or     %[tL1], %[tL1], %[tR1]\n"
		"lhzx   %[tL0], %[pal], %[tL0]\n"
		"lhzx   %[tL1], %[pal], %[tL1]\n"
		"slwi   %[w0], %[tL0], 16\n"
		"or     %[w0], %[w0], %[tL1]\n"
		"oris   %[w0], %[w0], 0x8000\n"
		"ori    %[w0], %[w0], 0x8000\n"
		"stw    %[w0], 12(%[dst])\n"

		// Row 2, Left Half
		"lbz    %[tL0], 512(%[srcL])\n"
		"lbz    %[tR0], 512(%[srcR])\n"
		"lbz    %[tL1], 513(%[srcL])\n"
		"lbz    %[tR1], 513(%[srcR])\n"
		"rlwinm %[tL0], %[tL0], 7, 19, 24\n"
		"rlwinm %[tR0], %[tR0], 1, 25, 30\n"
		"rlwinm %[tL1], %[tL1], 7, 19, 24\n"
		"rlwinm %[tR1], %[tR1], 1, 25, 30\n"
		"or     %[tL0], %[tL0], %[tR0]\n"
		"or     %[tL1], %[tL1], %[tR1]\n"
		"lhzx   %[tL0], %[pal], %[tL0]\n"
		"lhzx   %[tL1], %[pal], %[tL1]\n"
		"slwi   %[w0], %[tL0], 16\n"
		"or     %[w0], %[w0], %[tL1]\n"
		"oris   %[w0], %[w0], 0x8000\n"
		"ori    %[w0], %[w0], 0x8000\n"
		"stw    %[w0], 16(%[dst])\n"

		// Row 2, Right Half
		"lbz    %[tL0], 514(%[srcL])\n"
		"lbz    %[tR0], 514(%[srcR])\n"
		"lbz    %[tL1], 515(%[srcL])\n"
		"lbz    %[tR1], 515(%[srcR])\n"
		"rlwinm %[tL0], %[tL0], 7, 19, 24\n"
		"rlwinm %[tR0], %[tR0], 1, 25, 30\n"
		"rlwinm %[tL1], %[tL1], 7, 19, 24\n"
		"rlwinm %[tR1], %[tR1], 1, 25, 30\n"
		"or     %[tL0], %[tL0], %[tR0]\n"
		"or     %[tL1], %[tL1], %[tR1]\n"
		"lhzx   %[tL0], %[pal], %[tL0]\n"
		"lhzx   %[tL1], %[pal], %[tL1]\n"
		"slwi   %[w0], %[tL0], 16\n"
		"or     %[w0], %[w0], %[tL1]\n"
		"oris   %[w0], %[w0], 0x8000\n"
		"ori    %[w0], %[w0], 0x8000\n"
		"stw    %[w0], 20(%[dst])\n"

		// Row 3, Left Half
		"lbz    %[tL0], 768(%[srcL])\n"
		"lbz    %[tR0], 768(%[srcR])\n"
		"lbz    %[tL1], 769(%[srcL])\n"
		"lbz    %[tR1], 769(%[srcR])\n"
		"rlwinm %[tL0], %[tL0], 7, 19, 24\n"
		"rlwinm %[tR0], %[tR0], 1, 25, 30\n"
		"rlwinm %[tL1], %[tL1], 7, 19, 24\n"
		"rlwinm %[tR1], %[tR1], 1, 25, 30\n"
		"or     %[tL0], %[tL0], %[tR0]\n"
		"or     %[tL1], %[tL1], %[tR1]\n"
		"lhzx   %[tL0], %[pal], %[tL0]\n"
		"lhzx   %[tL1], %[pal], %[tL1]\n"
		"slwi   %[w0], %[tL0], 16\n"
		"or     %[w0], %[w0], %[tL1]\n"
		"oris   %[w0], %[w0], 0x8000\n"
		"ori    %[w0], %[w0], 0x8000\n"
		"stw    %[w0], 24(%[dst])\n"

		// Row 3, Right Half
		"lbz    %[tL0], 770(%[srcL])\n"
		"lbz    %[tR0], 770(%[srcR])\n"
		"lbz    %[tL1], 771(%[srcL])\n"
		"lbz    %[tR1], 771(%[srcR])\n"
		"rlwinm %[tL0], %[tL0], 7, 19, 24\n"
		"rlwinm %[tR0], %[tR0], 1, 25, 30\n"
		"rlwinm %[tL1], %[tL1], 7, 19, 24\n"
		"rlwinm %[tR1], %[tR1], 1, 25, 30\n"
		"or     %[tL0], %[tL0], %[tR0]\n"
		"or     %[tL1], %[tL1], %[tR1]\n"
		"lhzx   %[tL0], %[pal], %[tL0]\n"
		"lhzx   %[tL1], %[pal], %[tL1]\n"
		"slwi   %[w0], %[tL0], 16\n"
		"or     %[w0], %[w0], %[tL1]\n"
		"oris   %[w0], %[w0], 0x8000\n"
		"ori    %[w0], %[w0], 0x8000\n"
		"stw    %[w0], 28(%[dst])\n"

		"addi   %[srcL], %[srcL], 4\n"
		"addi   %[srcR], %[srcR], 4\n"
		"addi   %[dst], %[dst], 32\n"
		"bdnz   1b\n"

		"addi   %[srcL], %[r_src_row_L], 1024\n"
		"addi   %[srcR], %[r_src_row_R], 1024\n"
		"addi   %[dst], %[r_dst_row], 2048\n"
		"subic. %[height], %[height], 1\n"
		"bne    2b"

		: [r_src_row_L] "=&b" (r_src_row_L),
		  [r_src_row_R] "=&b" (r_src_row_R),
		  [r_dst_row] "=&b" (r_dst_row),
		  [tL0] "=&r" (tL0), [tR0] "=&r" (tR0),
		  [tL1] "=&r" (tL1), [tR1] "=&r" (tR1),
		  [w0] "=&r" (w0),
		  [dst] "+b" (dstBuf), [srcL] "+b" (srcBufL), [srcR] "+b" (srcBufR),
		  [width] "+r" (width), [height] "+r" (height)
		: [pal] "b" (anaglyph555)
		: "memory", "cc"
	);
}

void OgcEmulatorVideo::init(VideoDriver* driver)
{
	videoDriver = static_cast<OgcVideoDriver*>(driver);
	updateVideo = true;
}

void OgcEmulatorVideo::resetFbWidth(int width, GXRModeObj *rmode)
{
	if(rmode->fbWidth == width)
		return;

	rmode->fbWidth = width;

	if(rmode != vmode)
		return;

	GX_InvVtxCache();
	VIDEO_Configure(rmode);
	VIDEO_Flush();
}

/****************************************************************************
 * resetVideo
 *
 * Reset the video/rendering mode for the emulator rendering
****************************************************************************/
void OgcEmulatorVideo::resetVideo()
{
	GXRModeObj *rmode;
	Mtx44 p;

	// set VI mode and audio sample rate depending on if original mode is used

	if (GCSettings.videoMode == VIDEOMODE_ORIGINAL_240P)
	{
		int timing = GCSettings.timing == TIMING_DENDY ? TIMING_PAL : FCEUI_GetCurrentVidSystem(NULL, NULL);
		rmode = tvmodes[timing];

		if (FCEUI_GetCurrentVidSystem(NULL, NULL) == TIMING_PAL || GCSettings.timing == TIMING_DENDY) // PAL
			UpdateSampleRate(48070);
		else
			UpdateSampleRate(48220);
	}
	else
	{
		rmode = videoDriver->findVideoMode();

		if (GCSettings.videoAspectRatioCorrection == VIDEO_ASPECT_RATIO_CORRECTION_16_9)
			resetFbWidth(640, rmode);
		else
			resetFbWidth(512, rmode);

		if (FCEUI_GetCurrentVidSystem(NULL, NULL) == TIMING_PAL || GCSettings.timing == TIMING_DENDY) // PAL
			UpdateSampleRate(48080);
		else
			UpdateSampleRate(48130);
	}

	videoDriver->setupVideoMode(rmode); // reconfigure VI

	GXColor background = {0, 0, 0, 255};
	GX_SetCopyClear (background, GX_MAX_Z24);

	// reconfigure GX
	GX_SetViewport (0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
	GX_SetDispCopyYScale ((f32) rmode->xfbHeight / (f32) rmode->efbHeight);
	GX_SetScissor (0, 0, rmode->fbWidth, rmode->efbHeight);

	GX_SetDispCopySrc (0, 0, rmode->fbWidth, rmode->efbHeight);
	GX_SetDispCopyDst (rmode->fbWidth, rmode->xfbHeight);
	u8 sharp[7] = {0,0,21,22,21,0,0};
	u8 soft[7] = {8,8,10,12,10,8,8};
	u8* vfilter =
		GCSettings.videoHardwareSoften == VIDEO_HW_SOFTEN_SHARP ? sharp
		: GCSettings.videoHardwareSoften == VIDEO_HW_SOFTEN_SOFT ? soft
		: rmode->vfilter;

	u8 vf_enable = (rmode->xfbMode != VI_XFBMODE_SF || GCSettings.videoHardwareSoften != VIDEO_HW_SOFTEN_OFF) ? GX_TRUE : GX_FALSE;
	GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, vf_enable, vfilter);

	GX_SetFieldMode (rmode->field_rendering, ((rmode->viHeight == 2 * rmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));

	if (rmode->aa)
		GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
	else
		GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);

	GX_SetZMode (GX_TRUE, GX_LEQUAL, GX_TRUE);
	GX_SetColorUpdate (GX_TRUE);
	GX_SetBlendMode (GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);

	guOrtho(p, rmode->efbHeight/2, -(rmode->efbHeight/2), -(rmode->fbWidth/2), rmode->fbWidth/2, 100, 1000); // matrix, t, b, l, r, n, f
	GX_LoadProjectionMtx (p, GX_ORTHOGRAPHIC);

	// set aspect ratio
	drawInit ();
	updateScaling();

	// reinitialize texture
	GX_InvalidateTexAll ();
	GX_InitTexObj (&texobj, texturemem, NES_WIDTH*fscale, NES_HEIGHT*fscale, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE);

	if (!GCSettings.videoBilinearFilter)
		GX_InitTexObjFilterMode(&texobj, GX_NEAR, GX_NEAR);
	else
		GX_InitTexObjFilterMode(&texobj, GX_LINEAR, GX_LINEAR);

	GX_LoadTexObj (&texobj, GX_TEXMAP0);

	if(shouldApplyScanlines())
		initScanlineTexture();

	// clear texture memory
	memset(texturemem, 0, NES_WIDTH * fscale * NES_HEIGHT * fscale * 2);
}

/****************************************************************************
 * presentFrame
 *
 * Render a single frame
 ****************************************************************************/
void OgcEmulatorVideo::presentFrame(const uint8_t* buffer)
{
	videoDriver->waitForBufferReady(); // waits for prior copy, then flips whichfb

	// video has changed
	if(updateVideo)
	{
		updateVideo = false;
		resetVideo(); // reset video to emulator rendering settings
	}

	u8 borderwidth = getBorderWidth();
	u8 borderheight = getBorderHeight();

	const s32 widthLimit = NES_WIDTH - (borderwidth << 1);
	const s32 heightLimit = NES_HEIGHT - (borderheight << 1);

	updateFilterScale();

	if (fscale > 1) {
		FilterMethod((u8 *)buffer, NES_WIDTH, texturemem, NES_WIDTH * fscale * 2, NES_WIDTH, NES_HEIGHT);
		DCStoreRange(texturemem, NES_WIDTH * fscale * NES_HEIGHT * fscale * 2);
	}
	else {
		// Native 1x: Populate using original 8-bit lookup swizzler
		MakeTexture(buffer, texturemem, widthLimit, heightLimit);

		// Flush linear size 256x240 @ RGB565 -> 122880 bytes
		DCStoreRange(texturemem, NES_WIDTH * NES_HEIGHT * 2);
	}

	// clear texture objects
	GX_InvalidateTexAll();

	// Apply dynamic scissor box to crop out overscan borders cleanly using GX
	applyOverscanScissor(borderwidth, borderheight);

	// render textured quad
	drawSquare(view);
	GX_DrawDone();

	videoDriver->presentBuffer();
}

/****************************************************************************
 * presentStereoFrame
 *
 * Render a single frame
 ****************************************************************************/
void OgcEmulatorVideo::presentStereoFrame(const uint8_t* bufferLeft, const uint8_t* bufferRight)
{
	videoDriver->waitForBufferReady(); // waits for prior copy, then flips whichfb

	// video has changed
	if(updateVideo)
	{
		updateVideo = false;
		resetVideo(); // reset video to emulator rendering settings
	}

	// May need to regenerate the anaglyph 3D palette that is used below
	if (!AnaglyphPaletteValid)
		GenerateAnaglyphPalette();

	u8 borderwidth = getBorderWidth();
	u8 borderheight = getBorderHeight();

	const s32 widthLimit = NES_WIDTH - (borderwidth << 1);
	const s32 heightLimit = NES_HEIGHT - (borderheight << 1);

	// populate the texture with red/cyan anaglyph
	MakeStereoTexture(bufferLeft, bufferRight, texturemem, widthLimit, heightLimit);

	// load texture into GX
	DCFlushRange(texturemem, NES_WIDTH * NES_HEIGHT * 2);

	// clear texture objects
	GX_InvalidateTexAll();

	// render textured quad
	drawSquare(view);
	GX_DrawDone();

	videoDriver->presentBuffer();
}

static void OptimisedAnaglyph(u8 *r, u8 *g, u8 *b, u8 lr, u8 lg, u8 lb, u8 rr, u8 rg, u8 rb)
{
	// The left eye needs to see a bit of every colour mixed into the red channel
	// otherwise it will have trouble matching it to the right eye.
	// the left eye also needs to be brighter. 
	int ar = (lr * 600 + lg * 300 + lb * 200) / 1000;
	if (ar > 255)
		ar = 255;
	*r = ar;
	int ag = (rg * 700 + rr * 200) / 1000;
	if (ag > 255)
		ag = 255;
	*g = ag;
	*b = rb;
}
#if 0
// This 3D palette is for high contrast white on black games like Falsion
static void RedBlueMonoAnaglyph(u8 *r, u8 *g, u8 *b, u8 lr, u8 lg, u8 lb, u8 rr, u8 rg, u8 rb)
{
	// The left eye needs to see a bit of every colour mixed into the red channel
	// otherwise it will have trouble matching it to the right eye.
	// the left eye also needs to be brighter. 
	int ar = (lr * 300 + lg * 500 + lb * 200) / 1000;
	if (ar > 255)
		ar = 255;
	*r = ar;
	*g = 0;
	int ab = (rr * 300 + rg * 500 + rb * 200) / 1000;
	if (ab > 255)
		ab = 255;
	*b = ab;
}

// This 3D palette is for high contrast white on black games like Falsion
static void RedGreenMonoAnaglyph(u8 *r, u8 *g, u8 *b, u8 lr, u8 lg, u8 lb, u8 rr, u8 rg, u8 rb)
{
	// The left eye needs to see a bit of every colour mixed into the red channel
	// otherwise it will have trouble matching it to the right eye.
	// the left eye also needs to be brighter. 
	int ar = (lr * 300 + lg * 500 + lb * 200) / 1000;
	if (ar > 255)
		ar = 255;
	*r = ar;
	int ab = (rr * 300 + rg * 500 + rb * 200) / 1000;
	if (ab > 255)
		ab = 255;
	*g = ab;
	*b = 0;
}

// This 3D palette is for high contrast white on black games like Falsion
static void RedCyanMonoAnaglyph(u8 *r, u8 *g, u8 *b, u8 lr, u8 lg, u8 lb, u8 rr, u8 rg, u8 rb)
{
	// The left eye needs to see a bit of every colour mixed into the red channel
	// otherwise it will have trouble matching it to the right eye.
	// the left eye also needs to be brighter. 
	int ar = (lr * 300 + lg * 500 + lb * 200) / 1000;
	if (ar > 255)
		ar = 255;
	*r = ar;
	int ab = (rr * 300 + rg * 500 + rb * 200) / 2000;
	if (ab > 255)
		ab = 255;
	*g = ab;
	*b = ab;
}

// This 3D palette is good for games which were already in anaglyph
static void FullColourAnaglyph(u8 *r, u8 *g, u8 *b, u8 lr, u8 lg, u8 lb, u8 rr, u8 rg, u8 rb)
{
	// The left eye needs to see a bit of every colour mixed into the red channel
	// otherwise it will have trouble matching it to the right eye.
	// the left eye also needs to be brighter. 
	*r = lr;
	*g = rg;
	*b = rb;
}
#endif
// Create an RGB 555 colour (used in textures) for this stereoscopic 3D combination of 2 NES colours.
static void GenerateAnaglyphPalette()
{
	for (int left = 0; left < 64; left++)
	{
		// Left-eye colour is invariant across the inner loop; read it once.
		const u8 lr = pcpalette[left].r;
		const u8 lg = pcpalette[left].g;
		const u8 lb = pcpalette[left].b;
		for (int right = 0; right < 64; right++)
		{
			u8 ar, ag, ab;
			OptimisedAnaglyph(&ar, &ag, &ab, lr, lg, lb, pcpalette[right].r, pcpalette[right].g, pcpalette[right].b);
			// Pack into RGB5A3 (5 bits each for R, G, B)
			anaglyph555[left][right] = ((ar & 0xf8) << 7) | ((ag & 0xf8) << 2) | ((ab & 0xf8) >> 3);
		}
	}
	AnaglyphPaletteValid = true;
}

// We need to know the OUT1 pin of the expansion port for Famicom 3D System glasses
extern uint8 shutter_3d;
// We need to know the palette in RAM for red/cyan anaglyph 3D games (3D World Runner and Rad Racer)
extern uint8 PALRAM[0x20];
bool old_shutter_3d_mode = 0, old_anaglyph_3d_mode = 0;
uint8 prev_shutter_3d = 0, prev_prev_shutter_3d = 0;
uint8 pal_3d = 0, prev_pal_3d = 0, prev_prev_pal_3d = 0; 

bool CheckForAnaglyphPalette()
{
	// It can also have none of these when all blacks
	bool hasRed = false, hasCyan = false, hasOther = false;
	pal_3d = 0;

	// first 12 background colours are used for anaglyph (last 4 are for status bar)
	for (int i = 0; i < 12; i++)
	{
		switch (PALRAM[i] & 63)
		{
			case 0x00:
			case 0x0F: // blacks
				break;
			case 0x01:
			case 0x11:
			case 0x0A:
			case 0x1A:
			case 0x0C:
			case 0x1C:
			case 0x2C: // cyan
				hasCyan = true;
				break;
			case 0x05:
			case 0x15:
			case 0x06:
			case 0x16: // reds
				hasRed = true;
				break;
			default:
				hasOther = true;
		}
	}

	if (hasOther || (hasRed && hasCyan))
		return false;

	// last 8 sprite colours are used for anaglyph (first 8 are for screen-level sprites)
	for (int i = 24; i < 32; i++)
	{
		switch (PALRAM[i] & 63)
		{
			case 0x00:
			case 0x0F: // blacks
				break;
			case 0x01:
			case 0x11:
			case 0x0A:
			case 0x1A:
			case 0x0C:
			case 0x1C:
			case 0x2c: // cyan
				hasCyan = true;
				break;
			case 0x05:
			case 0x15:
			case 0x06:
			case 0x16: // reds
				hasRed = true;
				break;
			default:
				hasOther = true;
		}
	}

	if (hasOther || (hasRed && hasCyan) || (!hasRed && !hasCyan))
		return false;

	eye_3d = hasCyan;

	if (hasCyan)
		pal_3d = 2;
	else
		pal_3d = 1;

	return true;
}

// Handles automatically entering and exiting stereoscopic 3D mode, and detecting which eye to draw
void Check3D()
{
	// Stereoscopic 3D game mode detection
	shutter_3d_mode = (shutter_3d != prev_shutter_3d && shutter_3d == prev_prev_shutter_3d);
	prev_prev_shutter_3d = prev_shutter_3d;
	prev_shutter_3d = shutter_3d;
	if (shutter_3d_mode)
	{
		fskip = 0;
		eye_3d = !shutter_3d;
	}
	else if (old_shutter_3d_mode)
	{
		// Exited stereoscopic 3d mode, reset frameskip to 0
		fskip = 0;
		fskipc = 0;
		frameskip = 0;
	}
	else
	{
		// Only check anaglyph when it's not a Famicom 3D System game
		// Games are detected as anaglyph, only when they alternate between a very limited red palette
		// and a very limited blue/green palette. It's very unlikely other games will do that, but
		// not impossible.
		anaglyph_3d_mode = CheckForAnaglyphPalette() && pal_3d != prev_pal_3d && pal_3d == prev_prev_pal_3d && prev_pal_3d != 0;
		prev_prev_pal_3d = prev_pal_3d;
		prev_pal_3d = pal_3d;
		if (anaglyph_3d_mode)
		{
			fskip = 0;
		}
		else if (old_anaglyph_3d_mode)
		{
			// Exited stereoscopic 3d mode, reset frameskip to 0
			fskip = 0;
			fskipc = 0;
			frameskip = 0;
		}
		// TODO: make a backup of palette whenever not in anaglyph mode,
		// and use it to override anaglyph's horible palette for full colour 3D
		// note the difficulty will be that palette entries get rearranged to
		// animate the road and will still need to be rearranged in our backup palette
	}
	old_shutter_3d_mode = shutter_3d_mode;
	old_anaglyph_3d_mode = anaglyph_3d_mode;
}
