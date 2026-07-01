/****************************************************************************
 * FCE Ultra
 * Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2023
 *
 * gcvideo.cpp
 *
 * Video rendering
 ****************************************************************************/

#include <gccore.h>
#include <unistd.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogc/texconv.h>
#include <ogc/lwp_watchdog.h>
#include <ogc/machine/processor.h>
#include <ogc/cond.h>

#include "fceugx.h"
#include "fceusupport.h"
#include "gcvideo.h"
#include "gcaudio.h"
#include "menu.h"
#include "pad.h"
#include "gui/gui.h"
#include "videofilters.h"

int FDSTimer = 0;
u32 FrameTimer = 0;
int FDSSwitchRequested;

/*** External 2D Video ***/
/*** 2D Video Globals ***/
GXRModeObj *vmode  = NULL; // Graphics Mode Object
static u32 *xfb[2] = { NULL, NULL }; // Framebuffers
static int whichfb = 0; // Frame buffer toggle
int screenheight = 480;
int screenwidth = 640;
bool progressive = false;
static int oldRenderMode = -1; // set to GCSettings.render when changing (temporarily) to another mode

/*** 3D GX ***/
#define TEX_WIDTH 512
#define TEX_HEIGHT 512
#define TEXTUREMEM_SIZE (TEX_WIDTH * TEX_HEIGHT * 2)
#define DEFAULT_FIFO_SIZE ( 256 * 1024 )

static u8 gp_fifo[DEFAULT_FIFO_SIZE] ATTRIBUTE_ALIGN(32);
static volatile u32 copynow = GX_FALSE;
static GXTexObj texobj;
static GXTexObj scanlineTexObj;
static Mtx view;
static Mtx GXmodelView2D;

/*** Texture memory ***/
static unsigned char texturemem[TEXTUREMEM_SIZE] ATTRIBUTE_ALIGN (32);
static unsigned char scanline_tex_data[32] ATTRIBUTE_ALIGN (32);

static int UpdateVideo = 1;
static bool vmode_60hz = true;

u8 * gameScreenPng = NULL;
int gameScreenPngSize = 0;

#define NES_WIDTH 256
#define NES_HEIGHT 240

// Need something to hold the PC palette
struct pcpal {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} pcpalette[256];

static unsigned int gcpalette[256];	// Much simpler GC palette
unsigned short rgb565[256];	// Texture map palette
bool shutter_3d_mode, anaglyph_3d_mode, eye_3d;
bool AnaglyphPaletteValid = false; //CAK: Has the anaglyph palette below been generated yet?
static unsigned short anaglyph565[64][64]; //CAK: Texture map left right combination anaglyph palette
static void GenerateAnaglyphPalette(); //CAK: function prototype for generating the anaglyph palette

static long long prev;
static long long now;

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
static GXRModeObj PAL_240p =
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
static GXRModeObj NTSC_240p =
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

/****************************************************************************
 * setFrameTimer()
 * change frame timings depending on whether ROM is NTSC or PAL
 ***************************************************************************/

static u32 normaldiff;

void setFrameTimer()
{
	if (FCEUI_GetCurrentVidSystem(NULL, NULL) == TIMING_PAL || GCSettings.timing == TIMING_DENDY) // PAL
		normaldiff = 20000; // 50hz
	else
		normaldiff = 16667; // 60hz
	prev = gettime();
}

void SyncSpeed()
{
	// same timing as game - no adjustment necessary 
	if((vmode_60hz && normaldiff == 16667) || (!vmode_60hz && normaldiff == 20000)) 
		if (!shutter_3d_mode && !anaglyph_3d_mode) return; //CAK: But don't exit if in a 30/25Hz 3D mode.

	//CAK: Note that the 3D modes (except Pulfrich) still call this function at 60/50Hz, but half the 
	//     time there is no video rendering to go with it, so we need some delays.

	now = gettime();
	u32 diff = diff_usec(prev, now);
	
	if(turbomode)
	{
		// do nothing
	}
	else if (diff > normaldiff)
	{
		frameskip++; //CAK: In 3D this will be ignored, then reset to 0 when leaving 3D
	}
	else // ahead, so hold up
	{	
		while (diff_usec(prev, now) < normaldiff)
		{
			now = gettime();
			usleep(50);
		}
	}
	prev = now;
}

/****************************************************************************
 * VideoThreading
 ***************************************************************************/
static lwp_t vbthread = LWP_THREAD_NULL;
static lwpq_t render_queue;          // Queue for the main thread to sleep on
static lwpq_t vb_queue;              // Queue for the VSync thread to sleep on
static volatile bool vb_done = true; // Tracks if the VSync thread has completed its wait

/****************************************************************************
 * vbgetback
 *
 * This callback enables the emulator to keep running while waiting for a
 * vertical blank
 ***************************************************************************/
static void *
vbgetback (void *arg)
{
	while (1)
	{
		LWP_ThreadSleep(vb_queue);     // Sleep until kicked off by copy_to_xfb
		VIDEO_WaitVSync ();	         /**< Wait for video vertical blank */
		vb_done = true;
		LWP_ThreadSignal(render_queue); // Instantly alert the main thread if it is waiting
	}

	return NULL;
}

/****************************************************************************
 * copy_to_xfb
 *
 * Stock code to copy the GX buffer to the current display mode.
 * Also increments the frameticker, as it's called for each vb.
 ***************************************************************************/
static inline void
copy_to_xfb (u32 arg)
{
	if (copynow == GX_TRUE)
	{
		GX_CopyDisp (xfb[whichfb], GX_TRUE);
		GX_Flush ();
		copynow = GX_FALSE;
		LWP_ThreadSignal(render_queue); // Wake up the main thread if it is waiting for the copy
	}

	FrameTimer++;

	// FDS switch disk requested - need to eject, select, and insert
	// but not all at once!
	if(FDSSwitchRequested)
	{
		switch(FDSSwitchRequested)
		{
			case 1:
				FDSSwitchRequested++;
				FCEUI_FDSInsert(); // eject disk
				FDSTimer = 0;
				break;
			case 2:
				if(FDSTimer > 60)
				{
					FDSSwitchRequested++;
					FDSTimer = 0;
					FCEUI_FDSSelect(); // select other side
					FCEUI_FDSInsert(); // insert disk
				}
				break;
			case 3:
				if(FDSTimer > 200)
				{
					FDSSwitchRequested = 0;
					FDSTimer = 0;
				}
				break;
		}
		FDSTimer++;
	}
}

/****************************************************************************
 * ApplyOverscanScissor
 *
 * Dynamically calculates a GX Scissor box to crop out the overscan borders
 * at the hardware level. This perfectly hides overscan without modifying
 * the CPU-bound filters, preventing memory stalls or stretching.
 ***************************************************************************/
static inline void ApplyOverscanScissor(u8 borderwidth, u8 borderheight)
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

