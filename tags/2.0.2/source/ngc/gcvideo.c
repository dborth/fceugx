/****************************************************************************
 * FCE Ultra 0.98.12
 * Nintendo Wii/Gamecube Port
 *
 * Tantric September 2008
 *
 * gcvideo.c
 *
 * Video rendering
 ****************************************************************************/

#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiiuse/wpad.h>

#include "gcvideo.h"
#include "images/nesback.h"

extern unsigned int SMBTimer;

//#define FORCE_PAL50 1

#define TEX_WIDTH 256
#define TEX_HEIGHT 512
#define WIDTH 640
#define DEFAULT_FIFO_SIZE 256 * 1024

unsigned int *xfb[2];	/*** Framebuffer - used throughout ***/
GXRModeObj *vmode;
int screenheight;

/*** Need something to hold the PC palette ***/
struct pcpal {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} pcpalette[256];

unsigned int gcpalette[256];	/*** Much simpler GC palette ***/
unsigned short rgb565[256];	/*** Texture map palette ***/
static unsigned char gp_fifo[DEFAULT_FIFO_SIZE] __attribute__((__aligned__(32)));
static unsigned char texturemem[TEX_WIDTH * TEX_HEIGHT * 2] __attribute__((__aligned__(32)));
GXTexObj texobj;
GXColor background = {0, 0, 0, 0xff};
static Mtx projectionMatrix,modelViewMatrix;
void CheesyScale(unsigned char *XBuf);
int whichfb = 0;
int copynow = GX_FALSE;

/****************************************************************************
 * GX Chip Copy to XFB
 ****************************************************************************/
static void copy_to_xfb() {
    if (copynow == GX_TRUE) {
        GX_CopyDisp(xfb[whichfb],GX_TRUE);
        GX_Flush();
        copynow = GX_FALSE;
    }
    SMBTimer++;
}

/****************************************************************************
 * Initialise the GX
 ****************************************************************************/
void StartGX() {
    /*** Clear out FIFO area ***/
    memset(&gp_fifo, 0, DEFAULT_FIFO_SIZE);

    /*** Initialise GX ***/
    GX_Init(&gp_fifo, DEFAULT_FIFO_SIZE);
    GX_SetCopyClear(background, 0x00ffffff);

    /*** Additions from libogc ***/
    GX_SetViewport(10,0,vmode->fbWidth,vmode->efbHeight,0,1);
    GX_SetDispCopyYScale((f32)vmode->xfbHeight/(f32)vmode->efbHeight);
    GX_SetDispCopySrc(0,0,vmode->fbWidth,vmode->efbHeight);
    GX_SetDispCopyDst(vmode->fbWidth,vmode->xfbHeight);

    GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);

    GX_SetCullMode(GX_CULL_NONE);
    GX_SetZMode(GX_FALSE,GX_ALWAYS,GX_TRUE);
    GX_SetColorUpdate(GX_TRUE);
    GX_CopyDisp(xfb[whichfb],GX_TRUE);

    /*** Additions from ogc spaceship ***/
    GX_SetDispCopyGamma(GX_GM_1_0);

    GX_ClearVtxDesc();
    GX_SetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);
    GX_SetVtxAttrFmt(GX_VTXFMT0,GX_VA_TEX0,GX_TEX_ST,GX_F32,0);
    GX_SetVtxDesc(GX_VA_POS,GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0,GX_DIRECT);

    GX_SetNumChans(0);              /* default, color = vertex color */
    GX_SetNumTexGens(1);
    GX_SetTexCoordGen(GX_TEXCOORD0,GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY);
    GX_SetTevOrder(GX_TEVSTAGE0,GX_TEXCOORD0,GX_TEXMAP0,GX_COLORNULL);
    GX_SetTevOp(GX_TEVSTAGE0,GX_REPLACE);

    GX_InitTexObj(&texobj,&texturemem,TEX_WIDTH,TEX_HEIGHT,
            GX_TF_RGB565,GX_REPEAT,GX_REPEAT,GX_FALSE);

    DCFlushRange(&texturemem, TEX_WIDTH * TEX_HEIGHT * 2);
    GX_LoadTexObj(&texobj,GX_TEXMAP0);
    GX_InvalidateTexAll();

    /* load projection matrix */
    /*** Setting Height to 648 get's it right ? ***/
    guOrtho(projectionMatrix,10,610,640,0,-1,1);
    GX_LoadProjectionMtx(projectionMatrix,GX_ORTHOGRAPHIC);

    /* load model view matrix */
    c_guMtxScale(modelViewMatrix,660,640,1);
    GX_LoadPosMtxImm(modelViewMatrix,GX_PNMTX0);

}

