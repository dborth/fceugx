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

// color palettes
#define MAXPAL 12

struct st_palettes {
    char name[32], desc[32];
    unsigned int data[64];
};

void InitVideo ();
void ResetVideo_Emu ();
void RenderFrame(unsigned char *XBuf);
void RenderStereoFrames(unsigned char *XBufLeft, unsigned char *XBufRight); // Stereoscopic 3D
void setFrameTimer();
void SyncSpeed();
void SetPalette();
void ResetVideo_Menu ();
void ClearScreenshot();
void TakeScreenshot();
void Menu_Render ();
void Check3D();

void* createTexture(int width, int height);
void loadTextureData(void* texture, const uint8_t* rgba, int width, int height);
void destroyTexture(void * texture);

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

extern GXRModeObj *vmode;
extern struct st_palettes palettes[];
extern int FDSSwitchRequested;
extern bool progressive;
extern u32 FrameTimer;
extern bool shutter_3d_mode;
extern bool anaglyph_3d_mode;
extern bool eye_3d;

#endif