static void InitScanlineTexture() {
	// GX_TF_I8 represents one byte per pixel.
	// We create an 8x4 tile: Rows 0 and 2 are white (0xFF), Rows 1 and 3 are dark (0xA0).
	for (int y = 0; y < 4; y++) {
		u8 intensity = (y % 2 == 0) ? 0xFF : 0xA0; // 0xA0 controls the scanline darkness
		for (int x = 0; x < 8; x++) {
			scanline_tex_data[y * 8 + x] = intensity;
		}
	}

	// CRITICAL: Flush the CPU data cache. GX reads directly from main memory.
	DCStoreRange(scanline_tex_data, 32);

	// Initialize the texture object. Wrap modes MUST be GX_REPEAT to tile across the screen.
	GX_InitTexObj(&scanlineTexObj, scanline_tex_data, 8, 4, GX_TF_I8, GX_REPEAT, GX_REPEAT, GX_FALSE);

	// CRITICAL: Filter mode MUST be GX_NEAR. GX_LINEAR will blur the lines into a muddy gray.
	GX_InitTexObjFilterMode(&scanlineTexObj, GX_NEAR, GX_NEAR);

	// Load the scanline texture into MAP1
	GX_LoadTexObj(&scanlineTexObj, GX_TEXMAP1);
}

static void SetupScanlineFilterTEV() {
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


/****************************************************************************
 * Scaler Support Functions
 ***************************************************************************/
static inline void
draw_init ()
{
	GX_ClearVtxDesc ();
	GX_SetVtxDesc (GX_VA_POS, GX_INDEX8);
	GX_SetVtxDesc (GX_VA_TEX0, GX_DIRECT);

	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	bool scanlines = (GCSettings.FilterMethod == FILTER_SCANLINES && !shutter_3d_mode && !anaglyph_3d_mode);

	if(scanlines) {
		SetupScanlineFilterTEV();
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

static inline void
draw_square (Mtx v)
{
	Mtx m;			// model matrix.
	Mtx mv;			// modelview matrix.

	guMtxIdentity (m);
	guMtxTransApply (m, m, 0, 0, -100);
	guMtxConcat (v, m, mv);

	GX_LoadPosMtxImm (mv, GX_PNMTX0);
	GX_Begin (GX_QUADS, GX_VTXFMT0, 4);

	bool scanlines = (GCSettings.FilterMethod == FILTER_SCANLINES && !shutter_3d_mode && !anaglyph_3d_mode);

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
 * StopGX
 *
 * Stops GX (when exiting)
 ***************************************************************************/
void StopGX()
{
	GX_AbortFrame();
	GX_Flush();

	VIDEO_SetBlack(true);
	VIDEO_Flush();
}

/****************************************************************************
 * UpdateScaling
 *
 * This function updates the quad aspect ratio.
 ***************************************************************************/
static inline void
UpdateScaling()
{
	int xscale, yscale;

	// update scaling
	if (GCSettings.render == RENDER_ORIGINAL)
	{
		xscale = 512 / 2; // use GX scaler instead VI
		yscale = NES_HEIGHT / 2;
	}
	else // unfiltered and filtered mode
	{
		xscale = NES_WIDTH;
		yscale = vmode->efbHeight / 2;
	}

	if (GCSettings.widescreen)
	{
		if(GCSettings.render == RENDER_ORIGINAL)
			xscale = (3*xscale)/4;
		else
			xscale = NES_WIDTH; // match the original console's width for "widescreen" to prevent flickering
	}

	xscale *= GCSettings.zoomHor;
	yscale *= GCSettings.zoomVert;

	// update vertex position matrix
	square[0] = square[9] = (-xscale) + GCSettings.xshift;
	square[3] = square[6] = (xscale) + GCSettings.xshift;
	square[1] = square[4] = (yscale) - GCSettings.yshift;
	square[7] = square[10] = (-yscale) - GCSettings.yshift;
	DCFlushRange (square, 32); // update memory BEFORE the GPU accesses it!
	draw_init ();
}

/****************************************************************************
 * FindVideoMode
 *
 * Finds the optimal video mode, or uses the user-specified one
 * Also configures original video modes
 ***************************************************************************/
static GXRModeObj * FindVideoMode()
{
	GXRModeObj * mode;
	
	// choose the desired video mode
	switch(GCSettings.videomode)
	{
		case VIDEOMODE_NTSC: // NTSC (480i)
			mode = &TVNtsc480IntDf;
			break;
		case VIDEOMODE_PROGRESSIVE: // Progressive (480p)
			mode = &TVNtsc480Prog;
			break;
		case VIDEOMODE_PAL: // PAL (50Hz)
			mode = &TVPal576IntDfScale;
			break;
		case VIDEOMODE_PAL60: // PAL (60Hz)
			mode = &TVEurgb60Hz480IntDf;
			break;
		default:
			mode = VIDEO_GetPreferredMode(NULL);
			break;
	}

	// configure original modes
	switch (mode->viTVMode >> 2)
	{
		case VI_PAL:
			// 576 lines (PAL 50Hz)
			vmode_60hz = false;

			// Original Video modes (forced to PAL 50Hz)
			// set video signal mode
			NTSC_240p.viTVMode = VI_TVMODE_PAL_DS;
			NTSC_240p.viYOrigin = (VI_MAX_HEIGHT_PAL/2 - 480/2)/2;
			break;

		case VI_NTSC:
			// 480 lines (NTSC 60Hz)
			vmode_60hz = true;

			// Original Video modes (forced to NTSC 60Hz)
			// set video signal mode
			PAL_240p.viTVMode = VI_TVMODE_NTSC_DS;
			PAL_240p.viYOrigin = (VI_MAX_HEIGHT_NTSC/2 - 480/2)/2;
			NTSC_240p.viTVMode = VI_TVMODE_NTSC_DS;
			break;

		default:
			// 480 lines (PAL 60Hz)
			vmode_60hz = true;

			// Original Video modes (forced to PAL 60Hz)
			// set video signal mode
			PAL_240p.viTVMode = VI_TVMODE(mode->viTVMode >> 2, VI_NON_INTERLACE);
			PAL_240p.viYOrigin = (VI_MAX_HEIGHT_NTSC/2 - 480/2)/2;
			NTSC_240p.viTVMode = VI_TVMODE(mode->viTVMode >> 2, VI_NON_INTERLACE);
			break;
	}

	// check for progressive scan
	if ((mode->viTVMode & 3) == VI_PROGRESSIVE)
		progressive = true;
	else
		progressive = false;

	#ifdef HW_RVL
	if (CONF_GetAspectRatio() == CONF_ASPECT_16_9)
		mode->viWidth = 678;
	else
		mode->viWidth = 672;

	if(vmode_60hz)
	{
		mode->viXOrigin = (VI_MAX_WIDTH_NTSC - mode->viWidth) / 2;
		mode->viYOrigin = (VI_MAX_HEIGHT_NTSC - mode->viHeight) / 2;
	}
	else
	{
		mode->viXOrigin = (VI_MAX_WIDTH_PAL - mode->viWidth) / 2;
		mode->viYOrigin = (VI_MAX_HEIGHT_PAL - mode->viHeight) / 2;
	}
	#endif

	return mode;
}

/****************************************************************************
 * SetupVideoMode
 *
 * Sets up the given video mode
 ***************************************************************************/
static void SetupVideoMode(GXRModeObj * mode)
{
	if(vmode == mode)
		return;
	
	VIDEO_SetPostRetraceCallback (NULL);
	copynow = GX_FALSE;
	VIDEO_Configure (mode);
	VIDEO_Flush();

	// Clear framebuffers etc.
	VIDEO_ClearFrameBuffer (mode, xfb[0], COLOR_BLACK);
	VIDEO_ClearFrameBuffer (mode, xfb[1], COLOR_BLACK);
	VIDEO_SetNextFramebuffer (xfb[0]);

	VIDEO_SetBlack (false);
	VIDEO_Flush ();
	VIDEO_WaitForFlush ();
	
	VIDEO_SetPostRetraceCallback ((VIRetraceCallback)copy_to_xfb);
	vmode = mode;
}

/****************************************************************************
 * InitVideo
 *
 * This function MUST be called at startup.
 * - also sets up menu video mode
 ***************************************************************************/
void
InitVideo ()
{
	VIDEO_Init();

	// Allocate the video buffers
	xfb[0] = (u32 *) memalign(32, 640*576*2);
	xfb[1] = (u32 *) memalign(32, 640*576*2);
	DCInvalidateRange(xfb[0], 640*576*2);
	DCInvalidateRange(xfb[1], 640*576*2);
	xfb[0] = (u32 *) MEM_K0_TO_K1 (xfb[0]);
	xfb[1] = (u32 *) MEM_K0_TO_K1 (xfb[1]);

	GXRModeObj *rmode = FindVideoMode();

	#ifdef HW_RVL
	if (CONF_GetAspectRatio() == CONF_ASPECT_16_9 && (*(u32*)(0xCD8005A0) >> 16) == 0xCAFE) // Wii U
	{
		write32(0xd8006a0, 0x30000004), mask32(0xd8006a8, 0, 2);
	}
	#endif

	SetupVideoMode(rmode);

	// Setup synchronization queues
	LWP_InitQueue(&render_queue);
	LWP_InitQueue(&vb_queue);
	vb_done = true;
	LWP_CreateThread (&vbthread, vbgetback, NULL, NULL, 0, 68);

	// Initialize GX
	GXColor background = { 0, 0, 0, 0xff };
	memset (&gp_fifo, 0, DEFAULT_FIFO_SIZE);
	GX_Init (&gp_fifo, DEFAULT_FIFO_SIZE);
	GX_SetCopyClear (background, GX_MAX_Z24);
	GX_SetDispCopyGamma (GX_GM_1_0);
	GX_SetCullMode (GX_CULL_NONE);
}

void ResetFbWidth(int width, GXRModeObj *rmode)
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
 * ResetVideo_Emu
 *
 * Reset the video/rendering mode for the emulator rendering
****************************************************************************/
void
ResetVideo_Emu ()
{
	GXRModeObj *rmode;
	Mtx44 p;

	// set VI mode and audio sample rate depending on if original mode is used

	if (GCSettings.render == RENDER_ORIGINAL)
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
		rmode = FindVideoMode();
		
		if (GCSettings.widescreen)
			ResetFbWidth(640, rmode);
		else
			ResetFbWidth(512, rmode);
		
		if (FCEUI_GetCurrentVidSystem(NULL, NULL) == TIMING_PAL || GCSettings.timing == TIMING_DENDY) // PAL
			UpdateSampleRate(48080);
		else
			UpdateSampleRate(48130);
	}

	SetupVideoMode(rmode); // reconfigure VI

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
		GCSettings.render == RENDER_FILTERED_SHARP ? sharp
		: GCSettings.render == RENDER_FILTERED_SOFT ? soft
		: rmode->vfilter;
	GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, (rmode->xfbMode == VI_XFBMODE_SF) ? GX_FALSE : GX_TRUE, vfilter);

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
	draw_init ();
	UpdateScaling();

	int fscale = 1;
	if (GCSettings.FilterMethod != FILTER_NONE &&
		GCSettings.FilterMethod != FILTER_SCANLINES &&
		!shutter_3d_mode && !anaglyph_3d_mode)
	{
		fscale = GetFilterScale();
	}

	// reinitialize texture
	GX_InvalidateTexAll ();
	GX_InitTexObj (&texobj, texturemem, NES_WIDTH*fscale, NES_HEIGHT*fscale, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);

	if (GCSettings.render == RENDER_ORIGINAL || GCSettings.render == RENDER_UNFILTERED)
		GX_InitTexObjFilterMode(&texobj, GX_NEAR, GX_NEAR); // original/unfiltered video mode: force texture filtering OFF

	GX_LoadTexObj (&texobj, GX_TEXMAP0);

	bool scanlines = (GCSettings.FilterMethod == FILTER_SCANLINES && !shutter_3d_mode && !anaglyph_3d_mode);
	if(scanlines)
		InitScanlineTexture();

	// clear texture memory
	memset(texturemem, 0, NES_WIDTH * fscale * NES_HEIGHT * fscale * 2);
}

/****************************************************************************
 * Texture Generation Helper (Gekko/Broadway ASM Optimized)
 ****************************************************************************/

void MakeTexture(const void *src, void *dst, s32 width, s32 height)
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
		: [pal] "b" (rgb565)
		: "memory", "cc"
	);
}

