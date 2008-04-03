/****************************************************************************
 * GCVideo
 *
 * This module contains all GameCube video routines
 ****************************************************************************/

#include <gccore.h>
#include <ogcsys.h>
#include <string.h>
#include "../../iplfont/iplfont.h"
#include "nesback.h"

//#define FORCE_PAL50 1

#define TEX_WIDTH 256
#define TEX_HEIGHT 512
#define WIDTH 640
#define DEFAULT_FIFO_SIZE 256 * 1024

unsigned int *xfb[2];	/*** Framebuffer - used throughout ***/
GXRModeObj *vmode;

/*** Backdrop ***/
extern unsigned char backdrop[614400];

/*** Need something to hold the PC palette ***/
struct pcpal {
	unsigned char r;
	unsigned char g;
	unsigned char b;
} pcpalette[256];

unsigned int gcpalette[256];	/*** Much simpler GC palette ***/
unsigned short rgb565[256];	/*** Texture map palette ***/
static unsigned char gp_fifo[DEFAULT_FIFO_SIZE] __attribute__((__aligned__(32)));
static unsigned char texturemem[ TEX_WIDTH * TEX_HEIGHT * 2 ] __attribute__((__aligned__(32)));
GXTexObj texobj;
GXColor background = {0, 0, 0, 0xff};
static Mtx projectionMatrix,modelViewMatrix;
void CheesyScale( unsigned char *XBuf );
int whichfb = 0;
extern int font_height;
int copynow = GX_FALSE;

extern int font_width;

int GetTextWidth( char *text )
{
        unsigned int i, w = 0;

        for ( i = 0; i < strlen(text); i++ )
                w += font_width;

        return w;
}

int CentreTextPosition( char *text )
{
        return ( ( 640 - GetTextWidth(text) ) >> 1 );
}

void WriteCentre( int y, char *text )
{
        write_font( CentreTextPosition(text), y, text);
}

void WaitPrompt( char *msg )
{
        int quit = 0;

        while ( PAD_ButtonsDown(0) & PAD_BUTTON_A ) {} ;

        while( !(PAD_ButtonsDown(0) & PAD_BUTTON_A ) && (quit == 0 ))
        {
		ClearScreen();

                WriteCentre( 220, msg);
                WriteCentre( 220 + font_height, "Press A to Continue");

                if ( PAD_ButtonsDown(0) & PAD_BUTTON_A )
                        quit = 1;

                SetScreen();
        }
}

/**
 * Wait for user to press A or B. Returns 0 = B; 1 = A
 */
int WaitButtonAB ()
{
    int btns;
  
    while ( (PAD_ButtonsDown (0) & (PAD_BUTTON_A | PAD_BUTTON_B)) );
  
    while ( TRUE )
    {
        btns = PAD_ButtonsDown (0);
        if ( btns & PAD_BUTTON_A )
            return 1;
        else if ( btns & PAD_BUTTON_B )
            return 0;
    }
}

/**
 * Show a prompt with choice of two options. Returns 1 if A button was pressed
   and 0 if B button was pressed.
 */
int WaitPromptChoice (char *msg, char *bmsg, char *amsg)
{
  char choiceOption[80];  
  sprintf (choiceOption, "B = %s   :   A = %s", bmsg, amsg);

  ClearScreen ();  
  WriteCentre(220, msg);
  WriteCentre(220 + font_height, choiceOption);  
  SetScreen ();
  
  return WaitButtonAB ();
}

void ShowAction( char *msg )
{
        memcpy (xfb[whichfb], &backdrop, 1280 * 480);
        /*ClearScreen();*/
        WriteCentre( 220 + ( font_height >> 1), msg);
        SetScreen();
}

/****************************************************************************
 * GX Chip Copy to XFB
 ****************************************************************************/
static void copy_to_xfb()
{
        if ( copynow == GX_TRUE ) {
                GX_CopyDisp(xfb[whichfb],GX_TRUE);
                GX_Flush();
                copynow = GX_FALSE;
        }
}

/****************************************************************************
 * Initialise the GX
 ****************************************************************************/

