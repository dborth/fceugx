/****************************************************************************
 * Intro and Credits
 *
 * Just own up to who did what 
 ****************************************************************************/

#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../iplfont/iplfont.h"

#define MARGIN 0
//#define PSOSDLOADID 0x7c6000a6

#define JOY_UP  		0x10
#define JOY_DOWN        0x20

static int currpal = 0;
static int timing = 0;

static int FSDisable = 1;

static int slimit = 1;

extern int scrollerx;

extern void FCEUI_DisableSpriteLimitation( int a );

extern void FCEUD_SetPalette(unsigned char index, unsigned char r, unsigned char g, unsigned char b);
extern void FCEU_ResetPalette(void);
extern void WaitPrompt( char *text );
//extern void MCManage( int mode );
extern void ManageState(int mode, int slot, int device);

extern void StartGX();

extern signed int CARDSLOT;
extern int PADCAL;
extern int PADTUR;

extern void scroller(int y, unsigned char text[][512], int nlines);

extern void FCEUI_DisableFourScore(int a);

extern int line;

extern char backdrop[640 * 480 * 2];

extern int UseSDCARD;

extern unsigned char DecodeJoy( unsigned short pp );
extern unsigned char GetAnalog(int Joy);

#define MAXPAL 12

#define SCROLLY 395
#define SOFTRESET_ADR ((volatile u32*)0xCC003024)

