/****************************************************************************
 * FCE Ultra 0.98.12
 * Nintendo Wii/Gamecube Port
 *
 * Tantric September 2008
 *
 * gcvideo.h
 *
 * Video rendering
 ****************************************************************************/

#ifndef _GCVIDEO_H_
#define _GCVIDEO_H_

void clearscreen ();
void showscreen ();
void InitGCVideo ();
void ResetVideo_Emu ();
void ResetVideo_Menu ();
void RenderFrame(unsigned char *XBuf);
void setFrameTimer();
void zoom (float speed);
void zoom_reset ();

// color palettes
#define MAXPAL 12

struct st_palettes {
    char *name, *desc;
    unsigned int data[64];
};

extern struct st_palettes palettes[];
extern int FDSSwitchRequested;
extern int vmode_60hz;
extern bool progressive;

#endif
