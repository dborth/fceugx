/****************************************************************************
 * FCE Ultra
 * Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2009
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

#include "fceugx.h"
#include "fceusupport.h"
#include "gcvideo.h"
#include "gcaudio.h"
#include "menu.h"
#include "pad.h"
#include "gui/gui.h"

int FDSTimer = 0;
u32 FrameTimer = 0;
int FDSSwitchRequested;

/*** External 2D Video ***/
/*** 2D Video Globals ***/
GXRModeObj *vmode  = NULL; // Graphics Mode Object
static unsigned int *xfb[2] = { NULL, NULL }; // Framebuffers
static int whichfb = 0; // Frame buffer toggle
int screenheight = 480;
int screenwidth = 640;
bool progressive = false;
static int oldRenderMode = -1; // set to GCSettings.render when changing (temporarily) to another mode

/*** 3D GX ***/
#define TEX_WIDTH 256
#define TEX_HEIGHT 240
#define DEFAULT_FIFO_SIZE ( 256 * 1024 )
static u8 gp_fifo[DEFAULT_FIFO_SIZE] ATTRIBUTE_ALIGN(32);
static u32 copynow = GX_FALSE;
static GXTexObj texobj;
static Mtx view;
static Mtx GXmodelView2D;

/*** Texture memory ***/
static unsigned char texturemem[TEX_WIDTH * TEX_HEIGHT * 4] ATTRIBUTE_ALIGN (32);

static int UpdateVideo = 1;
static bool vmode_60hz = true;

u8 * gameScreenPng = NULL;
int gameScreenPngSize = 0;

#define HASPECT 256
#define VASPECT 240

// Need something to hold the PC palette
struct pcpal {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} pcpalette[256];

static unsigned int gcpalette[256];	// Much simpler GC palette
static unsigned short rgb565[256];	// Texture map palette
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
   -HASPECT,  VASPECT, 0,	// 0
    HASPECT,  VASPECT, 0,	// 1
    HASPECT, -VASPECT, 0,	// 2
   -HASPECT, -VASPECT, 0	// 3
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
	(VI_MAX_WIDTH_PAL - 640)/2,         // viXOrigin
	(VI_MAX_HEIGHT_PAL - 480)/2,        // viYOrigin
	640,             // viWidth
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
	(VI_MAX_WIDTH_NTSC - 640)/2,	// viXOrigin
	(VI_MAX_HEIGHT_NTSC - 480)/2,	// viYOrigin
	640,             // viWidth
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
	if (FCEUI_GetCurrentVidSystem(NULL, NULL) == 1) // PAL
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
#define TSTACK 16384
static lwp_t vbthread = LWP_THREAD_NULL;
static unsigned char vbstack[TSTACK];

/****************************************************************************
 * vbgetback
 *
 * This callback enables the emulator to keep running while waiting for a
 * vertical blank.
 *
 * Putting LWP to good use :)
 ***************************************************************************/