void MakeStereoTexture(const void *srcLeft, const void *srcRight, void *dst, s32 width, s32 height)
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
		: [pal] "b" (anaglyph565)
		: "memory", "cc"
	);
}

/****************************************************************************
 * RenderFrame
 *
 * Render a single frame
 ****************************************************************************/

void RenderFrame(unsigned char *XBuf)
{
	// Ensure previous frame copy and background VSync block have finished cleanly
	while (!vb_done || (copynow == GX_TRUE))
	{
		LWP_ThreadSleep(render_queue); // Halts main thread with 0 CPU load until signals occur
	}

	// swap framebuffers
	whichfb ^= 1;

	// video has changed
	if(UpdateVideo)
	{
		UpdateVideo = 0;
		ResetVideo_Emu(); // reset video to emulator rendering settings
	}

	u8 borderheight = 0;
	u8 borderwidth = 0;

	if(GCSettings.hideoverscan == HIDEOVERSCAN_VERTICAL || GCSettings.hideoverscan == HIDEOVERSCAN_BOTH)
		borderheight = 8;
	if(GCSettings.hideoverscan == HIDEOVERSCAN_HORIZONTAL || GCSettings.hideoverscan == HIDEOVERSCAN_BOTH)
		borderwidth = 8;

	const s32 widthLimit = NES_WIDTH - (borderwidth << 1);
	const s32 heightLimit = NES_HEIGHT - (borderheight << 1);

	int fscale = 1;
	if (GCSettings.FilterMethod != FILTER_NONE &&
		GCSettings.FilterMethod != FILTER_SCANLINES &&
		!shutter_3d_mode && !anaglyph_3d_mode)
	{
		fscale = GetFilterScale();
	}
	if (fscale > 1) {
		FilterMethod((u8 *)XBuf, NES_WIDTH, texturemem, NES_WIDTH * fscale * 2, NES_WIDTH, NES_HEIGHT);
		DCStoreRange(texturemem, NES_WIDTH * NES_HEIGHT * 2);
	}
	else {
		// Native 1x: Populate using original 8-bit lookup swizzler
		MakeTexture(XBuf, texturemem, widthLimit, heightLimit);

		// Flush linear size 256x240 @ RGB565 -> 122880 bytes
		DCStoreRange(texturemem, NES_WIDTH * NES_HEIGHT * 2);
	}

	// clear texture objects
	GX_InvalidateTexAll();

	// Apply dynamic scissor box to crop out overscan borders cleanly using GX
	ApplyOverscanScissor(borderwidth, borderheight);

	// render textured quad
	draw_square(view);
	GX_DrawDone();

	if(ScreenshotRequested)
	{
		if(GCSettings.render == RENDER_ORIGINAL) // we can't take a screenshot in Original mode
		{
			oldRenderMode = RENDER_ORIGINAL;
			GCSettings.render = RENDER_UNFILTERED; // switch to unfiltered mode
			UpdateVideo = 1; // request the switch
		}
		else
		{
			ScreenshotRequested = 0;
			TakeScreenshot();
			if(oldRenderMode != -1)
			{
				GCSettings.render = oldRenderMode;
				oldRenderMode = -1;
			}
			ConfigRequested = 1;
		}
	}

	// EFB is ready to be copied into XFB
	VIDEO_SetNextFramebuffer(xfb[whichfb]);
	VIDEO_Flush();

	copynow = GX_TRUE;

	// Reset state and signal background VSync thread to begin waiting for next blanking interval
	vb_done = false;
	LWP_ThreadSignal(vb_queue);
}

