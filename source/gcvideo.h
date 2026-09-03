/****************************************************************************
 * FCE Ultra
 * Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2023
 *
 * gcvideo.h
 *
 * Video rendering
 ****************************************************************************/

#ifndef _GCVIDEO_H_
#define _GCVIDEO_H_

#include "libgui/Gui.h"

#define IMAGE_BUFFER_SIZE (640 * 480 * 4)
#define IMAGE_DECODE_SCRATCH_SIZE (IMAGE_BUFFER_SIZE + (480 * sizeof(void*)))
#define PNG_FILE_BUFFER_SIZE (512 * 1024)

#define NES_WIDTH 256
#define NES_HEIGHT 240

// color palettes
#define MAXPAL 12

struct st_palettes {
    char name[32], desc[32];
    unsigned int data[64];
};

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} pcpal;

extern pcpal pcpalette[256];
extern unsigned short rgb555[256];

void setFrameTimer();
void SyncSpeed();
void SetPalette();
void ClearScreenshot();
void TakeScreenshot();
void Check3D();

typedef struct
{
	u8 * buffer;
	int size;
	int width;
	int height;
	float scaleX;
	float scaleY;
	int xoffset;
	int yoffset;
} GameScreenPng;

extern GameScreenPng gameScreenPng;

extern struct st_palettes palettes[];
extern int FDSSwitchRequested;
extern bool shutter_3d_mode;
extern bool AnaglyphPaletteValid;
extern bool anaglyph_3d_mode;
extern bool eye_3d;

#endif