void StartGX()
{
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
void GXDraw( unsigned char *XBuf )
{

        float gs = 1.0;
        float gt = 1.0;
	int width, height,t,xb;
	unsigned short *texture;

        memset(&texturemem, 0, TEX_WIDTH * TEX_HEIGHT * 2);
	texture = ( unsigned short *)&texturemem[ 16 * TEX_WIDTH ];

	/*** Now draw the texture ***/
	t = 0;
	for ( height = 0; height < 120; height++ )
	{
		xb = height * 512;
		for( width = 256; width > 0; width -= 4 )
		{
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
 * initDisplay
 *
 * It should be noted that this function forces the system to use a
 * 640x480 viewport for either NTSC or PAL.
 *
 * Helps keep the rendering at 2x sweet
 ****************************************************************************/
void initDisplay()
{
	
	/*** Start VIDEO Subsystem ***/
	VIDEO_Init();

	/*** Determine display mode
             NOTE: Force 60Hz 640x480 for PAL or NTSC ***/

/*	switch(VIDEO_GetCurrentTvMode())
	{
		case VI_NTSC:
			vmode = &TVNtsc480IntDf;
			break;
		case VI_PAL:
		case VI_MPAL:
			vmode = &TVMpal480IntDf;
			break;
		default:
			vmode = &TVNtsc480IntDf;
			break;
	}*/
	//vmode = &TVPal528IntDf;
	
	// works for NTSC and PAL on GC and Wii :)
	vmode = &TVNtsc480IntDf;

	xfb[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(vmode));
        xfb[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(vmode));
	/*init_font();*/

	VIDEO_Configure(vmode);
	VIDEO_SetNextFramebuffer(xfb[0]);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();

	if(vmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
	VIDEO_SetPostRetraceCallback(PAD_ScanPads);
	/*** Setup a console - guard against spurious printf ***/
	VIDEO_SetPreRetraceCallback(copy_to_xfb);
	VIDEO_SetNextFramebuffer(xfb[0]);

	PAD_Init();
	StartGX();
	DVD_Init();
}

/****************************************************************************
 * RenderFrame
 *
 * Render a single frame at 2x zoom
 ****************************************************************************/
#define NESWIDTH 256
#define NESHEIGHT 240
void RenderFrame( char *XBuf, int style )
{

	int gcdispOffset = 32;	/*** Offset to centre on screen ***/
	int w,h;
	int c,i;

	whichfb ^= 1;

	switch( style ) {
		
		case 0 :
			VIDEO_ClearFrameBuffer( vmode, xfb[whichfb], COLOR_BLACK);

			/*** Simply go through each row ***/
			for( h = 0; h < NESHEIGHT; h++ )
			{
				for( w = 0; w < NESWIDTH; w++ )
				{
					c = ( h << 8 ) + w;
					i = gcdispOffset + w;
					/*** Fast Zoom - Repeat each row, use 1 Xbuf == 2 GC
                             		To speed up more, use indexed palette array ***/

					xfb[whichfb][i] = gcpalette[ (unsigned char)XBuf[ c ] ];
					xfb[whichfb][i + 320] = gcpalette[ (unsigned char)XBuf[ c ] ];
				}
				gcdispOffset += 640;
			}
			break;

		case 1:
			CheesyScale( XBuf );
			break;

		case 2:
			GXDraw( XBuf );
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

unsigned int rgbcolor( unsigned char r1, unsigned char g1, unsigned char b1,
		   unsigned char r2, unsigned char g2, unsigned char b2)
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

	return ( (y1 << 24) | (cb << 16) | (y2 << 8) | cr );
}

/****************************************************************************
 * SetPalette
 *
 * A shadow copy of the palette is maintained, in case the NES Emu kernel
 * requests a copy.
 ****************************************************************************/
void FCEUD_SetPalette(unsigned char index, unsigned char r, unsigned char g, unsigned char b)
{
	/*** Make PC compatible copy ***/
	pcpalette[index].r = r;
	pcpalette[index].g = g;
	pcpalette[index].b = b;

	/*** Generate Gamecube palette ***/
	gcpalette[index] = rgbcolor(r,g,b,r,g,b);

	/*** Generate RGB565 texture palette ***/
	rgb565[index] = ( ( r & 0xf8 ) << 8 ) |
			( ( g & 0xfc ) << 3 ) |
			( ( b & 0xf8 ) >> 3 );
}

/****************************************************************************
 * GetPalette
 ****************************************************************************/
void FCEUD_GetPalette(unsigned char i, unsigned char *r, unsigned char *g, unsigned char *b)
{
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
void CheesyScale( unsigned char *XBuf )
{

	static int newrow[320];		/*** New cheesy row ***/
	unsigned int cheesypal[256];	/*** Enhanced Cheesy Palette ***/
	int i,j,c,p = 0;
	unsigned char p1,p2;
	unsigned int ofs, gcdispOffset = 0;
	int h, n, nw;

	/*** Stretch ***/
	for ( h = 0; h < NESHEIGHT; h++ )
	{
		j = c = p = 0;
		for ( i = 0; i < NESWIDTH; i++ )
		{

			/*** Every fifth pixel is stretched by adding 
		     	the mid colour range ***/
			n = ( h << 8 ) + i;
			newrow[j++] = XBuf[ n ];
			c++;
			if ( c == 4 )
			{	/*** Done 4 pixels, so add the fifth ***/
				p1 = XBuf[n];
				p2 = XBuf[n+1];
				cheesypal[p] = rgbcolor( pcpalette[p1].r, pcpalette[p1].g, pcpalette[p1].b,
						 pcpalette[p2].r, pcpalette[p2].g, pcpalette[p2].b );
				newrow[j++] = 0x8000 + p;
				p++;
				c = 0;
			}
		}


		/*** Now update the screen display with the new colours ***/
		ofs = gcdispOffset;
		for ( nw = 0; nw < 320; nw++ )
		{
			if ( newrow[nw] & 0x8000 ) {
				xfb[whichfb][ofs + nw] = cheesypal[newrow[nw] & 0xff ];
				xfb[whichfb][ofs + 320 + nw] = cheesypal[newrow[nw] & 0xff];
			}
			else
			{
				xfb[whichfb][ofs + nw] = gcpalette[ newrow[nw] ]; 
				xfb[whichfb][ofs + nw + 320] = gcpalette[ newrow[nw] ];
			}
		}

		gcdispOffset += 640;

	}

}