/****************************************************************************
 * RenderStereoFrames
 *
 * Render a single frame
 ****************************************************************************/

void RenderStereoFrames(unsigned char *XBufLeft, unsigned char *XBufRight)
{
	// Ensure previous frame copy and background VSync block have finished cleanly
	while (!vb_done || (copynow == GX_TRUE))
	{
		LWP_ThreadSleep(render_queue); // Halts main thread with 0 CPU load until signals occur
	}

	// swap framebuffers
	whichfb ^= 1;

	// video has changed
	if(UpdateVideo)
	{
		UpdateVideo = 0;
		ResetVideo_Emu(); // reset video to emulator rendering settings
	}
	
	//CAK: May need to regenerate the anaglyph 3D palette that is used below
	if (!AnaglyphPaletteValid)
		GenerateAnaglyphPalette();

	u8 borderheight = 0;
	u8 borderwidth = 0;

	if(GCSettings.hideoverscan == HIDEOVERSCAN_VERTICAL || GCSettings.hideoverscan == HIDEOVERSCAN_BOTH)
		borderheight = 8;
	if(GCSettings.hideoverscan == HIDEOVERSCAN_HORIZONTAL || GCSettings.hideoverscan == HIDEOVERSCAN_BOTH)
		borderwidth = 8;

	const s32 widthLimit = NES_WIDTH - (borderwidth << 1);
	const s32 heightLimit = NES_HEIGHT - (borderheight << 1);

	// populate the texture with red/cyan anaglyph
	MakeStereoTexture(XBufLeft, XBufRight, texturemem, widthLimit, heightLimit);

	// load texture into GX
	DCFlushRange(texturemem, NES_WIDTH * NES_HEIGHT * 2);

	// clear texture objects
	GX_InvalidateTexAll();

	// render textured quad
	draw_square(view);
	GX_DrawDone();

	if(ScreenshotRequested)
	{
		if(GCSettings.render == RENDER_ORIGINAL) // we can't take a screenshot in Original mode
		{
			oldRenderMode = RENDER_ORIGINAL;
			GCSettings.render = RENDER_UNFILTERED; // switch to unfiltered mode
			UpdateVideo = 1; // request the switch
		}
		else
		{
			ScreenshotRequested = 0;
			TakeScreenshot();
			if(oldRenderMode != -1)
			{
				GCSettings.render = oldRenderMode;
				oldRenderMode = -1;
			}
			ConfigRequested = 1;
		}
	}

	// EFB is ready to be copied into XFB
	VIDEO_SetNextFramebuffer(xfb[whichfb]);
	VIDEO_Flush();

	copynow = GX_TRUE;

	// Reset state and signal background VSync thread to begin waiting for next blanking interval
	vb_done = false;
	LWP_ThreadSignal(vb_queue);
}

/****************************************************************************
 * TakeScreenshot
 *
 * Copies the current screen into a GX texture
 ***************************************************************************/
void TakeScreenshot()
{
	IMGCTX pngContext = PNGU_SelectImageFromBuffer(savebuffer);

	if (pngContext != NULL)
	{
		gameScreenPngSize = PNGU_EncodeFromEFB(pngContext, vmode->fbWidth, vmode->efbHeight);
		PNGU_ReleaseImageContext(pngContext);

		if (gameScreenPngSize <= 0) {
			gameScreenPngSize = 0;
			return;
		}

		gameScreenPng = (u8 *) malloc(gameScreenPngSize);
		if (gameScreenPng == NULL) {
			gameScreenPngSize = 0;
			return;
		}
		memcpy(gameScreenPng, savebuffer, gameScreenPngSize);
	}
}

void ClearScreenshot()
{
	if(gameScreenPng)
	{
		gameScreenPngSize = 0;
		free(gameScreenPng);
		gameScreenPng = NULL;
	}
}

/****************************************************************************
 * ResetVideo_Menu
 *
 * Reset the video/rendering mode for the menu
****************************************************************************/
void
ResetVideo_Menu ()
{
	Mtx44 p;
	f32 yscale;
	u32 xfbHeight;
	GXRModeObj * rmode = FindVideoMode();

	SetupVideoMode(rmode); // reconfigure VI

	// clears the bg to color and clears the z buffer
	GXColor background = {0, 0, 0, 255};
	GX_SetCopyClear (background, GX_MAX_Z24);

	yscale = GX_GetYScaleFactor(vmode->efbHeight,vmode->xfbHeight);
	xfbHeight = GX_SetDispCopyYScale(yscale);
	GX_SetScissor(0,0,vmode->fbWidth,vmode->efbHeight);
	GX_SetDispCopySrc(0,0,vmode->fbWidth,vmode->efbHeight);
	GX_SetDispCopyDst(vmode->fbWidth,xfbHeight);
	GX_SetCopyFilter(vmode->aa,vmode->sample_pattern,GX_TRUE,vmode->vfilter);
	GX_SetFieldMode(vmode->field_rendering,((vmode->viHeight==2*vmode->xfbHeight)?GX_ENABLE:GX_DISABLE));

	if (vmode->aa)
		GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
	else
		GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);

	// setup the vertex descriptor
	// tells the flipper to expect direct data
	GX_ClearVtxDesc();
	GX_InvVtxCache ();
	GX_InvalidateTexAll();

	GX_SetVtxDesc(GX_VA_TEX0, GX_NONE);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc (GX_VA_CLR0, GX_DIRECT);

	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	GX_SetZMode (GX_FALSE, GX_LEQUAL, GX_TRUE);

	GX_SetNumChans(1);
	GX_SetNumTexGens(1);
	GX_SetNumTevStages(1);
	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

	guMtxIdentity(GXmodelView2D);
	guMtxTransApply (GXmodelView2D, GXmodelView2D, 0.0F, 0.0F, -50.0F);
	GX_LoadPosMtxImm(GXmodelView2D,GX_PNMTX0);

	guOrtho(p,0,479,0,639,0,300);
	GX_LoadProjectionMtx(p, GX_ORTHOGRAPHIC);

	GX_SetViewport(0,0,vmode->fbWidth,vmode->efbHeight,0,1);
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	GX_SetAlphaUpdate(GX_TRUE);
}

