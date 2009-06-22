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
#include <wiiuse/wpad.h>

#define PI 				3.14159265f
#define PADCAL			50
#define MAXJP 			11
#define RAPID_A 		256
#define RAPID_B			512

extern int rumbleRequest[4];
extern u32 btnmap[2][4][12];

void SetControllers();
void ResetControls(int wc = 0, int cc = 0);
void ShutoffRumble();
void DoRumble(int i);
s8 WPAD_StickX(u8 chan,u8 right);
s8 WPAD_StickY(u8 chan, u8 right);
void GetJoy();
void DrawCursor();
bool MenuRequested();

#endif
