/*****************************************************************************
 * IPL FONT Engine
 *
 * Based on Qoob MP3 Player Font
 *****************************************************************************/

#include <gccore.h>
#include <string.h>
#include "sfont.h"


#define MARGIN 0 //42

extern unsigned int *xfb[2];
extern int whichfb;

extern char backdrop[(640 * 480 * 2) + 32];

int font_height = SFONTHEIGHT;
int font_width = SFONTWIDTH;

void blit_char(int x, int y, unsigned char c, unsigned int selected)
{
int a;
int b;

	for (a = 0; a < SFONTHEIGHT; a++) {
		for (b = 0; b < SFONTWIDTH << 1; b += 2) {
			if (b < 16) {
			if (sfont[c][a] >> (31 - b) & 1) {
				xfb[whichfb][(y + a) * 320 + ((x + b)/2)] &= 0x00ffffff;
				xfb[whichfb][(y + a) * 320 + ((x + b)/2)] |= 0xff000000 & scol[selected][a];
			}
			if (sfont[c][a] >> (30 - b) & 1) {
				xfb[whichfb][(y + a) * 320 + ((x + b)/2)] &= 0xff000000;
				xfb[whichfb][(y + a) * 320 + ((x + b)/2)] |= 0x00ffffff & scol[selected][a];
			}
			if (sfont[c][a] >> (31 - (b + 16)) & 1) {
				xfb[whichfb][(y + a) * 320 + ((x + b)/2)] &= 0x00ffffff;
				xfb[whichfb][(y + a) * 320 + ((x + b)/2)] |= 0xff000000 & 0x00800080;
			}
			if (sfont[c][a] >> (30 - (b + 16)) & 1) {
				xfb[whichfb][(y + a) * 320 + ((x + b)/2)] &= 0xff000000;
				xfb[whichfb][(y + a) * 320 + ((x + b)/2)] |= 0x00ffffff & 0x00800080;
			}
			}
		}
	}
}



void write_font(int x, int y, const char *string)
{
	while (*string)
	{
		blit_char(x, y, *string, 0);
		x += SFONTWIDTH;

		string++;
	}
}

void writex(int x, int y, int sx, int sy, const unsigned char *string, int selected)
{
	int ox = x;
	while ((*string) && ((x) < (ox + sx)))
	{
		blit_char(x, y, *string, selected);
		x += SFONTWIDTH;
		
		string++;
	}
	
	int ay;
	for (ay=0; ay<sy; ay++)
	{
		int ax;
		for (ax=x; ax<(ox + sx); ax += 2)
			xfb[whichfb][(ay+y)*320+ax/2] = 0x00800080;
	}
}

int line = 0;

int scrollerx = 320 - MARGIN;

void scroller(int y, unsigned char text[][512], int nlines)
{
int a;
int b;
int f;
int l;
int s = 0;

int string = 0;

memcpy (&xfb[whichfb][y*320], &backdrop[y*1280], 1280 * SFONTHEIGHT);

	while (text[line][string] != 0)
	{
		for (a = 0; a < SFONTHEIGHT; a++) {
			if (scrollerx + s >= 320 - MARGIN) break;
			if (SFONTWIDTH + scrollerx + s >= 320 - MARGIN)
				l = 320 - MARGIN - (scrollerx + s);
			else
				l = SFONTWIDTH;
			if (scrollerx + s < MARGIN && SFONTWIDTH + scrollerx + s >= MARGIN)
				f = MARGIN - (scrollerx + s);
			else
				f = 0;
			if (SFONTWIDTH + scrollerx + s >= MARGIN)
				for (b = f << 1; b < l << 1; b += 2) {
			if (b < 16) {
					if (sfont[text[line][string]][a] >> (31 - b) & 1) {
						xfb[whichfb][(y + a) * 320 + (scrollerx + s + (b / 2))] &= 0x00ffffff;
						xfb[whichfb][(y + a) * 320 + (scrollerx + s + (b / 2))] |= 0xff000000 & scol[1][a];
					}
					if (sfont[text[line][string]][a] >> (30 - b) & 1) {
						xfb[whichfb][(y + a) * 320 + (scrollerx + s + (b / 2))] &= 0xff000000;
						xfb[whichfb][(y + a) * 320 + (scrollerx + s + (b / 2))] |= 0x00ffffff & scol[1][a];
					}
					if (sfont[text[line][string]][a] >> (31 - (b + 16)) & 1) {
						xfb[whichfb][(y + a) * 320 + (scrollerx + s + (b / 2))] &= 0x00ffffff;
						xfb[whichfb][(y + a) * 320 + (scrollerx + s + (b / 2))] |= 0x00000000;
					}
					if (sfont[text[line][string]][a] >> (30 - (b + 16)) & 1) {
						xfb[whichfb][(y + a) * 320 + (scrollerx + s + (b / 2))] &= 0xff000000;
						xfb[whichfb][(y + a) * 320 + (scrollerx + s + (b / 2))] |= 0x00800080;
					}
					}
				}
			else 
				b = -1;
		}
		s += SFONTWIDTH >> 1;
		string++;
	}

	scrollerx--;

	if (b < 0) {
		scrollerx = 320 - MARGIN;
		line++;
		if (line >= nlines)  line = 0;
	}

}
