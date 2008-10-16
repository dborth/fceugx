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
void initDisplay();
void RenderFrame(char *XBuf, int style);

// color palettes
#define MAXPAL 12

struct st_palettes {
    char *name, *desc;
    unsigned int data[64];
};

extern struct st_palettes palettes[];

#endif
