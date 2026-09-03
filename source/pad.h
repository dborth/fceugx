/****************************************************************************
 * FCE Ultra GX
 *
 * Daryl Borth 2008-2026
 *
 * pad.h
 *
 * Controller input
 ****************************************************************************/

#ifndef _PAD_H_
#define _PAD_H_

#define PI 				3.14159265f
#define PADCAL			50
#define WIIDRCCAL		20
#define MAXJP 			11
#define RAPID_A 		256
#define RAPID_B			512

extern int playerMapping[4];
extern uint32_t btnmap[2][6][12];

void SetControllers();
void ResetControls(int cc = -1, int wc = -1);
void GetJoy();
bool isMenuRequested();
void UpdatePads();

#endif