static void *
vbgetback (void *arg)
{
	while (1)
	{
		VIDEO_WaitVSync ();
		LWP_SuspendThread (vbthread);
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
 * Scaler Support Functions
 ***************************************************************************/
static inline void
draw_init ()
{
	GX_ClearVtxDesc ();
	GX_SetVtxDesc (GX_VA_POS, GX_INDEX8);
	GX_SetVtxDesc (GX_VA_CLR0, GX_INDEX8);
	GX_SetVtxDesc (GX_VA_TEX0, GX_DIRECT);

	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	GX_SetArray (GX_VA_POS, square, 3 * sizeof (s16));

	GX_SetNumTexGens (1);
	GX_SetNumChans (0);

	GX_SetTexCoordGen (GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

	GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);

	memset (&view, 0, sizeof (Mtx));
	guLookAt(view, &cam.pos, &cam.up, &cam.view);
	GX_LoadPosMtxImm (view, GX_PNMTX0);

	GX_InvVtxCache ();	// update vertex cache
}

static inline void
draw_vert (u8 pos, u8 c, f32 s, f32 t)
{
	GX_Position1x8 (pos);
	GX_Color1x8 (c);
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
	draw_vert (0, 0, 0.0, 0.0);
	draw_vert (1, 0, 1.0, 0.0);
	draw_vert (2, 0, 1.0, 1.0);
	draw_vert (3, 0, 0.0, 1.0);
	GX_End ();
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

	VIDEO_SetBlack(TRUE);
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
	if (GCSettings.render == 0)	// original render mode
	{
		xscale = 512 / 2; // use GX scaler instead VI
		yscale = TEX_HEIGHT / 2;
	}
	else // unfiltered and filtered mode
	{
		xscale = 256;
		yscale = vmode->efbHeight / 2;
	}

	if (GCSettings.widescreen)
	{
		if(GCSettings.render == 0)
			xscale = (3*xscale)/4;
		else
			xscale = 256; // match the original console's width for "widescreen" to prevent flickering
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
		case 1: // NTSC (480i)
			mode = &TVNtsc480IntDf;
			break;
		case 2: // Progressive (480p)
			mode = &TVNtsc480Prog;
			break;
		case 3: // PAL (50Hz)
			mode = &TVPal528IntDf;
			break;
		case 4: // PAL (60Hz)
			mode = &TVEurgb60Hz480IntDf;
			break;
		default:
			mode = VIDEO_GetPreferredMode(NULL);
			
			if(mode == &TVPal576IntDfScale)
				mode = &TVPal528IntDf;

			#ifdef HW_DOL
			/* we have component cables, but the preferred mode is interlaced
			 * why don't we switch into progressive?
			 * on the Wii, the user can do this themselves on their Wii Settings */
			if(VIDEO_HaveComponentCable())
				mode = &TVNtsc480Prog;
			#endif

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
			NTSC_240p.viYOrigin = (VI_MAX_HEIGHT_PAL - 480)/2;
			break;

		case VI_NTSC:
			// 480 lines (NTSC 60Hz)
			vmode_60hz = true;

			// Original Video modes (forced to NTSC 60Hz)
			// set video signal mode
			PAL_240p.viTVMode = VI_TVMODE_NTSC_DS;
			PAL_240p.viYOrigin = (VI_MAX_HEIGHT_NTSC - 480)/2;
			NTSC_240p.viTVMode = VI_TVMODE_NTSC_DS;
			break;

		default:
			// 480 lines (PAL 60Hz)
			vmode_60hz = true;

			// Original Video modes (forced to PAL 60Hz)
			// set video signal mode
			PAL_240p.viTVMode = VI_TVMODE(mode->viTVMode >> 2, VI_NON_INTERLACE);
			PAL_240p.viYOrigin = (VI_MAX_HEIGHT_NTSC - 480)/2;
			NTSC_240p.viTVMode = VI_TVMODE(mode->viTVMode >> 2, VI_NON_INTERLACE);
			break;
	}

	// check for progressive scan
	if (mode->viTVMode == VI_TVMODE_NTSC_PROG)
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

	VIDEO_SetBlack (FALSE);
	VIDEO_Flush ();
	VIDEO_WaitVSync ();
		
	if (mode->viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync();
	else
		while (VIDEO_GetNextField())
			VIDEO_WaitVSync();
	
	VIDEO_SetPostRetraceCallback ((VIRetraceCallback)copy_to_xfb);
	vmode = mode;
}

/****************************************************************************
 * InitGCVideo
 *
 * This function MUST be called at startup.
 * - also sets up menu video mode
 ***************************************************************************/
void
InitGCVideo ()
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
	SetupVideoMode(rmode);
	LWP_CreateThread (&vbthread, vbgetback, NULL, vbstack, TSTACK, 68);

	// Initialize GX
	GXColor background = { 0, 0, 0, 0xff };
	memset (&gp_fifo, 0, DEFAULT_FIFO_SIZE);
	GX_Init (&gp_fifo, DEFAULT_FIFO_SIZE);
	GX_SetCopyClear (background, 0x00ffffff);
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

	if (GCSettings.render == 0)
	{
		rmode = tvmodes[FCEUI_GetCurrentVidSystem(NULL, NULL)];
		UpdateSampleRate(48220);
		SetSampleRate();
	}
	else
	{
		rmode = FindVideoMode();
		
		if (GCSettings.widescreen)
			ResetFbWidth(640, rmode);
		else
			ResetFbWidth(512, rmode);
		
		UpdateSampleRate(48130);
		SetSampleRate();
	}

	SetupVideoMode(rmode); // reconfigure VI

	GXColor background = {0, 0, 0, 255};
	GX_SetCopyClear (background, 0x00ffffff);

	// reconfigure GX
	GX_SetViewport (0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
	GX_SetDispCopyYScale ((f32) rmode->xfbHeight / (f32) rmode->efbHeight);
	GX_SetScissor (0, 0, rmode->fbWidth, rmode->efbHeight);

	GX_SetDispCopySrc (0, 0, rmode->fbWidth, rmode->efbHeight);
	GX_SetDispCopyDst (rmode->fbWidth, rmode->xfbHeight);
	GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, (rmode->xfbMode == VI_XFBMODE_SF) ? GX_FALSE : GX_TRUE, rmode->vfilter);

	GX_SetFieldMode (rmode->field_rendering, ((rmode->viHeight == 2 * rmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));
	
	if (rmode->aa)
		GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
	else
		GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);

	GX_SetZMode (GX_TRUE, GX_LEQUAL, GX_TRUE);
	GX_SetColorUpdate (GX_TRUE);

	guOrtho(p, rmode->efbHeight/2, -(rmode->efbHeight/2), -(rmode->fbWidth/2), rmode->fbWidth/2, 100, 1000); // matrix, t, b, l, r, n, f
	GX_LoadProjectionMtx (p, GX_ORTHOGRAPHIC);

	// set aspect ratio
	draw_init ();
	UpdateScaling();

	// reinitialize texture
	GX_InvalidateTexAll ();
	GX_InitTexObj (&texobj, texturemem, TEX_WIDTH, TEX_HEIGHT, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);	// initialize the texture obj we are going to use
	if (!(GCSettings.render&1))
		GX_InitTexObjLOD(&texobj,GX_NEAR,GX_NEAR_MIP_NEAR,2.5,9.0,0.0,GX_FALSE,GX_FALSE,GX_ANISO_1); // original/unfiltered video mode: force texture filtering OFF
	GX_LoadTexObj (&texobj, GX_TEXMAP0);
	memset(texturemem, 0, TEX_WIDTH * TEX_HEIGHT * 2); // clear texture memory
}

/****************************************************************************
 * RenderFrame
 *
 * Render a single frame
 ****************************************************************************/

void RenderFrame(unsigned char *XBuf)
{
	// Ensure previous vb has complete
	while ((LWP_ThreadIsSuspended (vbthread) == 0) || (copynow == GX_TRUE))
		usleep (50);

	// swap framebuffers
	whichfb ^= 1;

	// video has changed
	if(UpdateVideo)
	{
		UpdateVideo = 0;
		ResetVideo_Emu(); // reset video to emulator rendering settings
	}

	int width, height;

	u8 borderheight = 0;
	u8 borderwidth = 0;

	// 0 = off, 1 = vertical, 2 = horizontal, 3 = both
	if(GCSettings.hideoverscan == 1 || GCSettings.hideoverscan == 3)
		borderheight = 8;
	if(GCSettings.hideoverscan >= 2)
		borderwidth = 8;

	u16 *texture = (unsigned short *)texturemem + (borderheight << 8) + (borderwidth << 2);
	u8 *src1 = XBuf + (borderheight << 8) + borderwidth;
	u8 *src2 = XBuf + (borderheight << 8) + borderwidth + 256;
	u8 *src3 = XBuf + (borderheight << 8) + borderwidth + 512;
	u8 *src4 = XBuf + (borderheight << 8) + borderwidth + 768;

	// fill the texture
	for (height = 0; height < 240 - (borderheight << 1); height += 4)
	{
		for (width = 0; width < 256 - (borderwidth << 1); width += 4)
		{
			// Row one
			*texture++ = rgb565[*src1++];
			*texture++ = rgb565[*src1++];
			*texture++ = rgb565[*src1++];
			*texture++ = rgb565[*src1++];

			// Row two
			*texture++ = rgb565[*src2++];
			*texture++ = rgb565[*src2++];
			*texture++ = rgb565[*src2++];
			*texture++ = rgb565[*src2++];

			// Row three
			*texture++ = rgb565[*src3++];
			*texture++ = rgb565[*src3++];
			*texture++ = rgb565[*src3++];
			*texture++ = rgb565[*src3++];

			// Row four
			*texture++ = rgb565[*src4++];
			*texture++ = rgb565[*src4++];
			*texture++ = rgb565[*src4++];
			*texture++ = rgb565[*src4++];
		}
		src1 += 768 + (borderwidth << 1); // line 4*N
		src2 += 768 + (borderwidth << 1); // line 4*(N+1)
		src3 += 768 + (borderwidth << 1); // line 4*(N+2)
		src4 += 768 + (borderwidth << 1); // line 4*(N+3)

		texture += (borderwidth << 3);
	}

	// load texture into GX
	DCFlushRange(texturemem, TEX_WIDTH * TEX_HEIGHT * 4);

	// clear texture objects
	GX_InvalidateTexAll();

	// render textured quad
	draw_square(view);
	GX_DrawDone();

	if(ScreenshotRequested)
	{
		if(GCSettings.render == 0) // we can't take a screenshot in Original mode
		{
			oldRenderMode = 0;
			GCSettings.render = 2; // switch to unfiltered mode
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

	// Return to caller, don't waste time waiting for vb
	LWP_ResumeThread (vbthread);
}

/****************************************************************************
 * RenderFrame
 *
 * Render a single frame
 ****************************************************************************/

void RenderStereoFrames(unsigned char *XBufLeft, unsigned char *XBufRight)
{
	// Ensure previous vb has complete
	while ((LWP_ThreadIsSuspended (vbthread) == 0) || (copynow == GX_TRUE))
		usleep (50);

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

	int width, height;

	u8 borderheight = 0;
	u8 borderwidth = 0;

	// 0 = off, 1 = vertical, 2 = horizontal, 3 = both
	if(GCSettings.hideoverscan == 1 || GCSettings.hideoverscan == 3)
		borderheight = 8;
	if(GCSettings.hideoverscan >= 2)
		borderwidth = 8;

	u16 *texture = (unsigned short *)texturemem + (borderheight << 8) + (borderwidth << 2);
	u8 *Lsrc1 = XBufLeft + (borderheight << 8) + borderwidth;
	u8 *Lsrc2 = XBufLeft + (borderheight << 8) + borderwidth + 256;
	u8 *Lsrc3 = XBufLeft + (borderheight << 8) + borderwidth + 512;
	u8 *Lsrc4 = XBufLeft + (borderheight << 8) + borderwidth + 768;
	u8 *Rsrc1 = XBufRight + (borderheight << 8) + borderwidth;
	u8 *Rsrc2 = XBufRight + (borderheight << 8) + borderwidth + 256;
	u8 *Rsrc3 = XBufRight + (borderheight << 8) + borderwidth + 512;
	u8 *Rsrc4 = XBufRight + (borderheight << 8) + borderwidth + 768;

	// fill the texture with red/cyan anaglyph
	for (height = 0; height < 240 - (borderheight << 1); height += 4)
	{
		for (width = 0; width < 256 - (borderwidth << 1); width += 4)
		{
			// Row one
			*texture++ = anaglyph565[(*Lsrc1++) & 63][(*Rsrc1++) & 63];
			*texture++ = anaglyph565[(*Lsrc1++) & 63][(*Rsrc1++) & 63];
			*texture++ = anaglyph565[(*Lsrc1++) & 63][(*Rsrc1++) & 63];
			*texture++ = anaglyph565[(*Lsrc1++) & 63][(*Rsrc1++) & 63];
			// Row two
			*texture++ = anaglyph565[(*Lsrc2++) & 63][(*Rsrc2++) & 63];
			*texture++ = anaglyph565[(*Lsrc2++) & 63][(*Rsrc2++) & 63];
			*texture++ = anaglyph565[(*Lsrc2++) & 63][(*Rsrc2++) & 63];
			*texture++ = anaglyph565[(*Lsrc2++) & 63][(*Rsrc2++) & 63];
			// Row three
			*texture++ = anaglyph565[(*Lsrc3++) & 63][(*Rsrc3++) & 63];
			*texture++ = anaglyph565[(*Lsrc3++) & 63][(*Rsrc3++) & 63];
			*texture++ = anaglyph565[(*Lsrc3++) & 63][(*Rsrc3++) & 63];
			*texture++ = anaglyph565[(*Lsrc3++) & 63][(*Rsrc3++) & 63];
			// Row four
			*texture++ = anaglyph565[(*Lsrc4++) & 63][(*Rsrc4++) & 63];
			*texture++ = anaglyph565[(*Lsrc4++) & 63][(*Rsrc4++) & 63];
			*texture++ = anaglyph565[(*Lsrc4++) & 63][(*Rsrc4++) & 63];
			*texture++ = anaglyph565[(*Lsrc4++) & 63][(*Rsrc4++) & 63];
		}
		Lsrc1 += 768 + (borderwidth << 1); // line 4*N
		Lsrc2 += 768 + (borderwidth << 1); // line 4*(N+1)
		Lsrc3 += 768 + (borderwidth << 1); // line 4*(N+2)
		Lsrc4 += 768 + (borderwidth << 1); // line 4*(N+3)
		Rsrc1 += 768 + (borderwidth << 1); // line 4*N
		Rsrc2 += 768 + (borderwidth << 1); // line 4*(N+1)
		Rsrc3 += 768 + (borderwidth << 1); // line 4*(N+2)
		Rsrc4 += 768 + (borderwidth << 1); // line 4*(N+3)

		texture += (borderwidth << 3);
	}

	// load texture into GX
	DCFlushRange(texturemem, TEX_WIDTH * TEX_HEIGHT * 4);

	// clear texture objects
	GX_InvalidateTexAll();

	// render textured quad
	draw_square(view);
	GX_DrawDone();

	if(ScreenshotRequested)
	{
		if(GCSettings.render == 0) // we can't take a screenshot in Original mode
		{
			oldRenderMode = 0;
			GCSettings.render = 2; // switch to unfiltered mode
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

	// Return to caller, don't waste time waiting for vb
	LWP_ResumeThread (vbthread);
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
		gameScreenPng = (u8 *)malloc(gameScreenPngSize);
		memcpy(gameScreenPng, savebuffer, gameScreenPngSize);
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
	GX_SetCopyClear (background, 0x00ffffff);

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
	VIDEO_WaitVSync();
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
		for (int right = 0; right < 64; right++)
		{
			u8 ar, ag, ab;
			OptimisedAnaglyph(&ar, &ag, &ab, pcpalette[left].r, pcpalette[left].g, pcpalette[left].b, pcpalette[right].r, pcpalette[right].g, pcpalette[right].b);
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
	// Unsaturated-V5 Palette By FirebrandX
    { "accurate", "Accurate Colors",
          { 0x6b6b6b, 0x001e87, 0x1f0b96, 0x3b0c87,
			0x590d61, 0x5e0528, 0x551100, 0x461b00,
			0x303200, 0x0a4800, 0x004e00, 0x004619,
			0x00395a, 0x000000, 0x000000, 0x000000,
			0xb2b2b2, 0x1a53d1, 0x4835ee, 0x7123ec,
			0x9a1eb7, 0xa51e62, 0xa52d19, 0x874b00,
			0x676900, 0x298400, 0x038b00, 0x008240,
			0x007096, 0x000000, 0x000000, 0x000000,
			0xffffff, 0x63adfd, 0x908afe, 0xb977fc,
			0xe771fe, 0xf76fc9, 0xf5836a, 0xdd9c29,
			0xbdb807, 0x84d107, 0x5bdc3b, 0x48d77d,
			0x48c6d8, 0x555555, 0x000000, 0x000000,
			0xffffff, 0xc4e3fe, 0xd7d5fe, 0xe6cdfe,
			0xf9cafe, 0xfec9f0, 0xfed1c7, 0xf7dcac,
			0xe8e89c, 0xd1f29d, 0xbff4b1, 0xb7f5cd,
			0xb7ebf2, 0xbebebe, 0x000000, 0x000000 }
    },
    // YUV-V3 Palette By FirebrandX
	{ "vivid", "Vivid Colors",
          { 0x666666, 0x002a88, 0x1412a7, 0x3b00a4,
			0x5c007e, 0x6e0040, 0x6c0700, 0x561d00,
			0x333500, 0x0c4800, 0x005200, 0x004c18,
			0x003e5b, 0x000000, 0x000000, 0x000000,
			0xadadad, 0x155fd9, 0x4240ff, 0x7527fe,
			0xa01acc, 0xb71e7b, 0xb53120, 0x994e00,
			0x6b6d00, 0x388700, 0x0d9300, 0x008c47,
			0x007aa0, 0x000000, 0x000000, 0x000000,
			0xffffff, 0x64b0ff, 0x9290ff, 0xc676ff,
			0xf26aff, 0xff6ecc, 0xff8170, 0xea9e22,
			0xbcbe00, 0x88d800, 0x5ce430, 0x45e082,
			0x48cdde, 0x4f4f4f, 0x000000, 0x000000,
			0xffffff, 0xc0dfff, 0xd3d2ff, 0xe8c8ff,
			0xfac2ff, 0xffc4ea, 0xffccc5, 0xf7d8a5,
			0xe4e594, 0xcfef96, 0xbdf4ab, 0xb3f3cc,
			0xb5ebf2, 0xb8b8b8, 0x000000, 0x000000 }
	},
	// Wii Virtual Console palette by SuperrSonic's
    { "wiivc", "Wii VC Colors",
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
    },
	// 3DS Virtual Console palette by SuperrSonic's
    { "3dsvc", "3DS VC Colors",
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
    },
	{ "asqrealc", "FCEUGX Colors",
		{ 0x6c6c6c, 0x00268e, 0x0000a8, 0x400094,
			0x700070, 0x780040, 0x700000, 0x621600,
			0x442400, 0x343400, 0x005000, 0x004444,
			0x004060, 0x000000, 0x101010, 0x101010,
			0xbababa, 0x205cdc, 0x3838ff, 0x8020f0,
			0xc000c0, 0xd01474, 0xd02020, 0xac4014,
			0x7c5400, 0x586400, 0x008800, 0x007468,
			0x00749c, 0x202020, 0x101010, 0x101010,
			0xffffff, 0x4ca0ff, 0x8888ff, 0xc06cff,
			0xff50ff, 0xff64b8, 0xff7878, 0xff9638,
			0xdbab00, 0xa2ca20, 0x4adc4a, 0x2ccca4,
			0x1cc2ea, 0x585858, 0x101010, 0x101010,
			0xffffff, 0xb0d4ff, 0xc4c4ff, 0xe8b8ff,
			0xffb0ff, 0xffb8e8, 0xffc4c4, 0xffd4a8,
			0xffe890, 0xf0f4a4, 0xc0ffc0, 0xacf4f0,
			0xa0e8ff, 0xc2c2c2, 0x202020, 0x101010 }
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
