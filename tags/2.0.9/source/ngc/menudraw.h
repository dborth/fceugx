/****************************************************************************
 * FCE Ultra 0.98.12
 * Nintendo Wii/Gamecube Port
 *
 * Tantric September 2008
 *
 * menudraw.h
 *
 * Menu drawing routines
 ****************************************************************************/

#ifndef _MENUDRAW_H_
#define _MENUDRAW_H_

#include <gccore.h>
#include "filesel.h"

#define PAGESIZE 13 // max item listing on a screen

int FT_Init ();
void setfontsize (int pixelsize);
void setfontcolour (u8 r, u8 g, u8 b);
void DrawText (int x, int y, const char *text);
void Credits ();
void RomInfo ();
void WaitButtonA ();
int RunMenu (char items[][50], int maxitems, const char *title, int fontsize, int x);
void DrawMenu (char items[][50], const char *title, int maxitems, int selected, int fontsize, int x);
void ShowCheats (char items[][50], char itemvalues[][50], int maxitems, int offset, int selection);
void ShowFiles (BROWSERENTRY * browserList, int maxfiles, int offset, int selection);

void WaitPrompt (const char *msg);
int WaitPromptChoice (const char *msg, const char* bmsg, const char* amsg);
void ShowAction (const char *msg);
void ShowProgress (const char *msg, int done, int total);
void DrawPolygon (int vertices, int *varray, u8 r, u8 g, u8 b);
void DrawLineFast( int startx, int endx, int y, u8 r, u8 g, u8 b);

#endif