/****************************************************************************
 * GXDraw
 *
 * Using the texture map draw with quads
 ****************************************************************************/
void GXDraw(unsigned char *XBuf) {
    float gs = 1.0;
    float gt = 1.0;
    int width, height,t,xb;
    unsigned short *texture;

    memset(&texturemem, 0, TEX_WIDTH * TEX_HEIGHT * 2);
    texture = (unsigned short *)&texturemem[16 * TEX_WIDTH];

    /*** Now draw the texture ***/
    t = 0;
    for(height = 0; height < 120; height++) {
        xb = height * 512;
        for(width = 256; width > 0; width -= 4) {
            /*** Row one ***/
            texture[t++] = rgb565[XBuf[xb + width-1]];
            texture[t++] = rgb565[XBuf[xb + width-2]];
            texture[t++] = rgb565[XBuf[xb + width-3]];
            texture[t++] = rgb565[XBuf[xb + width-4]];

            /*** Row three ***/
            texture[t++] = rgb565[XBuf[xb + width-1]];
            texture[t++] = rgb565[XBuf[xb + width-2]];
            texture[t++] = rgb565[XBuf[xb + width-3]];
            texture[t++] = rgb565[XBuf[xb + width-4]];

            /*** Row one ***/
            texture[t++] = rgb565[XBuf[xb + 256 + width-1]];
            texture[t++] = rgb565[XBuf[xb + 256 + width-2]];
            texture[t++] = rgb565[XBuf[xb + 256 + width-3]];
            texture[t++] = rgb565[XBuf[xb + 256 + width-4]];

            /*** Row three ***/
            texture[t++] = rgb565[XBuf[xb + 256 + width-1]];
            texture[t++] = rgb565[XBuf[xb + 256 + width-2]];
            texture[t++] = rgb565[XBuf[xb + 256 + width-3]];
            texture[t++] = rgb565[XBuf[xb + 256 + width-4]];
        }
    }

    DCFlushRange(&texturemem, TEX_WIDTH * TEX_HEIGHT * 2);

    /* setup GX */
    GX_InvalidateTexAll();

    // ok render the triangles now
    GX_Begin(GX_QUADS,GX_VTXFMT0,4);
    {
        GX_Position3f32(0,0,0);
        GX_TexCoord2f32(0,0);

        GX_Position3f32(0,1,0);
        GX_TexCoord2f32(0,gt);

        GX_Position3f32(1,1,0);
        GX_TexCoord2f32(gs,gt);

        GX_Position3f32(1,0,0);
        GX_TexCoord2f32(gs,0);
    }
    GX_End();
    GX_DrawDone();
    copynow = GX_TRUE;
}

/****************************************************************************
 * UpdatePadsCB
 *
 * called by postRetraceCallback in InitGCVideo - scans gcpad and wpad
 ****************************************************************************/
void
UpdatePadsCB ()
{
#ifdef HW_RVL
	WPAD_ScanPads();
#endif
	PAD_ScanPads();
}

/****************************************************************************
 * initDisplay
 *
 * It should be noted that this function forces the system to use a
 * 640x480 viewport for either NTSC or PAL.
 *
 * Helps keep the rendering at 2x sweet
 ****************************************************************************/
