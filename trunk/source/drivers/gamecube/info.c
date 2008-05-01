/****************************************************************************
 * Intro and Credits
 *
 * Just own up to who did what 
 ****************************************************************************/

#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iplfont/iplfont.h"
#include "intl.h"

#define MARGIN 0

#define JOY_UP  	0x10
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

void Reboot() {
#ifdef HW_RVL
    // Thanks to hell_hibou
    SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
#else
#define SOFTRESET_ADR ((volatile u32*)0xCC003024)
    *SOFTRESET_ADR = 0x00000000;
#endif
}
    
#define MAXPAL 12
#define SCROLLY 395

/* color palettes */
struct {
    char *name, *desc;
    unsigned int data[64];
} palettes[] = {
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

/****************************************************************************
 * SetScreen
 ****************************************************************************/
void SetScreen() {
    VIDEO_SetNextFramebuffer( xfb[whichfb] );
    VIDEO_Flush();
    VIDEO_WaitVSync();
}

void ClearScreen() {
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
void DrawMenu(char *title, char items[][MENU_STRING_LENGTH], int maxitems, int select) {
    int i,w,p,h;

    ClearScreen();

    /*** Draw Title Centred ***/
    //h = (480 - ((maxitems + 3) * font_height)) / 2;
    write_font(CentreTextPosition(title), 22, title);

    p = (480 - (maxitems * font_height)) / 2 + 10;
    //p = h + (font_height << 1);

    for( i = 0; i < maxitems; i++ ) {
        w = CentreTextPosition(items[i]);
        h = GetTextWidth(items[i]);

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
extern unsigned short gcpadmap[10];
char PADMap( int padvalue, int padnum ) {
    char padkey;

    switch( padvalue ) {
        default:
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
char mpads[6] = { 0, 1, 2, 3, 4, 5 };
int PADCON = 0;

void ConfigPAD() {
    int PadMenuCount = 10;
    char PadMenu[10][MENU_STRING_LENGTH] = { 
        { MENU_CONFIG_A }, { MENU_CONFIG_B }, { MENU_CONFIG_TURBO_A },
        { MENU_CONFIG_TURBO_B }, { MENU_CONFIG_START }, { MENU_CONFIG_SELECT },
        { MENU_CONFIG_FOUR_SCORE }, { MENU_CONFIG_CLIP }, { MENU_CONFIG_SPEED },
        { MENU_EXIT }
    };
    enum PAD_MENU {
        PAD_A, PAD_B, PAD_TURBO_A, PAD_TURBO_B,
        PAD_START, PAD_SELECT, PAD_FOUR_SCORE,
        PAD_CLIP, PAD_SPEED, PAD_EXIT
    };
    unsigned char PadMenuText[][512] = {
        { MENU_CONFIG_TEXT1 }, { MENU_CONFIG_TEXT2 }
    };

    int menu = 0;
    int quit = 0;
    short j;
    int redraw = 1;
    int i = 0;

    line = 0;
    scrollerx = 320 - MARGIN;

    while ( quit == 0 ) {
        if ( redraw ) {
            sprintf(PadMenu[PAD_A], MENU_CONFIG_A " - %c", PADMap(mpads[0], 0));
            sprintf(PadMenu[PAD_B], MENU_CONFIG_B " - %c", PADMap(mpads[1], 1));
            sprintf(PadMenu[PAD_TURBO_A], MENU_CONFIG_TURBO_A " - %c", PADMap(mpads[4], 4));
            sprintf(PadMenu[PAD_TURBO_B], MENU_CONFIG_TURBO_B " - %c", PADMap(mpads[5], 5));
            sprintf(PadMenu[PAD_START], MENU_CONFIG_START " - %c", PADMap(mpads[2], 2));
            sprintf(PadMenu[PAD_SELECT], MENU_CONFIG_SELECT " - %c", PADMap(mpads[3], 3));
            sprintf(PadMenu[PAD_FOUR_SCORE], MENU_CONFIG_FOUR_SCORE " - %s", FSDisable ? "OFF" : "ON");
            sprintf(PadMenu[PAD_CLIP], MENU_CONFIG_CLIP " - %d", PADCAL);
            sprintf(PadMenu[PAD_SPEED], MENU_CONFIG_SPEED " - %.2f pps", (float)60/PADTUR);
            DrawMenu(MENU_CONFIG_TITLE, PadMenu, PadMenuCount, menu);
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
                case PAD_A: i = 0; break;
                case PAD_B: i = 1; break;
                case PAD_TURBO_A: i = 4; break;
                case PAD_TURBO_B: i = 5; break;
                case PAD_START: i = 2; break;
                case PAD_SELECT: i = 3; break;
                case PAD_FOUR_SCORE: i = -1;
                        FSDisable ^= 1;
                        FCEUI_DisableFourScore(FSDisable);
                        break;

                case PAD_CLIP: i = -1;
                        PADCAL += 5;
                        if ( PADCAL > 70 )
                            PADCAL = 30;
                        break;

                case PAD_SPEED:i = -1;
                        PADTUR += 1;
                        if ( PADTUR > 10 ) PADTUR += 4;
                        if ( PADTUR > 30 )
                            PADTUR = 2;
                        break;

                case PAD_EXIT:
                    quit = 1;
                    break;
                default: break;
            }

            if ( (quit == 0) && (i >= 0) ) {
                mpads[i]++;
                if ( mpads[i] == 6 ) mpads[i] = 0;
            }
        }

        if ( j & PAD_BUTTON_B ) {
            quit = 1;
        }

        if ( menu < 0 ) menu = PadMenuCount - 1;
        if ( menu == PadMenuCount ) menu = 0;

        scroller(SCROLLY, PadMenuText, 2);
        VIDEO_WaitVSync();
    }

    return;
}

/****************************************************************************
 * Save Game Manager
 ****************************************************************************/
int SdSlotCount = 3;
char SdSlots[3][10] = {
    { "Slot A" }, { "Slot B" }, { "Wii SD"}
};
enum SLOTS {
    SLOT_A, SLOT_B, SLOT_WIISD
};
int ChosenSlot = 0;
int ChosenDevice = 1;

int StateManager() {
    int SaveMenuCount = 5;
    char SaveMenu[5][MENU_STRING_LENGTH] = { 
        { MENU_SAVE_SAVE }, { MENU_SAVE_LOAD },
        { MENU_SAVE_DEVICE }, { "Slot" },
        { MENU_EXIT }
    };
    enum SAVE_MENU {
        SAVE_SAVE, SAVE_LOAD,
        SAVE_DEVICE, SAVE_SLOT,
        SAVE_EXIT
    };
    unsigned char SaveMenuText[][512] = {
        //Save game
        { MENU_SAVE_TEXT1 },
        { MENU_SAVE_TEXT2 }
    };


    int ChosenMenu = 0;
    int quit = 0;
    short j;
    int redraw = 1;

    line = 0;
    scrollerx = 320 - MARGIN;

    while ( quit == 0 ) {
        if ( redraw ) {
            sprintf(SaveMenu[SAVE_SLOT], "%s: %s", ChosenDevice ? "SDCard" : "MemCard",
                SdSlots[ChosenSlot]);
            sprintf(SaveMenu[SAVE_DEVICE], MENU_SAVE_DEVICE ": %s", ChosenDevice ? "SDCard" : "MemCard");
            DrawMenu(MENU_SAVE_TITLE, SaveMenu, SaveMenuCount, ChosenMenu);
            redraw = 0;
        } 

        j = PAD_ButtonsDown(0);
        if ( j & PAD_BUTTON_DOWN ) {
            ChosenMenu++;
            redraw = 1;
        }

        if ( j & PAD_BUTTON_UP ) {
            ChosenMenu--;
            redraw = 1;
        }

        if ( j & PAD_BUTTON_A ) {
            redraw = 1;
            switch( ChosenMenu ) {
                case SAVE_SAVE: 
                    ManageState(0, ChosenSlot, ChosenDevice); //Save
                    break;
                case SAVE_LOAD:
                    ManageState(1, ChosenSlot, ChosenDevice); //Load
                    break;
                case SAVE_DEVICE: 
                    ChosenDevice ^= 1;
                    break;
                case SAVE_SLOT:
                    ChosenSlot++;
                    if (ChosenSlot >= SdSlotCount)
                        ChosenSlot = 0;
                    break;
                case SAVE_EXIT: 
                    quit = 1;
                    break;
                default: 
                    break;
            }
        }

        if (j & PAD_BUTTON_RIGHT) {
            if (ChosenMenu == SAVE_SLOT) {
                ChosenSlot++;
                if (ChosenSlot >= SdSlotCount)
                    ChosenSlot = SdSlotCount - 1;
                redraw = 1;
            } else if (ChosenMenu == SAVE_DEVICE) {
                ChosenDevice ^= 1;
                redraw = 1;
            }
        }

        if (j & PAD_BUTTON_LEFT) {
            if (ChosenMenu == SAVE_SLOT) {
                ChosenSlot--;
                if (ChosenSlot < 0)
                    ChosenSlot = 0;
                redraw = 1;
            } else if (ChosenMenu == SAVE_DEVICE) {
                ChosenDevice ^= 1;
                redraw = 1;
            }
        }

        if ( j & PAD_BUTTON_B ) quit = 1;

        if ( ChosenMenu < 0 )
            ChosenMenu = SaveMenuCount - 1;

        if ( ChosenMenu == SaveMenuCount )
            ChosenMenu = 0;

        scroller(SCROLLY, SaveMenuText, 2);
        VIDEO_WaitVSync();

    }

    return 0;
}

/****************************************************************************
 * Video Enhancement Screen
 ****************************************************************************/
int VideoEnhancements() {	
    int VideoMenuCount = 5;
    char VideoMenu[5][MENU_STRING_LENGTH] = { 
        { MENU_VIDEO_SCALER }, { MENU_VIDEO_PALETTE },
        { MENU_VIDEO_SPRITE }, { MENU_VIDEO_TIMING },
        { MENU_EXIT }
    };
    enum VIDEO_MENU {
        VIDEO_SCALER, VIDEO_PALETTE,
        VIDEO_SPRITE, VIDEO_TIMING,
        VIDEO_EXIT
    };
    unsigned char VideoMenuText[][512] = {
        //Screen Configurator
        { MENU_VIDEO_TEXT1 },
        { MENU_VIDEO_TEXT2 }
    };

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
        if ( redraw ) {
            sprintf(VideoMenu[VIDEO_SCALER], MENU_VIDEO_SCALER " - %s",
                (screenscaler == 0) ? "2x" : (screenscaler == 1) ? "Cheesy" : "GX");
            sprintf(VideoMenu[VIDEO_PALETTE], MENU_VIDEO_PALETTE " - %s",
                currpal ? palettes[currpal-1].name : MENU_VIDEO_DEFAULT);
            sprintf(VideoMenu[VIDEO_SPRITE], MENU_VIDEO_SPRITE " - %s", slimit ? MENU_ON : MENU_OFF);
            sprintf(VideoMenu[VIDEO_TIMING], MENU_VIDEO_TIMING " - %s", timing ? "PAL " : "NTSC");
            DrawMenu(MENU_VIDEO_TITLE, VideoMenu, VideoMenuCount, menu );
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
            switch ( menu ) {
                case VIDEO_SCALER:
                    if (++screenscaler > 2)
                        screenscaler = 0;
                    break;
                case VIDEO_PALETTE:
                    if ( ++currpal > MAXPAL )
                        currpal = 0;

                    if ( currpal == 0 ) {
                        /*** Do palette reset ***/
                        FCEU_ResetPalette();
                    } else {
                        /*** Now setup this palette ***/
                        for ( i = 0; i < 64; i++ ) {
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

                case VIDEO_SPRITE:
                        slimit ^=1;
                        FCEUI_DisableSpriteLimitation( slimit );
                        break;

                case VIDEO_TIMING:
                        timing ^= 1;
                        FCEUI_SetVidSystem( timing );
                        break;

                case VIDEO_EXIT:
                        quit = 1;
                        break;

                default: break;

            }
        }

        if ( j & PAD_BUTTON_B ) quit = 1;
        if ( menu < 0 )
            menu = VideoMenuCount - 1;

        if ( menu == VideoMenuCount )
            menu = 0;

        scroller(SCROLLY, VideoMenuText, 2);	
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

void ShowROMInfo() {
    int i,p;
    char *title = MENU_INFO_TITLE;
    char info[128];

    ClearScreen();

    p = (480 - (7 * font_height)) / 2 + 5;

    write_font( CentreTextPosition( title ), p, title );
    p += ( font_height << 1 );

    for ( i = 0; i < 5; i++ ) {
        switch (i) {
            case 0: sprintf(info, MENU_INFO_ROM " : %d", head.ROM_size ); break;
            case 1: sprintf(info, MENU_INFO_VROM " : %d", head.VROM_size ); break;
            case 2: sprintf(info, MENU_INFO_CRC " : %08x", iNESGameCRC32 ); break;
            case 3: sprintf(info, MENU_INFO_MAPPER " : %d", MapperNo ); break;
            case 4: sprintf(info, MENU_INFO_MIRROR " : %d", iNESMirroring );break;
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
int MediaSelect() {
    int MediaMenuCount = 5;
    char MediaMenu[5][MENU_STRING_LENGTH] = { 
        { MENU_MEDIA_SDCARD }, { "SDCard: Slot A" },
        { MENU_MEDIA_DVD }, { MENU_MEDIA_STOPDVD },
        { MENU_EXIT }
    };

    unsigned char MediaMenuText[][512] = {
        //Screen Configurator
        { MENU_MEDIA_TEXT1 },
        { MENU_MEDIA_TEXT2 },
        { MENU_MEDIA_TEXT3 }
    };

    enum MEDIA_MENU {
        MEDIA_SDCARD, MEDIA_SLOT,
        MEDIA_DVD, MEDIA_STOPDVD,
        MEDIA_EXIT
    };

    int ChosenMenu = 0;
    int quit = 0;
    short j;
    int redraw = 1;

    line = 0;
    scrollerx = 320 - MARGIN;

#ifdef HW_RVL
    strcpy(MediaMenu[MEDIA_DVD], MediaMenu[MEDIA_EXIT]);
    MediaMenuCount = 3;
    ChosenSlot = SLOT_WIISD; // default to WiiSD
#else
    SdSlotCount = 2;
#endif

    while ( quit == 0 ) {
        if ( redraw ) {
            sprintf(MediaMenu[MEDIA_SLOT], "SDCard: %s", SdSlots[ChosenSlot]);
            DrawMenu(MENU_MEDIA_TITLE, MediaMenu, MediaMenuCount, ChosenMenu );
            redraw = 0;
        }

        j = PAD_ButtonsDown(0);
        if ( j & PAD_BUTTON_DOWN ) {
            ChosenMenu++;
            redraw = 1;
        }

        if ( j & PAD_BUTTON_UP ) {
            ChosenMenu--;
            redraw = 1;
        }

        if ( j & PAD_BUTTON_A ) {
            redraw = 1;
            switch ( ChosenMenu ) {
                case MEDIA_SDCARD:
#ifdef HW_RVL
                        if (ChosenSlot == SLOT_WIISD) {
                            OpenWiiSD();
                        } else
#endif
                            OpenSD();
                        return 1;
                        break;
                case MEDIA_SLOT:
                        ChosenSlot++;
                        if (ChosenSlot >= SdSlotCount)
                            ChosenSlot = 0;
                        redraw = 1;
                        break;
                case MEDIA_DVD:
#ifdef HW_RVL
                        // In Wii mode, this is just exit
                        quit = 1;
#else
                        UseSDCARD = 0; //DVD
                        OpenDVD();
                        return 1;
#endif
                        break;
#ifndef HW_RVL
                case MEDIA_STOPDVD:
                        ShowAction((char*)MENU_MEDIA_STOPPING);
                        dvd_motor_off();
                        WaitPrompt((char*)MENU_MEDIA_STOPPED);
                case MEDIA_EXIT:
                        quit = 1;
                        break;
#endif

                default: break ;
            }
        }

        if ( (j & PAD_BUTTON_RIGHT) && (ChosenMenu == MEDIA_SLOT) ) {
            ChosenSlot++;
            if (ChosenSlot >= SdSlotCount)
                ChosenSlot = SdSlotCount - 1;
            redraw = 1;
        }

        if ( (j & PAD_BUTTON_LEFT) && (ChosenMenu == MEDIA_SLOT) ) {
            ChosenSlot--;
            if (ChosenSlot < 0)
                ChosenSlot = 0;
            redraw = 1;
        }

        if ( j & PAD_BUTTON_B )
            quit = 1;

        if ( ChosenMenu == MediaMenuCount  )
            ChosenMenu = 0;		

        if ( ChosenMenu < 0 )
            ChosenMenu = MediaMenuCount - 1;

        scroller(SCROLLY, &MediaMenuText[0], 3);
        VIDEO_WaitVSync();
    }

    return 0;
}

/****************************************************************************
 * Credits screen
 *****************************************************************************/
void ShowCredits(){
    char CreditsText[12][512] = {
        //{ "Technical" },
        { MENU_CREDITS_GCPORT " " MENU_CREDITS_BY " softdev" },
        { MENU_CREDITS_ORIG " " MENU_CREDITS_BY " BERO" },
        { MENU_CREDITS_FCEU " " MENU_CREDITS_BY " Xodnizel" },
        { "DevkitPPC/libogc " MENU_CREDITS_BY " wntrmute, shagkur" },
        { "IPLFont " MENU_CREDITS_BY " Qoob" },
        { MENU_CREDITS_DVD " Ninjamod" },
        { "Zlib " MENU_CREDITS_BY " Jean-loup Gailly" },
        { MENU_CREDITS_MISC " " MENU_CREDITS_BY " KruLLo" },
        { MENU_CREDITS_EXTRAS " " MENU_CREDITS_BY " Askot" },
        { MENU_CREDITS_THANK },
        { "brakken, mithos, luciddream, HonkeyKong," },
        { "dsbomb " MENU_CREDITS_WII },
    };

    int i,p;
    char *title = MENU_CREDITS_TITLE;
    char info[128];

    ClearScreen();

    //p = (480 - (7 * font_height)) / 2 + 5; //150
    //p = 105;

    write_font( CentreTextPosition( title ), 22, title );
    //p += ( font_height << 1 );

    p = 109;
    for ( i = 0; i < 12; i++ ) {
        sprintf(info, CreditsText[i]);
        write_font( CentreTextPosition( info ), p, info );
        p+=24;
        //p+=font_height;
    }

    SetScreen();

    while ( !(PAD_ButtonsDown(0) & (PAD_BUTTON_A | PAD_BUTTON_B)) )
    { VIDEO_WaitVSync(); }
}

/****************************************************************************
 * Main Menu
 ****************************************************************************/
int MainMenu() {
    int MainMenuCount = 10;
    char MainMenu[10][MENU_STRING_LENGTH] = { 
        { MENU_MAIN_PLAY },
        { MENU_MAIN_RESET },
        { MENU_MAIN_LOAD },
        { MENU_MAIN_SAVE },
        { MENU_MAIN_INFO },
        { MENU_MAIN_JOYPADS },
        { MENU_MAIN_OPTIONS },
        { MENU_MAIN_RELOAD },
        { MENU_MAIN_REBOOT },
        { MENU_MAIN_CREDITS }
    };
    enum MAIN_MENU {
        MAIN_PLAY, MAIN_RESET, MAIN_LOAD,
        MAIN_SAVE, MAIN_INFO, MAIN_JOYPADS,
        MAIN_VIDEO, MAIN_RELOAD, MAIN_REBOOT,
        MAIN_CREDITS
    };
    unsigned char MainMenuText[][512] = {
        //Main Menu
        { MENU_MAIN_TEXT1 },
        { MENU_MAIN_TEXT2 },
        { MENU_MAIN_TEXT3 },
        { "* * *" },
        { MENU_MAIN_TEXT4 },
        { MENU_MAIN_TEXT5 },
        { MENU_MAIN_TEXT6 }
    };

    int menu = 0;
    int quit = 0;
    short j;
    int redraw = 1;

#ifdef HW_RVL
    void (*PSOReload)() = (void(*)())0x90000020;
#else
    void (*PSOReload)() = (void(*)())0x80001800;
#endif

    /*** Stop any running Audio ***/
    AUDIO_StopDMA();

    line = 0;
    scrollerx = 320 - MARGIN;

    while ( quit == 0 ) {
        if ( redraw )
            DrawMenu(MENU_CREDITS_TITLE, MainMenu, MainMenuCount, menu );

        redraw = 0;

        j = PAD_ButtonsDown(0);
        if (j & PAD_BUTTON_DOWN) {
            menu++;
            redraw = 1;
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
                case MAIN_PLAY:
                    quit = 1; 
                    break;

                case MAIN_RESET:
                    ResetNES();
                    quit = 1;
                    break;

                case MAIN_LOAD:
                    if (MediaSelect()) {
                    	if (GCMemROM() >= 0)
                            return 1; /* Fix by Garglub. Thanks! */
                    }
                    scrollerx = 320 - MARGIN; 
                    break;

                case MAIN_SAVE:
                    if (StateManager()) return 2;
                    scrollerx = 320 - MARGIN;
                    break;

                case MAIN_INFO:
                    ShowROMInfo(); 
                    break;

                case MAIN_JOYPADS:
                    ConfigPAD(); 
                    scrollerx = 320 - MARGIN; 
                    break;

                case MAIN_VIDEO:
                    if (VideoEnhancements()) return 2; 
                    scrollerx = 320 - MARGIN;
                    break;

                case MAIN_RELOAD:
                    PSOReload();
                    break;

                case MAIN_REBOOT:
                    Reboot();
                    break;

                case MAIN_CREDITS:
                    ShowCredits();
                    break;

                default: break ;
            }
        }

        if (j & PAD_BUTTON_B ) {
            quit = 1;
        }
        if ( menu == MainMenuCount  )
            menu = 0;		

        if ( menu < 0 )
            menu = MainMenuCount - 1;

        scroller(SCROLLY, MainMenuText, 7);
        VIDEO_WaitVSync();

    }

    /*** Remove any still held buttons ***/
    while(PAD_ButtonsHeld(0)) VIDEO_WaitVSync();

    /*** Stop the DVD from causing clicks while playing ***/
    uselessinquiry ();

    return 0;
}