/****************************************************************************
 * Menu_Render
 *
 * Renders everything current sent to GX, and flushes video
 ***************************************************************************/
void Menu_Render()
{
	whichfb ^= 1; // flip framebuffer
	GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
	GX_SetColorUpdate(GX_TRUE);
	GX_CopyDisp(xfb[whichfb],GX_TRUE);
	GX_DrawDone();
	VIDEO_SetNextFramebuffer(xfb[whichfb]);
	VIDEO_Flush();
	VIDEO_WaitForFlush();
}

/****************************************************************************
 * Menu_DrawImg
 *
 * Draws the specified image on screen using GX
 ***************************************************************************/
void Menu_DrawImg(f32 xpos, f32 ypos, u16 width, u16 height, u8 data[],
	f32 degrees, f32 scaleX, f32 scaleY, u8 alpha)
{
	if(data == NULL)
		return;

	GXTexObj texObj;

	GX_InitTexObj(&texObj, data, width,height, GX_TF_RGBA8,GX_CLAMP, GX_CLAMP,GX_FALSE);
	GX_LoadTexObj(&texObj, GX_TEXMAP0);
	GX_InvalidateTexAll();

	GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
	GX_SetVtxDesc (GX_VA_TEX0, GX_DIRECT);

	Mtx m,m1,m2, mv;
	width  >>= 1;
	height >>= 1;

	guMtxIdentity (m1);
	guMtxScaleApply(m1,m1,scaleX,scaleY,1.0);
	guVector axis = (guVector) {0 , 0, 1 };
	guMtxRotAxisDeg (m2, &axis, degrees);
	guMtxConcat(m2,m1,m);

	guMtxTransApply(m,m, xpos+width,ypos+height,0);
	guMtxConcat (GXmodelView2D, m, mv);
	GX_LoadPosMtxImm (mv, GX_PNMTX0);

	GX_Begin(GX_QUADS, GX_VTXFMT0,4);
	GX_Position3f32(-width, -height,  0);
	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
	GX_TexCoord2f32(0, 0);

	GX_Position3f32(width, -height,  0);
	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
	GX_TexCoord2f32(1, 0);

	GX_Position3f32(width, height,  0);
	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
	GX_TexCoord2f32(1, 1);

	GX_Position3f32(-width, height,  0);
	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
	GX_TexCoord2f32(0, 1);
	GX_End();
	GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);

	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
	GX_SetVtxDesc (GX_VA_TEX0, GX_NONE);
}

/****************************************************************************
 * Menu_DrawRectangle
 *
 * Draws a rectangle at the specified coordinates using GX
 ***************************************************************************/
void Menu_DrawRectangle(f32 x, f32 y, f32 width, f32 height, GXColor color, u8 filled)
{
	long n = 4;
	f32 x2 = x+width;
	f32 y2 = y+height;
	guVector v[] = {{x,y,0.0f}, {x2,y,0.0f}, {x2,y2,0.0f}, {x,y2,0.0f}, {x,y,0.0f}};
	u8 fmt = GX_TRIANGLEFAN;

	if(!filled)
	{
		fmt = GX_LINESTRIP;
		n = 5;
	}

	GX_Begin(fmt, GX_VTXFMT0, n);
	for(long i=0; i<n; ++i)
	{
		GX_Position3f32(v[i].x, v[i].y,  v[i].z);
		GX_Color4u8(color.r, color.g, color.b, color.a);
	}
	GX_End();
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
//CAK: This 3D palette is for high contrast white on black games like Falsion
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

//CAK: This 3D palette is for high contrast white on black games like Falsion
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

//CAK: This 3D palette is for high contrast white on black games like Falsion
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

//CAK: This 3D palette is good for games which were already in anaglyph
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
//CAK: Create an RGB 565 colour (used in textures) for this stereoscopic 3D combination of 2 NES colours.
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
			anaglyph565[left][right] = ((ar & 0xf8) << 8) | ((ag & 0xfc) << 3) | ((ab & 0xf8) >> 3);
		}
	}
	AnaglyphPaletteValid = true;
}

/****************************************************************************
 * rgbcolor
 *
 * Support routine for gcpalette
 ****************************************************************************/

static unsigned int rgbcolor(u8 r1, u8 g1, u8 b1, u8 r2, u8 g2, u8 b2)
{
    int y1,cb1,cr1,y2,cb2,cr2,cb,cr;

    y1=(299*r1+587*g1+114*b1)/1000;
    cb1=(-16874*r1-33126*g1+50000*b1+12800000)/100000;
    cr1=(50000*r1-41869*g1-8131*b1+12800000)/100000;

    y2=(299*r2+587*g2+114*b2)/1000;
    cb2=(-16874*r2-33126*g2+50000*b2+12800000)/100000;
    cr2=(50000*r2-41869*g2-8131*b2+12800000)/100000;

    cb=(cb1+cb2) >> 1;
    cr=(cr1+cr2) >> 1;

    return ((y1 << 24) | (cb << 16) | (y2 << 8) | cr);
}

/****************************************************************************
 * SetPalette
 *
 * A shadow copy of the palette is maintained, in case the NES Emu kernel
 * requests a copy.
 ****************************************************************************/
void FCEUD_SetPalette(u8 index, u8 r, u8 g, u8 b)
{
    /*** Make PC compatible copy ***/
    pcpalette[index].r = r;
    pcpalette[index].g = g;
    pcpalette[index].b = b;

    /*** Generate Gamecube palette ***/
    gcpalette[index] = rgbcolor(r,g,b,r,g,b);

    /*** Generate RGB565 texture palette ***/
    rgb565[index] = ((r & 0xf8) << 8) |
        ((g & 0xfc) << 3) |
        ((b & 0xf8) >> 3);

	/*** Will need to generate stereoscopic palette later. ***/
	AnaglyphPaletteValid = false;
}

/****************************************************************************
 * GetPalette
 ****************************************************************************/
void FCEUD_GetPalette(u8 i, u8 *r, u8 *g, u8 *b)
{
    *r = pcpalette[i].r;
    *g = pcpalette[i].g;
    *b = pcpalette[i].b;
}

