/****************************************************************************
 * FCE Ultra 0.98.12
 * Nintendo Wii/Gamecube Port
 *
 * Tantric September 2008
 *
 * pad.h
 *
 * Controller input
 ****************************************************************************/

#ifndef _PAD_H_
#define _PAD_H_

#include <gccore.h>

#define PI 				3.14159265f
#define PADCAL			50
#define MAXJP 			9

extern unsigned int gcpadmap[];
extern unsigned int wmpadmap[];
extern unsigned int ccpadmap[];
extern unsigned int ncpadmap[];

s8 WPAD_StickX(u8 chan,u8 right);
s8 WPAD_StickY(u8 chan, u8 right);
void InitialisePads();
void GetJoy();
void ToggleFourScore(int set);
void ToggleZapper(int set);
void DrawCursor();

#endif