/* color palettes */
struct
{
    char *name, *desc;
    unsigned int data[64];
}
palettes[] =
{
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

extern int whichfb;
extern unsigned int *xfb[2];
extern GXRModeObj *vmode; 
extern int font_size[256];
extern int font_height;
extern int screenscaler;
/*extern long blit_lookup[4];
  extern long blit_lookup_inv[4];*/

/****************************************************************************
 * SetScreen
 ****************************************************************************/

void SetScreen()
{

    VIDEO_SetNextFramebuffer( xfb[whichfb] );
    VIDEO_Flush();
    VIDEO_WaitVSync();

}

void ClearScreen()
{
    whichfb ^= 1;
    /*VIDEO_ClearFrameBuffer(vmode, xfb[whichfb], 0x258e2573);*/
    memcpy (xfb[whichfb], &backdrop, 1280 * 480);
}

/***************************************************************************
 * Configuration Menu
 *
 * Called when the user presses the Z Trigger on Pad 0
 * 
 * Options
 * 
 * 	1. Emulator Options
 *		1.1 Select Screen Size
 *		1.2 Select Palette
 *		1.3 Timing
 *		1.4 Reset NES
 *
 *	2. Save Game Manager
 *		1.1 Save Current State
 *		1.2 Load Existing State
 *		1.3 Delete Save Game
 *
 *	3. Game Information
 *		3.1 Show Game Info
 *
 *	4. Load ROM
 *		4.1 Load ROM from DVD-R
 ***************************************************************************/

void DrawMenu(char items[][30], int maxitems, int select)
{
    int i,w,p,h;

    ClearScreen();

    /*** Draw Title Centred ***/

    p = (480 - (maxitems * font_height)) / 2 + 10;

    for( i = 0; i < maxitems; i++ )
    {
        w = CentreTextPosition(items[i]);
        h = GetTextWidth(items[i]);

        /*		if ( i == select )
                        writex( w, p, h, font_height, items[i], blit_lookup_inv );
                        else
                        writex( w, p, h, font_height, items[i], blit_lookup );*/

        writex( w, p, h, font_height, items[i], i == select );

        p += font_height;
    }

    SetScreen();
}

/****************************************************************************
 * PADMap
 *
 * Remap a pad to the correct key
 ****************************************************************************/
extern unsigned short gcpadmap[12];
char PADMap( int padvalue, int padnum )
{
    char padkey;

    switch( padvalue )
    {
        case 0: gcpadmap[padnum] = PAD_BUTTON_A; padkey = 'A'; break;
        case 1: gcpadmap[padnum] = PAD_BUTTON_B; padkey = 'B'; break;
        case 2: gcpadmap[padnum] = PAD_BUTTON_X; padkey = 'X'; break;
        case 3: gcpadmap[padnum] = PAD_BUTTON_Y; padkey = 'Y'; break;
        case 4: gcpadmap[padnum] = PAD_BUTTON_START; padkey = 'S'; break;
        case 5: gcpadmap[padnum] = PAD_TRIGGER_Z; padkey = 'Z'; break;
    }

    return padkey;
}

/****************************************************************************
 * PAD Configuration
 *
 * This screen simply let's the user swap A/B/X/Y around.
 ****************************************************************************/

int configpadcount = 10;
char padmenu[10][30] = { 
    { "NES BUTTON A - A" }, { "    BUTTON B - B" }, { "    START    - S" }, 
    { "    SELECT   - Z" }, { "    TURBO A  - X" }, { "    TURBO B  - Y" },
    { "    FOUR SCORE - OFF" }, { "  ANALOG CLIP - 40"}, { " TURBO SPEED - 30.00 pps" }, 
    { "Return to previous" } 
};

unsigned char text[][512] = {
    //PAD Configuration
    { "Configure Your Gamepad" },
    { "Up, up, down, down, left, right, left, right, B, A, start" }
};


char mpads[2];

int PADCON = 0;

void ConfigPAD()
{

    int menu = 0;
    int quit = 0;
    short j;
    int redraw = 1;
    int i;

    for ( i = 0; i < 6; i++ )
    {
        mpads[i] = padmenu[i][15] == 'A' ? 0 :
            padmenu[i][15] == 'B' ? 1 :
            padmenu[i][15] == 'X' ? 2 :
            padmenu[i][15] == 'Y' ? 3 :
            padmenu[i][15] == 'S' ? 4 :
            padmenu[i][15] == 'Z' ? 5 : 0;
    }

    line = 0;
    scrollerx = 320 - MARGIN;

    while ( quit == 0 )
    {
        if ( redraw ) DrawMenu(&padmenu[0], configpadcount, menu);

        redraw = 0;
        j = PAD_ButtonsDown(0);

        if ( j & PAD_BUTTON_DOWN ) {
            menu++;
            redraw = 1;
        }

        if ( j & PAD_BUTTON_UP ) {
            menu--;
            redraw = 1;
        }

        if ( j & PAD_BUTTON_A ) {
            redraw = 1;
            switch( menu ) {

                case 0: i = 0; break;

                case 1: i = 1; break;

                case 2: i = 2; break;

                case 3: i = 3; break;

                case 4: i = 4; break;

                case 5: i = 5; break;

                case 6: i = -1;
                        FSDisable ^= 1;
                        if ( FSDisable )
                            strcpy(padmenu[6], "    FOUR SCORE - OFF");
                        else
                            strcpy(padmenu[6], "    FOUR SCORE - ON ");

                        FCEUI_DisableFourScore(FSDisable);
                        break;

                case 7: i = -1;
                        PADCAL += 5;
                        if ( PADCAL > 90 )
                            PADCAL = 40;

                        sprintf(padmenu[7],"  ANALOG CLIP - %d", PADCAL);
                        break;

                case 8: i = -1;
                        PADTUR += 1;
                        if ( PADTUR > 10 ) PADTUR += 4;
                        if ( PADTUR > 30 )
                            PADTUR = 2;

                        sprintf(padmenu[8]," TURBO SPEED - %.2f pps", (float)60/PADTUR);
                        break;

                case 9: quit=1; return; break;
                default: break;
            }

            if ( i >= 0 ) {
                mpads[i]++;
                if ( mpads[i] == 6 ) mpads[i] = 0;

                padmenu[i][15] = PADMap( mpads[i], i );
            }
        }

        if ( j & PAD_BUTTON_B ) {
            quit=1; return;
        }

        if ( menu < 0 ) menu = configpadcount - 1;

        if ( menu == configpadcount ) menu = 0;

        scroller(SCROLLY, &text[0], 2);
        VIDEO_WaitVSync();
    }

    return;
}

/****************************************************************************
 * Save Game Manager
 ****************************************************************************/

int mccount = 5;
char mcmenu[5][30] = { 
    { "Use: SLOT A" }, { "Device:  MCARD" },
    { "Save Game State" }, { "Load Game State" },
    { "Return to Main Menu" } 
};

unsigned char sgmtext[][512] = {
    //Save game
    { "From where do you wish to load/save your game?" },
    { "Hard time making up your mind?" }
};

int slot = 0;
int device = 0;

int StateManager()
{

    int menu = 0;
    int quit = 0;
    short j;
    int redraw = 1;
    //int i;

    line = 0;
    scrollerx = 320 - MARGIN;

    while ( quit == 0 )
    {

        if ( redraw ){
            sprintf(mcmenu[0], (slot == 0) ? "Use: SLOT A" : "Use: SLOT B");
            sprintf(mcmenu[1], (device == 0) ? "Device:  MCARD" : "Device: SDCARD");
            DrawMenu(&mcmenu[0], mccount, menu);
        } 

        redraw = 0;

        j = PAD_ButtonsDown(0);

        if ( j & PAD_BUTTON_DOWN ) {
            menu++;
            redraw = 1;
        }

        if ( j & PAD_BUTTON_UP ) {
            menu--;
            redraw = 1;
        }

        if ( j & PAD_BUTTON_A ) {
            redraw = 1;
            switch( menu ) {
                case 0 :
                    slot ^= 1;
                    break;
                case 1 : 
                    device ^= 1;
                    break;
                case 2 : 
                    ManageState(0, slot, device); //Save
                    break;
                case 3 :
                    ManageState(1, slot, device); //Load
                    break;
                case 4 : 
                    quit = 1;//return 0 ; 
                    break;
                default: 
                    break;
            }
        }

        if ( j & PAD_BUTTON_B ) quit = 1;

        if ( menu < 0 )
            menu = mccount - 1;

        if ( menu == mccount )
            menu = 0;

        scroller(SCROLLY, &sgmtext[0], 2);
        VIDEO_WaitVSync();

    }

    return 0;
}

/****************************************************************************
 * Video Enhancement Screen
 ****************************************************************************/

int vecount = 5;
char vemenu[5][30] = { 
    { "Screen Scaler - GX" }, { "Palette - Default" }, 
    { "8 Sprite Limit - ON " },{ "Timing - NTSC" }, 
    { "Return to Main Menu" } 
};

unsigned char vestext[][512] = {
    //Screen Configurator
    { "Wow, these colors and shapes sure are beautiful, brings back the memories." },
    { "Be sure not to mess these settings up, You don't want to ruin the experience! :D" }
};

int VideoEnhancements()
{	
    int menu = 0;
    int quit = 0;
    short j;
    int redraw = 1;
    int i;
    unsigned char r,g,b;

    line = 0;
    scrollerx = 320 - MARGIN;

    while ( quit == 0)
    {
        if ( redraw )
            DrawMenu(&vemenu[0], vecount, menu );

        redraw = 0;

        j = PAD_ButtonsDown(0);

        if ( j & PAD_BUTTON_DOWN ) {
            menu++;
            redraw = 1;
        }

        if ( j & PAD_BUTTON_UP ) {
            menu--;
            redraw = 1;
        }

        if ( j & PAD_BUTTON_A ) {
            redraw = 1;
            switch ( menu ) {
                case 0:	/*** Scaler ***/
                    screenscaler++;
                    screenscaler = ( screenscaler > 2 ) ? 0 : screenscaler;

                    switch ( screenscaler )
                    {
                        case 2: strcpy(vemenu[0], "Screen Scaler - GX"); break;
                        case 1: strcpy(vemenu[0], "Screen Scaler - Cheesy"); break;
                        case 0:	strcpy(vemenu[0], "Screen Scaler - 2x"); break;
                    }
                    break;
                case 1: /*** Palette ***/
                    currpal++;
                    if ( currpal > MAXPAL )
                        currpal = 0;

                    if ( currpal == 0 ) {
                        strcpy(vemenu[1],"Palette - Default");
                        /*** Do palette reset ***/
                        FCEU_ResetPalette();
                    } else {
                        strcpy(vemenu[1],"Palette - ");
                        strcat(vemenu[1], palettes[currpal-1].name);

                        /*** Now setup this palette ***/
                        for ( i = 0; i < 64; i++ )
                        {
                            r = palettes[currpal-1].data[i] >> 16;
                            g = ( palettes[currpal-1].data[i] & 0xff00 ) >> 8;
                            b = ( palettes[currpal-1].data[i] & 0xff );
                            FCEUD_SetPalette( i, r, g, b);
                            FCEUD_SetPalette( i+64, r, g, b);
                            FCEUD_SetPalette( i+128, r, g, b);
                            FCEUD_SetPalette( i+192, r, g, b);

                        }
                    }

                    break;

                case 2: slimit ^=1;
                        if ( slimit )
                            strcpy(vemenu[2], "8 Sprite Limit - ON ");
                        else
                            strcpy(vemenu[2], "8 Sprite Limit - OFF");
                        FCEUI_DisableSpriteLimitation( slimit );
                        break;

                case 3: timing ^= 1;
                        if ( timing )
                            strcpy(vemenu[3], "Timing - PAL ");
                        else
                            strcpy(vemenu[3], "Timing - NTSC");

                        FCEUI_SetVidSystem( timing );

                        break;

                case 4: quit = 1; break;

                default: break;

            }
        }

        if ( j & PAD_BUTTON_B ) quit = 1;
        if ( menu < 0 )
            menu = vecount - 1;

        if ( menu == vecount )
            menu = 0;

        scroller(SCROLLY, &vestext[0], 2);	
        VIDEO_WaitVSync();

    }

    return 0;

}

/****************************************************************************
 * ROM Information
 ****************************************************************************/

typedef struct {
    char ID[4]; /*NES^Z*/
    u8 ROM_size;
    u8 VROM_size;
    u8 ROM_type;
    u8 ROM_type2;
    u8 reserve[8];
} iNES_HEADER;

extern int MapperNo;
extern iNES_HEADER head;
extern u32 ROM_size;
extern u32 VROM_size;
extern u32 iNESGameCRC32;
extern u8 iNESMirroring;

void ShowROMInfo()
{

    int i,p;
    char *title = "ROM Information";
    char info[128];

    ClearScreen();

    p = (480 - (7 * font_height)) / 2 + 5;

    write_font( CentreTextPosition( title ), p, title );
    p += ( font_height << 1 );

    for ( i = 0; i < 5; i++ )
    {
        switch (i) {
            case 0: sprintf(info, "ROM Size : %d", head.ROM_size ); break;
            case 1: sprintf(info, "VROM Size : %d", head.VROM_size ); break;
            case 2: sprintf(info, "iNES CRC : %08x", iNESGameCRC32 ); break;
            case 3: sprintf(info, "Mapper : %d", MapperNo ); break;
            case 4: sprintf(info, "Mirroring : %d", iNESMirroring );break;
        }

        write_font( CentreTextPosition( info ), p, info );
        p+=font_height;

    }

    SetScreen();

    while ( !(PAD_ButtonsDown(0) & (PAD_BUTTON_A | PAD_BUTTON_B)) )
    { VIDEO_WaitVSync(); }

}

/****************************************************************************
 * Media Select Screen
 ****************************************************************************/

int mediacount = 3;
char mediamenu[3][30] = { 
    { "Load from DVD" }, { "Load from SDCARD"}, 
    //{ "Rom loading in SDCARD: SLOT A" }, 
    { "Return to previous" } 
};

unsigned char msstext[][512] = {
    //Screen Configurator
    { "What are You waiting for? Load some games!" },
    { "Still here?" },
    { "How can You wait this long?! The games are waiting for You!!" }
};

//int choosenSDSlot = 0;

int MediaSelect()
{
    int menu = 0;
    int quit = 0;
    short j;
    int redraw = 1;

    line = 0;
    scrollerx = 320 - MARGIN;

    while ( quit == 0 )
    {
        if ( redraw )
            DrawMenu(&mediamenu[0], mediacount, menu );

        redraw = 0;

        j = PAD_ButtonsDown(0);

        if ( j & PAD_BUTTON_DOWN ) {
            menu++;
            redraw = 1;
        }

        if ( j & PAD_BUTTON_UP ) {
            menu--;
            redraw = 1;
        }

        if ( j & PAD_BUTTON_A ) {
            redraw = 1;
            switch ( menu ) {
                case 0:	UseSDCARD = 0; //DVD
                        OpenDVD();
                        return 1;
                        break;

                case 1:	UseSDCARD = 1; //SDCard
                        OpenSD();
                        return 1;
                        break;
                        /*case 2:	choosenSDSlot ^= 1; //Pick Slot
                          sprintf(mediamenu[2], (!choosenSDSlot) ? "Rom loading in SDCARD: SLOT A" : "Rom loading in SDCARD: SLOT B");						
                          break;*/
                case 2: quit = 1; //Previous
                        break;

                default: break ;
            }
        }

        if ( j & PAD_BUTTON_B )
            quit = 1;

        if ( menu == mediacount  )
            menu = 0;		

        if ( menu < 0 )
            menu = mediacount - 1;

        scroller(SCROLLY, &msstext[0], 3);
        VIDEO_WaitVSync();

    }

    return 0;
}

/****************************************************************************
 * Credits screen
 *****************************************************************************/
char credits[12][512] = {
    //{ "Technical" },
    { "Gamecube port by softdev" },
    { "Original FCE by BERO" },
    { "FCE Ultra by Xodnizel" },
    { "DevkitPPC/libogc by wntrmute and shagkur" },
    { "IPLFont by Qoob" },
    { "DVD Codes Courtesy of Ninjamod" },
    { "Zlib by Jean-loup Gailly" },
    { "Misc. addons by KruLLo" },
    { "Extras features Askot" },
    { "Thank you to" },
    { "brakken, mithos, luciddream, HonkeyKong," },
    { "dsbomb for bringing it to the Wii" },
};

void ShowCredits(){

    int i,p;
    char *title = "Credits";
    char info[128];

    ClearScreen();

    //p = (480 - (7 * font_height)) / 2 + 5; //150
    p = 105;

    write_font( CentreTextPosition( title ), p, title );
    //p += ( font_height << 1 );

    p = 133;
    for ( i = 0; i < 12; i++ )
    {
        sprintf(info, credits[i]);
        write_font( CentreTextPosition( info ), p, info );
        p+=24;
        //p+=font_height;
    }

    SetScreen();

    while ( !(PAD_ButtonsDown(0) & (PAD_BUTTON_A | PAD_BUTTON_B)) )
    { VIDEO_WaitVSync(); }

}
/****************************************************************************
 * Configuration Screen
 ****************************************************************************/
int configmenucount = 11;
char configmenu[11][30] = { 
    { "Play Game" }, 
    { "Reset NES" }, 
    { "Load New Game" }, 
    { "State Manager" }, 
    { "ROM Information" }, 
    { "Configure Joypads" }, 
    { "Video Options" }, 
    { "Stop DVD Motor" }, 
    { "PSO/SD Reload" } ,
    { "Reboot Gamecube" },	
    { "Credits" } 
};

unsigned char cstext[][512] = {
    //ConfigScreen
    { "FCE Ultra GameCube Edition - Version 1.0.9 \"SUPER-DELUXE\" ;)" },
    { "Press L + R anytime to return to this menu!" },
    { "Press START + B + X anytime for PSO/SD-reload" },
    { "* * *" },
    { "FCE Ultra GC is a modified port of the FCE Ultra 0.98.12 Nintendo Entertainment system for x86 (Windows/Linux) PC's. In English you can play NES games on your GameCube using either a softmod and/or modchip from a DVD or via a networked connection to your PC." },
    { "Disclaimer - Use at your own RISK!" },
    { "Official Homepage: http://www.tehskeen.net" }
};

int ConfigScreen()
{
    int menu = 0;
    int quit = 0;
    short j;
    int redraw = 1;

    //int *psoid = (int *) 0x80001800;
    void (*PSOReload) () = (void (*)()) 0x80001800;

    /*** Stop any running Audio ***/
    AUDIO_StopDMA();

    line = 0;
    scrollerx = 320 - MARGIN;

    while ( quit == 0 )
    {
        if ( redraw )
            /*DrawMenu("FCEU GC Configuration", &configmenu[0], configmenucount, menu );*/
            DrawMenu(&configmenu[0], configmenucount, menu );

        redraw = 0;

        //while (menu == 9 && !(PAD_ButtonsDown(0) & PAD_BUTTON_UP) && !(PAD_ButtonsDown(0) & PAD_BUTTON_DOWN))
        //	{ scroller(SCROLLY, &credits[0], 12);VIDEO_WaitVSync(); }

        /*if (menu == 9) {
          scrollerx = 320 - MARGIN;
          line = 0;
          }*/

        j = PAD_ButtonsDown(0);

        if (j & PAD_BUTTON_DOWN) {
            menu++;
            redraw = 1;
            /*if (menu == 9) {
              scrollerx = 320 - MARGIN;
              line = 0;
              }*/
        }

        if (j & PAD_BUTTON_UP) {
            menu--;
            redraw = 1;
            if (menu < 0) {
                scrollerx = 320 - MARGIN;
                line = 0;
            }
        }

        if (j & PAD_BUTTON_A ) {
            redraw = 1;
            switch ( menu ) {
                case 0:	// Play Game		
                    quit = 1; 
                    break;

                case 1:	// Reset NES
                    ResetNES();
                    return 1;
                    break;

                case 2:	// Load new Game
                    //if (MediaSelect()) {
                    //	if (GCMemROM() >= 0) return 1;/* Fix by Garglub. Thanks! */
                    //}
                    MediaSelect();
                    scrollerx = 320 - MARGIN; 
                    break;

                case 3: // State Manager
                    if (StateManager()) return 2;
                    scrollerx = 320 - MARGIN;
                    break;

                case 4:	// Game Information
                    ShowROMInfo(); 
                    break;

                case 5: // COnfigure Joypads
                    ConfigPAD(); 
                    scrollerx = 320 - MARGIN; 
                    break;

                case 6: // Video Options
                    if (VideoEnhancements()) return 2; 
                    scrollerx = 320 - MARGIN;
                    break;

                case 7:	// Stop DVD Motor
                    ShowAction("Stopping Motor");
                    dvd_motor_off();
                    WaitPrompt("DVD Motor Stopped"); 
                    break;

                case 8:	// PSO/SD Reload
                    PSOReload ();
                    break;

                case 9: // Reboot
                    *SOFTRESET_ADR = 0x00000000;
                    break;

                case 10: // Credits 
                    ShowCredits();
                    break;

                default: break ;
            }
        }

        if ( menu == configmenucount  )
            menu = 0;		

        if ( menu < 0 )
            menu = configmenucount - 1;

        scroller(SCROLLY, &cstext[0], 7);
        VIDEO_WaitVSync();

    }

    /*** Remove any still held buttons ***/
    while(PAD_ButtonsHeld(0)) VIDEO_WaitVSync();

    /*** Stop the DVD from causing clicks while playing ***/
    uselessinquiry ();

    return 0;
}