void initDisplay() {
    /*** Start VIDEO Subsystem ***/
    VIDEO_Init();

    vmode = VIDEO_GetPreferredMode(NULL);
    VIDEO_Configure(vmode);

    screenheight = vmode->xfbHeight;

    xfb[0] = (u32 *) MEM_K0_TO_K1 (SYS_AllocateFramebuffer (vmode));
    xfb[1] = (u32 *) MEM_K0_TO_K1 (SYS_AllocateFramebuffer (vmode));

    VIDEO_SetNextFramebuffer(xfb[0]);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();

    if(vmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
    VIDEO_SetPostRetraceCallback((VIRetraceCallback)UpdatePadsCB);
    /*** Setup a console - guard against spurious printf ***/
    VIDEO_SetPreRetraceCallback((VIRetraceCallback)copy_to_xfb);
    VIDEO_SetNextFramebuffer(xfb[0]);

    PAD_Init();
    StartGX();
}

/****************************************************************************
 * RenderFrame
 *
 * Render a single frame at 2x zoom
 ****************************************************************************/
#define NESWIDTH 256
#define NESHEIGHT 240
void RenderFrame(char *XBuf, int style) {
    int gcdispOffset = 32;	/*** Offset to centre on screen ***/
    int w,h;
    int c,i;

    whichfb ^= 1;
    switch(style) {
        case 0 :
            VIDEO_ClearFrameBuffer(vmode, xfb[whichfb], COLOR_BLACK);

            /*** Simply go through each row ***/
            for(h = 0; h < NESHEIGHT; h++) {
                for(w = 0; w < NESWIDTH; w++) {
                    c = (h << 8) + w;
                    i = gcdispOffset + w;
                    /*** Fast Zoom - Repeat each row, use 1 Xbuf == 2 GC
                      To speed up more, use indexed palette array ***/

                    xfb[whichfb][i] = gcpalette[(unsigned char)XBuf[c]];
                    xfb[whichfb][i + 320] = gcpalette[(unsigned char)XBuf[c]];
                }
                gcdispOffset += 640;
            }
            break;

        case 1:
            CheesyScale((unsigned char *)XBuf);
            break;

        case 2:
            GXDraw((unsigned char *)XBuf);
            break;
    }

    /*** Now resync with VSync ***/
    VIDEO_SetNextFramebuffer(xfb[whichfb]);
    VIDEO_Flush();
    VIDEO_WaitVSync();
}

/****************************************************************************
 * rgbcolor
 *
 * Support routine for gcpalette
 ****************************************************************************/

unsigned int rgbcolor(unsigned char r1, unsigned char g1, unsigned char b1,
        unsigned char r2, unsigned char g2, unsigned char b2) {
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
void FCEUD_SetPalette(unsigned char index, unsigned char r, unsigned char g,
        unsigned char b) {
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
}

/****************************************************************************
 * GetPalette
 ****************************************************************************/
void FCEUD_GetPalette(unsigned char i, unsigned char *r, unsigned char *g,
        unsigned char *b) {
    *r = pcpalette[i].r;
    *g = pcpalette[i].g;
    *b = pcpalette[i].b;
}

/****************************************************************************
 * NES Cheesy Scaler
 *
 * This scaler simply attempts to correct the 1.25 aspect by
 * stretching the initial 256 pixels to 320.
 * The standard 2x2 scaler can then be applied
 ****************************************************************************/
void CheesyScale(unsigned char *XBuf) {
    static int newrow[320];		/*** New cheesy row ***/
    unsigned int cheesypal[256];	/*** Enhanced Cheesy Palette ***/
    int i,j,c,p = 0;
    unsigned char p1,p2;
    unsigned int ofs, gcdispOffset = 0;
    int h, n, nw;

    /*** Stretch ***/
    for (h = 0; h < NESHEIGHT; h++) {
        j = c = p = 0;
        for (i = 0; i < NESWIDTH; i++) {
            /*** Every fifth pixel is stretched by adding
              the mid colour range ***/
            n = (h << 8) + i;
            newrow[j++] = XBuf[n];
            c++;
            if (c == 4) {  /*** Done 4 pixels, so add the fifth ***/
                p1 = XBuf[n];
                p2 = XBuf[n+1];
                cheesypal[p] = rgbcolor(pcpalette[p1].r, pcpalette[p1].g,
                    pcpalette[p1].b, pcpalette[p2].r, pcpalette[p2].g,
                    pcpalette[p2].b);
                newrow[j++] = 0x8000 + p;
                p++;
                c = 0;
            }
        }


        /*** Now update the screen display with the new colours ***/
        ofs = gcdispOffset;
        for (nw = 0; nw < 320; nw++) {
            if (newrow[nw] & 0x8000) {
                xfb[whichfb][ofs + nw] = cheesypal[newrow[nw] & 0xff ];
                xfb[whichfb][ofs + 320 + nw] = cheesypal[newrow[nw] & 0xff];
            } else {
                xfb[whichfb][ofs + nw] = gcpalette[newrow[nw]];
                xfb[whichfb][ofs + nw + 320] = gcpalette[newrow[nw]];
            }
        }

        gcdispOffset += 640;
    }
}

void
clearscreen ()
{
	int colour = COLOR_BLACK;
	whichfb ^= 1;
	VIDEO_ClearFrameBuffer (vmode, xfb[whichfb], colour);
	memcpy (xfb[whichfb], &bg, 1280 * 480);
}

void
showscreen ()
{
	copynow = GX_FALSE;
	VIDEO_SetNextFramebuffer (xfb[whichfb]);
	VIDEO_Flush ();
	VIDEO_WaitVSync ();
}

struct st_palettes palettes[] = {
    /* The default NES palette must be the first entry in the array */
    { "loopy", "Loopy's NES palette",
        { 0x757575, 0x271b8f, 0x0000ab, 0x47009f,
            0x8f0077, 0xab0013, 0xa70000, 0x7f0b00,
            0x432f00, 0x004700, 0x005100, 0x003f17,
            0x1b3f5f, 0x000000, 0x000000, 0x000000,
            0xbcbcbc, 0x0073ef, 0x233bef, 0x8300f3,
            0xbf00bf, 0xe7005b, 0xdb2b00, 0xcb4f0f,
            0x8b7300, 0x009700, 0x00ab00, 0x00933b,
            0x00838b, 0x000000, 0x000000, 0x000000,
            0xffffff, 0x3fbfff, 0x5f97ff, 0xa78bfd,
            0xf77bff, 0xff77b7, 0xff7763, 0xff9b3b,
            0xf3bf3f, 0x83d313, 0x4fdf4b, 0x58f898,
            0x00ebdb, 0x000000, 0x000000, 0x000000,
            0xffffff, 0xabe7ff, 0xc7d7ff, 0xd7cbff,
            0xffc7ff, 0xffc7db, 0xffbfb3, 0xffdbab,
            0xffe7a3, 0xe3ffa3, 0xabf3bf, 0xb3ffcf,
            0x9ffff3, 0x000000, 0x000000, 0x000000 }
    },
    { "quor", "Quor's palette from Nestra 0.63",
        { 0x3f3f3f, 0x001f3f, 0x00003f, 0x1f003f,
            0x3f003f, 0x3f0020, 0x3f0000, 0x3f2000,
            0x3f3f00, 0x203f00, 0x003f00, 0x003f20,
            0x003f3f, 0x000000, 0x000000, 0x000000,
            0x7f7f7f, 0x405f7f, 0x40407f, 0x5f407f,
            0x7f407f, 0x7f4060, 0x7f4040, 0x7f6040,
            0x7f7f40, 0x607f40, 0x407f40, 0x407f60,
            0x407f7f, 0x000000, 0x000000, 0x000000,
            0xbfbfbf, 0x809fbf, 0x8080bf, 0x9f80bf,
            0xbf80bf, 0xbf80a0, 0xbf8080, 0xbfa080,
            0xbfbf80, 0xa0bf80, 0x80bf80, 0x80bfa0,
            0x80bfbf, 0x000000, 0x000000, 0x000000,
            0xffffff, 0xc0dfff, 0xc0c0ff, 0xdfc0ff,
            0xffc0ff, 0xffc0e0, 0xffc0c0, 0xffe0c0,
            0xffffc0, 0xe0ffc0, 0xc0ffc0, 0xc0ffe0,
            0xc0ffff, 0x000000, 0x000000, 0x000000 }
    },
    { "chris", "Chris Covell's NES palette",
        { 0x808080, 0x003DA6, 0x0012B0, 0x440096,
            0xA1005E, 0xC70028, 0xBA0600, 0x8C1700,
            0x5C2F00, 0x104500, 0x054A00, 0x00472E,
            0x004166, 0x000000, 0x050505, 0x050505,
            0xC7C7C7, 0x0077FF, 0x2155FF, 0x8237FA,
            0xEB2FB5, 0xFF2950, 0xFF2200, 0xD63200,
            0xC46200, 0x358000, 0x058F00, 0x008A55,
            0x0099CC, 0x212121, 0x090909, 0x090909,
            0xFFFFFF, 0x0FD7FF, 0x69A2FF, 0xD480FF,
            0xFF45F3, 0xFF618B, 0xFF8833, 0xFF9C12,
            0xFABC20, 0x9FE30E, 0x2BF035, 0x0CF0A4,
            0x05FBFF, 0x5E5E5E, 0x0D0D0D, 0x0D0D0D,
            0xFFFFFF, 0xA6FCFF, 0xB3ECFF, 0xDAABEB,
            0xFFA8F9, 0xFFABB3, 0xFFD2B0, 0xFFEFA6,
            0xFFF79C, 0xD7E895, 0xA6EDAF, 0xA2F2DA,
            0x99FFFC, 0xDDDDDD, 0x111111, 0x111111 }
    },
    { "matt", "Matthew Conte's NES palette",
        { 0x808080, 0x0000bb, 0x3700bf, 0x8400a6,
            0xbb006a, 0xb7001e, 0xb30000, 0x912600,
            0x7b2b00, 0x003e00, 0x00480d, 0x003c22,
            0x002f66, 0x000000, 0x050505, 0x050505,
            0xc8c8c8, 0x0059ff, 0x443cff, 0xb733cc,
            0xff33aa, 0xff375e, 0xff371a, 0xd54b00,
            0xc46200, 0x3c7b00, 0x1e8415, 0x009566,
            0x0084c4, 0x111111, 0x090909, 0x090909,
            0xffffff, 0x0095ff, 0x6f84ff, 0xd56fff,
            0xff77cc, 0xff6f99, 0xff7b59, 0xff915f,
            0xffa233, 0xa6bf00, 0x51d96a, 0x4dd5ae,
            0x00d9ff, 0x666666, 0x0d0d0d, 0x0d0d0d,
            0xffffff, 0x84bfff, 0xbbbbff, 0xd0bbff,
            0xffbfea, 0xffbfcc, 0xffc4b7, 0xffccae,
            0xffd9a2, 0xcce199, 0xaeeeb7, 0xaaf7ee,
            0xb3eeff, 0xdddddd, 0x111111, 0x111111 }
    },
    { "pasofami", "Palette from PasoFami/99",
        { 0x7f7f7f, 0x0000ff, 0x0000bf, 0x472bbf,
            0x970087, 0xab0023, 0xab1300, 0x8b1700,
            0x533000, 0x007800, 0x006b00, 0x005b00,
            0x004358, 0x000000, 0x000000, 0x000000,
            0xbfbfbf, 0x0078f8, 0x0058f8, 0x6b47ff,
            0xdb00cd, 0xe7005b, 0xf83800, 0xe75f13,
            0xaf7f00, 0x00b800, 0x00ab00, 0x00ab47,
            0x008b8b, 0x000000, 0x000000, 0x000000,
            0xf8f8f8, 0x3fbfff, 0x6b88ff, 0x9878f8,
            0xf878f8, 0xf85898, 0xf87858, 0xffa347,
            0xf8b800, 0xb8f818, 0x5bdb57, 0x58f898,
            0x00ebdb, 0x787878, 0x000000, 0x000000,
            0xffffff, 0xa7e7ff, 0xb8b8f8, 0xd8b8f8,
            0xf8b8f8, 0xfba7c3, 0xf0d0b0, 0xffe3ab,
            0xfbdb7b, 0xd8f878, 0xb8f8b8, 0xb8f8d8,
            0x00ffff, 0xf8d8f8, 0x000000, 0x000000 }
    },
    { "crashman", "CrashMan's NES palette",
        { 0x585858, 0x001173, 0x000062, 0x472bbf,
            0x970087, 0x910009, 0x6f1100, 0x4c1008,
            0x371e00, 0x002f00, 0x005500, 0x004d15,
            0x002840, 0x000000, 0x000000, 0x000000,
            0xa0a0a0, 0x004499, 0x2c2cc8, 0x590daa,
            0xae006a, 0xb00040, 0xb83418, 0x983010,
            0x704000, 0x308000, 0x207808, 0x007b33,
            0x1c6888, 0x000000, 0x000000, 0x000000,
            0xf8f8f8, 0x267be1, 0x5870f0, 0x9878f8,
            0xff73c8, 0xf060a8, 0xd07b37, 0xe09040,
            0xf8b300, 0x8cbc00, 0x40a858, 0x58f898,
            0x00b7bf, 0x787878, 0x000000, 0x000000,
            0xffffff, 0xa7e7ff, 0xb8b8f8, 0xd8b8f8,
            0xe6a6ff, 0xf29dc4, 0xf0c0b0, 0xfce4b0,
            0xe0e01e, 0xd8f878, 0xc0e890, 0x95f7c8,
            0x98e0e8, 0xf8d8f8, 0x000000, 0x000000 }
    },
    { "mess", "palette from the MESS NES driver",
        { 0x747474, 0x24188c, 0x0000a8, 0x44009c,
            0x8c0074, 0xa80010, 0xa40000, 0x7c0800,
            0x402c00, 0x004400, 0x005000, 0x003c14,
            0x183c5c, 0x000000, 0x000000, 0x000000,
            0xbcbcbc, 0x0070ec, 0x2038ec, 0x8000f0,
            0xbc00bc, 0xe40058, 0xd82800, 0xc84c0c,
            0x887000, 0x009400, 0x00a800, 0x009038,
            0x008088, 0x000000, 0x000000, 0x000000,
            0xfcfcfc, 0x3cbcfc, 0x5c94fc, 0x4088fc,
            0xf478fc, 0xfc74b4, 0xfc7460, 0xfc9838,
            0xf0bc3c, 0x80d010, 0x4cdc48, 0x58f898,
            0x00e8d8, 0x000000, 0x000000, 0x000000,
            0xfcfcfc, 0xa8e4fc, 0xc4d4fc, 0xd4c8fc,
            0xfcc4fc, 0xfcc4d8, 0xfcbcb0, 0xfcd8a8,
            0xfce4a0, 0xe0fca0, 0xa8f0bc, 0xb0fccc,
            0x9cfcf0, 0x000000, 0x000000, 0x000000 }
    },
    { "zaphod-cv", "Zaphod's VS Castlevania palette",
        { 0x7f7f7f, 0xffa347, 0x0000bf, 0x472bbf,
            0x970087, 0xf85898, 0xab1300, 0xf8b8f8,
            0xbf0000, 0x007800, 0x006b00, 0x005b00,
            0xffffff, 0x9878f8, 0x000000, 0x000000,
            0xbfbfbf, 0x0078f8, 0xab1300, 0x6b47ff,
            0x00ae00, 0xe7005b, 0xf83800, 0x7777ff,
            0xaf7f00, 0x00b800, 0x00ab00, 0x00ab47,
            0x008b8b, 0x000000, 0x000000, 0x472bbf,
            0xf8f8f8, 0xffe3ab, 0xf87858, 0x9878f8,
            0x0078f8, 0xf85898, 0xbfbfbf, 0xffa347,
            0xc800c8, 0xb8f818, 0x7f7f7f, 0x007800,
            0x00ebdb, 0x000000, 0x000000, 0xffffff,
            0xffffff, 0xa7e7ff, 0x5bdb57, 0xe75f13,
            0x004358, 0x0000ff, 0xe7005b, 0x00b800,
            0xfbdb7b, 0xd8f878, 0x8b1700, 0xffe3ab,
            0x00ffff, 0xab0023, 0x000000, 0x000000 }
    },
    { "zaphod-smb", "Zaphod's VS SMB palette",
        { 0x626a00, 0x0000ff, 0x006a77, 0x472bbf,
            0x970087, 0xab0023, 0xab1300, 0xb74800,
            0xa2a2a2, 0x007800, 0x006b00, 0x005b00,
            0xffd599, 0xffff00, 0x009900, 0x000000,
            0xff66ff, 0x0078f8, 0x0058f8, 0x6b47ff,
            0x000000, 0xe7005b, 0xf83800, 0xe75f13,
            0xaf7f00, 0x00b800, 0x5173ff, 0x00ab47,
            0x008b8b, 0x000000, 0x91ff88, 0x000088,
            0xf8f8f8, 0x3fbfff, 0x6b0000, 0x4855f8,
            0xf878f8, 0xf85898, 0x595958, 0xff009d,
            0x002f2f, 0xb8f818, 0x5bdb57, 0x58f898,
            0x00ebdb, 0x787878, 0x000000, 0x000000,
            0xffffff, 0xa7e7ff, 0x590400, 0xbb0000,
            0xf8b8f8, 0xfba7c3, 0xffffff, 0x00e3e1,
            0xfbdb7b, 0xffae00, 0xb8f8b8, 0xb8f8d8,
            0x00ff00, 0xf8d8f8, 0xffaaaa, 0x004000 }
    },
    { "vs-drmar", "VS Dr. Mario palette",
        { 0x5f97ff, 0x000000, 0x000000, 0x47009f,
            0x00ab00, 0xffffff, 0xabe7ff, 0x000000,
            0x000000, 0x000000, 0x000000, 0x000000,
            0xe7005b, 0x000000, 0x000000, 0x000000,
            0x5f97ff, 0x000000, 0x000000, 0x000000,
            0x000000, 0x8b7300, 0xcb4f0f, 0x000000,
            0xbcbcbc, 0x000000, 0x000000, 0x000000,
            0x000000, 0x000000, 0x000000, 0x000000,
            0x00ebdb, 0x000000, 0x000000, 0x000000,
            0x000000, 0xff9b3b, 0x000000, 0x000000,
            0x83d313, 0x000000, 0x3fbfff, 0x000000,
            0x0073ef, 0x000000, 0x000000, 0x000000,
            0x00ebdb, 0x000000, 0x000000, 0x000000,
            0x000000, 0x000000, 0xf3bf3f, 0x000000,
            0x005100, 0x000000, 0xc7d7ff, 0xffdbab,
            0x000000, 0x000000, 0x000000, 0x000000 }
    },
    { "vs-cv", "VS Castlevania palette",
        { 0xaf7f00, 0xffa347, 0x008b8b, 0x472bbf,
            0x970087, 0xf85898, 0xab1300, 0xf8b8f8,
            0xf83800, 0x007800, 0x006b00, 0x005b00,
            0xffffff, 0x9878f8, 0x00ab00, 0x000000,
            0xbfbfbf, 0x0078f8, 0xab1300, 0x6b47ff,
            0x000000, 0xe7005b, 0xf83800, 0x6b88ff,
            0xaf7f00, 0x00b800, 0x6b88ff, 0x00ab47,
            0x008b8b, 0x000000, 0x000000, 0x472bbf,
            0xf8f8f8, 0xffe3ab, 0xf87858, 0x9878f8,
            0x0078f8, 0xf85898, 0xbfbfbf, 0xffa347,
            0x004358, 0xb8f818, 0x7f7f7f, 0x007800,
            0x00ebdb, 0x000000, 0x000000, 0xffffff,
            0xffffff, 0xa7e7ff, 0x5bdb57, 0x6b88ff,
            0x004358, 0x0000ff, 0xe7005b, 0x00b800,
            0xfbdb7b, 0xffa347, 0x8b1700, 0xffe3ab,
            0xb8f818, 0xab0023, 0x000000, 0x007800 }
    },
    /* The default VS palette must be the last entry in the array */
    { "vs-smb", "VS SMB/VS Ice Climber palette",
        { 0xaf7f00, 0x0000ff, 0x008b8b, 0x472bbf,
            0x970087, 0xab0023, 0x0000ff, 0xe75f13,
            0xbfbfbf, 0x007800, 0x5bdb57, 0x005b00,
            0xf0d0b0, 0xffe3ab, 0x00ab00, 0x000000,
            0xbfbfbf, 0x0078f8, 0x0058f8, 0x6b47ff,
            0x000000, 0xe7005b, 0xf83800, 0xf87858,
            0xaf7f00, 0x00b800, 0x6b88ff, 0x00ab47,
            0x008b8b, 0x000000, 0x000000, 0x3fbfff,
            0xf8f8f8, 0x006b00, 0x8b1700, 0x9878f8,
            0x6b47ff, 0xf85898, 0x7f7f7f, 0xe7005b,
            0x004358, 0xb8f818, 0x0078f8, 0x58f898,
            0x00ebdb, 0xfbdb7b, 0x000000, 0x000000,
            0xffffff, 0xa7e7ff, 0xb8b8f8, 0xf83800,
            0xf8b8f8, 0xfba7c3, 0xffffff, 0x00ffff,
            0xfbdb7b, 0xffa347, 0xb8f8b8, 0xb8f8d8,
            0xb8f818, 0xf8d8f8, 0x000000, 0x007800 }
    }
};