void SetPalette()
{
	if ( GCSettings.currpal == 0 )
	{
		// Do palette reset
		FCEU_ResetPalette();
	}
	else
	{
		// Now setup this palette
		u8 i,r,g,b;

		for ( i = 0; i < 64; i++ )
		{
			r = palettes[GCSettings.currpal-1].data[i] >> 16;
			g = ( palettes[GCSettings.currpal-1].data[i] & 0xff00 ) >> 8;
			b = ( palettes[GCSettings.currpal-1].data[i] & 0xff );
			FCEUD_SetPalette( i, r, g, b);
			FCEUD_SetPalette( i+64, r, g, b);
			FCEUD_SetPalette( i+128, r, g, b);
			FCEUD_SetPalette( i+192, r, g, b);
		}
	}
}

struct st_palettes palettes[] = {
    { "digital-prime-fbx", "Digital Prime (FBX)",
        { 0x696969, 0x00148F, 0x1E029B, 0x3F008A,
		    0x600060, 0x660017, 0x570D00, 0x451B00,
		    0x243400, 0x004200, 0x004500, 0x003C1F,
		    0x00315C, 0x000000, 0x000000, 0x000000,
		    0xAFAFAF, 0x0F51DD, 0x442FF3, 0x7220E2,
		    0xA319B3, 0xAE1C51, 0xA43400, 0x884D00,
		    0x676D00, 0x208000, 0x008B00, 0x007F42,
		    0x006C97, 0x010101, 0x000000, 0x000000,
		    0xFFFFFF, 0x65AAFF, 0x8C96FF, 0xB983FF,
		    0xDD6FFF, 0xEA6FBD, 0xEB8466, 0xDCA21F,
		    0xBAB403, 0x7ECB07, 0x54D33E, 0x3CD284,
		    0x3EC7CC, 0x4B4B4B, 0x000000, 0x000000,
		    0xFFFFFF, 0xBDE2FF, 0xCECFFF, 0xE6C2FF,
		    0xF6BCFF, 0xF9C2ED, 0xFACFC6, 0xF8DEAC,
		    0xEEE9A1, 0xD0F59F, 0xBBF5AF, 0xB3F5CD,
		    0xB9EDF0, 0xB9B9B9, 0x000000, 0x000000 }
    },
    { "smooth-fbx", "Smooth (FBX)",
        { 0x6A6D6A, 0x001380, 0x1E008A, 0x39007A,
		    0x550056, 0x5A0018, 0x4F1000, 0x3D1C00,
		    0x253200, 0x003D00, 0x004000, 0x003924,
		    0x002E55, 0x000000, 0x000000, 0x000000,
		    0xB9BCB9, 0x1850C7, 0x4B30E3, 0x7322D6,
		    0x951FA9, 0x9D285C, 0x983700, 0x7F4C00,
		    0x5E6400, 0x227700, 0x027E02, 0x007645,
		    0x006E8A, 0x000000, 0x000000, 0x000000,
		    0xFFFFFF, 0x68A6FF, 0x8C9CFF, 0xB586FF,
		    0xD975FD, 0xE377B9, 0xE58D68, 0xD49D29,
		    0xB3AF0C, 0x7BC211, 0x55CA47, 0x46CB81,
		    0x47C1C5, 0x4A4D4A, 0x000000, 0x000000,
		    0xFFFFFF, 0xCCEAFF, 0xDDDEFF, 0xECDAFF,
		    0xF8D7FE, 0xFCD6F5, 0xFDDBCF, 0xF9E7B5,
            0xF1F0AA, 0xDAFAA9, 0xC9FFBC, 0xC3FBD7,
            0xC4F6F6, 0xBEC1BE, 0x000000, 0x000000 }
    },
	{ "pvm-style-d93-fbx", "PVM Style D93 (FBX)",
	    { 0x696B63, 0x001774, 0x1E0087, 0x340073,
		    0x560057, 0x5E0013, 0x531A00, 0x3B2400,
		    0x243000, 0x063A00, 0x003F00, 0x003B1E,
		    0x00334E, 0x000000, 0x000000, 0x000000,
		    0xB9BBB3, 0x1453B9, 0x4D2CDA, 0x671EDE,
		    0x98189C, 0x9D2344, 0xA03E00, 0x8D5500,
		    0x656D00, 0x2C7900, 0x008100, 0x007D42,
		    0x00788A, 0x000000, 0x000000, 0x000000,
		    0xFFFFFF, 0x69A8FF, 0x9691FF, 0xB28AFA,
		    0xEA7DFA, 0xF37BC7, 0xF28E59, 0xE6AD27,
		    0xD7C805, 0x90DF07, 0x64E53C, 0x45E27D,
		    0x48D5D9, 0x4E5048, 0x000000, 0x000000,
		    0xFFFFFF, 0xD2EAFF, 0xE2E2FF, 0xE9D8FF,
		    0xF5D2FF, 0xF8D9EA, 0xFADEB9, 0xF9E89B,
		    0xF3F28C, 0xD3FA91, 0xB8FCA8, 0xAEFACA,
            0xCAF3F3, 0xBEC0B8, 0x000000, 0x000000 }
	},
	{ "composite-direct-fbx", "Composite Direct (FBX)",
		{ 0x656565, 0x00127D, 0x18008E, 0x360082,
		    0x56005D, 0x5A0018, 0x4F0500, 0x381900,
		    0x1D3100, 0x003D00, 0x004100, 0x003B17,
		    0x002E55, 0x000000, 0x000000, 0x000000,
		    0xAFAFAF, 0x194EC8, 0x472FE3, 0x6B1FD7,
		    0x931BAE, 0x9E1A5E, 0x993200, 0x7B4B00,
		    0x5B6700, 0x267A00, 0x008200, 0x007A3E,
		    0x006E8A, 0x000000, 0x000000, 0x000000,
		    0xFFFFFF, 0x64A9FF, 0x8E89FF, 0xB676FF,
		    0xE06FFF, 0xEF6CC4, 0xF0806A, 0xD8982C,
		    0xB9B40A, 0x83CB0C, 0x5BD63F, 0x4AD17E,
		    0x4DC7CB, 0x4C4C4C, 0x000000, 0x000000,
		    0xFFFFFF, 0xC7E5FF, 0xD9D9FF, 0xE9D1FF,
		    0xF9CEFF, 0xFFCCF1, 0xFFD4CB, 0xF8DFB1,
		    0xEDEAA4, 0xD6F4A4, 0xC5F8B8, 0xBEF6D3,
            0xBFF1F1, 0xB9B9B9, 0x000000, 0x000000 }
	},
	{ "magnum-fbx", "Magnum (FBX)",
        { 0x606060, 0x00148F, 0x1E029B, 0x3F008A,
            0x600060, 0x660017, 0x570D00, 0x3C1F00,
            0x1B3300, 0x004200, 0x004500, 0x003C1F,
            0x00315C, 0x000000, 0x000000, 0x000000,
            0xA6A6A6, 0x0F4BD4, 0x412DEB, 0x6C1DD9,
            0x9C17AB, 0xA71A4D, 0x993200, 0x7C4A00,
            0x546400, 0x1A7800, 0x007F00, 0x00763E,
            0x00678F, 0x010101, 0x000000, 0x000000,
            0xFFFFFF, 0x4C9CFF, 0x7278FF, 0x9B64FF,
            0xD964FF, 0xF064B6, 0xF37B50, 0xD5930F,
            0xB3AF0C, 0x78C403, 0x41D232, 0x34CA76,
            0x37BCC6, 0x4A4A4A, 0x000000, 0x000000,
            0xFFFFFF, 0xAED6FF, 0xBFC0FF, 0xCAB9FF,
            0xE6BEFD, 0xF5BEE1, 0xF5C8B9, 0xF0D7A0,
            0xE3E095, 0xC6EC95, 0xB5EBA8, 0xAAE6C3,
            0xABE5E7, 0xB1B1B1, 0x000000, 0x000000 }
	},
	{ "nes-classic-fbx-fs", "NES Classic (FBX-FS)",
	    { 0x60615F, 0x000083, 0x1D0195, 0x340875,
		    0x51055E, 0x56000F, 0x4C0700, 0x372308,
		    0x203A0B, 0x0F4B0E, 0x194C16, 0x02421E,
		    0x023154, 0x000000, 0x000000, 0x000000,
		    0xA9AAA8, 0x104BBF, 0x4712D8, 0x6300CA,
		    0x8800A9, 0x930B46, 0x8A2D04, 0x6F5206,
		    0x5C7114, 0x1B8D12, 0x199509, 0x178448,
		    0x206B8E, 0x000000, 0x000000, 0x000000,
		    0xFBFBFB, 0x6699F8, 0x8974F9, 0xAB58F8,
		    0xD557EF, 0xDE5FA9, 0xDC7F59, 0xC7A224,
		    0xA7BE03, 0x75D703, 0x60E34F, 0x3CD68D,
		    0x56C9CC, 0x414240, 0x000000, 0x000000,
		    0xFBFBFB, 0xBED4FA, 0xC9C7F9, 0xD7BEFA,
		    0xE8B8F9, 0xF5BAE5, 0xF3CAC2, 0xDFCDA7,
		    0xD9E09C, 0xC9EB9E, 0xC0EDB8, 0xB5F4C7,
		    0xB9EAE9, 0xABABAB, 0x000000, 0x000000 }
	},
	{ "rgb", "PC-10",
	    { 0x6D6D6D, 0x002492, 0x0000DB, 0x6D49DB,
		    0x92006D, 0xB6006D, 0xB62400, 0x924900,
		    0x6D4900, 0x244900, 0x006D24, 0x009200,
		    0x004949, 0x000000, 0x000000, 0x000000,
		    0xB6B6B6, 0x006DDB, 0x0049FF, 0x9200FF,
		    0xB600FF, 0xFF0092, 0xFF0000, 0xDB6D00,
		    0x926D00, 0x249200, 0x009200, 0x00B66D,
		    0x009292, 0x242424, 0x000000, 0x000000,
		    0xFFFFFF, 0x6DB6FF, 0x9292FF, 0xDB6DFF,
		    0xFF00FF, 0xFF6DFF, 0xFF9200, 0xFFB600,
		    0xDBDB00, 0x6DDB00, 0x00FF00, 0x49FFDB,
		    0x00FFFF, 0x494949, 0x000000, 0x000000,
		    0xFFFFFF, 0xB6DBFF, 0xDBB6FF, 0xFFB6FF,
		    0xFF92FF, 0xFFB6B6, 0xFFDB92, 0xFFFF49,
		    0xFFFF6D, 0xB6FF49, 0x92FF6D, 0x49FFDB,
		    0x92DBFF, 0x929292, 0x000000, 0x000000 }
    },
	{ "sony-cxa2025as-us", "Sony CXA",
        { 0x585858, 0x00238C, 0x00139B, 0x2D0585,
		    0x5D0052, 0x7A0017, 0x7A0800, 0x5F1800,
		    0x352A00, 0x093900, 0x003F00, 0x003C22,
		    0x00325D, 0x000000, 0x000000, 0x000000,
		    0xA1A1A1, 0x0053EE, 0x153CFE, 0x6028E4,
		    0xA91D98, 0xD41E41, 0xD22C00, 0xAA4400,
		    0x6C5E00, 0x2D7300, 0x007D06, 0x007852,
		    0x0069A9, 0x000000, 0x000000, 0x000000,
		    0xFFFFFF, 0x1FA5FE, 0x5E89FE, 0xB572FE,
		    0xFE65F6, 0xFE6790, 0xFE773C, 0xFE9308,
		    0xC4B200, 0x79CA10, 0x3AD54A, 0x11D1A4,
		    0x06BFFE, 0x424242, 0x000000, 0x000000,
		    0xFFFFFF, 0xA0D9FE, 0xBDCCFE, 0xE1C2FE,
		    0xFEBCFB, 0xFEBDD0, 0xFEC5A9, 0xFED18E,
		    0xE9DE86, 0xC7E992, 0xA8EEB0, 0x95ECD9,
            0x91E4FE, 0xACACAC, 0x000000, 0x000000 }
	},
	{ "wavebeam", "Wavebeam",
		{ 0x6B6B6B, 0x001B88, 0x21009A, 0x40008C,
		    0x600067, 0x64001E, 0x590800, 0x481600,
		    0x283600, 0x004500, 0x004908, 0x00421D,
		    0x003659, 0x000000, 0x000000, 0x000000,
		    0xB4B4B4, 0x1555D3, 0x4337EF, 0x7425DF,
		    0x9C19B9, 0xAC0F64, 0xAA2C00, 0x8A4B00,
		    0x666B00, 0x218300, 0x008A00, 0x008144,
		    0x007691, 0x000000, 0x000000, 0x000000,
		    0xFFFFFF, 0x63B2FF, 0x7C9CFF, 0xC07DFE,
		    0xE977FF, 0xF572CD, 0xF4886B, 0xDDA029,
		    0xBDBD0A, 0x89D20E, 0x5CDE3E, 0x4BD886,
		    0x4DCFD2, 0x525252, 0x000000, 0x000000,
		    0xFFFFFF, 0xBCDFFF, 0xD2D2FF, 0xE1C8FF,
		    0xEFC7FF, 0xFFC3E1, 0xFFCAC6, 0xF2DAAD,
		    0xEBE3A0, 0xD2EDA2, 0xBCF4B4, 0xB5F1CE,
		    0xB6ECF1, 0xBFBFBF, 0x000000, 0x000000 }
	},
	{ "pal", "PAL",
	    { 0x808080, 0x0000BA, 0x3700BF, 0x8400A6,
		    0xBB006A, 0xB7001E, 0xB30000, 0x912600,
		    0x7B2B00, 0x003E00, 0x00480D, 0x003C22,
		    0x002F66, 0x000000, 0x050505, 0x050505,
		    0xC8C8C8, 0x0059FF, 0x443CFF, 0xB733CC,
		    0xFE33AA, 0xFE375E, 0xFE371A, 0xD54B00,
		    0xC46200, 0x3C7B00, 0x1D8415, 0x009566,
		    0x0084C4, 0x111111, 0x090909, 0x090909,
		    0xFEFEFE, 0x0095FF, 0x6F84FF, 0xD56FFF,
		    0xFE77CC, 0xFE6F99, 0xFE7B59, 0xFE915F,
		    0xFEA233, 0xA6BF00, 0x51D96A, 0x4DD5AE,
		    0x00D9FF, 0x666666, 0x0D0D0D, 0x0D0D0D,
		    0xFEFEFE, 0x84BFFF, 0xBBBBFF, 0xD0BBFF,
		    0xFEBFEA, 0xFEBFCC, 0xFEC4B7, 0xFECCAE,
		    0xFED9A2, 0xCCE199, 0xAEEEB7, 0xAAF8EE,
		    0xB3EEFF, 0xDDDDDD, 0x111111, 0x111111 }
	},
	{ "restored-wii-vc", "Restored Wii VC",
	    { 0x666666, 0x000095, 0x10008B, 0x39007D,
                    0x5C0068, 0x660000, 0x5C0000, 0x391800,
		    0x223700, 0x004316, 0x004300, 0x003916,
   		    0x022D5E, 0x000000, 0x000000, 0x000000,
		    0xA39EA3, 0x0043B9, 0x4502F1, 0x6902CF,
		    0x8C00AC, 0x960050, 0x962E02, 0x7E4200,
		    0x5C6600, 0x227D02, 0x167D02, 0x027D46,
		    0x02667E, 0x161616, 0x000000, 0x000000,
		    0xF2F2F2, 0x689EFF, 0x8C7BFF, 0xB970FF,
		    0xE671F2, 0xF266B9, 0xFE8968, 0xCF9E46,
		    0xACA03B, 0x7EBC02, 0x4EC745, 0x45C77E,
		    0x50C7C6, 0x4E4E4E, 0x000000, 0x000000,
		    0xFFFFFF, 0xC4DCFE, 0xC6C7F4, 0xDBC7FF,
		    0xE9BDFF, 0xF2C6DC, 0xF4D2C4, 0xDBC8AE,
		    0xDBDDA0, 0xCFE9AE, 0xB9EAAC, 0xAEDCB9,
		    0xA1D2C6, 0xDEDEDE, 0x000000, 0x000000 }
	},
	{ "wii-vc", "Wii Virtual Console",
	    { 0x494949, 0x00006a, 0x090063, 0x290059,
		    0x42004a, 0x490000, 0x420000, 0x291100,
		    0x182700, 0x003010, 0x003000, 0x002910,
		    0x012043, 0x000000, 0x000000, 0x000000,
		    0x747174, 0x003084, 0x3101ac, 0x4b0194,
		    0x64007b, 0x6b0039, 0x6b2101, 0x5a2f00,
		    0x424900, 0x185901, 0x105901, 0x015932,
		    0x01495a, 0x101010, 0x000000, 0x000000,
		    0xadadad, 0x4a71b6, 0x6458d5, 0x8450e6,
		    0xa451ad, 0xad4984, 0xb5624a, 0x947132,
		    0x7b722a, 0x5a8601, 0x388e31, 0x318e5a,
		    0x398e8d, 0x383838, 0x000000, 0x000000,
		    0xb6b6b6, 0x8c9db5, 0x8d8eae, 0x9c8ebc,
		    0xa687bc, 0xad8d9d, 0xae968c, 0x9c8f7c,
		    0x9c9e72, 0x94a67c, 0x84a77b, 0x7c9d84,
		    0x73968d, 0xdedede, 0x000000, 0x000000 }
	}
};

