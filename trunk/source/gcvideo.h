/****************************************************************************
 * FCE Ultra
 * Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2009
 *
 * gcvideo.h
 *
 * Video rendering
 ****************************************************************************/

#ifndef _GCVIDEO_H_
#define _GCVIDEO_H_

// color palettes
#define MAXPAL 13

struct st_palettes {
    char name[32], desc[32];
    unsigned int data[64];
};

void InitGCVideo ();
void StopGX();
void ResetVideo_Emu ();
void RenderFrame(unsigned char *XBuf);
void RenderStereoFrames(unsigned char *XBufLeft, unsigned char *XBufRight); //CAK: Stereoscopic 3D
void setFrameTimer();
void SyncSpeed();
void SetPalette();
void ResetVideo_Menu ();
void TakeScreenshot();
void Menu_Render ();
void Menu_DrawImg(f32 xpos, f32 ypos, u16 width, u16 height, u8 data[], f32 degrees, f32 scaleX, f32 scaleY, u8 alphaF );
void Menu_DrawRectangle(f32 x, f32 y, f32 width, f32 height, GXColor color, u8 filled);
void Check3D();

extern GXRModeObj *vmode;
extern int screenheight;
extern int screenwidth;
extern u8 * gameScreenTex;
extern u8 * gameScreenPng;
extern int gameScreenPngSize;
extern struct st_palettes palettes[];
extern int FDSSwitchRequested;
extern bool progressive;
extern u32 FrameTimer;
extern bool shutter_3d_mode;
extern bool anaglyph_3d_mode;
extern bool eye_3d;

#endif
