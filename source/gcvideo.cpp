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
static u32 *xfb[2] = { NULL, NULL }; // Framebuffers
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
	}
	else
	{
		rmode = FindVideoMode();
		
		if (GCSettings.widescreen)
			ResetFbWidth(640, rmode);
		else
			ResetFbWidth(512, rmode);
		
		UpdateSampleRate(48130);
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
	u8 sharp[7] = {0,0,21,22,21,0,0};
	u8 soft[7] = {8,8,10,12,10,8,8};
	u8* vfilter =
		GCSettings.render == 3 ? sharp
		: GCSettings.render == 4 ? soft
		: rmode->vfilter;
	GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, (rmode->xfbMode == VI_XFBMODE_SF) ? GX_FALSE : GX_TRUE, vfilter);

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