//CAK: We need to know the OUT1 pin of the expansion port for Famicom 3D System glasses
extern uint8 shutter_3d;
//CAK: We need to know the palette in RAM for red/cyan anaglyph 3D games (3D World Runner and Rad Racer)
extern uint8 PALRAM[0x20];
bool old_shutter_3d_mode = 0, old_anaglyph_3d_mode = 0;
uint8 prev_shutter_3d = 0, prev_prev_shutter_3d = 0;
uint8 pal_3d = 0, prev_pal_3d = 0, prev_prev_pal_3d = 0; 

bool CheckForAnaglyphPalette()
{
	//CAK: It can also have none of these when all blacks
	bool hasRed = false, hasCyan = false, hasOther = false;
	pal_3d = 0;

	//CAK: first 12 background colours are used for anaglyph (last 4 are for status bar)
	for (int i = 0; i < 12; i++)
	{
		switch (PALRAM[i] & 63)
		{
			case 0x00:
			case 0x0F: //CAK: blacks
				break;
			case 0x01:
			case 0x11:
			case 0x0A:
			case 0x1A:
			case 0x0C:
			case 0x1C:
			case 0x2C: //CAK: cyan
				hasCyan = true;
				break;
			case 0x05:
			case 0x15:
			case 0x06:
			case 0x16: //CAK: reds
				hasRed = true;
				break;
			default:
				hasOther = true;
		}
	}

	if (hasOther || (hasRed && hasCyan))
		return false;

	//CAK: last 8 sprite colours are used for anaglyph (first 8 are for screen-level sprites)
	for (int i = 24; i < 32; i++)
	{
		switch (PALRAM[i] & 63)
		{
			case 0x00:
			case 0x0F: //CAK: blacks
				break;
			case 0x01:
			case 0x11:
			case 0x0A:
			case 0x1A:
			case 0x0C:
			case 0x1C:
			case 0x2c: //CAK: cyan
				hasCyan = true;
				break;
			case 0x05:
			case 0x15:
			case 0x06:
			case 0x16: //CAK: reds
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

//CAK: Handles automatically entering and exiting stereoscopic 3D mode, and detecting which eye to draw
void Check3D()
{
	//CAK: Stereoscopic 3D game mode detection
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
		//CAK: exited stereoscopic 3d mode, reset frameskip to 0
		fskip = 0;
		fskipc = 0;
		frameskip = 0;
	}
	else
	{
		//CAK: Only check anaglyph when it's not a Famicom 3D System game
		//Games are detected as anaglyph, only when they alternate between a very limited red palette
		//and a very limited blue/green palette. It's very unlikely other games will do that, but
		//not impossible.
		anaglyph_3d_mode = CheckForAnaglyphPalette() && pal_3d != prev_pal_3d && pal_3d == prev_prev_pal_3d && prev_pal_3d != 0;
		prev_prev_pal_3d = prev_pal_3d;
		prev_pal_3d = pal_3d;
		if (anaglyph_3d_mode)
		{
			fskip = 0;
		}
		else if (old_anaglyph_3d_mode)
		{
			//CAK: exited stereoscopic 3d mode, reset frameskip to 0
			fskip = 0;
			fskipc = 0;
			frameskip = 0;
		}
		//CAK: TODO: make a backup of palette whenever not in anaglyph mode,
		//and use it to override anaglyph's horible palette for full colour 3D
		//note the difficulty will be that palette entries get rearranged to
		//animate the road and will still need to be rearranged in our backup palette
	}
	old_shutter_3d_mode = shutter_3d_mode;
	old_anaglyph_3d_mode = anaglyph_3d_mode;
}
