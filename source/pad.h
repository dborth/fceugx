/****************************************************************************
 * FCE Ultra
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

#include <gctypes.h>
#include <wiiuse/wpad.h>
#include <wupc/wupc.h>

#define PI 				3.14159265f
#define PADCAL			50
#define WUPCCAL			400
#define MAXJP 			11
#define RAPID_A 		256
#define RAPID_B			512

extern int rumbleRequest[4];
extern u32 btnmap[2][4][12];

void SetControllers();
void ResetControls(int cc = -1, int wc = -1);
void ShutoffRumble();
void DoRumble(int i);
void GetJoy();
bool MenuRequested();
void SetupPads();
void UpdatePads();

#endif
